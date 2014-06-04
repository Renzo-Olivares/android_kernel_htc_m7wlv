/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/kernel.h>
#include "mali_kernel_utilization.h"
#include "mali_osk.h"
#include "mali_platform.h"
#include "mali_kernel_common.h"

#define MALI_GPU_UTILIZATION_TIMEOUT 300

static _mali_osk_lock_t *time_data_lock;

static u64 period_start_time = 0;
static u64 work_start_time_gp = 0;
static u64 work_start_time_pp = 0;
static u64 accumulated_work_time_gp = 0;
static u64 accumulated_work_time_pp = 0;

static _mali_osk_timer_t *utilization_timer = NULL;
static mali_bool timer_running = MALI_FALSE;


static void calculate_gpu_utilization(void* arg)
{
	u64 time_now;
	u64 time_period;
	u64 max_accumulated_work_time;
	u32 leading_zeroes;
	u32 shift_val;
	u32 work_normalized;
	u32 period_normalized;
	u32 utilization;

	_mali_osk_lock_wait(time_data_lock, _MALI_OSK_LOCKMODE_RW);

	if( (accumulated_work_time_gp == 0 && work_start_time_gp == 0)&& (accumulated_work_time_pp == 0 && work_start_time_pp == 0))
	{
		
		timer_running = MALI_FALSE;

		_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);

		
		mali_gpu_utilization_handler(0);

		return;
	}

	time_now = _mali_osk_time_get_ns();
	time_period = time_now - period_start_time;

	
	if (work_start_time_gp!= 0)
	{
		accumulated_work_time_gp += (time_now - work_start_time_gp);
		work_start_time_gp = time_now;
	}
	if (work_start_time_pp != 0)
	{
		accumulated_work_time_pp += (time_now - work_start_time_pp);
		work_start_time_pp = time_now;
	}
	if(accumulated_work_time_pp>accumulated_work_time_gp)
		max_accumulated_work_time=accumulated_work_time_pp;
	else
		max_accumulated_work_time=accumulated_work_time_gp;

	
	leading_zeroes = _mali_osk_clz((u32)(time_period >> 32));
	shift_val = 32 - leading_zeroes;
	work_normalized = (u32)(max_accumulated_work_time >> shift_val);
	period_normalized = (u32)(time_period >> shift_val);

	if (period_normalized > 0x00FFFFFF)
	{
		
		period_normalized >>= 8;
	}
	else
	{
		work_normalized <<= 8;
	}

	utilization = work_normalized / period_normalized;

	accumulated_work_time_gp = 0;
	accumulated_work_time_pp = 0;
	period_start_time = time_now; 

	_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);

	_mali_osk_timer_add(utilization_timer, _mali_osk_time_mstoticks(MALI_GPU_UTILIZATION_TIMEOUT));


	mali_gpu_utilization_handler(utilization);
}

_mali_osk_errcode_t mali_utilization_init(void)
{
	time_data_lock = _mali_osk_lock_init(_MALI_OSK_LOCKFLAG_ORDERED | _MALI_OSK_LOCKFLAG_SPINLOCK_IRQ |
	                     _MALI_OSK_LOCKFLAG_NONINTERRUPTABLE, 0, _MALI_OSK_LOCK_ORDER_UTILIZATION);

	if (NULL == time_data_lock)
	{
		return _MALI_OSK_ERR_FAULT;
	}

	utilization_timer = _mali_osk_timer_init();
	if (NULL == utilization_timer)
	{
		_mali_osk_lock_term(time_data_lock);
		return _MALI_OSK_ERR_FAULT;
	}
	_mali_osk_timer_setcallback(utilization_timer, calculate_gpu_utilization, NULL);

	return _MALI_OSK_ERR_OK;
}

void mali_utilization_suspend(void)
{
	if (NULL != utilization_timer)
	{
		_mali_osk_timer_del(utilization_timer);
		timer_running = MALI_FALSE;
	}
}

void mali_utilization_term(void)
{
	if (NULL != utilization_timer)
	{
		_mali_osk_timer_del(utilization_timer);
		timer_running = MALI_FALSE;
		_mali_osk_timer_term(utilization_timer);
		utilization_timer = NULL;
	}

	_mali_osk_lock_term(time_data_lock);
}

void mali_utilization_core_start(u64 time_now,enum mali_core_event core_event,u32 active_gps,u32 active_pps)
{
	if((MALI_CORE_EVENT_GP_START==core_event)&&(1==active_gps))
	{

		_mali_osk_lock_wait(time_data_lock, _MALI_OSK_LOCKMODE_RW);

		if (time_now < period_start_time)
		{
			time_now = period_start_time;
		}
		work_start_time_gp = time_now;

		if (timer_running != MALI_TRUE)
		{
			timer_running = MALI_TRUE;
			period_start_time = work_start_time_gp; 

			_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);

			_mali_osk_timer_add(utilization_timer, _mali_osk_time_mstoticks(MALI_GPU_UTILIZATION_TIMEOUT));
		}
		else
		{
			_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);
		}

	}

	else if((MALI_CORE_EVENT_PP_START==core_event)&&(1==active_pps))
	{

		_mali_osk_lock_wait(time_data_lock, _MALI_OSK_LOCKMODE_RW);

		if (time_now < period_start_time)
		{
			time_now = period_start_time;
		}
		work_start_time_pp = time_now;

		if (timer_running != MALI_TRUE)
		{
			timer_running = MALI_TRUE;
			period_start_time = work_start_time_pp; 

			_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);

			_mali_osk_timer_add(utilization_timer, _mali_osk_time_mstoticks(MALI_GPU_UTILIZATION_TIMEOUT));
		}
		else
		{
			_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);
		}
	}
}

void mali_utilization_core_end(u64 time_now,enum mali_core_event core_event,u32 active_gps,u32 active_pps)
{
	if((MALI_CORE_EVENT_GP_STOP==core_event)&&(0==active_gps))
	{
		_mali_osk_lock_wait(time_data_lock, _MALI_OSK_LOCKMODE_RW);

		if (time_now < work_start_time_gp)
		{
			time_now = work_start_time_gp;
		}
		accumulated_work_time_gp+= (time_now - work_start_time_gp);
		work_start_time_gp=0;

		_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);
	}

	else if((MALI_CORE_EVENT_PP_STOP==core_event)&&(0==active_pps))
	{
		_mali_osk_lock_wait(time_data_lock, _MALI_OSK_LOCKMODE_RW);

		if (time_now < work_start_time_pp)
		{
			time_now = work_start_time_pp;
		}
		accumulated_work_time_pp+= (time_now - work_start_time_pp);
		work_start_time_pp=0;

		_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);
	}
}
