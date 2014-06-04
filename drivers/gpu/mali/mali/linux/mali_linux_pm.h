
/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MALI_LINUX_PM_H__
#define __MALI_LINUX_PM_H__

#ifdef CONFIG_PM
typedef enum
{
	_MALI_DEVICE_SUSPEND,                         
	_MALI_DEVICE_RESUME,                          
	_MALI_DEVICE_MAX_POWER_STATES,                
} _mali_device_power_states;

typedef enum
{
	_MALI_DVFS_PAUSE_EVENT = _MALI_DEVICE_MAX_POWER_STATES,     
	_MALI_DVFS_RESUME_EVENT,                                        
	_MALI_MAX_DEBUG_OPERATIONS,
} _mali_device_dvfs_events;

extern _mali_device_power_states mali_device_state;
extern _mali_device_power_states mali_dvfs_device_state;
extern  _mali_osk_lock_t *lock;
extern short is_wake_up_needed;
extern int timeout_fired;
extern struct platform_device mali_gpu_device;

extern struct task_struct *dvfs_pm_thread;

extern struct task_struct *pm_thread;

int mali_device_suspend(u32 event_id, struct task_struct **pwr_mgmt_thread);
int mali_device_resume(u32 event_id, struct task_struct **pwr_mgmt_thread);
int mali_get_ospmm_thread_state(void);

#endif 
#endif 
