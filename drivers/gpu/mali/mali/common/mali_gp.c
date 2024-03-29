/*
 * Copyright (C) 2011-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mali_gp.h"
#include "mali_hw_core.h"
#include "mali_group.h"
#include "mali_osk.h"
#include "regs/mali_gp_regs.h"
#include "mali_kernel_common.h"
#include "mali_kernel_core.h"
#if MALI_TIMELINE_PROFILING_ENABLED
#include "mali_osk_profiling.h"
#endif
#include "mali_pm.h"

struct mali_gp_core
{
	struct mali_hw_core  hw_core;           
	struct mali_group   *group;             
	_mali_osk_irq_t     *irq;               
	struct mali_gp_job  *running_job;       
	_mali_osk_timer_t   *timeout_timer;     
	u32                  timeout_job_id;    
	mali_bool            core_timed_out;    
	u32                  counter_src0;      
	u32                  counter_src1;      
	u32                  counter_src0_used; 
	u32                  counter_src1_used; 
};

static struct mali_gp_core *mali_global_gp_core = NULL;

static _mali_osk_errcode_t mali_gp_upper_half(void *data);
static void mali_gp_bottom_half(void *data);
static void mali_gp_irq_probe_trigger(void *data);
static _mali_osk_errcode_t mali_gp_irq_probe_ack(void *data);
static void mali_gp_post_process_job(struct mali_gp_core *core, mali_bool suspend);
static void mali_gp_timeout(void *data);

struct mali_gp_core *mali_gp_create(const _mali_osk_resource_t * resource, struct mali_group *group)
{
	struct mali_gp_core* core = NULL;

	MALI_DEBUG_ASSERT(NULL == mali_global_gp_core);
	MALI_DEBUG_PRINT(2, ("Mali GP: Creating Mali GP core: %s\n", resource->description));

	core = _mali_osk_malloc(sizeof(struct mali_gp_core));
	if (NULL != core)
	{
		core->group = group;
		core->running_job = NULL;
		core->counter_src0 = MALI_HW_CORE_NO_COUNTER;
		core->counter_src1 = MALI_HW_CORE_NO_COUNTER;
		core->counter_src0_used = MALI_HW_CORE_NO_COUNTER;
		core->counter_src1_used = MALI_HW_CORE_NO_COUNTER;
		if (_MALI_OSK_ERR_OK == mali_hw_core_create(&core->hw_core, resource, MALIGP2_REGISTER_ADDRESS_SPACE_SIZE))
		{
			_mali_osk_errcode_t ret;

			mali_group_lock(group);
			ret = mali_gp_reset(core);
			mali_group_unlock(group);

			if (_MALI_OSK_ERR_OK == ret)
			{
				
				core->irq = _mali_osk_irq_init(resource->irq,
				                               mali_gp_upper_half,
				                               mali_gp_bottom_half,
				                               mali_gp_irq_probe_trigger,
				                               mali_gp_irq_probe_ack,
				                               core,
				                               "mali_gp_irq_handlers");
				if (NULL != core->irq)
				{
					
					core->timeout_timer = _mali_osk_timer_init();
					if(NULL != core->timeout_timer)
					{
						_mali_osk_timer_setcallback(core->timeout_timer, mali_gp_timeout, (void *)core);
						MALI_DEBUG_PRINT(4, ("Mali GP: set global gp core from 0x%08X to 0x%08X\n", mali_global_gp_core, core));
						mali_global_gp_core = core;

						return core;
					}
					else
					{
						MALI_PRINT_ERROR(("Failed to setup timeout timer for GP core %s\n", core->hw_core.description));
						
						_mali_osk_irq_term(core->irq);
					}
				}
				else
				{
					MALI_PRINT_ERROR(("Failed to setup interrupt handlers for GP core %s\n", core->hw_core.description));
				}
			}
			mali_hw_core_delete(&core->hw_core);
		}

		_mali_osk_free(core);
	}
	else
	{
		MALI_PRINT_ERROR(("Failed to allocate memory for GP core\n"));
	}

	return NULL;
}

void mali_gp_delete(struct mali_gp_core *core)
{
	MALI_DEBUG_ASSERT_POINTER(core);

	_mali_osk_timer_term(core->timeout_timer);
	_mali_osk_irq_term(core->irq);
	mali_hw_core_delete(&core->hw_core);
	mali_global_gp_core = NULL;
	_mali_osk_free(core);
}

void mali_gp_stop_bus(struct mali_gp_core *core)
{
	MALI_DEBUG_ASSERT_POINTER(core);

	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_CMD, MALIGP2_REG_VAL_CMD_STOP_BUS);
}

_mali_osk_errcode_t mali_gp_stop_bus_wait(struct mali_gp_core *core)
{
	int i;
	const int request_loop_count = 20;

	MALI_DEBUG_ASSERT_POINTER(core);

	
	mali_gp_stop_bus(core);

	
	for (i = 0; i < request_loop_count; i++)
	{
		if (mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_STATUS) & MALIGP2_REG_VAL_STATUS_BUS_STOPPED)
		{
			break;
		}
		_mali_osk_time_ubusydelay(10);
	}

	if (request_loop_count == i)
	{
		MALI_PRINT_ERROR(("Mali GP: Failed to stop bus on %s\n", core->hw_core.description));
		return _MALI_OSK_ERR_FAULT;
	}
	return _MALI_OSK_ERR_OK;
}

void mali_gp_hard_reset(struct mali_gp_core *core)
{
	const int reset_finished_loop_count = 15;
	const u32 reset_wait_target_register = MALIGP2_REG_ADDR_MGMT_WRITE_BOUND_LOW;
	const u32 reset_invalid_value = 0xC0FFE000;
	const u32 reset_check_value = 0xC01A0000;
	const u32 reset_default_value = 0;
	int i;

	MALI_DEBUG_ASSERT_POINTER(core);
	MALI_DEBUG_PRINT(4, ("Mali GP: Hard reset of core %s\n", core->hw_core.description));
	MALI_ASSERT_GROUP_LOCKED(core->group);

	mali_gp_post_process_job(core, MALI_FALSE);

	mali_hw_core_register_write(&core->hw_core, reset_wait_target_register, reset_invalid_value);

	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_CMD, MALIGP2_REG_VAL_CMD_RESET);

	for (i = 0; i < reset_finished_loop_count; i++)
	{
		mali_hw_core_register_write(&core->hw_core, reset_wait_target_register, reset_check_value);
		if (reset_check_value == mali_hw_core_register_read(&core->hw_core, reset_wait_target_register))
		{
			break;
		}
	}

	if (i == reset_finished_loop_count)
	{
		MALI_PRINT_ERROR(("Mali GP: The hard reset loop didn't work, unable to recover\n"));
	}

	mali_hw_core_register_write(&core->hw_core, reset_wait_target_register, reset_default_value); 
	
	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_CLEAR, MALIGP2_REG_VAL_IRQ_MASK_ALL);
	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_MASK, MALIGP2_REG_VAL_IRQ_MASK_USED);

}

_mali_osk_errcode_t mali_gp_reset(struct mali_gp_core *core)
{
	int i;
	const int request_loop_count = 20;

	MALI_DEBUG_ASSERT_POINTER(core);
	MALI_DEBUG_PRINT(4, ("Mali GP: Reset of core %s\n", core->hw_core.description));
	MALI_ASSERT_GROUP_LOCKED(core->group);

	mali_gp_post_process_job(core, MALI_FALSE);

	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_MASK, 0); 

#if defined(USING_MALI200)

	

	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_CMD, MALIGP2_REG_VAL_CMD_STOP_BUS);

	for (i = 0; i < request_loop_count; i++)
	{
		if (mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_STATUS) & MALIGP2_REG_VAL_STATUS_BUS_STOPPED)
		{
			break;
		}
		_mali_osk_time_ubusydelay(10);
	}

	if (request_loop_count == i)
	{
		MALI_PRINT_ERROR(("Mali GP: Failed to stop bus for core %s, unable to recover\n", core->hw_core.description));
		return _MALI_OSK_ERR_FAULT;
	}

	
	mali_gp_hard_reset(core);

#elif defined(USING_MALI400)

	

	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_CLEAR, MALI400GP_REG_VAL_IRQ_RESET_COMPLETED);
	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_CMD, MALI400GP_REG_VAL_CMD_SOFT_RESET);

	for (i = 0; i < request_loop_count; i++)
	{
		if (mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_RAWSTAT) & MALI400GP_REG_VAL_IRQ_RESET_COMPLETED)
		{
			break;
		}
		_mali_osk_time_ubusydelay(10);
	}

	if (request_loop_count == i)
	{
		MALI_PRINT_ERROR(("Mali GP: Failed to reset core %s, unable to recover\n", core->hw_core.description));
		return _MALI_OSK_ERR_FAULT;
	}
#else
#error "no supported mali core defined"
#endif

	
	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_CLEAR, MALIGP2_REG_VAL_IRQ_MASK_ALL);
	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_MASK, MALIGP2_REG_VAL_IRQ_MASK_USED);

	return _MALI_OSK_ERR_OK;
}

void mali_gp_job_start(struct mali_gp_core *core, struct mali_gp_job *job)
{
	u32 startcmd = 0;
	u32 *frame_registers = mali_gp_job_get_frame_registers(job);
	core->counter_src0_used = core->counter_src0;
	core->counter_src1_used = core->counter_src1;

	MALI_DEBUG_ASSERT_POINTER(core);
	MALI_ASSERT_GROUP_LOCKED(core->group);

	if (mali_gp_job_has_vs_job(job))
	{
		startcmd |= (u32) MALIGP2_REG_VAL_CMD_START_VS;
	}

	if (mali_gp_job_has_plbu_job(job))
	{
		startcmd |= (u32) MALIGP2_REG_VAL_CMD_START_PLBU;
	}

	MALI_DEBUG_ASSERT(0 != startcmd);

	mali_hw_core_register_write_array_relaxed(&core->hw_core, MALIGP2_REG_ADDR_MGMT_VSCL_START_ADDR, frame_registers, MALIGP2_NUM_REGS_FRAME);

#if PROFILING_PRINT_L2_HITRATE_ON_GP_FINISH
	{
		
		mali_l2_cache_core_set_counter_src0(mali_l2_cache_core_get_glob_l2_core(0), 20);
		mali_l2_cache_core_set_counter_src1(mali_l2_cache_core_get_glob_l2_core(0), 21);
	}
#endif

	
	if (MALI_HW_CORE_NO_COUNTER != core->counter_src0_used || MALI_HW_CORE_NO_COUNTER != core->counter_src0_used)
	{
		
		if (MALI_HW_CORE_NO_COUNTER != core->counter_src0_used)
		{
			mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PERF_CNT_0_SRC, core->counter_src0_used);
			mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PERF_CNT_0_ENABLE, MALIGP2_REG_VAL_PERF_CNT_ENABLE);
		}
		if (MALI_HW_CORE_NO_COUNTER != core->counter_src1_used)
		{
			mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PERF_CNT_1_SRC, core->counter_src1_used);
			mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PERF_CNT_1_ENABLE, MALIGP2_REG_VAL_PERF_CNT_ENABLE);
		}
	}
	else
	{
		
		u32 perf_counter_flag = mali_gp_job_get_perf_counter_flag(job);
		if (0 != perf_counter_flag)
		{
			if (perf_counter_flag & _MALI_PERFORMANCE_COUNTER_FLAG_SRC0_ENABLE)
			{
				core->counter_src0_used = mali_gp_job_get_perf_counter_src0(job);
				mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PERF_CNT_0_SRC, core->counter_src0_used);
				mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PERF_CNT_0_ENABLE, MALIGP2_REG_VAL_PERF_CNT_ENABLE);
			}

			if (perf_counter_flag & _MALI_PERFORMANCE_COUNTER_FLAG_SRC1_ENABLE)
			{
				core->counter_src1_used = mali_gp_job_get_perf_counter_src1(job);
				mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PERF_CNT_1_SRC, core->counter_src1_used);
				mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PERF_CNT_1_ENABLE, MALIGP2_REG_VAL_PERF_CNT_ENABLE);
			}
		}
	}

	MALI_DEBUG_PRINT(3, ("Mali GP: Starting job (0x%08x) on core %s with command 0x%08X\n", job, core->hw_core.description, startcmd));

	
	_mali_osk_write_mem_barrier();

	
	mali_hw_core_register_write_relaxed(&core->hw_core, MALIGP2_REG_ADDR_MGMT_CMD, startcmd);

	
	_mali_osk_write_mem_barrier();

	

	_mali_osk_timer_add(core->timeout_timer, _mali_osk_time_mstoticks(mali_max_job_runtime));
	core->timeout_job_id = mali_gp_job_get_id(job);

#if MALI_TIMELINE_PROFILING_ENABLED
	_mali_osk_profiling_add_event(MALI_PROFILING_EVENT_TYPE_SINGLE | MALI_PROFILING_MAKE_EVENT_CHANNEL_GP(0) | MALI_PROFILING_EVENT_REASON_SINGLE_HW_FLUSH,
	                          job->frame_builder_id, job->flush_id, 0, 0, 0);
	_mali_osk_profiling_add_event(MALI_PROFILING_EVENT_TYPE_START|MALI_PROFILING_MAKE_EVENT_CHANNEL_GP(0), job->pid, job->tid, 0, 0, 0);
#endif

	core->running_job = job;
}

void mali_gp_resume_with_new_heap(struct mali_gp_core *core, u32 start_addr, u32 end_addr)
{
	u32 irq_readout;

	MALI_DEBUG_ASSERT_POINTER(core);
	MALI_ASSERT_GROUP_LOCKED(core->group);

	irq_readout = mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_RAWSTAT);

	if (irq_readout & MALIGP2_REG_VAL_IRQ_PLBU_OUT_OF_MEM)
	{
		mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_CLEAR, (MALIGP2_REG_VAL_IRQ_PLBU_OUT_OF_MEM | MALIGP2_REG_VAL_IRQ_HANG));
		mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_MASK, MALIGP2_REG_VAL_IRQ_MASK_USED); 
		mali_hw_core_register_write_relaxed(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PLBU_ALLOC_START_ADDR, start_addr);
		mali_hw_core_register_write_relaxed(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PLBU_ALLOC_END_ADDR, end_addr);

		MALI_DEBUG_PRINT(3, ("Mali GP: Resuming job\n"));

		mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_CMD, MALIGP2_REG_VAL_CMD_UPDATE_PLBU_ALLOC);
		_mali_osk_write_mem_barrier();

#if MALI_TIMELINE_PROFILING_ENABLED
		_mali_osk_profiling_add_event(MALI_PROFILING_EVENT_TYPE_RESUME|MALI_PROFILING_MAKE_EVENT_CHANNEL_GP(0), 0, 0, 0, 0, 0);
#endif
	}
}

void mali_gp_abort_job(struct mali_gp_core *core)
{
	MALI_DEBUG_ASSERT_POINTER(core);
	MALI_ASSERT_GROUP_LOCKED(core->group);

	if (_MALI_OSK_ERR_FAULT != mali_gp_reset(core))
	{
		_mali_osk_timer_del(core->timeout_timer);
	}
}

u32 mali_gp_core_get_version(struct mali_gp_core *core)
{
	MALI_DEBUG_ASSERT_POINTER(core);
	return mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_VERSION);
}

mali_bool mali_gp_core_set_counter_src0(struct mali_gp_core *core, u32 counter)
{
	MALI_DEBUG_ASSERT_POINTER(core);

	core->counter_src0 = counter;
	return MALI_TRUE;
}

mali_bool mali_gp_core_set_counter_src1(struct mali_gp_core *core, u32 counter)
{
	MALI_DEBUG_ASSERT_POINTER(core);

	core->counter_src1 = counter;
	return MALI_TRUE;
}

u32 mali_gp_core_get_counter_src0(struct mali_gp_core *core)
{
	MALI_DEBUG_ASSERT_POINTER(core);
	return core->counter_src0;
}

u32 mali_gp_core_get_counter_src1(struct mali_gp_core *core)
{
	MALI_DEBUG_ASSERT_POINTER(core);
	return core->counter_src1;
}

struct mali_gp_core *mali_gp_get_global_gp_core(void)
{
	return mali_global_gp_core;
}

static _mali_osk_errcode_t mali_gp_upper_half(void *data)
{
	struct mali_gp_core *core = (struct mali_gp_core *)data;
	u32 irq_readout = 0;

#if MALI_SHARED_INTERRUPTS
	mali_pm_lock();
	if (MALI_TRUE == mali_pm_is_powered_on())
	{
#endif
		irq_readout = mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_STAT);
#if MALI_SHARED_INTERRUPTS
	}
	mali_pm_unlock();
#endif

	if (MALIGP2_REG_VAL_IRQ_MASK_NONE != irq_readout)
	{
		
		mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_MASK, MALIGP2_REG_VAL_IRQ_MASK_NONE);

#if MALI_TIMELINE_PROFILING_ENABLED
		_mali_osk_profiling_add_event(MALI_PROFILING_EVENT_TYPE_SINGLE|MALI_PROFILING_MAKE_EVENT_CHANNEL_GP(0)|MALI_PROFILING_EVENT_REASON_SINGLE_HW_INTERRUPT, irq_readout, 0, 0, 0, 0);
#endif

		
		_mali_osk_irq_schedulework(core->irq);
		return _MALI_OSK_ERR_OK;
	}

	return _MALI_OSK_ERR_FAULT;
}

static void mali_gp_bottom_half(void *data)
{
	struct mali_gp_core *core = (struct mali_gp_core *)data;
	u32 irq_readout;
	u32 irq_errors;

#if MALI_TIMELINE_PROFILING_ENABLED
#if 0  
	_mali_osk_profiling_add_event( MALI_PROFILING_EVENT_TYPE_START| MALI_PROFILING_EVENT_CHANNEL_SOFTWARE ,  _mali_osk_get_pid(), _mali_osk_get_tid()+11000, 0, 0, 0);
#endif
#endif

	mali_group_lock(core->group); 

	if ( MALI_FALSE == mali_group_power_is_on(core->group) )
	{
		MALI_PRINT_ERROR(("Interrupt bottom half of %s when core is OFF.", core->hw_core.description));
		mali_group_unlock(core->group);
		return;
	}

	irq_readout = mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_RAWSTAT) & MALIGP2_REG_VAL_IRQ_MASK_USED;
	MALI_DEBUG_PRINT(4, ("Mali GP: Bottom half IRQ 0x%08X from core %s\n", irq_readout, core->hw_core.description));

	if (irq_readout & (MALIGP2_REG_VAL_IRQ_VS_END_CMD_LST|MALIGP2_REG_VAL_IRQ_PLBU_END_CMD_LST))
	{
		u32 core_status = mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_STATUS);
		if (0 == (core_status & MALIGP2_REG_VAL_STATUS_MASK_ACTIVE))
		{
			mali_gp_post_process_job(core, MALI_FALSE);
			MALI_DEBUG_PRINT(4, ("Mali GP: Job completed, calling group handler\n"));
			mali_group_bottom_half(core->group, GROUP_EVENT_GP_JOB_COMPLETED); 
			return;
		}
	}

	irq_errors = irq_readout & ~(MALIGP2_REG_VAL_IRQ_VS_END_CMD_LST|MALIGP2_REG_VAL_IRQ_PLBU_END_CMD_LST|MALIGP2_REG_VAL_IRQ_HANG|MALIGP2_REG_VAL_IRQ_PLBU_OUT_OF_MEM);
	if (0 != irq_errors)
	{
		mali_gp_post_process_job(core, MALI_FALSE);
		MALI_PRINT_ERROR(("Mali GP: Unknown interrupt 0x%08X from core %s, aborting job\n", irq_readout, core->hw_core.description));
		mali_group_bottom_half(core->group, GROUP_EVENT_GP_JOB_FAILED); 
		return;
	}
	else if (MALI_TRUE == core->core_timed_out) 
	{
		if (core->timeout_job_id == mali_gp_job_get_id(core->running_job))
		{
			mali_gp_post_process_job(core, MALI_FALSE);
			MALI_DEBUG_PRINT(2, ("Mali GP: Job %d timed out\n", mali_gp_job_get_id(core->running_job)));
			mali_group_bottom_half(core->group, GROUP_EVENT_GP_JOB_TIMED_OUT);
		}
		else
		{
			MALI_DEBUG_PRINT(2, ("Mali GP: Job %d timed out but current job is %d\n", core->timeout_job_id, mali_gp_job_get_id(core->running_job)));
			mali_group_unlock(core->group); 
		}
		core->core_timed_out = MALI_FALSE;
		return;
	}
	else if (irq_readout & MALIGP2_REG_VAL_IRQ_PLBU_OUT_OF_MEM)
	{
		mali_gp_post_process_job(core, MALI_TRUE);
		MALI_DEBUG_PRINT(3, ("Mali GP: PLBU needs more heap memory\n"));
		mali_group_bottom_half(core->group, GROUP_EVENT_GP_OOM); 
		return;
	}
	else if (irq_readout & MALIGP2_REG_VAL_IRQ_HANG)
	{
		
		mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_CLEAR, MALIGP2_REG_VAL_IRQ_HANG);
	}

	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_MASK, MALIGP2_REG_VAL_IRQ_MASK_USED);
	mali_group_unlock(core->group);

#if MALI_TIMELINE_PROFILING_ENABLED
#if 0  
	_mali_osk_profiling_add_event( MALI_PROFILING_EVENT_TYPE_STOP| MALI_PROFILING_EVENT_CHANNEL_SOFTWARE ,  _mali_osk_get_pid(), _mali_osk_get_tid()+11000, 0, 0, 0);
#endif
#endif
}

static void mali_gp_irq_probe_trigger(void *data)
{
	struct mali_gp_core *core = (struct mali_gp_core *)data;

	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_MASK, MALIGP2_REG_VAL_IRQ_MASK_USED);     
	mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_RAWSTAT, MALIGP2_REG_VAL_CMD_FORCE_HANG);
	_mali_osk_mem_barrier();
}

static _mali_osk_errcode_t mali_gp_irq_probe_ack(void *data)
{
	struct mali_gp_core *core = (struct mali_gp_core *)data;
	u32 irq_readout;

	irq_readout = mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_STAT);
	if (MALIGP2_REG_VAL_IRQ_FORCE_HANG & irq_readout)
	{
		mali_hw_core_register_write(&core->hw_core, MALIGP2_REG_ADDR_MGMT_INT_CLEAR, MALIGP2_REG_VAL_IRQ_FORCE_HANG);
		_mali_osk_mem_barrier();
		return _MALI_OSK_ERR_OK;
	}

	return _MALI_OSK_ERR_FAULT;
}


static void mali_gp_post_process_job(struct mali_gp_core *core, mali_bool suspend)
{
	MALI_ASSERT_GROUP_LOCKED(core->group);

	if (NULL != core->running_job)
	{
		u32 val0 = 0;
		u32 val1 = 0;
#if MALI_TIMELINE_PROFILING_ENABLED
		u32 event_id;
#endif

#if PROFILING_PRINT_L2_HITRATE_ON_GP_FINISH
	{
		u32 src0, value0, src1, value1, sum, per_thousand, per_thousand_now, diff0, diff1;
		static u32 print_nr=0;
		static u32 prev0=0;
		static u32 prev1=0;
		if ( !(++print_nr&511) )
		{
			mali_l2_cache_core_get_counter_values(mali_l2_cache_core_get_glob_l2_core(0), &src0, &value0, &src1, &value1);
			MALI_DEBUG_ASSERT( src0==20 ); 
			MALI_DEBUG_ASSERT( src1==21 ); 

			sum = value0+value1;
			if ( sum > 1000000 )
			{
				per_thousand = value0 / (sum/1000);
			}
			else
			{
				per_thousand = (value0*1000) / (sum);
			}
			diff0 = value0-prev0;
			diff1 = value1-prev1;

			sum = diff0 + diff1 ;
			if ( sum > 1000000 )
			{
				per_thousand_now = diff0 / (sum/1000);
			}
			else
			{
				per_thousand_now = (diff0*1000) / (sum);
			}

			prev0=value0;
			prev1=value1;
			if (per_thousand_now<=1000)
			{
				MALI_DEBUG_PRINT(2, ("Mali L2: Read hits/misses:  %d/%d  =  %d thousand_parts total, since previous: %d\n", value0, value1, per_thousand, per_thousand_now));
			}

		}
	}
#endif

		if (MALI_HW_CORE_NO_COUNTER != core->counter_src0_used)
		{
			val0 = mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PERF_CNT_0_VALUE);
			if (mali_gp_job_get_perf_counter_flag(core->running_job) &&
			    _MALI_PERFORMANCE_COUNTER_FLAG_SRC0_ENABLE && mali_gp_job_get_perf_counter_src0(core->running_job) == core->counter_src0_used)
			{
				
				mali_gp_job_set_perf_counter_value0(core->running_job, val0);
			}
			else
			{
				
				mali_gp_job_set_perf_counter_value0(core->running_job, MALI_HW_CORE_INVALID_VALUE);
			}

#if MALI_TIMELINE_PROFILING_ENABLED
			_mali_osk_profiling_report_hw_counter(COUNTER_VP_C0, val0);
#endif

		}

		if (MALI_HW_CORE_NO_COUNTER != core->counter_src1_used)
		{
			val1 = mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PERF_CNT_1_VALUE);
			if (mali_gp_job_get_perf_counter_flag(core->running_job) &&
			    _MALI_PERFORMANCE_COUNTER_FLAG_SRC1_ENABLE && mali_gp_job_get_perf_counter_src1(core->running_job) == core->counter_src1_used)
			{
				
				mali_gp_job_set_perf_counter_value1(core->running_job, val1);
			}
			else
			{
				
				mali_gp_job_set_perf_counter_value1(core->running_job, MALI_HW_CORE_INVALID_VALUE);
			}

#if MALI_TIMELINE_PROFILING_ENABLED
			_mali_osk_profiling_report_hw_counter(COUNTER_VP_C1, val1);
#endif
		}

#if MALI_TIMELINE_PROFILING_ENABLED
		if (MALI_TRUE == suspend)
		{
			event_id = MALI_PROFILING_EVENT_TYPE_SUSPEND|MALI_PROFILING_MAKE_EVENT_CHANNEL_GP(0);
		}
		else
		{
			event_id = MALI_PROFILING_EVENT_TYPE_STOP|MALI_PROFILING_MAKE_EVENT_CHANNEL_GP(0);
		}
		_mali_osk_profiling_add_event(event_id, val0, val1, core->counter_src0_used | (core->counter_src1_used << 8), 0, 0);
#endif

		mali_gp_job_set_current_heap_addr(core->running_job,
		                                  mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_PLBU_ALLOC_START_ADDR));

		if (MALI_TRUE != suspend)
		{
			
			core->running_job = NULL;
			_mali_osk_timer_del(core->timeout_timer);
		}
	}
}

static void mali_gp_timeout(void *data)
{
	struct mali_gp_core * core = ((struct mali_gp_core *)data);

	MALI_DEBUG_PRINT(3, ("Mali GP: TIMEOUT callback \n"));
	core->core_timed_out = MALI_TRUE;
	_mali_osk_irq_schedulework(core->irq);
}

#if 0
void mali_gp_print_state(struct mali_gp_core *core)
{
	MALI_DEBUG_PRINT(2, ("Mali GP: State: 0x%08x\n", mali_hw_core_register_read(&core->hw_core, MALIGP2_REG_ADDR_MGMT_STATUS) ));
}
#endif

#if MALI_STATE_TRACKING
u32 mali_gp_dump_state(struct mali_gp_core *core, char *buf, u32 size)
{
	int n = 0;

	n += _mali_osk_snprintf(buf + n, size - n, "\tGP: %s\n", core->hw_core.description);

	return n;
}
#endif
