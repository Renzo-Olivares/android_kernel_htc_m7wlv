/*
 * drivers/base/power/main.c - Where the driver meets power management.
 *
 * Copyright (c) 2003 Patrick Mochel
 * Copyright (c) 2003 Open Source Development Lab
 *
 * This file is released under the GPLv2
 *
 *
 * The driver model core calls device_pm_add() when a device is registered.
 * This will initialize the embedded device_pm_info object in the device
 * and add it to the list of power-controlled devices. sysfs entries for
 * controlling device power management will also be added.
 *
 * A separate list is used for keeping track of power info, because the power
 * domain dependencies may differ from the ancestral dependencies that the
 * subsystem list maintains.
 */

#include <linux/device.h>
#include <linux/kallsyms.h>
#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/resume-trace.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/async.h>
#include <linux/suspend.h>
#include <linux/timer.h>

#include "../base.h"
#include "power.h"

typedef int (*pm_callback_t)(struct device *);


LIST_HEAD(dpm_list);
LIST_HEAD(dpm_prepared_list);
LIST_HEAD(dpm_suspended_list);
LIST_HEAD(dpm_late_early_list);
LIST_HEAD(dpm_noirq_list);

struct suspend_stats suspend_stats;
static DEFINE_MUTEX(dpm_list_mtx);
static pm_message_t pm_transition;

struct dpm_watchdog {
	struct device		*dev;
	struct task_struct	*tsk;
	struct timer_list	timer;
};

static int async_error;

bool dpm_enable_debug = false;

void device_pm_init(struct device *dev)
{
	dev->power.is_prepared = false;
	dev->power.is_suspended = false;
	init_completion(&dev->power.completion);
	complete_all(&dev->power.completion);
	dev->power.wakeup = NULL;
	spin_lock_init(&dev->power.lock);
	pm_runtime_init(dev);
	INIT_LIST_HEAD(&dev->power.entry);
	dev->power.power_state = PMSG_INVALID;
}

void device_pm_lock(void)
{
	mutex_lock(&dpm_list_mtx);
}

void device_pm_unlock(void)
{
	mutex_unlock(&dpm_list_mtx);
}

void device_pm_add(struct device *dev)
{
	pr_debug("PM: Adding info for %s:%s\n",
		 dev->bus ? dev->bus->name : "No Bus", dev_name(dev));
	mutex_lock(&dpm_list_mtx);
	if (dev->parent && dev->parent->power.is_prepared)
		dev_warn(dev, "parent %s should not be sleeping\n",
			dev_name(dev->parent));
	list_add_tail(&dev->power.entry, &dpm_list);
	dev_pm_qos_constraints_init(dev);
	mutex_unlock(&dpm_list_mtx);
}

void device_pm_remove(struct device *dev)
{
	pr_debug("PM: Removing info for %s:%s\n",
		 dev->bus ? dev->bus->name : "No Bus", dev_name(dev));
	complete_all(&dev->power.completion);
	mutex_lock(&dpm_list_mtx);
	dev_pm_qos_constraints_destroy(dev);
	list_del_init(&dev->power.entry);
	mutex_unlock(&dpm_list_mtx);
	device_wakeup_disable(dev);
	pm_runtime_remove(dev);
}

void device_pm_move_before(struct device *deva, struct device *devb)
{
	pr_debug("PM: Moving %s:%s before %s:%s\n",
		 deva->bus ? deva->bus->name : "No Bus", dev_name(deva),
		 devb->bus ? devb->bus->name : "No Bus", dev_name(devb));
	
	list_move_tail(&deva->power.entry, &devb->power.entry);
}

void device_pm_move_after(struct device *deva, struct device *devb)
{
	pr_debug("PM: Moving %s:%s after %s:%s\n",
		 deva->bus ? deva->bus->name : "No Bus", dev_name(deva),
		 devb->bus ? devb->bus->name : "No Bus", dev_name(devb));
	
	list_move(&deva->power.entry, &devb->power.entry);
}

void device_pm_move_last(struct device *dev)
{
	pr_debug("PM: Moving %s:%s to end of list\n",
		 dev->bus ? dev->bus->name : "No Bus", dev_name(dev));
	list_move_tail(&dev->power.entry, &dpm_list);
}

static ktime_t initcall_debug_start(struct device *dev)
{
	ktime_t calltime = ktime_set(0, 0);

	if (initcall_debug) {
		pr_info("calling  %s+ @ %i, parent: %s\n",
			dev_name(dev), task_pid_nr(current),
			dev->parent ? dev_name(dev->parent) : "none");
		calltime = ktime_get();
	}

	return calltime;
}

static void initcall_debug_report(struct device *dev, ktime_t calltime,
				  int error)
{
	ktime_t delta, rettime;

	if (initcall_debug) {
		rettime = ktime_get();
		delta = ktime_sub(rettime, calltime);
		pr_info("call %s+ returned %d after %Ld usecs\n", dev_name(dev),
			error, (unsigned long long)ktime_to_ns(delta) >> 10);
	}
}

static void dpm_wait(struct device *dev, bool async)
{
	if (!dev)
		return;

	if (async || (pm_async_enabled && dev->power.async_suspend))
		wait_for_completion(&dev->power.completion);
}

static int dpm_wait_fn(struct device *dev, void *async_ptr)
{
	dpm_wait(dev, *((bool *)async_ptr));
	return 0;
}

static void dpm_wait_for_children(struct device *dev, bool async)
{
       device_for_each_child(dev, &async, dpm_wait_fn);
}

static pm_callback_t pm_op(const struct dev_pm_ops *ops, pm_message_t state)
{
	switch (state.event) {
#ifdef CONFIG_SUSPEND
	case PM_EVENT_SUSPEND:
		return ops->suspend;
	case PM_EVENT_RESUME:
		return ops->resume;
#endif 
#ifdef CONFIG_HIBERNATE_CALLBACKS
	case PM_EVENT_FREEZE:
	case PM_EVENT_QUIESCE:
		return ops->freeze;
	case PM_EVENT_HIBERNATE:
		return ops->poweroff;
	case PM_EVENT_THAW:
	case PM_EVENT_RECOVER:
		return ops->thaw;
		break;
	case PM_EVENT_RESTORE:
		return ops->restore;
#endif 
	}

	return NULL;
}

static pm_callback_t pm_late_early_op(const struct dev_pm_ops *ops,
				      pm_message_t state)
{
	switch (state.event) {
#ifdef CONFIG_SUSPEND
	case PM_EVENT_SUSPEND:
		return ops->suspend_late;
	case PM_EVENT_RESUME:
		return ops->resume_early;
#endif 
#ifdef CONFIG_HIBERNATE_CALLBACKS
	case PM_EVENT_FREEZE:
	case PM_EVENT_QUIESCE:
		return ops->freeze_late;
	case PM_EVENT_HIBERNATE:
		return ops->poweroff_late;
	case PM_EVENT_THAW:
	case PM_EVENT_RECOVER:
		return ops->thaw_early;
	case PM_EVENT_RESTORE:
		return ops->restore_early;
#endif 
	}

	return NULL;
}

static pm_callback_t pm_noirq_op(const struct dev_pm_ops *ops, pm_message_t state)
{
	switch (state.event) {
#ifdef CONFIG_SUSPEND
	case PM_EVENT_SUSPEND:
		return ops->suspend_noirq;
	case PM_EVENT_RESUME:
		return ops->resume_noirq;
#endif 
#ifdef CONFIG_HIBERNATE_CALLBACKS
	case PM_EVENT_FREEZE:
	case PM_EVENT_QUIESCE:
		return ops->freeze_noirq;
	case PM_EVENT_HIBERNATE:
		return ops->poweroff_noirq;
	case PM_EVENT_THAW:
	case PM_EVENT_RECOVER:
		return ops->thaw_noirq;
	case PM_EVENT_RESTORE:
		return ops->restore_noirq;
#endif 
	}

	return NULL;
}

static char *pm_verb(int event)
{
	switch (event) {
	case PM_EVENT_SUSPEND:
		return "suspend";
	case PM_EVENT_RESUME:
		return "resume";
	case PM_EVENT_FREEZE:
		return "freeze";
	case PM_EVENT_QUIESCE:
		return "quiesce";
	case PM_EVENT_HIBERNATE:
		return "hibernate";
	case PM_EVENT_THAW:
		return "thaw";
	case PM_EVENT_RESTORE:
		return "restore";
	case PM_EVENT_RECOVER:
		return "recover";
	default:
		return "(unknown PM event)";
	}
}

static void pm_dev_dbg(struct device *dev, pm_message_t state, char *info)
{
	dev_dbg(dev, "%s%s%s\n", info, pm_verb(state.event),
		((state.event & PM_EVENT_SLEEP) && device_may_wakeup(dev)) ?
		", may wakeup" : "");
}

static void pm_dev_err(struct device *dev, pm_message_t state, char *info,
			int error)
{
	printk(KERN_ERR "PM: Device %s failed to %s%s: error %d\n",
		dev_name(dev), pm_verb(state.event), info, error);
}

static void dpm_show_time(ktime_t starttime, pm_message_t state, char *info)
{
	ktime_t calltime;
	u64 usecs64;
	int usecs;

	calltime = ktime_get();
	usecs64 = ktime_to_ns(ktime_sub(calltime, starttime));
	do_div(usecs64, NSEC_PER_USEC);
	usecs = usecs64;
	if (usecs == 0)
		usecs = 1;
	pr_info("PM: %s%s%s of devices complete after %ld.%03ld msecs\n",
		info ?: "", info ? " " : "", pm_verb(state.event),
		usecs / USEC_PER_MSEC, usecs % USEC_PER_MSEC);
}

static int dpm_run_callback(pm_callback_t cb, struct device *dev,
			    pm_message_t state, char *info)
{
	ktime_t calltime;
	int error;

	if (!cb)
		return 0;

	calltime = initcall_debug_start(dev);

	pm_dev_dbg(dev, state, info);
	error = cb(dev);
	suspend_report_result(cb, error);

	initcall_debug_report(dev, calltime, error);

	return error;
}

static void dpm_wd_handler(unsigned long data)
{
	struct dpm_watchdog *wd = (void *)data;
	struct device *dev      = wd->dev;
	struct task_struct *tsk = wd->tsk;

	dev_emerg(dev, "**** DPM device timeout ****\n");
	show_stack(tsk, NULL);

	BUG();
}

static void dpm_wd_set(struct dpm_watchdog *wd, struct device *dev)
{
	struct timer_list *timer = &wd->timer;

	wd->dev = dev;
	wd->tsk = get_current();

	init_timer_on_stack(timer);
	timer->expires = jiffies + HZ * 12;
	timer->function = dpm_wd_handler;
	timer->data = (unsigned long)wd;
	add_timer(timer);
}

static void dpm_wd_clear(struct dpm_watchdog *wd)
{
	struct timer_list *timer = &wd->timer;

	del_timer_sync(timer);
	destroy_timer_on_stack(timer);
}


static int device_resume_noirq(struct device *dev, pm_message_t state)
{
	pm_callback_t callback = NULL;
	char *info = NULL;
	int error = 0;

	TRACE_DEVICE(dev);
	TRACE_RESUME(0);

	if (dev->pm_domain) {
		info = "noirq power domain ";
		callback = pm_noirq_op(&dev->pm_domain->ops, state);
	} else if (dev->type && dev->type->pm) {
		info = "noirq type ";
		callback = pm_noirq_op(dev->type->pm, state);
	} else if (dev->class && dev->class->pm) {
		info = "noirq class ";
		callback = pm_noirq_op(dev->class->pm, state);
	} else if (dev->bus && dev->bus->pm) {
		info = "noirq bus ";
		callback = pm_noirq_op(dev->bus->pm, state);
	}

	if (!callback && dev->driver && dev->driver->pm) {
		info = "noirq driver ";
		callback = pm_noirq_op(dev->driver->pm, state);
	}

	error = dpm_run_callback(callback, dev, state, info);
	if (dpm_enable_debug && callback) {
		printk("-------- resume %s  %pf with %d\n", dev->kobj.name, callback, error);
	}

	TRACE_RESUME(error);
	return error;
}

static void dpm_resume_noirq(pm_message_t state)
{
	ktime_t starttime = ktime_get();

	mutex_lock(&dpm_list_mtx);
	while (!list_empty(&dpm_noirq_list)) {
		struct device *dev = to_device(dpm_noirq_list.next);
		int error;

		get_device(dev);
		list_move_tail(&dev->power.entry, &dpm_late_early_list);
		mutex_unlock(&dpm_list_mtx);

		error = device_resume_noirq(dev, state);
		if (error) {
			suspend_stats.failed_resume_noirq++;
			dpm_save_failed_step(SUSPEND_RESUME_NOIRQ);
			dpm_save_failed_dev(dev_name(dev));
			pm_dev_err(dev, state, " noirq", error);
		}

		mutex_lock(&dpm_list_mtx);
		put_device(dev);
	}
	mutex_unlock(&dpm_list_mtx);
	dpm_show_time(starttime, state, "noirq");
	resume_device_irqs();
}

static int device_resume_early(struct device *dev, pm_message_t state)
{
	pm_callback_t callback = NULL;
	char *info = NULL;
	int error = 0;

	TRACE_DEVICE(dev);
	TRACE_RESUME(0);

	if (dev->pm_domain) {
		info = "early power domain ";
		callback = pm_late_early_op(&dev->pm_domain->ops, state);
	} else if (dev->type && dev->type->pm) {
		info = "early type ";
		callback = pm_late_early_op(dev->type->pm, state);
	} else if (dev->class && dev->class->pm) {
		info = "early class ";
		callback = pm_late_early_op(dev->class->pm, state);
	} else if (dev->bus && dev->bus->pm) {
		info = "early bus ";
		callback = pm_late_early_op(dev->bus->pm, state);
	}

	if (!callback && dev->driver && dev->driver->pm) {
		info = "early driver ";
		callback = pm_late_early_op(dev->driver->pm, state);
	}

	error = dpm_run_callback(callback, dev, state, info);
	if (dpm_enable_debug && callback) {
		printk("-------- resume %s %pf with %d\n", dev->kobj.name, callback, error);
	}

	TRACE_RESUME(error);
	return error;
}

static void dpm_resume_early(pm_message_t state)
{
	ktime_t starttime = ktime_get();

	mutex_lock(&dpm_list_mtx);
	while (!list_empty(&dpm_late_early_list)) {
		struct device *dev = to_device(dpm_late_early_list.next);
		int error;

		get_device(dev);
		list_move_tail(&dev->power.entry, &dpm_suspended_list);
		mutex_unlock(&dpm_list_mtx);

		error = device_resume_early(dev, state);
		if (error) {
			suspend_stats.failed_resume_early++;
			dpm_save_failed_step(SUSPEND_RESUME_EARLY);
			dpm_save_failed_dev(dev_name(dev));
			pm_dev_err(dev, state, " early", error);
		}

		mutex_lock(&dpm_list_mtx);
		put_device(dev);
	}
	mutex_unlock(&dpm_list_mtx);
	dpm_show_time(starttime, state, "early");
}

void dpm_resume_start(pm_message_t state)
{
	dpm_resume_noirq(state);
	dpm_resume_early(state);
}
EXPORT_SYMBOL_GPL(dpm_resume_start);

static int device_resume(struct device *dev, pm_message_t state, bool async)
{
	pm_callback_t callback = NULL;
	char *info = NULL;
	int error = 0;
	bool put = false;
	struct dpm_watchdog wd;

	TRACE_DEVICE(dev);
	TRACE_RESUME(0);

	dpm_wait(dev->parent, async);
	device_lock(dev);

	dev->power.is_prepared = false;
	dpm_wd_set(&wd, dev);

	if (!dev->power.is_suspended)
		goto Unlock;

	pm_runtime_enable(dev);
	put = true;

	if (dev->pm_domain) {
		info = "power domain ";
		callback = pm_op(&dev->pm_domain->ops, state);
		goto Driver;
	}

	if (dev->type && dev->type->pm) {
		info = "type ";
		callback = pm_op(dev->type->pm, state);
		goto Driver;
	}

	if (dev->class) {
		if (dev->class->pm) {
			info = "class ";
			callback = pm_op(dev->class->pm, state);
			goto Driver;
		} else if (dev->class->resume) {
			info = "legacy class ";
			callback = dev->class->resume;
			goto End;
		}
	}

	if (dev->bus) {
		if (dev->bus->pm) {
			info = "bus ";
			callback = pm_op(dev->bus->pm, state);
		} else if (dev->bus->resume) {
			info = "legacy bus ";
			callback = dev->bus->resume;
			goto End;
		}
	}

 Driver:
	if (!callback && dev->driver && dev->driver->pm) {
		info = "driver ";
		callback = pm_op(dev->driver->pm, state);
	}

 End:
	error = dpm_run_callback(callback, dev, state, info);
	dev->power.is_suspended = false;
	if (dpm_enable_debug && callback) {
		printk("-------- resume %s %pf with %d\n", dev->kobj.name, callback, error);
	}

 Unlock:
	device_unlock(dev);
	dpm_wd_clear(&wd);
	complete_all(&dev->power.completion);

	TRACE_RESUME(error);

	if (put)
		pm_runtime_put_sync(dev);

	return error;
}

static void async_resume(void *data, async_cookie_t cookie)
{
	struct device *dev = (struct device *)data;
	int error;

	error = device_resume(dev, pm_transition, true);
	if (error)
		pm_dev_err(dev, pm_transition, " async", error);
	put_device(dev);
}

static bool is_async(struct device *dev)
{
	return dev->power.async_suspend && pm_async_enabled
		&& !pm_trace_is_enabled();
}

void dpm_resume(pm_message_t state)
{
	struct device *dev;
	ktime_t starttime = ktime_get();

	might_sleep();

	mutex_lock(&dpm_list_mtx);
	pm_transition = state;
	async_error = 0;

	list_for_each_entry(dev, &dpm_suspended_list, power.entry) {
		INIT_COMPLETION(dev->power.completion);
		if (is_async(dev)) {
			get_device(dev);
			async_schedule(async_resume, dev);
		}
	}

	while (!list_empty(&dpm_suspended_list)) {
		dev = to_device(dpm_suspended_list.next);
		get_device(dev);
		if (!is_async(dev)) {
			int error;

			mutex_unlock(&dpm_list_mtx);

			error = device_resume(dev, state, false);
			if (error) {
				suspend_stats.failed_resume++;
				dpm_save_failed_step(SUSPEND_RESUME);
				dpm_save_failed_dev(dev_name(dev));
				pm_dev_err(dev, state, "", error);
			}

			mutex_lock(&dpm_list_mtx);
		}
		if (!list_empty(&dev->power.entry))
			list_move_tail(&dev->power.entry, &dpm_prepared_list);
		put_device(dev);
	}
	mutex_unlock(&dpm_list_mtx);
	async_synchronize_full();
	dpm_show_time(starttime, state, NULL);
}

static void device_complete(struct device *dev, pm_message_t state)
{
	void (*callback)(struct device *) = NULL;
	char *info = NULL;

	device_lock(dev);

	if (dev->pm_domain) {
		info = "completing power domain ";
		callback = dev->pm_domain->ops.complete;
	} else if (dev->type && dev->type->pm) {
		info = "completing type ";
		callback = dev->type->pm->complete;
	} else if (dev->class && dev->class->pm) {
		info = "completing class ";
		callback = dev->class->pm->complete;
	} else if (dev->bus && dev->bus->pm) {
		info = "completing bus ";
		callback = dev->bus->pm->complete;
	}

	if (!callback && dev->driver && dev->driver->pm) {
		info = "completing driver ";
		callback = dev->driver->pm->complete;
	}

	if (callback) {
		pm_dev_dbg(dev, state, info);
		callback(dev);
	}

	device_unlock(dev);
}

void dpm_complete(pm_message_t state)
{
	struct list_head list;

	might_sleep();

	INIT_LIST_HEAD(&list);
	mutex_lock(&dpm_list_mtx);
	while (!list_empty(&dpm_prepared_list)) {
		struct device *dev = to_device(dpm_prepared_list.prev);

		get_device(dev);
		dev->power.is_prepared = false;
		list_move(&dev->power.entry, &list);
		mutex_unlock(&dpm_list_mtx);

		device_complete(dev, state);

		mutex_lock(&dpm_list_mtx);
		put_device(dev);
	}
	list_splice(&list, &dpm_list);
	mutex_unlock(&dpm_list_mtx);
}

void dpm_resume_end(pm_message_t state)
{
	dpm_resume(state);
	dpm_complete(state);
}
EXPORT_SYMBOL_GPL(dpm_resume_end);



static pm_message_t resume_event(pm_message_t sleep_state)
{
	switch (sleep_state.event) {
	case PM_EVENT_SUSPEND:
		return PMSG_RESUME;
	case PM_EVENT_FREEZE:
	case PM_EVENT_QUIESCE:
		return PMSG_RECOVER;
	case PM_EVENT_HIBERNATE:
		return PMSG_RESTORE;
	}
	return PMSG_ON;
}

static int device_suspend_noirq(struct device *dev, pm_message_t state)
{
	pm_callback_t callback = NULL;
	char *info = NULL;
	int error = 0;

	if (dev->pm_domain) {
		info = "noirq power domain ";
		callback = pm_noirq_op(&dev->pm_domain->ops, state);
	} else if (dev->type && dev->type->pm) {
		info = "noirq type ";
		callback = pm_noirq_op(dev->type->pm, state);
	} else if (dev->class && dev->class->pm) {
		info = "noirq class ";
		callback = pm_noirq_op(dev->class->pm, state);
	} else if (dev->bus && dev->bus->pm) {
		info = "noirq bus ";
		callback = pm_noirq_op(dev->bus->pm, state);
	}

	if (!callback && dev->driver && dev->driver->pm) {
		info = "noirq driver ";
		callback = pm_noirq_op(dev->driver->pm, state);
	}
	error = dpm_run_callback(callback, dev, state, info);
	if (dpm_enable_debug && callback) {
		printk("-------- suspend %s %pf with %d\n", dev->kobj.name, callback, error);
	}

	return error;
}

static int dpm_suspend_noirq(pm_message_t state)
{
	ktime_t starttime = ktime_get();
	int error = 0;

	suspend_device_irqs();
	mutex_lock(&dpm_list_mtx);
	while (!list_empty(&dpm_late_early_list)) {
		struct device *dev = to_device(dpm_late_early_list.prev);

		get_device(dev);
		mutex_unlock(&dpm_list_mtx);

		error = device_suspend_noirq(dev, state);

		mutex_lock(&dpm_list_mtx);
		if (error) {
			pm_dev_err(dev, state, " noirq", error);
			suspend_stats.failed_suspend_noirq++;
			dpm_save_failed_step(SUSPEND_SUSPEND_NOIRQ);
			dpm_save_failed_dev(dev_name(dev));
			put_device(dev);
			break;
		}
		if (!list_empty(&dev->power.entry))
			list_move(&dev->power.entry, &dpm_noirq_list);
		put_device(dev);

		if (pm_wakeup_pending()) {
			error = -EBUSY;
			break;
		}
	}
	mutex_unlock(&dpm_list_mtx);
	if (error)
		dpm_resume_noirq(resume_event(state));
	else
		dpm_show_time(starttime, state, "noirq");
	return error;
}

static int device_suspend_late(struct device *dev, pm_message_t state)
{
	pm_callback_t callback = NULL;
	char *info = NULL;
	int error = 0;

	if (dev->pm_domain) {
		info = "late power domain ";
		callback = pm_late_early_op(&dev->pm_domain->ops, state);
	} else if (dev->type && dev->type->pm) {
		info = "late type ";
		callback = pm_late_early_op(dev->type->pm, state);
	} else if (dev->class && dev->class->pm) {
		info = "late class ";
		callback = pm_late_early_op(dev->class->pm, state);
	} else if (dev->bus && dev->bus->pm) {
		info = "late bus ";
		callback = pm_late_early_op(dev->bus->pm, state);
	}

	if (!callback && dev->driver && dev->driver->pm) {
		info = "late driver ";
		callback = pm_late_early_op(dev->driver->pm, state);
	}
	error = dpm_run_callback(callback, dev, state, info);
	if (dpm_enable_debug && callback) {
		printk("-------- suspend %s %pf with %d\n", dev->kobj.name, callback, error);
	}

	return error;
}

static int dpm_suspend_late(pm_message_t state)
{
	ktime_t starttime = ktime_get();
	int error = 0;

	mutex_lock(&dpm_list_mtx);
	while (!list_empty(&dpm_suspended_list)) {
		struct device *dev = to_device(dpm_suspended_list.prev);

		get_device(dev);
		mutex_unlock(&dpm_list_mtx);

		error = device_suspend_late(dev, state);

		mutex_lock(&dpm_list_mtx);
		if (error) {
			pm_dev_err(dev, state, " late", error);
			suspend_stats.failed_suspend_late++;
			dpm_save_failed_step(SUSPEND_SUSPEND_LATE);
			dpm_save_failed_dev(dev_name(dev));
			put_device(dev);
			break;
		}
		if (!list_empty(&dev->power.entry))
			list_move(&dev->power.entry, &dpm_late_early_list);
		put_device(dev);

		if (pm_wakeup_pending()) {
			error = -EBUSY;
			break;
		}
	}
	mutex_unlock(&dpm_list_mtx);
	if (error)
		dpm_resume_early(resume_event(state));
	else
		dpm_show_time(starttime, state, "late");

	return error;
}

int dpm_suspend_end(pm_message_t state)
{
	int error = dpm_suspend_late(state);
	if (error)
		return error;

	error = dpm_suspend_noirq(state);
	if (error) {
		dpm_resume_early(state);
		return error;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(dpm_suspend_end);

static int legacy_suspend(struct device *dev, pm_message_t state,
			  int (*cb)(struct device *dev, pm_message_t state))
{
	int error;
	ktime_t calltime;

	calltime = initcall_debug_start(dev);

	error = cb(dev, state);
	suspend_report_result(cb, error);

	initcall_debug_report(dev, calltime, error);

	return error;
}

static int __device_suspend(struct device *dev, pm_message_t state, bool async)
{
	pm_callback_t callback = NULL;
	char *info = NULL;
	int error = 0;
	struct dpm_watchdog wd;

	dpm_wait_for_children(dev, async);

	if (async_error)
		goto Complete;

	pm_runtime_get_noresume(dev);
	if (pm_runtime_barrier(dev) && device_may_wakeup(dev))
		pm_wakeup_event(dev, 0);

	if (pm_wakeup_pending()) {
		pm_runtime_put_sync(dev);
		async_error = -EBUSY;
		goto Complete;
	}

	dpm_wd_set(&wd, dev);

	device_lock(dev);

	if (dev->pm_domain) {
		info = "power domain ";
		callback = pm_op(&dev->pm_domain->ops, state);
		goto Run;
	}

	if (dev->type && dev->type->pm) {
		info = "type ";
		callback = pm_op(dev->type->pm, state);
		goto Run;
	}

	if (dev->class) {
		if (dev->class->pm) {
			info = "class ";
			callback = pm_op(dev->class->pm, state);
			goto Run;
		} else if (dev->class->suspend) {
			pm_dev_dbg(dev, state, "legacy class ");
			error = legacy_suspend(dev, state, dev->class->suspend);
			goto End;
		}
	}

	if (dev->bus) {
		if (dev->bus->pm) {
			info = "bus ";
			callback = pm_op(dev->bus->pm, state);
		} else if (dev->bus->suspend) {
			pm_dev_dbg(dev, state, "legacy bus ");
			error = legacy_suspend(dev, state, dev->bus->suspend);
			goto End;
		}
	}

 Run:
	if (!callback && dev->driver && dev->driver->pm) {
		info = "driver ";
		callback = pm_op(dev->driver->pm, state);
	}

	error = dpm_run_callback(callback, dev, state, info);
	if (dpm_enable_debug && callback) {
		printk("-------- suspend %s %pf with %d\n", dev->kobj.name, callback, error);
	}

 End:
	if (!error) {
		dev->power.is_suspended = true;
		if (dev->power.wakeup_path
		    && dev->parent && !dev->parent->power.ignore_children)
			dev->parent->power.wakeup_path = true;
	}

	device_unlock(dev);

	dpm_wd_clear(&wd);

 Complete:
	complete_all(&dev->power.completion);

	if (error) {
		pm_runtime_put_sync(dev);
		async_error = error;
	} else if (dev->power.is_suspended) {
		__pm_runtime_disable(dev, false);
	}

	return error;
}

static void async_suspend(void *data, async_cookie_t cookie)
{
	struct device *dev = (struct device *)data;
	int error;

	error = __device_suspend(dev, pm_transition, true);
	if (error) {
		dpm_save_failed_dev(dev_name(dev));
		pm_dev_err(dev, pm_transition, " async", error);
	}

	put_device(dev);
}

static int device_suspend(struct device *dev)
{
	INIT_COMPLETION(dev->power.completion);

	if (pm_async_enabled && dev->power.async_suspend) {
		get_device(dev);
		async_schedule(async_suspend, dev);
		return 0;
	}

	return __device_suspend(dev, pm_transition, false);
}

int dpm_suspend(pm_message_t state)
{
	ktime_t starttime = ktime_get();
	int error = 0;

	might_sleep();

	mutex_lock(&dpm_list_mtx);
	pm_transition = state;
	async_error = 0;
	while (!list_empty(&dpm_prepared_list)) {
		struct device *dev = to_device(dpm_prepared_list.prev);

		get_device(dev);
		mutex_unlock(&dpm_list_mtx);
		error = device_suspend(dev);

		mutex_lock(&dpm_list_mtx);
		if (error) {
			pm_dev_err(dev, state, "", error);
			dpm_save_failed_dev(dev_name(dev));
			put_device(dev);
			break;
		}
		if (!list_empty(&dev->power.entry))
			list_move(&dev->power.entry, &dpm_suspended_list);
		put_device(dev);
		if (async_error)
			break;
	}
	mutex_unlock(&dpm_list_mtx);
	async_synchronize_full();
	if (!error)
		error = async_error;
	if (error) {
		suspend_stats.failed_suspend++;
		dpm_save_failed_step(SUSPEND_SUSPEND);
	} else
		dpm_show_time(starttime, state, NULL);
	return error;
}

static int device_prepare(struct device *dev, pm_message_t state)
{
	int (*callback)(struct device *) = NULL;
	char *info = NULL;
	int error = 0;

	device_lock(dev);

	dev->power.wakeup_path = device_may_wakeup(dev);

	if (dev->pm_domain) {
		info = "preparing power domain ";
		callback = dev->pm_domain->ops.prepare;
	} else if (dev->type && dev->type->pm) {
		info = "preparing type ";
		callback = dev->type->pm->prepare;
	} else if (dev->class && dev->class->pm) {
		info = "preparing class ";
		callback = dev->class->pm->prepare;
	} else if (dev->bus && dev->bus->pm) {
		info = "preparing bus ";
		callback = dev->bus->pm->prepare;
	}

	if (!callback && dev->driver && dev->driver->pm) {
		info = "preparing driver ";
		callback = dev->driver->pm->prepare;
	}

	if (callback) {
		error = callback(dev);
		suspend_report_result(callback, error);
	}

	device_unlock(dev);

	return error;
}

int dpm_prepare(pm_message_t state)
{
	int error = 0;

	might_sleep();

	mutex_lock(&dpm_list_mtx);
	while (!list_empty(&dpm_list)) {
		struct device *dev = to_device(dpm_list.next);

		get_device(dev);
		mutex_unlock(&dpm_list_mtx);

		error = device_prepare(dev, state);

		mutex_lock(&dpm_list_mtx);
		if (error) {
			if (error == -EAGAIN) {
				put_device(dev);
				error = 0;
				continue;
			}
			printk(KERN_INFO "PM: Device %s not prepared "
				"for power transition: code %d\n",
				dev_name(dev), error);
			put_device(dev);
			break;
		}
		dev->power.is_prepared = true;
		if (!list_empty(&dev->power.entry))
			list_move_tail(&dev->power.entry, &dpm_prepared_list);
		put_device(dev);
	}
	mutex_unlock(&dpm_list_mtx);
	return error;
}

int dpm_suspend_start(pm_message_t state)
{
	int error;

	error = dpm_prepare(state);
	if (error) {
		suspend_stats.failed_prepare++;
		dpm_save_failed_step(SUSPEND_PREPARE);
	} else
		error = dpm_suspend(state);
	return error;
}
EXPORT_SYMBOL_GPL(dpm_suspend_start);

void __suspend_report_result(const char *function, void *fn, int ret)
{
	if (ret)
		printk(KERN_ERR "%s(): %pF returns %d\n", function, fn, ret);
}
EXPORT_SYMBOL_GPL(__suspend_report_result);

int device_pm_wait_for_dev(struct device *subordinate, struct device *dev)
{
	dpm_wait(dev, subordinate->power.async_suspend);
	return async_error;
}
EXPORT_SYMBOL_GPL(device_pm_wait_for_dev);
