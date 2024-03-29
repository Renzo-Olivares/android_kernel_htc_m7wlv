/*
 *  linux/drivers/mmc/core/host.c
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *  Copyright (C) 2007-2008 Pierre Ossman
 *  Copyright (C) 2010 Linus Walleij
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  MMC host class device management
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/idr.h>
#include <linux/pagemap.h>
#include <linux/export.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/suspend.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>

#include "core.h"
#include "host.h"

#define cls_dev_to_mmc_host(d)	container_of(d, struct mmc_host, class_dev)

#ifdef CONFIG_MMC_PERF_PROFILING
#define MMC_STATS_INFO_INTERVAL 3000 
#else
#define MMC_STATS_INFO_INTERVAL 5000 
#endif
int g_perf_enable = false;
struct mmc_host *g_host = NULL;
static void mmc_stats_timer_hdlr(unsigned long data);
static void mmc_host_classdev_release(struct device *dev)
{
	struct mmc_host *host = cls_dev_to_mmc_host(dev);
	kfree(host);
}

static struct class mmc_host_class = {
	.name		= "mmc_host",
	.dev_release	= mmc_host_classdev_release,
};

int mmc_register_host_class(void)
{
	return class_register(&mmc_host_class);
}

void mmc_unregister_host_class(void)
{
	class_unregister(&mmc_host_class);
}

static DEFINE_IDR(mmc_host_idr);
static DEFINE_SPINLOCK(mmc_host_lock);

#ifdef CONFIG_MMC_CLKGATE
static ssize_t clkgate_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mmc_host *host = cls_dev_to_mmc_host(dev);
	return snprintf(buf, PAGE_SIZE, "%lu\n", host->clkgate_delay);
}

static ssize_t clkgate_delay_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mmc_host *host = cls_dev_to_mmc_host(dev);
	unsigned long flags, value;

	if (kstrtoul(buf, 0, &value))
		return -EINVAL;

	spin_lock_irqsave(&host->clk_lock, flags);
	host->clkgate_delay = value;
	spin_unlock_irqrestore(&host->clk_lock, flags);
	return count;
}

static void mmc_host_clk_gate_delayed(struct mmc_host *host)
{
	unsigned long tick_ns;
	unsigned long freq = host->ios.clock;
	unsigned long flags;

	if (!freq) {
		pr_debug("%s: frequency set to 0 in disable function, "
			 "this means the clock is already disabled.\n",
			 mmc_hostname(host));
		return;
	}
	spin_lock_irqsave(&host->clk_lock, flags);

	if (!host->clk_requests) {
		spin_unlock_irqrestore(&host->clk_lock, flags);
		tick_ns = DIV_ROUND_UP(1000000000, freq);
		ndelay(host->clk_delay * tick_ns);
	} else {
		
		spin_unlock_irqrestore(&host->clk_lock, flags);
		return;
	}
	mutex_lock(&host->clk_gate_mutex);
	spin_lock_irqsave(&host->clk_lock, flags);
	if (!host->clk_requests) {
		spin_unlock_irqrestore(&host->clk_lock, flags);
		
		mmc_gate_clock(host);
		spin_lock_irqsave(&host->clk_lock, flags);
		pr_debug("%s: gated MCI clock\n", mmc_hostname(host));
	}
	spin_unlock_irqrestore(&host->clk_lock, flags);
	mutex_unlock(&host->clk_gate_mutex);
}

static void mmc_host_clk_gate_work(struct work_struct *work)
{
	struct mmc_host *host = container_of(work, struct mmc_host,
					      clk_gate_work.work);

	mmc_host_clk_gate_delayed(host);
}

void mmc_host_clk_hold(struct mmc_host *host)
{
	unsigned long flags;

	
	cancel_delayed_work_sync(&host->clk_gate_work);
	mutex_lock(&host->clk_gate_mutex);
	spin_lock_irqsave(&host->clk_lock, flags);
	if (host->clk_gated) {
		spin_unlock_irqrestore(&host->clk_lock, flags);
		mmc_ungate_clock(host);
		spin_lock_irqsave(&host->clk_lock, flags);
		pr_debug("%s: ungated MCI clock\n", mmc_hostname(host));
	}
	host->clk_requests++;
	spin_unlock_irqrestore(&host->clk_lock, flags);
	mutex_unlock(&host->clk_gate_mutex);
}

static bool mmc_host_may_gate_card(struct mmc_card *card)
{
	
	if (!card)
		return true;
	return !(card->quirks & MMC_QUIRK_BROKEN_CLK_GATING);
}

void mmc_host_clk_release(struct mmc_host *host)
{
	unsigned long flags;

	spin_lock_irqsave(&host->clk_lock, flags);
	host->clk_requests--;
	if (mmc_host_may_gate_card(host->card) &&
	    !host->clk_requests)
		queue_delayed_work(system_nrt_wq, &host->clk_gate_work,
				msecs_to_jiffies(host->clkgate_delay));
	spin_unlock_irqrestore(&host->clk_lock, flags);
}

unsigned int mmc_host_clk_rate(struct mmc_host *host)
{
	unsigned long freq;
	unsigned long flags;

	spin_lock_irqsave(&host->clk_lock, flags);
	if (host->clk_gated)
		freq = host->clk_old;
	else
		freq = host->ios.clock;
	spin_unlock_irqrestore(&host->clk_lock, flags);
	return freq;
}

static inline void mmc_host_clk_init(struct mmc_host *host)
{
	host->clk_requests = 0;
	
	host->clk_delay = 8;
	host->clkgate_delay = 0;
	host->clk_gated = false;
	INIT_DELAYED_WORK(&host->clk_gate_work, mmc_host_clk_gate_work);
	spin_lock_init(&host->clk_lock);
	mutex_init(&host->clk_gate_mutex);
}

static inline void mmc_host_clk_exit(struct mmc_host *host)
{
	if (cancel_delayed_work_sync(&host->clk_gate_work))
		mmc_host_clk_gate_delayed(host);
	if (host->clk_gated)
		mmc_host_clk_hold(host);
	
	WARN_ON(host->clk_requests > 1);
}

static inline void mmc_host_clk_sysfs_init(struct mmc_host *host)
{
	host->clkgate_delay_attr.show = clkgate_delay_show;
	host->clkgate_delay_attr.store = clkgate_delay_store;
	sysfs_attr_init(&host->clkgate_delay_attr.attr);
	host->clkgate_delay_attr.attr.name = "clkgate_delay";
	host->clkgate_delay_attr.attr.mode = S_IRUGO | S_IWUSR;
	if (device_create_file(&host->class_dev, &host->clkgate_delay_attr))
		pr_err("%s: Failed to create clkgate_delay sysfs entry\n",
				mmc_hostname(host));
}
#else

static inline void mmc_host_clk_init(struct mmc_host *host)
{
}

static inline void mmc_host_clk_exit(struct mmc_host *host)
{
}

static inline void mmc_host_clk_sysfs_init(struct mmc_host *host)
{
}

#endif

struct mmc_host *mmc_alloc_host(int extra, struct device *dev)
{
	int err;
	struct mmc_host *host;

	if (!idr_pre_get(&mmc_host_idr, GFP_KERNEL))
		return NULL;

	host = kzalloc(sizeof(struct mmc_host) + extra, GFP_KERNEL);
	if (!host)
		return NULL;

	spin_lock(&mmc_host_lock);
	err = idr_get_new(&mmc_host_idr, host, &host->index);
	spin_unlock(&mmc_host_lock);
	if (err)
		goto free;

	dev_set_name(&host->class_dev, "mmc%d", host->index);

	host->parent = dev;
	host->class_dev.parent = dev;
	host->class_dev.class = &mmc_host_class;
	device_initialize(&host->class_dev);

	mmc_host_clk_init(host);

	spin_lock_init(&host->lock);
	init_waitqueue_head(&host->wq);
	wake_lock_init(&host->detect_wake_lock, WAKE_LOCK_SUSPEND,
		kasprintf(GFP_KERNEL, "%s_detect", mmc_hostname(host)));
	INIT_DELAYED_WORK(&host->detect, mmc_rescan);
	INIT_DELAYED_WORK(&host->remove, mmc_remove_sd_card);
#ifdef CONFIG_PM
	host->pm_notify.notifier_call = mmc_pm_notify;
#endif
	if (0 == host->index)
	{
		g_host = host;
		setup_timer(&host->stats_timer, mmc_stats_timer_hdlr, (unsigned long)host);
		mod_timer(&host->stats_timer, (jiffies + msecs_to_jiffies(MMC_STATS_INFO_INTERVAL)));
	}
	host->max_segs = 1;
	host->max_seg_size = PAGE_CACHE_SIZE;

	host->max_req_size = PAGE_CACHE_SIZE;
	host->max_blk_size = 512;
	host->max_blk_count = PAGE_CACHE_SIZE / 512;

	return host;

free:
	kfree(host);
	return NULL;
}

EXPORT_SYMBOL(mmc_alloc_host);


static ssize_t
show_perf(struct device *dev, struct device_attribute *attr, char *buf)
{
	
	int64_t rtime_drv, wtime_drv;
	unsigned long rbytes_drv, wbytes_drv;

	if (g_host) {
		spin_lock(&g_host->lock);

		rbytes_drv = g_host->perf.rbytes_drv;
		wbytes_drv = g_host->perf.wbytes_drv;

		rtime_drv = ktime_to_us(g_host->perf.rtime_drv);
		wtime_drv = ktime_to_us(g_host->perf.wtime_drv);

		spin_unlock(&g_host->lock);
	}

	return snprintf(buf, PAGE_SIZE, "Write performance at driver Level:"
					"%lu bytes in %lld microseconds\n"
					"Read performance at driver Level:"
					"%lu bytes in %lld microseconds\n",
					wbytes_drv, wtime_drv,
					rbytes_drv, rtime_drv);
}

static ssize_t
set_perf(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int64_t value;
	

	sscanf(buf, "%lld", &value);
	if (g_host) {
		spin_lock(&g_host->lock);
		if (!value) {
			memset(&g_host->perf, 0, sizeof(g_host->perf));
			g_host->perf_enable = false;
		} else {
			g_host->perf_enable = true;
			
		}
		spin_unlock(&g_host->lock);
	}

	return count;
}

static DEVICE_ATTR(perf, S_IRUGO | S_IWUSR,
		show_perf, set_perf);

#ifdef CONFIG_MMC_PERF_PROFILING
void mmc_stats_timer_hdlr(unsigned long data)
{
	struct mmc_host *host = (struct mmc_host *)data;
	unsigned long rtime, wtime;
	unsigned long rbytes, wbytes, rcnt, wcnt;
	unsigned long wperf = 0, rperf = 0;
	unsigned long flags;


	if (host && host->perf_enable) {
		spin_lock_irqsave(&host->lock, flags);

		rbytes = host->perf.rbytes_drv;
		wbytes = host->perf.wbytes_drv;
		rcnt = host->perf.rcount;
		wcnt = host->perf.wcount;
		rtime = (unsigned long)ktime_to_ms(host->perf.rtime_drv);
		wtime = (unsigned long)ktime_to_ms(host->perf.wtime_drv);

		host->perf.rbytes_drv = host->perf.wbytes_drv = 0;
		host->perf.rcount = host->perf.wcount = 0;
		host->perf.rtime_drv = ktime_set(0, 0);
		host->perf.wtime_drv = ktime_set(0, 0);

		spin_unlock_irqrestore(&host->lock, flags);

		if (wtime)
			wperf = ((wbytes / 1024) * 1000) / wtime;
		if (rtime)
			rperf = ((rbytes / 1024) * 1000) / rtime;

		
		if (!strcmp(mmc_hostname(host), "mmc0")) {
			pr_info("%s Statistics: write %lu KB in %lu ms, perf %lu KB/s, rq %lu\n",
				mmc_hostname(host), wbytes / 1024, wtime, wperf, wcnt);
			pr_info("%s Statistics: read %lu KB in %lu ms, perf %lu KB/s, rq %lu\n",
				mmc_hostname(host), rbytes / 1024, rtime, rperf, rcnt);
		}

	}

	mod_timer(&host->stats_timer, (jiffies +
		  msecs_to_jiffies(MMC_STATS_INFO_INTERVAL)));
	return;
}
#else
void mmc_stats_timer_hdlr(unsigned long data)
{
	struct mmc_host *host = (struct mmc_host *)data;
	unsigned long rtime, wtime;
	unsigned long rbytes, wbytes, rcnt, wcnt;
	unsigned long wperf = 0, rperf = 0;
	unsigned long flags;
	if (!host || !host->perf_enable)
		return;

	spin_lock_irqsave(&host->lock, flags);

	rbytes = host->perf.rbytes_drv;
	wbytes = host->perf.wbytes_drv;
	rcnt = host->perf.rcount;
	wcnt = host->perf.wcount;
	rtime = (unsigned long)ktime_to_ms(host->perf.rtime_drv);
	wtime = (unsigned long)ktime_to_ms(host->perf.wtime_drv);

	host->perf.rbytes_drv = host->perf.wbytes_drv = 0;
	host->perf.rcount = host->perf.wcount = 0;
	host->perf.rtime_drv = ktime_set(0, 0);
	host->perf.wtime_drv = ktime_set(0, 0);

	spin_unlock_irqrestore(&host->lock, flags);

	if (wtime)
		wperf = ((wbytes / 1024) * 1000) / wtime;
	if (rtime)
		rperf = ((rbytes / 1024) * 1000) / rtime;

	
	if (!strcmp(mmc_hostname(host), "mmc0") && wperf && (wtime > 500))
		pr_info("%s Statistics: write %lu KB in %lu ms, perf %lu KB/s, rq %lu\n",
			mmc_hostname(host), wbytes / 1024, wtime, wperf, wcnt);
	if (!strcmp(mmc_hostname(host), "mmc0") && rperf && (rtime > 500))
		pr_info("%s Statistics: read %lu KB in %lu ms, perf %lu KB/s, rq %lu\n",
			mmc_hostname(host), rbytes / 1024, rtime, rperf, rcnt);

	mod_timer(&host->stats_timer, (jiffies +
		  msecs_to_jiffies(MMC_STATS_INFO_INTERVAL)));
	return;
}



#endif

static struct attribute *dev_attrs[] = {
	&dev_attr_perf.attr,
	
	NULL,
};
static struct attribute_group dev_attr_grp = {
	.attrs = dev_attrs,
};



int mmc_add_host(struct mmc_host *host)
{
	int err;

	WARN_ON((host->caps & MMC_CAP_SDIO_IRQ) &&
		!host->ops->enable_sdio_irq);

	err = device_add(&host->class_dev);
	if (err)
		return err;

	led_trigger_register_simple(dev_name(&host->class_dev), &host->led);

#ifdef CONFIG_DEBUG_FS
	mmc_add_host_debugfs(host);
#endif
	mmc_host_clk_sysfs_init(host);
	err = sysfs_create_group(&host->parent->kobj, &dev_attr_grp);
	if (err)
		pr_err("%s: failed to create sysfs group with err %d\n",
							 __func__, err);

	mmc_start_host(host);
	if (!(host->pm_flags & MMC_PM_IGNORE_PM_NOTIFY))
		register_pm_notifier(&host->pm_notify);

	return 0;
}

EXPORT_SYMBOL(mmc_add_host);

void mmc_remove_host(struct mmc_host *host)
{
	if (!(host->pm_flags & MMC_PM_IGNORE_PM_NOTIFY))
		unregister_pm_notifier(&host->pm_notify);

	mmc_stop_host(host);

#ifdef CONFIG_DEBUG_FS
	mmc_remove_host_debugfs(host);
#endif
	sysfs_remove_group(&host->parent->kobj, &dev_attr_grp);
	device_del(&host->class_dev);

	led_trigger_unregister_simple(host->led);

	mmc_host_clk_exit(host);
}

EXPORT_SYMBOL(mmc_remove_host);

void mmc_free_host(struct mmc_host *host)
{
	spin_lock(&mmc_host_lock);
	idr_remove(&mmc_host_idr, host->index);
	spin_unlock(&mmc_host_lock);
	wake_lock_destroy(&host->detect_wake_lock);

	put_device(&host->class_dev);
}

EXPORT_SYMBOL(mmc_free_host);
