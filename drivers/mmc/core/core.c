/*
 *  linux/drivers/mmc/core/core.c
 *
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *  SD support Copyright (C) 2004 Ian Molton, All Rights Reserved.
 *  Copyright (C) 2005-2008 Pierre Ossman, All Rights Reserved.
 *  MMCv4 support Copyright (C) 2006 Philip Langdale, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/scatterlist.h>
#include <linux/log2.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/suspend.h>
#include <linux/fault-inject.h>
#include <linux/random.h>
#include <linux/wakelock.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>

#include "core.h"
#include "bus.h"
#include "host.h"
#include "sdio_bus.h"

#include "mmc_ops.h"
#include "sd_ops.h"
#include "sdio_ops.h"

static struct workqueue_struct *workqueue;

bool use_spi_crc = 1;
module_param(use_spi_crc, bool, 0);

#ifdef CONFIG_MMC_UNSAFE_RESUME
bool mmc_assume_removable;
#else
bool mmc_assume_removable = 1;
#endif
EXPORT_SYMBOL(mmc_assume_removable);
module_param_named(removable, mmc_assume_removable, bool, 0644);
MODULE_PARM_DESC(
	removable,
	"MMC/SD cards are removable and may be removed during suspend");

static int mmc_schedule_delayed_work(struct delayed_work *work,
				     unsigned long delay)
{
	return queue_delayed_work(workqueue, work, delay);
}

int mmc_schedule_card_removal_work(struct delayed_work *work,
				     unsigned long delay)
{
	return queue_delayed_work(workqueue, work, delay);
}

static void mmc_flush_scheduled_work(void)
{
	flush_workqueue(workqueue);
}

#ifdef CONFIG_FAIL_MMC_REQUEST

static void mmc_should_fail_request(struct mmc_host *host,
				    struct mmc_request *mrq)
{
	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data *data = mrq->data;
	static const int data_errors[] = {
		-ETIMEDOUT,
		-EILSEQ,
		-EIO,
	};

	if (!data)
		return;

	if (cmd->error || data->error ||
	    !should_fail(&host->fail_mmc_request, data->blksz * data->blocks))
		return;

	data->error = data_errors[random32() % ARRAY_SIZE(data_errors)];
	data->bytes_xfered = (random32() % (data->bytes_xfered >> 9)) << 9;
}

#else 

static inline void mmc_should_fail_request(struct mmc_host *host,
					   struct mmc_request *mrq)
{
}

#endif 

void mmc_request_done(struct mmc_host *host, struct mmc_request *mrq)
{
	struct mmc_command *cmd = mrq->cmd;
	int err = cmd->error;
#ifdef CONFIG_MMC_PERF_PROFILING
	ktime_t diff;
#endif

	if (err && cmd->retries && mmc_host_is_spi(host)) {
		if (cmd->resp[0] & R1_SPI_ILLEGAL_COMMAND)
			cmd->retries = 0;
	}

	if (err && cmd->retries && !mmc_card_removed(host->card)) {
		if (mrq->done)
			mrq->done(mrq);
	} else {
		mmc_should_fail_request(host, mrq);

		

		pr_debug("%s: req done (CMD%u): %d: %08x %08x %08x %08x\n",
			mmc_hostname(host), cmd->opcode, err,
			cmd->resp[0], cmd->resp[1],
			cmd->resp[2], cmd->resp[3]);

		if (mrq->data) {
#ifdef CONFIG_MMC_PERF_PROFILING
			if (host->perf_enable) {
				diff = ktime_sub(ktime_get(), host->perf.start);
				if (mrq->data->flags == MMC_DATA_READ) {
					host->perf.rbytes_drv +=
							mrq->data->bytes_xfered;
					host->perf.rtime_drv =
						ktime_add(host->perf.rtime_drv,
							diff);
					host->perf.rcount++;
				} else {
					host->perf.wbytes_drv +=
						mrq->data->bytes_xfered;
					host->perf.wtime_drv =
						ktime_add(host->perf.wtime_drv,
							diff);
					host->perf.wcount++;
				}
			}
#endif
			pr_debug("%s:     %d bytes transferred: %d\n",
				mmc_hostname(host),
				mrq->data->bytes_xfered, mrq->data->error);
		}

		if (mrq->stop) {
			pr_debug("%s:     (CMD%u): %d: %08x %08x %08x %08x\n",
				mmc_hostname(host), mrq->stop->opcode,
				mrq->stop->error,
				mrq->stop->resp[0], mrq->stop->resp[1],
				mrq->stop->resp[2], mrq->stop->resp[3]);
		}

		if (mrq->done)
			mrq->done(mrq);

		mmc_host_clk_release(host);
	}
}

EXPORT_SYMBOL(mmc_request_done);

static void
mmc_start_request(struct mmc_host *host, struct mmc_request *mrq)
{
#ifdef CONFIG_MMC_DEBUG
	unsigned int i, sz;
	struct scatterlist *sg;
#endif

	if (mrq->sbc) {
		pr_debug("<%s: starting CMD%u arg %08x flags %08x>\n",
			 mmc_hostname(host), mrq->sbc->opcode,
			 mrq->sbc->arg, mrq->sbc->flags);
	}

	pr_debug("%s: starting CMD%u arg %08x flags %08x\n",
		 mmc_hostname(host), mrq->cmd->opcode,
		 mrq->cmd->arg, mrq->cmd->flags);

	if (mrq->data) {
		pr_debug("%s:     blksz %d blocks %d flags %08x "
			"tsac %d ms nsac %d\n",
			mmc_hostname(host), mrq->data->blksz,
			mrq->data->blocks, mrq->data->flags,
			mrq->data->timeout_ns / 1000000,
			mrq->data->timeout_clks);
	}

	if (mrq->stop) {
		pr_debug("%s:     CMD%u arg %08x flags %08x\n",
			 mmc_hostname(host), mrq->stop->opcode,
			 mrq->stop->arg, mrq->stop->flags);
	}

	WARN_ON(!host->claimed);

	mrq->cmd->error = 0;
	mrq->cmd->mrq = mrq;
	if (mrq->data) {
		BUG_ON(mrq->data->blksz > host->max_blk_size);
		BUG_ON(mrq->data->blocks > host->max_blk_count);
		BUG_ON(mrq->data->blocks * mrq->data->blksz >
			host->max_req_size);

#ifdef CONFIG_MMC_DEBUG
		sz = 0;
		for_each_sg(mrq->data->sg, sg, mrq->data->sg_len, i)
			sz += sg->length;
		BUG_ON(sz != mrq->data->blocks * mrq->data->blksz);
#endif

		mrq->cmd->data = mrq->data;
		mrq->data->error = 0;
		mrq->data->mrq = mrq;
		if (mrq->stop) {
			mrq->data->stop = mrq->stop;
			mrq->stop->error = 0;
			mrq->stop->mrq = mrq;
		}
#ifdef CONFIG_MMC_PERF_PROFILING
		if (host->perf_enable)
			host->perf.start = ktime_get();
#endif
	}
	mmc_host_clk_hold(host);
	
	host->ops->request(host, mrq);
}

static void mmc_wait_done(struct mmc_request *mrq)
{
	complete(&mrq->completion);
}

static int __mmc_start_req(struct mmc_host *host, struct mmc_request *mrq)
{
	init_completion(&mrq->completion);
	mrq->done = mmc_wait_done;
	if (mmc_card_removed(host->card)) {
		mrq->cmd->error = -ENOMEDIUM;
		complete(&mrq->completion);
		return -ENOMEDIUM;
	}
	mmc_start_request(host, mrq);
	return 0;
}

static void mmc_wait_for_req_done(struct mmc_host *host,
				  struct mmc_request *mrq)
{
	struct mmc_command *cmd;

	while (1) {
		wait_for_completion(&mrq->completion);

		cmd = mrq->cmd;
		if (!cmd->error || !cmd->retries ||
		    mmc_card_removed(host->card))
			break;

		pr_debug("%s: req failed (CMD%u): %d, retrying...\n",
			 mmc_hostname(host), cmd->opcode, cmd->error);
		cmd->retries--;
		cmd->error = 0;
		host->ops->request(host, mrq);
	}
}

static void mmc_pre_req(struct mmc_host *host, struct mmc_request *mrq,
		 bool is_first_req)
{
	if (host->ops->pre_req) {
		mmc_host_clk_hold(host);
		host->ops->pre_req(host, mrq, is_first_req);
		mmc_host_clk_release(host);
	}
}

static void mmc_post_req(struct mmc_host *host, struct mmc_request *mrq,
			 int err)
{
	if (host->ops->post_req) {
		mmc_host_clk_hold(host);
		host->ops->post_req(host, mrq, err);
		mmc_host_clk_release(host);
	}
}

struct mmc_async_req *mmc_start_req(struct mmc_host *host,
				    struct mmc_async_req *areq, int *error)
{
	int err = 0;
	int start_err = 0;
	struct mmc_async_req *data = host->areq;

	
	if (areq)
		mmc_pre_req(host, areq->mrq, !host->areq);

	if (host->areq) {
#ifdef CONFIG_MMC_PERF_PROFILING
		ktime_t io_diff = ktime_get();
		ktime_t wait_diff = io_diff, wait_ready = io_diff;
#endif
		mmc_wait_for_req_done(host, host->areq->mrq);
#ifdef CONFIG_MMC_PERF_PROFILING
		if (mmc_card_sd(host->card)) {
		       io_diff = ktime_sub(ktime_get(), host->areq->rq_stime);
		       if (ktime_to_us(io_diff) > 1000000)
		               pr_info("%s (%s), finish(1) cmd(%d) s_sec %d, size %d, time = %lld us\n",
		                       mmc_hostname(host), current->comm, host->areq->mrq->cmd->opcode,
		                       host->areq->mrq->cmd->arg , host->areq->mrq->data->blocks , ktime_to_us(io_diff));
		       wait_ready = ktime_get();
		}
#endif
		err = host->areq->err_check(host->card, host->areq);
#ifdef CONFIG_MMC_PERF_PROFILING
		if (mmc_card_sd(host->card)) {
		       wait_diff = ktime_sub(ktime_get(), wait_ready);
		       io_diff = ktime_sub(ktime_get(), host->areq->rq_stime);
		       if (ktime_to_us(io_diff) > 1000000)
		               pr_info("%s (%s), finish(2) cmd(%d) s_sec %d, size %d, ready time = %lld us, total time = %lld us\n",
		                       mmc_hostname(host), current->comm, host->areq->mrq->cmd->opcode, host->areq->mrq->cmd->arg ,
		                       host->areq->mrq->data->blocks, ktime_to_us(wait_diff), ktime_to_us(io_diff));
		}
#endif
	}

	if (!err && areq) {
#ifdef CONFIG_MMC_PERF_PROFILING
		if (mmc_card_sd(host->card))
		       areq->rq_stime = ktime_get();
#endif
		start_err = __mmc_start_req(host, areq->mrq);
	}

	if (host->areq)
		mmc_post_req(host, host->areq->mrq, 0);

	 
	if ((err || start_err) && areq)
			mmc_post_req(host, areq->mrq, -EINVAL);

	if (err)
		host->areq = NULL;
	else
		host->areq = areq;

	if (error)
		*error = err;
	return data;
}
EXPORT_SYMBOL(mmc_start_req);

void mmc_wait_for_req(struct mmc_host *host, struct mmc_request *mrq)
{
	__mmc_start_req(host, mrq);
	mmc_wait_for_req_done(host, mrq);
}
EXPORT_SYMBOL(mmc_wait_for_req);

int mmc_interrupt_hpi(struct mmc_card *card)
{
	int err;
	u32 status;

	BUG_ON(!card);

	if (!card->ext_csd.hpi_en) {
		pr_info("%s: HPI enable bit unset\n", mmc_hostname(card->host));
		return 1;
	}

	mmc_claim_host(card->host);
	err = mmc_send_status(card, &status);
	if (err) {
		pr_err("%s: Get card status fail\n", mmc_hostname(card->host));
		goto out;
	}

	if (R1_CURRENT_STATE(status) == R1_STATE_PRG) {
		do {
			err = mmc_send_hpi_cmd(card, &status);
			if (err)
				pr_debug("%s: abort HPI (%d error)\n",
					 mmc_hostname(card->host), err);

			err = mmc_send_status(card, &status);
			if (err)
				break;
		} while (R1_CURRENT_STATE(status) == R1_STATE_PRG);
	} else
		pr_debug("%s: Left prg-state\n", mmc_hostname(card->host));

out:
	mmc_release_host(card->host);
	return err;
}
EXPORT_SYMBOL(mmc_interrupt_hpi);

int mmc_wait_for_cmd(struct mmc_host *host, struct mmc_command *cmd, int retries)
{
	struct mmc_request mrq = {NULL};

	WARN_ON(!host->claimed);

	memset(cmd->resp, 0, sizeof(cmd->resp));
	cmd->retries = retries;

	mrq.cmd = cmd;
	cmd->data = NULL;

	mmc_wait_for_req(host, &mrq);

	return cmd->error;
}

EXPORT_SYMBOL(mmc_wait_for_cmd);

void mmc_set_data_timeout(struct mmc_data *data, const struct mmc_card *card)
{
	unsigned int mult;

	if (mmc_card_sdio(card)) {
		data->timeout_ns = 1000000000;
		data->timeout_clks = 0;
		return;
	}

	mult = mmc_card_sd(card) ? 100 : 10;

	if (data->flags & MMC_DATA_WRITE)
		mult <<= card->csd.r2w_factor;

	data->timeout_ns = card->csd.tacc_ns * mult;
	data->timeout_clks = card->csd.tacc_clks * mult;

	if (mmc_card_sd(card)) {
		unsigned int timeout_us, limit_us;

		timeout_us = data->timeout_ns / 1000;
		if (mmc_host_clk_rate(card->host))
			timeout_us += data->timeout_clks * 1000 /
				(mmc_host_clk_rate(card->host) / 1000);

		if (data->flags & MMC_DATA_WRITE)
			limit_us = 3000000;
		else
			limit_us = 100000;

		if (timeout_us > limit_us || mmc_card_blockaddr(card)) {
			data->timeout_ns = limit_us * 1000;
			data->timeout_clks = 0;
		}
	}

	if (mmc_card_long_read_time(card) && data->flags & MMC_DATA_READ) {
		data->timeout_ns = 300000000;
		data->timeout_clks = 0;
	}

	if (mmc_host_is_spi(card->host)) {
		if (data->flags & MMC_DATA_WRITE) {
			if (data->timeout_ns < 1000000000)
				data->timeout_ns = 1000000000;	
		} else {
			if (data->timeout_ns < 100000000)
				data->timeout_ns =  100000000;	
		}
	}
}
EXPORT_SYMBOL(mmc_set_data_timeout);

unsigned int mmc_align_data_size(struct mmc_card *card, unsigned int sz)
{
	sz = ((sz + 3) / 4) * 4;

	return sz;
}
EXPORT_SYMBOL(mmc_align_data_size);

int mmc_host_enable(struct mmc_host *host)
{

	if (!(host->caps & MMC_CAP_DISABLE))
		return 0;

	if (host->en_dis_recurs)
		return 0;

	if (host->nesting_cnt++)
		return 0;


	cancel_delayed_work_sync(&host->disable);

	if (host->enabled)
		return 0;

	if (host->ops->enable) {
		int err;

		host->en_dis_recurs = 1;
		mmc_host_clk_hold(host);
		err = host->ops->enable(host);
		mmc_host_clk_release(host);
		host->en_dis_recurs = 0;

		if (err) {
			pr_debug("%s: enable error %d\n",
				 mmc_hostname(host), err);
			return err;
		}
	}
	host->enabled = 1;
	return 0;
}
EXPORT_SYMBOL(mmc_host_enable);

static int mmc_host_do_disable(struct mmc_host *host, int lazy)
{
	if (host->ops->disable) {
		int err;

		host->en_dis_recurs = 1;
		mmc_host_clk_hold(host);
		err = host->ops->disable(host);
		mmc_host_clk_release(host);
		host->en_dis_recurs = 0;

		if (err < 0) {
			pr_debug("%s: disable error %d\n",
				 mmc_hostname(host), err);
			return err;
		}
		if (err > 0) {
			unsigned long delay = msecs_to_jiffies(err);

			mmc_schedule_delayed_work(&host->disable, delay);
		}
	}
	host->enabled = 0;
	return 0;
}

int mmc_host_disable(struct mmc_host *host)
{
	int err;

	if (!(host->caps & MMC_CAP_DISABLE))
		return 0;

	if (host->en_dis_recurs)
		return 0;

	if (--host->nesting_cnt)
		return 0;

	if (!host->enabled)
		return 0;

	err = mmc_host_do_disable(host, 0);
	return err;
}
EXPORT_SYMBOL(mmc_host_disable);

int __mmc_claim_host(struct mmc_host *host, atomic_t *abort)
{
	DECLARE_WAITQUEUE(wait, current);
	unsigned long flags;
	int stop;

	might_sleep();

	add_wait_queue(&host->wq, &wait);
	spin_lock_irqsave(&host->lock, flags);
	while (1) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		stop = abort ? atomic_read(abort) : 0;
		if (stop || !host->claimed || host->claimer == current)
			break;
		spin_unlock_irqrestore(&host->lock, flags);
		schedule();
		spin_lock_irqsave(&host->lock, flags);
	}
	set_current_state(TASK_RUNNING);
	if (!stop) {
		host->claimed = 1;
		host->claimer = current;
		host->claim_cnt += 1;
	} else
		wake_up(&host->wq);
	spin_unlock_irqrestore(&host->lock, flags);
	remove_wait_queue(&host->wq, &wait);
	if (host->ops->enable && !stop && host->claim_cnt == 1)
		host->ops->enable(host);
	return stop;
}

EXPORT_SYMBOL(__mmc_claim_host);

int mmc_try_claim_host(struct mmc_host *host)
{
	int claimed_host = 0;
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	if (!host->claimed || host->claimer == current) {
		host->claimed = 1;
		host->claimer = current;
		host->claim_cnt += 1;
		claimed_host = 1;
	}
	spin_unlock_irqrestore(&host->lock, flags);
	if (host->ops->enable && claimed_host && host->claim_cnt == 1)
		host->ops->enable(host);
	return claimed_host;
}
EXPORT_SYMBOL(mmc_try_claim_host);

void mmc_release_host(struct mmc_host *host)
{
	unsigned long flags;

	WARN_ON(!host->claimed);

	if(!host->claimed)
		return;

	if (host->ops->disable && host->claim_cnt == 1)
		host->ops->disable(host);

	spin_lock_irqsave(&host->lock, flags);
	if (--host->claim_cnt) {
		
		spin_unlock_irqrestore(&host->lock, flags);
	} else {
		host->claimed = 0;
		host->claimer = NULL;
		spin_unlock_irqrestore(&host->lock, flags);
		wake_up(&host->wq);
	}
}
EXPORT_SYMBOL(mmc_release_host);

static inline void mmc_set_ios(struct mmc_host *host)
{
	struct mmc_ios *ios = &host->ios;

	pr_debug("%s: clock %uHz busmode %u powermode %u cs %u Vdd %u "
		"width %u timing %u\n",
		 mmc_hostname(host), ios->clock, ios->bus_mode,
		 ios->power_mode, ios->chip_select, ios->vdd,
		 ios->bus_width, ios->timing);

	if (ios->clock > 0)
		mmc_set_ungated(host);
	if(host->ops)
		host->ops->set_ios(host, ios);
}

void mmc_set_chip_select(struct mmc_host *host, int mode)
{
	mmc_host_clk_hold(host);
	host->ios.chip_select = mode;
	mmc_set_ios(host);
	mmc_host_clk_release(host);
}

static void __mmc_set_clock(struct mmc_host *host, unsigned int hz)
{
	WARN_ON(hz < host->f_min);

	if (hz > host->f_max)
		hz = host->f_max;

	host->ios.clock = hz;
	mmc_set_ios(host);
}

void mmc_set_clock(struct mmc_host *host, unsigned int hz)
{
	mmc_host_clk_hold(host);
	__mmc_set_clock(host, hz);
	mmc_host_clk_release(host);
}

#ifdef CONFIG_MMC_CLKGATE
void mmc_gate_clock(struct mmc_host *host)
{
	unsigned long flags;

	spin_lock_irqsave(&host->clk_lock, flags);
	host->clk_old = host->ios.clock;
	host->ios.clock = 0;
	host->clk_gated = true;
	spin_unlock_irqrestore(&host->clk_lock, flags);
	mmc_set_ios(host);
}

void mmc_ungate_clock(struct mmc_host *host)
{
	if (host->clk_old) {
		BUG_ON(host->ios.clock);
		
		__mmc_set_clock(host, host->clk_old);
	}
}

void mmc_set_ungated(struct mmc_host *host)
{
	unsigned long flags;

	spin_lock_irqsave(&host->clk_lock, flags);
	host->clk_gated = false;
	spin_unlock_irqrestore(&host->clk_lock, flags);
}

#else
void mmc_set_ungated(struct mmc_host *host)
{
}
#endif

void mmc_set_bus_mode(struct mmc_host *host, unsigned int mode)
{
	mmc_host_clk_hold(host);
	host->ios.bus_mode = mode;
	mmc_set_ios(host);
	mmc_host_clk_release(host);
}

void mmc_set_bus_width(struct mmc_host *host, unsigned int width)
{
	mmc_host_clk_hold(host);
	host->ios.bus_width = width;
	mmc_set_ios(host);
	mmc_host_clk_release(host);
}

static int mmc_vdd_to_ocrbitnum(int vdd, bool low_bits)
{
	const int max_bit = ilog2(MMC_VDD_35_36);
	int bit;

	if (vdd < 1650 || vdd > 3600)
		return -EINVAL;

	if (vdd >= 1650 && vdd <= 1950)
		return ilog2(MMC_VDD_165_195);

	if (low_bits)
		vdd -= 1;

	
	bit = (vdd - 2000) / 100 + 8;
	if (bit > max_bit)
		return max_bit;
	return bit;
}

u32 mmc_vddrange_to_ocrmask(int vdd_min, int vdd_max)
{
	u32 mask = 0;

	if (vdd_max < vdd_min)
		return 0;

	
	vdd_max = mmc_vdd_to_ocrbitnum(vdd_max, false);
	if (vdd_max < 0)
		return 0;

	
	vdd_min = mmc_vdd_to_ocrbitnum(vdd_min, true);
	if (vdd_min < 0)
		return 0;

	
	while (vdd_max >= vdd_min)
		mask |= 1 << vdd_max--;

	return mask;
}
EXPORT_SYMBOL(mmc_vddrange_to_ocrmask);

#ifdef CONFIG_REGULATOR

int mmc_regulator_get_ocrmask(struct regulator *supply)
{
	int			result = 0;
	int			count;
	int			i;

	count = regulator_count_voltages(supply);
	if (count < 0)
		return count;

	for (i = 0; i < count; i++) {
		int		vdd_uV;
		int		vdd_mV;

		vdd_uV = regulator_list_voltage(supply, i);
		if (vdd_uV <= 0)
			continue;

		vdd_mV = vdd_uV / 1000;
		result |= mmc_vddrange_to_ocrmask(vdd_mV, vdd_mV);
	}

	return result;
}
EXPORT_SYMBOL(mmc_regulator_get_ocrmask);

#if 0
static int __regulator_force_disable(struct regulator *regulator)
{
	int i = 0;
	while(1 == regulator_is_enabled(regulator)) {
		regulator_disable(regulator);
		i++;
	};
	printk("__regulator_force_disable count= %d\n",i);
	return 0;
}
#endif

int mmc_regulator_set_ocr(struct mmc_host *mmc,
			struct regulator *supply,
			unsigned short vdd_bit)
{
	int			result = 0;
	int			min_uV, max_uV;

	if (vdd_bit) {
		int		tmp;
		int		voltage;

		tmp = vdd_bit - ilog2(MMC_VDD_165_195);
		if (tmp == 0) {
			min_uV = 1650 * 1000;
			max_uV = 1950 * 1000;
		} else {
			min_uV = 1900 * 1000 + tmp * 100 * 1000;
			max_uV = min_uV + 100 * 1000;
		}

		voltage = regulator_get_voltage(supply);

		if (mmc->caps2 & MMC_CAP2_BROKEN_VOLTAGE)
			min_uV = max_uV = voltage;

		if (voltage < 0)
			result = voltage;
		else if (voltage < min_uV || voltage > max_uV)
			result = regulator_set_voltage(supply, min_uV, max_uV);
		else
			result = 0;

		if (result == 0 && !mmc->regulator_enabled) {
			result = regulator_enable(supply);
			if (!result)
				mmc->regulator_enabled = true;
		}
	} else if (mmc->regulator_enabled) {
		result = regulator_disable(supply);
		if (result == 0)
			mmc->regulator_enabled = false;
	}

	if (result)
		dev_err(mmc_dev(mmc),
			"could not set regulator OCR (%d)\n", result);
	return result;
}
EXPORT_SYMBOL(mmc_regulator_set_ocr);

#endif 

u32 mmc_select_voltage(struct mmc_host *host, u32 ocr)
{
	int bit;

	ocr &= host->ocr_avail;

	bit = ffs(ocr);
	if (bit) {
		bit -= 1;

		ocr &= 3 << bit;

		mmc_host_clk_hold(host);
		host->ios.vdd = bit;
		mmc_set_ios(host);
		mmc_host_clk_release(host);
	} else {
		pr_warning("%s: host doesn't support card's voltages\n",
				mmc_hostname(host));
		ocr = 0;
	}

	return ocr;
}

int mmc_set_signal_voltage(struct mmc_host *host, int signal_voltage, bool cmd11)
{
	struct mmc_command cmd = {0};
	int err = 0;

	BUG_ON(!host);

	if ((signal_voltage != MMC_SIGNAL_VOLTAGE_330) && cmd11) {
		cmd.opcode = SD_SWITCH_VOLTAGE;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;

		err = mmc_wait_for_cmd(host, &cmd, 0);
		if (err)
			return err;

		if (!mmc_host_is_spi(host) && (cmd.resp[0] & R1_ERROR))
			return -EIO;
	}

	host->ios.signal_voltage = signal_voltage;

	if (host->ops->start_signal_voltage_switch) {
		mmc_host_clk_hold(host);
		err = host->ops->start_signal_voltage_switch(host, &host->ios);
		mmc_host_clk_release(host);
	}

	return err;
}

void mmc_set_timing(struct mmc_host *host, unsigned int timing)
{
	mmc_host_clk_hold(host);
	host->ios.timing = timing;
	mmc_set_ios(host);
	mmc_host_clk_release(host);
}

void mmc_set_driver_type(struct mmc_host *host, unsigned int drv_type)
{
	mmc_host_clk_hold(host);
	host->ios.drv_type = drv_type;
	mmc_set_ios(host);
	mmc_host_clk_release(host);
}

static void mmc_poweroff_notify(struct mmc_host *host)
{
	struct mmc_card *card;
	unsigned int timeout;
	unsigned int notify_type = EXT_CSD_NO_POWER_NOTIFICATION;
	int err = 0;

	card = host->card;
	mmc_claim_host(host);

	if (card && mmc_card_mmc(card) &&
	    (card->poweroff_notify_state == MMC_POWERED_ON)) {

		if (host->power_notify_type == MMC_HOST_PW_NOTIFY_SHORT) {
			notify_type = EXT_CSD_POWER_OFF_SHORT;
			timeout = card->ext_csd.generic_cmd6_time;
			card->poweroff_notify_state = MMC_POWEROFF_SHORT;
		} else {
			notify_type = EXT_CSD_POWER_OFF_LONG;
			timeout = card->ext_csd.power_off_longtime;
			card->poweroff_notify_state = MMC_POWEROFF_LONG;
		}

		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_POWER_OFF_NOTIFICATION,
				 notify_type, timeout);

		if (err && err != -EBADMSG)
			pr_err("Device failed to respond within %d poweroff "
			       "time. Forcefully powering down the device\n",
			       timeout);

		
		card->poweroff_notify_state = MMC_NO_POWER_NOTIFICATION;
	}
	mmc_release_host(host);
}

void mmc_power_up_for_mmc_reinit(struct mmc_host *host)
{
	int bit;
#if 0
	if (host->ios.power_mode == MMC_POWER_ON)
		return;
#endif
	mmc_host_clk_hold(host);

	
	if (host->ocr)
		bit = ffs(host->ocr) - 1;
	else
		bit = fls(host->ocr_avail) - 1;

	host->ios.vdd = bit;
	if (mmc_host_is_spi(host))
		host->ios.chip_select = MMC_CS_HIGH;
	else
		host->ios.chip_select = MMC_CS_DONTCARE;
	host->ios.bus_mode = MMC_BUSMODE_PUSHPULL;
	host->ios.power_mode = MMC_POWER_UP;
	host->ios.bus_width = MMC_BUS_WIDTH_1;
	host->ios.timing = MMC_TIMING_LEGACY;
	mmc_set_ios(host);

	mmc_delay(10);

	host->ios.clock = host->f_init;

	host->ios.power_mode = MMC_POWER_ON;
	mmc_set_ios(host);

	mmc_delay(10);

	mmc_host_clk_release(host);
}

void mmc_power_up(struct mmc_host *host)
{
	int bit;

	if (host->ios.power_mode == MMC_POWER_ON)
		return;

	mmc_host_clk_hold(host);

	
	if (host->ocr)
		bit = ffs(host->ocr) - 1;
	else
		bit = fls(host->ocr_avail) - 1;

	host->ios.vdd = bit;
	if (mmc_host_is_spi(host))
		host->ios.chip_select = MMC_CS_HIGH;
	else
		host->ios.chip_select = MMC_CS_DONTCARE;
	host->ios.bus_mode = MMC_BUSMODE_PUSHPULL;
	host->ios.power_mode = MMC_POWER_UP;
	host->ios.bus_width = MMC_BUS_WIDTH_1;
	host->ios.timing = MMC_TIMING_LEGACY;
	mmc_set_ios(host);

	mmc_delay(10);

	host->ios.clock = host->f_init;

	host->ios.power_mode = MMC_POWER_ON;
	mmc_set_ios(host);

	mmc_delay(10);

	mmc_host_clk_release(host);
}

void mmc_power_off(struct mmc_host *host)
{
	int err = 0;

	if (host->ios.power_mode == MMC_POWER_OFF)
		return;

	mmc_host_clk_hold(host);

	host->ios.clock = 0;
	host->ios.vdd = 0;

	if (host->card && mmc_card_is_sleep(host->card) &&
	    host->bus_ops->resume) {
		err = host->bus_ops->resume(host);

		if (!err)
			mmc_poweroff_notify(host);
		else
			pr_warning("%s: error %d during resume "
				   "(continue with poweroff sequence)\n",
				   mmc_hostname(host), err);
	}

	host->ocr = 1 << (fls(host->ocr_avail) - 1);

	if (!mmc_host_is_spi(host)) {
		host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;
		host->ios.chip_select = MMC_CS_DONTCARE;
	}
	host->ios.power_mode = MMC_POWER_OFF;
	host->ios.bus_width = MMC_BUS_WIDTH_1;
	host->ios.timing = MMC_TIMING_LEGACY;
	mmc_set_ios(host);

	mmc_delay(1);

	mmc_host_clk_release(host);
}

static void __mmc_release_bus(struct mmc_host *host)
{
	BUG_ON(!host);
	BUG_ON(host->bus_refs);
	BUG_ON(!host->bus_dead);

	host->bus_ops = NULL;
}

static inline void mmc_bus_get(struct mmc_host *host)
{
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	host->bus_refs++;
	spin_unlock_irqrestore(&host->lock, flags);
}

static inline void mmc_bus_put(struct mmc_host *host)
{
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	host->bus_refs--;
	if ((host->bus_refs == 0) && host->bus_ops)
		__mmc_release_bus(host);
	spin_unlock_irqrestore(&host->lock, flags);
}

int mmc_resume_bus(struct mmc_host *host)
{
	unsigned long flags;
	int ret = 0;

	if (!mmc_bus_needs_resume(host))
		return -EINVAL;

	printk("%s: Starting deferred resume\n", mmc_hostname(host));
	spin_lock_irqsave(&host->lock, flags);
	host->bus_resume_flags &= ~MMC_BUSRESUME_NEEDS_RESUME;
	host->rescan_disable = 0;
	spin_unlock_irqrestore(&host->lock, flags);

	mmc_bus_get(host);
	if (host->bus_ops && !host->bus_dead) {
		mmc_power_up(host);
		BUG_ON(!host->bus_ops->resume);
		ret = host->bus_ops->resume(host);
	}

#if 0
	if (host->bus_ops->detect && !host->bus_dead)
		host->bus_ops->detect(host);

	mmc_bus_put(host);

#else
	mmc_bus_put(host);
	mmc_detect_change(host, 0);
#endif
	printk("%s: Deferred resume %s\n", mmc_hostname(host),
		(ret == 0 ? "completed" : "Fail"));
	return ret;
}

EXPORT_SYMBOL(mmc_resume_bus);

void mmc_attach_bus(struct mmc_host *host, const struct mmc_bus_ops *ops)
{
	unsigned long flags;

	BUG_ON(!host);
	BUG_ON(!ops);

	WARN_ON(!host->claimed);

	spin_lock_irqsave(&host->lock, flags);

	BUG_ON(host->bus_ops);
	BUG_ON(host->bus_refs);

	host->bus_ops = ops;
	host->bus_refs = 1;
	host->bus_dead = 0;

	spin_unlock_irqrestore(&host->lock, flags);
}

void mmc_detach_bus(struct mmc_host *host)
{
	unsigned long flags;

	BUG_ON(!host);

	WARN_ON(!host->claimed);
	WARN_ON(!host->bus_ops);

	spin_lock_irqsave(&host->lock, flags);

	host->bus_dead = 1;

	spin_unlock_irqrestore(&host->lock, flags);

	mmc_bus_put(host);
}

void mmc_detect_change(struct mmc_host *host, unsigned long delay)
{
#ifdef CONFIG_MMC_DEBUG
	unsigned long flags;
	spin_lock_irqsave(&host->lock, flags);
	WARN_ON(host->removed);
	spin_unlock_irqrestore(&host->lock, flags);
#endif
	host->detect_change = 1;

	wake_lock(&host->detect_wake_lock);
	mmc_schedule_delayed_work(&host->detect, delay);
}

EXPORT_SYMBOL(mmc_detect_change);
int mmc_reinit_card(struct mmc_host *host)
{
	int err = 0;
	printk(KERN_INFO "%s: %s\n", mmc_hostname(host),
		__func__);

	mmc_bus_get(host);
	if (host->bus_ops && !host->bus_dead &&
		host->bus_ops->resume) {
		if (host->card && mmc_card_sd(host->card)) {
			mmc_power_off(host);
			mdelay(10);
		}

		if(host->card && mmc_card_mmc(host->card))
			mmc_power_up_for_mmc_reinit(host);
		else
			mmc_power_up(host);

		err = host->bus_ops->resume(host);
	}

	mmc_bus_put(host);
	printk(KERN_INFO "%s: %s return %d\n", mmc_hostname(host),
		__func__, err);
	return err;
}
EXPORT_SYMBOL(mmc_reinit_card);

void mmc_init_erase(struct mmc_card *card)
{
	unsigned int sz;

	if (is_power_of_2(card->erase_size))
		card->erase_shift = ffs(card->erase_size) - 1;
	else
		card->erase_shift = 0;

	if (mmc_card_sd(card) && card->ssr.au) {
		card->pref_erase = card->ssr.au;
		card->erase_shift = ffs(card->ssr.au) - 1;
	} else if (card->ext_csd.hc_erase_size) {
		card->pref_erase = card->ext_csd.hc_erase_size;
	} else {
		sz = (card->csd.capacity << (card->csd.read_blkbits - 9)) >> 11;
		if (sz < 128)
			card->pref_erase = 512 * 1024 / 512;
		else if (sz < 512)
			card->pref_erase = 1024 * 1024 / 512;
		else if (sz < 1024)
			card->pref_erase = 2 * 1024 * 1024 / 512;
		else
			card->pref_erase = 4 * 1024 * 1024 / 512;
		if (card->pref_erase < card->erase_size)
			card->pref_erase = card->erase_size;
		else {
			sz = card->pref_erase % card->erase_size;
			if (sz)
				card->pref_erase += card->erase_size - sz;
		}
	}
}

static unsigned int mmc_mmc_erase_timeout(struct mmc_card *card,
				          unsigned int arg, unsigned int qty)
{
	unsigned int erase_timeout;

	if (arg == MMC_DISCARD_ARG ||
	    (arg == MMC_TRIM_ARG && card->ext_csd.rev >= 6)) {
		erase_timeout = card->ext_csd.trim_timeout;
	} else if (card->ext_csd.erase_group_def & 1) {
		
		if (arg == MMC_TRIM_ARG)
			erase_timeout = card->ext_csd.trim_timeout;
		else
			erase_timeout = card->ext_csd.hc_erase_timeout;
	} else {
		
		unsigned int mult = (10 << card->csd.r2w_factor);
		unsigned int timeout_clks = card->csd.tacc_clks * mult;
		unsigned int timeout_us;

		
		if (card->csd.tacc_ns < 1000000)
			timeout_us = (card->csd.tacc_ns * mult) / 1000;
		else
			timeout_us = (card->csd.tacc_ns / 1000) * mult;

		timeout_clks <<= 1;
		timeout_us += (timeout_clks * 1000) /
			      (mmc_host_clk_rate(card->host) / 1000);

		erase_timeout = timeout_us / 1000;

		if (!erase_timeout)
			erase_timeout = 1;
	}

	
	if (arg & MMC_SECURE_ARGS) {
		if (arg == MMC_SECURE_ERASE_ARG)
			erase_timeout *= card->ext_csd.sec_erase_mult;
		else
			erase_timeout *= card->ext_csd.sec_trim_mult;
	}

	erase_timeout *= qty;

	if (mmc_host_is_spi(card->host) && erase_timeout < 1000)
		erase_timeout = 1000;

	return erase_timeout;
}

static unsigned int mmc_sd_erase_timeout(struct mmc_card *card,
					 unsigned int arg,
					 unsigned int qty)
{
	unsigned int erase_timeout;

	if (card->ssr.erase_timeout) {
		
		erase_timeout = card->ssr.erase_timeout * qty +
				card->ssr.erase_offset;
	} else {
		erase_timeout = 250 * qty;
	}

	
	if (erase_timeout < 1000)
		erase_timeout = 1000;

	return erase_timeout;
}

static unsigned int mmc_erase_timeout(struct mmc_card *card,
				      unsigned int arg,
				      unsigned int qty)
{
	if (mmc_card_sd(card))
		return mmc_sd_erase_timeout(card, arg, qty);
	else
		return mmc_mmc_erase_timeout(card, arg, qty);
}

static int mmc_do_erase(struct mmc_card *card, unsigned int from,
			unsigned int to, unsigned int arg)
{
	struct mmc_command cmd = {0};
	unsigned int qty = 0;
	int err;

	if (card->erase_shift)
		qty += ((to >> card->erase_shift) -
			(from >> card->erase_shift)) + 1;
	else if (mmc_card_sd(card))
		qty += to - from + 1;
	else
		qty += ((to / card->erase_size) -
			(from / card->erase_size)) + 1;

	if (!mmc_card_blockaddr(card)) {
		from <<= 9;
		to <<= 9;
	}

	if (mmc_card_sd(card))
		cmd.opcode = SD_ERASE_WR_BLK_START;
	else
		cmd.opcode = MMC_ERASE_GROUP_START;
	cmd.arg = from;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err) {
		pr_err("mmc_erase: group start error %d, "
		       "status %#x\n", err, cmd.resp[0]);
		err = -EIO;
		goto out;
	}

	memset(&cmd, 0, sizeof(struct mmc_command));
	if (mmc_card_sd(card))
		cmd.opcode = SD_ERASE_WR_BLK_END;
	else
		cmd.opcode = MMC_ERASE_GROUP_END;
	cmd.arg = to;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err) {
		pr_err("mmc_erase: group end error %d, status %#x\n",
		       err, cmd.resp[0]);
		err = -EIO;
		goto out;
	}

	memset(&cmd, 0, sizeof(struct mmc_command));
	cmd.opcode = MMC_ERASE;
	cmd.arg = arg;
	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	cmd.cmd_timeout_ms = mmc_erase_timeout(card, arg, qty);
	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err) {
		pr_err("mmc_erase: erase error %d, status %#x\n",
		       err, cmd.resp[0]);
		err = -EIO;
		goto out;
	}

	if (mmc_host_is_spi(card->host))
		goto out;

	do {
		memset(&cmd, 0, sizeof(struct mmc_command));
		cmd.opcode = MMC_SEND_STATUS;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
		
		err = mmc_wait_for_cmd(card->host, &cmd, 0);
		if (err || (cmd.resp[0] & 0xFDF92000)) {
			pr_err("error %d requesting status %#x\n",
				err, cmd.resp[0]);
			err = -EIO;
			goto out;
		}
	} while (!(cmd.resp[0] & R1_READY_FOR_DATA) ||
		 R1_CURRENT_STATE(cmd.resp[0]) == R1_STATE_PRG);
out:
	return err;
}

int mmc_erase(struct mmc_card *card, unsigned int from, unsigned int nr,
	      unsigned int arg)
{
	unsigned int rem, to = from + nr;

	if (!(card->host->caps & MMC_CAP_ERASE) ||
	    !(card->csd.cmdclass & CCC_ERASE))
		return -EOPNOTSUPP;

	if (!card->erase_size)
		return -EOPNOTSUPP;

	if (mmc_card_sd(card) && arg != MMC_ERASE_ARG)
		return -EOPNOTSUPP;

	if ((arg & MMC_SECURE_ARGS) &&
	    !(card->ext_csd.sec_feature_support & EXT_CSD_SEC_ER_EN))
		return -EOPNOTSUPP;

	if ((arg & MMC_TRIM_ARGS) &&
	    !(card->ext_csd.sec_feature_support & EXT_CSD_SEC_GB_CL_EN))
		return -EOPNOTSUPP;

	if (arg == MMC_SECURE_ERASE_ARG) {
		if (from % card->erase_size || nr % card->erase_size)
			return -EINVAL;
	}

	if (arg == MMC_ERASE_ARG) {
		rem = from % card->erase_size;
		if (rem) {
			rem = card->erase_size - rem;
			from += rem;
			if (nr > rem)
				nr -= rem;
			else
				return 0;
		}
		rem = nr % card->erase_size;
		if (rem)
			nr -= rem;
	}

	if (nr == 0)
		return 0;

	to = from + nr;

	if (to <= from)
		return -EINVAL;

	
	to -= 1;

	return mmc_do_erase(card, from, to, arg);
}
EXPORT_SYMBOL(mmc_erase);

int mmc_can_erase(struct mmc_card *card)
{
	if ((card->host->caps & MMC_CAP_ERASE) &&
	    (card->csd.cmdclass & CCC_ERASE) && card->erase_size)
		return 1;
	return 0;
}
EXPORT_SYMBOL(mmc_can_erase);

int mmc_can_trim(struct mmc_card *card)
{
	if (card->ext_csd.sec_feature_support & EXT_CSD_SEC_GB_CL_EN)
		return 1;
	return 0;
}
EXPORT_SYMBOL(mmc_can_trim);

int mmc_can_discard(struct mmc_card *card)
{
	if (card->ext_csd.feature_support & MMC_DISCARD_FEATURE)
		return 1;
	return 0;
}
EXPORT_SYMBOL(mmc_can_discard);

int mmc_can_sanitize(struct mmc_card *card)
{
	if (!mmc_can_trim(card) && !mmc_can_erase(card))
		return 0;
	if (!(card->host->caps2 & MMC_CAP2_SANITIZE))
		return 0;
	if (card->ext_csd.sec_feature_support & EXT_CSD_SEC_SANITIZE)
		return 1;
	return 0;
}
EXPORT_SYMBOL(mmc_can_sanitize);

int mmc_can_secure_erase_trim(struct mmc_card *card)
{
	if (card->ext_csd.sec_feature_support & EXT_CSD_SEC_ER_EN)
		return 1;
	return 0;
}
EXPORT_SYMBOL(mmc_can_secure_erase_trim);

int mmc_erase_group_aligned(struct mmc_card *card, unsigned int from,
			    unsigned int nr)
{
	if (!card->erase_size)
		return 0;
	if (from % card->erase_size || nr % card->erase_size)
		return 0;
	return 1;
}
EXPORT_SYMBOL(mmc_erase_group_aligned);

static unsigned int mmc_do_calc_max_discard(struct mmc_card *card,
					    unsigned int arg)
{
	struct mmc_host *host = card->host;
	unsigned int max_discard, x, y, qty = 0, max_qty, timeout;
	unsigned int last_timeout = 0;

	if (card->erase_shift)
		max_qty = UINT_MAX >> card->erase_shift;
	else if (mmc_card_sd(card))
		max_qty = UINT_MAX;
	else
		max_qty = UINT_MAX / card->erase_size;

	
	do {
		y = 0;
		for (x = 1; x && x <= max_qty && max_qty - x >= qty; x <<= 1) {
			timeout = mmc_erase_timeout(card, arg, qty + x);
			if (timeout > host->max_discard_to)
				break;
			if (timeout < last_timeout)
				break;
			last_timeout = timeout;
			y = x;
		}
		qty += y;
	} while (y);

	if (!qty)
		return 0;

	if (qty == 1)
		return 1;

	
	if (card->erase_shift)
		max_discard = --qty << card->erase_shift;
	else if (mmc_card_sd(card))
		max_discard = qty;
	else
		max_discard = --qty * card->erase_size;

	return max_discard;
}

unsigned int mmc_calc_max_discard(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	unsigned int max_discard, max_trim;

	if (!host->max_discard_to)
		return UINT_MAX;

	if (mmc_card_mmc(card) && !(card->ext_csd.erase_group_def & 1))
		return card->pref_erase;

	max_discard = mmc_do_calc_max_discard(card, MMC_ERASE_ARG);
	if (mmc_can_trim(card)) {
		max_trim = mmc_do_calc_max_discard(card, MMC_TRIM_ARG);
		if (max_trim < max_discard)
			max_discard = max_trim;
	} else if (max_discard < card->erase_size) {
		max_discard = 0;
	}
	pr_debug("%s: calculated max. discard sectors %u for timeout %u ms\n",
		 mmc_hostname(host), max_discard, host->max_discard_to);
	return max_discard;
}
EXPORT_SYMBOL(mmc_calc_max_discard);

int mmc_set_blocklen(struct mmc_card *card, unsigned int blocklen)
{
	struct mmc_command cmd = {0};

	if (mmc_card_blockaddr(card) || mmc_card_ddr_mode(card))
		return 0;

	cmd.opcode = MMC_SET_BLOCKLEN;
	cmd.arg = blocklen;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	return mmc_wait_for_cmd(card->host, &cmd, 5);
}
EXPORT_SYMBOL(mmc_set_blocklen);

static void mmc_hw_reset_for_init(struct mmc_host *host)
{
	if (!(host->caps & MMC_CAP_HW_RESET) || !host->ops->hw_reset)
		return;
	mmc_host_clk_hold(host);
	host->ops->hw_reset(host);
	mmc_host_clk_release(host);
}

int mmc_can_reset(struct mmc_card *card)
{

	if (!mmc_card_mmc(card))
		return 0;
#if 0
	rst_n_function = card->ext_csd.rst_n_function;
	if ((rst_n_function & EXT_CSD_RST_N_EN_MASK) != EXT_CSD_RST_N_ENABLED)
		return 0;
	return 1;
#endif
	return 1;
}

EXPORT_SYMBOL(mmc_can_reset);

static int mmc_do_hw_reset(struct mmc_host *host, int check)
{
	struct mmc_card *card = host->card;

	if (!host->bus_ops->power_restore)
		return -EOPNOTSUPP;

	if (!(host->caps & MMC_CAP_HW_RESET) || !host->ops->hw_reset){
		return -EOPNOTSUPP;
	}
	if (!card)
		return -EINVAL;

	if(!mmc_card_sd(card)){
		if (!mmc_can_reset(card))
			return -EOPNOTSUPP;
	}

	mmc_host_clk_hold(host);
	mmc_set_clock(host, host->f_init);

	host->ops->hw_reset(host);

	
	if (check) {
		struct mmc_command cmd = {0};
		int err;

		cmd.opcode = MMC_SEND_STATUS;
		if (!mmc_host_is_spi(card->host))
			cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
		err = mmc_wait_for_cmd(card->host, &cmd, 0);
		if (!err) {
			mmc_host_clk_release(host);
			return -ENOSYS;
		}
	}

	host->card->state &= ~(MMC_STATE_HIGHSPEED | MMC_STATE_HIGHSPEED_DDR);
	if (mmc_host_is_spi(host)) {
		host->ios.chip_select = MMC_CS_HIGH;
		host->ios.bus_mode = MMC_BUSMODE_PUSHPULL;
	} else {
		host->ios.chip_select = MMC_CS_DONTCARE;
		host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;
	}
	host->ios.bus_width = MMC_BUS_WIDTH_1;
	host->ios.timing = MMC_TIMING_LEGACY;
	mmc_set_ios(host);

	mmc_host_clk_release(host);

	return host->bus_ops->power_restore(host);
}

int mmc_hw_reset(struct mmc_host *host)
{
	return mmc_do_hw_reset(host, 0);
}
EXPORT_SYMBOL(mmc_hw_reset);

int mmc_hw_reset_check(struct mmc_host *host)
{
	return mmc_do_hw_reset(host, 1);
}
EXPORT_SYMBOL(mmc_hw_reset_check);

static int mmc_rescan_try_freq(struct mmc_host *host, unsigned freq)
{
	host->f_init = freq;

#ifdef CONFIG_MMC_DEBUG
	pr_info("%s: %s: trying to init card at %u Hz\n",
		mmc_hostname(host), __func__, host->f_init);
#endif
	mmc_power_up(host);

	mmc_hw_reset_for_init(host);

	
	mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_330, 0);

	sdio_reset(host);
	mmc_go_idle(host);

	mmc_send_if_cond(host, host->ocr_avail);

	
	if (!mmc_attach_sdio(host))
		return 0;
	if (!mmc_attach_sd(host))
		return 0;
	if (!mmc_attach_mmc(host))
		return 0;

	mmc_power_off(host);
	return -EIO;
}

int _mmc_detect_card_removed(struct mmc_host *host)
{
	int ret;

	if ((host->caps & MMC_CAP_NONREMOVABLE) || !host->bus_ops->alive)
		return 0;

	if (!host->card || mmc_card_removed(host->card))
		return 1;

       if(mmc_card_sd(host->card) && host->ops->get_cd)
               ret = !host->ops->get_cd(host);
       else
               ret = host->bus_ops->alive(host);
	if (ret) {
		if(mmc_card_sd(host->card))
			host->card->do_remove = 1;
		else
			mmc_card_set_removed(host->card);
		printk(KERN_ERR "%s: card remove detected\n", mmc_hostname(host));
	}

	return ret;
}

void mmc_remove_sd_card(struct work_struct *work)
{
	struct mmc_host *host =
		container_of(work, struct mmc_host, remove.work);
	printk(KERN_INFO "%s: %s\n", mmc_hostname(host),
		__func__);
#if 0
	if (!host->card || host->card->sd_removed) {
		printk(KERN_INFO "%s:%s card already removed\n",
			__func__, mmc_hostname(host));
		return;
	}

	if(!mmc_card_sd(host->card))
		return;
#endif
	wake_lock(&host->detect_wake_lock);
	mmc_bus_get(host);
	if (host->bus_ops && !host->bus_dead) {
		if (host->bus_ops->remove)
			host->bus_ops->remove(host);
		mmc_claim_host(host);
		mmc_detach_bus(host);
		mmc_release_host(host);
	}
	mmc_bus_put(host);
	mmc_power_off(host);
	wake_unlock(&host->detect_wake_lock);
	printk(KERN_INFO "%s: %s exit\n", mmc_hostname(host),
		__func__);

}
EXPORT_SYMBOL(mmc_remove_sd_card);

int mmc_detect_card_removed(struct mmc_host *host)
{
	struct mmc_card *card = host->card;
	int ret;

	WARN_ON(!host->claimed);

	if (!card)
		return 1;

	ret = mmc_card_removed(card);
	if (!host->detect_change && !(host->caps & MMC_CAP_NEEDS_POLL) &&
	    !(host->caps2 & MMC_CAP2_DETECT_ON_ERR))
		return ret;

	host->detect_change = 0;
	if (!ret) {
		ret = _mmc_detect_card_removed(host);
		if (ret && (host->caps2 & MMC_CAP2_DETECT_ON_ERR)) {
			cancel_delayed_work(&host->detect);
			mmc_detect_change(host, 0);
		}
	}

	return ret;
}
EXPORT_SYMBOL(mmc_detect_card_removed);

void mmc_rescan(struct work_struct *work)
{
	static const unsigned freqs[] = { 400000, 300000, 200000, 100000 };
	struct mmc_host *host =
		container_of(work, struct mmc_host, detect.work);
	int i;
	bool extend_wakelock = false;

	if (host->rescan_disable)
		return;

	mmc_bus_get(host);

	if (host->bus_ops && host->bus_ops->detect && !host->bus_dead
	    && !(host->caps & MMC_CAP_NONREMOVABLE))
		host->bus_ops->detect(host);

	host->detect_change = 0;

	if (host->bus_dead)
		extend_wakelock = 1;

	mmc_bus_put(host);
	mmc_bus_get(host);

	
	if (host->bus_ops != NULL) {
		mmc_bus_put(host);
		goto out;
	}

	mmc_bus_put(host);

	if (host->ops->get_cd && host->ops->get_cd(host) == 0)
		goto out;

	mmc_claim_host(host);
	for (i = 0; i < ARRAY_SIZE(freqs); i++) {
		if (!mmc_rescan_try_freq(host, max(freqs[i], host->f_min))) {
			extend_wakelock = true;
			break;
		}
		if (freqs[i] <= host->f_min)
			break;
	}
	mmc_release_host(host);

 out:
	if (extend_wakelock)
		wake_lock_timeout(&host->detect_wake_lock, HZ / 2);
	else
		wake_unlock(&host->detect_wake_lock);
	if (host->caps & MMC_CAP_NEEDS_POLL) {
		wake_lock(&host->detect_wake_lock);
		mmc_schedule_delayed_work(&host->detect, HZ);
	}
}

void mmc_start_host(struct mmc_host *host)
{
	mmc_power_off(host);
	mmc_detect_change(host, 0);
}

void mmc_stop_host(struct mmc_host *host)
{
#ifdef CONFIG_MMC_DEBUG
	unsigned long flags;
	spin_lock_irqsave(&host->lock, flags);
	host->removed = 1;
	spin_unlock_irqrestore(&host->lock, flags);
#endif

	if (cancel_delayed_work_sync(&host->detect))
		wake_unlock(&host->detect_wake_lock);
	mmc_flush_scheduled_work();

	
	host->pm_flags = 0;

	mmc_bus_get(host);
	if (host->bus_ops && !host->bus_dead) {
		
		if (host->bus_ops->remove)
			host->bus_ops->remove(host);

		mmc_claim_host(host);
		mmc_detach_bus(host);
		mmc_power_off(host);
		mmc_release_host(host);
		mmc_bus_put(host);
		return;
	}
	mmc_bus_put(host);

	BUG_ON(host->card);

	mmc_power_off(host);
}

int mmc_power_save_host(struct mmc_host *host)
{
	int ret = 0;

#ifdef CONFIG_MMC_DEBUG
	pr_info("%s: %s: powering down\n", mmc_hostname(host), __func__);
#endif

	mmc_bus_get(host);

	if (!host->bus_ops || host->bus_dead || !host->bus_ops->power_restore) {
		mmc_bus_put(host);
		return -EINVAL;
	}

	if (host->bus_ops->power_save)
		ret = host->bus_ops->power_save(host);

	mmc_bus_put(host);

	mmc_power_off(host);

	return ret;
}
EXPORT_SYMBOL(mmc_power_save_host);

int mmc_power_restore_host(struct mmc_host *host)
{
	int ret;

#ifdef CONFIG_MMC_DEBUG
	pr_info("%s: %s: powering up\n", mmc_hostname(host), __func__);
#endif

	mmc_bus_get(host);

	if (!host->bus_ops || host->bus_dead || !host->bus_ops->power_restore) {
		mmc_bus_put(host);
		return -EINVAL;
	}

	mmc_power_up(host);
	ret = host->bus_ops->power_restore(host);

	mmc_bus_put(host);

	return ret;
}
EXPORT_SYMBOL(mmc_power_restore_host);

int mmc_card_awake(struct mmc_host *host)
{
	int err = -ENOSYS;

	if (host->caps2 & MMC_CAP2_NO_SLEEP_CMD)
		return 0;

	mmc_bus_get(host);

	if (host->bus_ops && !host->bus_dead && host->bus_ops->awake)
		err = host->bus_ops->awake(host);

	mmc_bus_put(host);

	return err;
}
EXPORT_SYMBOL(mmc_card_awake);

int mmc_card_sleep(struct mmc_host *host)
{
	int err = -ENOSYS;

	if (host->caps2 & MMC_CAP2_NO_SLEEP_CMD)
		return 0;

	mmc_bus_get(host);

	if (host->bus_ops && !host->bus_dead && host->bus_ops->sleep)
		err = host->bus_ops->sleep(host);

	mmc_bus_put(host);

	return err;
}
EXPORT_SYMBOL(mmc_card_sleep);

int mmc_card_can_sleep(struct mmc_host *host)
{
	struct mmc_card *card = host->card;

	if (card && mmc_card_mmc(card) && card->ext_csd.rev >= 3)
		return 1;
	return 0;
}
EXPORT_SYMBOL(mmc_card_can_sleep);

int mmc_flush_cache(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	int err = 0;

	if (!(host->caps2 & MMC_CAP2_CACHE_CTRL))
		return err;

	if (mmc_card_mmc(card) &&
			(card->ext_csd.cache_size > 0) &&
			(card->ext_csd.cache_ctrl & 1)) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_FLUSH_CACHE, 1, 0);
		if (err)
			pr_err("%s: cache flush error %d\n",
					mmc_hostname(card->host), err);
	}

	return err;
}
EXPORT_SYMBOL(mmc_flush_cache);

int mmc_cache_ctrl(struct mmc_host *host, u8 enable)
{
	struct mmc_card *card = host->card;
	unsigned int timeout;
	int err = 0;

	if (!(host->caps2 & MMC_CAP2_CACHE_CTRL) ||
			mmc_card_is_removable(host))
		return err;

	mmc_claim_host(host);
	if (card && mmc_card_mmc(card) &&
			(card->ext_csd.cache_size > 0)) {
		enable = !!enable;

		if (card->ext_csd.cache_ctrl ^ enable) {
			timeout = enable ? card->ext_csd.generic_cmd6_time : 0;
			err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_CACHE_CTRL, enable, timeout);
			if (err)
				pr_err("%s: cache %s error %d\n",
						mmc_hostname(card->host),
						enable ? "on" : "off",
						err);
			else
				card->ext_csd.cache_ctrl = enable;
		}
	}
	mmc_release_host(host);

	return err;
}
EXPORT_SYMBOL(mmc_cache_ctrl);

#ifdef CONFIG_PM

int mmc_suspend_host(struct mmc_host *host)
{
	int err = 0;

	if (mmc_bus_needs_resume(host))
		return 0;

	if (cancel_delayed_work(&host->detect))
		wake_unlock(&host->detect_wake_lock);
	mmc_flush_scheduled_work();

	if(work_busy(&host->disable.work))
		printk("%s, %s, disable work  busy\n",  mmc_hostname(host), __func__ );
	else{
		printk("%s, %s, disable work  idle\n",  mmc_hostname(host), __func__ );
		mmc_flush_scheduled_work();
	}
	mmc_claim_host(host);
	err = mmc_cache_ctrl(host, 0);
	mmc_release_host(host);

	if (err)
		goto out;

	mmc_bus_get(host);
	if (host->bus_ops && !host->bus_dead) {

		if (host->bus_ops->suspend)
			err = host->bus_ops->suspend(host);

		if (err == -ENOSYS || !host->bus_ops->resume) {
			if (host->bus_ops->remove)
				host->bus_ops->remove(host);
			mmc_claim_host(host);
			mmc_detach_bus(host);
			mmc_power_off(host);
			mmc_release_host(host);
			host->pm_flags = 0;
			err = 0;
		}
	}
	mmc_bus_put(host);

	if (!err && !mmc_card_keep_power(host)){
		mmc_power_off(host);
	}
out:
	return err;
}

EXPORT_SYMBOL(mmc_suspend_host);

int mmc_resume_host(struct mmc_host *host)
{
	int err = 0;

	mmc_bus_get(host);
	if (mmc_bus_manual_resume(host)) {
		host->bus_resume_flags |= MMC_BUSRESUME_NEEDS_RESUME;
		mmc_bus_put(host);
		return 0;
	}

	if (host->bus_ops && !host->bus_dead) {
		if (!mmc_card_keep_power(host)) {
			mmc_power_up(host);
			mmc_select_voltage(host, host->ocr);
			if (mmc_card_sdio(host->card) &&
			    (host->caps & MMC_CAP_POWER_OFF_CARD)) {
				pm_runtime_disable(&host->card->dev);
				pm_runtime_set_active(&host->card->dev);
				pm_runtime_enable(&host->card->dev);
			}
		}
		BUG_ON(!host->bus_ops->resume);
		err = host->bus_ops->resume(host);
		if (err) {
			pr_warning("%s: error %d during resume "
					    "(card was removed?)\n",
					    mmc_hostname(host), err);
			
		}
	}
	mmc_bus_put(host);

	return err;
}
EXPORT_SYMBOL(mmc_resume_host);

int mmc_pm_notify(struct notifier_block *notify_block,
					unsigned long mode, void *unused)
{
	struct mmc_host *host = container_of(
		notify_block, struct mmc_host, pm_notify);
	unsigned long flags;


	switch (mode) {
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:

		spin_lock_irqsave(&host->lock, flags);
		if (mmc_bus_needs_resume(host)) {
			spin_unlock_irqrestore(&host->lock, flags);
			break;
		}
		host->rescan_disable = 1;
		host->power_notify_type = MMC_HOST_PW_NOTIFY_SHORT;
		spin_unlock_irqrestore(&host->lock, flags);
		if (cancel_delayed_work_sync(&host->detect))
			wake_unlock(&host->detect_wake_lock);

		if (!host->bus_ops || host->bus_ops->suspend)
			break;

		
		if (host->bus_ops->remove)
			host->bus_ops->remove(host);

		mmc_claim_host(host);
		mmc_detach_bus(host);
		mmc_power_off(host);
		mmc_release_host(host);
		host->pm_flags = 0;
		break;

	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:

		spin_lock_irqsave(&host->lock, flags);
		if (mmc_bus_manual_resume(host)) {
			spin_unlock_irqrestore(&host->lock, flags);
			break;
		}
		host->rescan_disable = 0;
		host->power_notify_type = MMC_HOST_PW_NOTIFY_LONG;
		spin_unlock_irqrestore(&host->lock, flags);
		mmc_detect_change(host, 0);

	}

	return 0;
}
#endif

#ifdef CONFIG_MMC_EMBEDDED_SDIO
void mmc_set_embedded_sdio_data(struct mmc_host *host,
				struct sdio_cis *cis,
				struct sdio_cccr *cccr,
				struct sdio_embedded_func *funcs,
				int num_funcs)
{
	host->embedded_sdio_data.cis = cis;
	host->embedded_sdio_data.cccr = cccr;
	host->embedded_sdio_data.funcs = funcs;
	host->embedded_sdio_data.num_funcs = num_funcs;
}

EXPORT_SYMBOL(mmc_set_embedded_sdio_data);
#endif

static int __init mmc_init(void)
{
	int ret;

	workqueue = alloc_ordered_workqueue("kmmcd", 0);
	if (!workqueue)
		return -ENOMEM;

	ret = mmc_register_bus();
	if (ret)
		goto destroy_workqueue;

	ret = mmc_register_host_class();
	if (ret)
		goto unregister_bus;

	ret = sdio_register_bus();
	if (ret)
		goto unregister_host_class;

	return 0;

unregister_host_class:
	mmc_unregister_host_class();
unregister_bus:
	mmc_unregister_bus();
destroy_workqueue:
	destroy_workqueue(workqueue);

	return ret;
}

static void __exit mmc_exit(void)
{
	sdio_unregister_bus();
	mmc_unregister_host_class();
	mmc_unregister_bus();
	destroy_workqueue(workqueue);
}

subsys_initcall(mmc_init);
module_exit(mmc_exit);

MODULE_LICENSE("GPL");
