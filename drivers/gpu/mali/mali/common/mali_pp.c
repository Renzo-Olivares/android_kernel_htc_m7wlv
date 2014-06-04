/*
 * Copyright (C) 2011-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mali_pp.h"
#include "mali_hw_core.h"
#include "mali_group.h"
#include "mali_osk.h"
#include "regs/mali_200_regs.h"
#include "mali_kernel_common.h"
#include "mali_kernel_core.h"
#if MALI_TIMELINE_PROFILING_ENABLED
#include "mali_osk_profiling.h"
#endif
#include "mali_pm.h"


#define MALI_MAX_NUMBER_OF_PP_CORES        8

struct mali_pp_core
{
	struct mali_hw_core  hw_core;           
	struct mali_group   *group;             
	_mali_osk_irq_t     *irq;               
	u32                  core_id;           
	struct mali_pp_job  *running_job;       
	u32                  running_sub_job;   
	_mali_osk_timer_t   *timeout_timer;     
	u32                  timeout_job_id;    
	mali_bool            core_timed_out;    
	u32                  counter_src0;      
	u32                  counter_src1;      
	u32                  counter_src0_used; 
	u32                  counter_src1_used; 
};

static struct mali_pp_core* mali_global_pp_cores[MALI_MAX_NUMBER_OF_PP_CORES];
static u32 mali_global_num_pp_cores = 0;

static _mali_osk_errcode_t mali_pp_upper_half(void *data);
static void mali_pp_bottom_half(void *data);
static void mali_pp_irq_probe_trigger(void *data);
static _mali_osk_errcode_t mali_pp_irq_probe_ack(void *data);
static void mali_pp_post_process_job(struct mali_pp_core *core);
static void mali_pp_timeout(void *data);

struct mali_pp_core *mali_pp_create(const _mali_osk_resource_t *resource, struct mali_group *group)
{
	struct mali_pp_core* core = NULL;

	MALI_DEBUG_PRINT(2, ("Mali PP: Creating Mali PP core: %s\n", resource->description));
	MALI_DEBUG_PRINT(2, ("Mali PP: Base address of PP core: 0x%x\n", resource->base));

	if (mali_global_num_pp_cores >= MALI_MAX_NUMBER_OF_PP_CORES)
	{
		MALI_PRINT_ERROR(("Mali PP: Too many PP core objects created\n"));
		return NULL;
	}

	core = _mali_osk_malloc(sizeof(struct mali_pp_core));
	if (NULL != core)
	{
		core->group = group;
		core->core_id = mali_global_num_pp_cores;
		core->running_job = NULL;
		core->counter_src0 = MALI_HW_CORE_NO_COUNTER;
		core->counter_src1 = MALI_HW_CORE_NO_COUNTER;
		core->counter_src0_used = MALI_HW_CORE_NO_COUNTER;
		core->counter_src1_used = MALI_HW_CORE_NO_COUNTER;
		if (_MALI_OSK_ERR_OK == mali_hw_core_create(&core->hw_core, resource, MALI200_REG_SIZEOF_REGISTER_BANK))
		{
			_mali_osk_errcode_t ret;

			mali_group_lock(group);
			ret = mali_pp_reset(core);
			mali_group_unlock(group);

			if (_MALI_OSK_ERR_OK == ret)
			{
				
				core->irq = _mali_osk_irq_init(resource->irq,
				                               mali_pp_upper_half,
				                               mali_pp_bottom_half,
				                               mali_pp_irq_probe_trigger,
				                               mali_pp_irq_probe_ack,
				                               core,
				                               "mali_pp_irq_handlers");
				if (NULL != core->irq)
				{
					
					core->timeout_timer = _mali_osk_timer_init();
					if(NULL != core->timeout_timer)
					{
						_mali_osk_timer_setcallback(core->timeout_timer, mali_pp_timeout, (void *)core);

						mali_global_pp_cores[mali_global_num_pp_cores] = core;
						mali_global_num_pp_cores++;

						return core;
					}
					else
					{
						MALI_PRINT_ERROR(("Failed to setup timeout timer for PP core %s\n", core->hw_core.description));
						
						_mali_osk_irq_term(core->irq);
					}
				}
				else
				{
					MALI_PRINT_ERROR(("Mali PP: Failed to setup interrupt handlers for PP core %s\n", core->hw_core.description));
				}
			}
			mali_hw_core_delete(&core->hw_core);
		}

		_mali_osk_free(core);
	}
	else
	{
		MALI_PRINT_ERROR(("Mali PP: Failed to allocate memory for PP core\n"));
	}

	return NULL;
}

void mali_pp_delete(struct mali_pp_core *core)
{
	u32 i;

	MALI_DEBUG_ASSERT_POINTER(core);

	_mali_osk_timer_term(core->timeout_timer);
	_mali_osk_irq_term(core->irq);
	mali_hw_core_delete(&core->hw_core);

	
	for (i = 0; i < mali_global_num_pp_cores; i++)
	{
		if (mali_global_pp_cores[i] == core)
		{
			mali_global_pp_cores[i] = NULL;
			mali_global_num_pp_cores--;
			break;
		}
	}

	_mali_osk_free(core);
}

void mali_pp_stop_bus(struct mali_pp_core *core)
{
	MALI_DEBUG_ASSERT_POINTER(core);
	
	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_CTRL_MGMT, MALI200_REG_VAL_CTRL_MGMT_STOP_BUS);
}

_mali_osk_errcode_t mali_pp_stop_bus_wait(struct mali_pp_core *core)
{
	int i;
	const int request_loop_count = 20;

	MALI_DEBUG_ASSERT_POINTER(core);
	MALI_ASSERT_GROUP_LOCKED(core->group);

	
	mali_pp_stop_bus(core);

	
	for (i = 0; i < request_loop_count; i++)
	{
		if (mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_STATUS) & MALI200_REG_VAL_STATUS_BUS_STOPPED)
			break;
		_mali_osk_time_ubusydelay(10);
	}

	if (request_loop_count == i)
	{
		MALI_PRINT_ERROR(("Mali PP: Failed to stop bus on %s. Status: 0x%08x\n", core->hw_core.description, mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_STATUS)));
		return _MALI_OSK_ERR_FAULT;
	}
	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t mali_pp_hard_reset(struct mali_pp_core *core)
{
	
	const int reset_finished_loop_count = 15;
	const u32 reset_invalid_value = 0xC0FFE000;
	const u32 reset_check_value = 0xC01A0000;
	int i;

	MALI_DEBUG_ASSERT_POINTER(core);
	MALI_DEBUG_PRINT(2, ("Mali PP: Hard reset of core %s\n", core->hw_core.description));
	MALI_ASSERT_GROUP_LOCKED(core->group);

	mali_pp_post_process_job(core); 

	
	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_WRITE_BOUNDARY_LOW, reset_invalid_value);

	
	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_CTRL_MGMT, MALI200_REG_VAL_CTRL_MGMT_FORCE_RESET);

	
	for (i = 0; i < reset_finished_loop_count; i++)
	{
		mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_WRITE_BOUNDARY_LOW, reset_check_value);
		if (reset_check_value == mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_WRITE_BOUNDARY_LOW))
		{
			break;
		}
		_mali_osk_time_ubusydelay(10);
	}

	if (i == reset_finished_loop_count)
	{
		MALI_PRINT_ERROR(("Mali PP: The hard reset loop didn't work, unable to recover\n"));
	}

	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_WRITE_BOUNDARY_LOW, 0x00000000); 
	
	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_CLEAR, MALI200_REG_VAL_IRQ_MASK_ALL);
	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_MASK, MALI200_REG_VAL_IRQ_MASK_USED);

	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t mali_pp_reset(struct mali_pp_core *core)
{
	int i;
	const int request_loop_count = 20;

	MALI_DEBUG_ASSERT_POINTER(core);
	MALI_DEBUG_PRINT(4, ("Mali PP: Reset of core %s\n", core->hw_core.description));
	MALI_ASSERT_GROUP_LOCKED(core->group);

	mali_pp_post_process_job(core); 

	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_MASK, 0); 

#if defined(USING_MALI200)

	

	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_CTRL_MGMT, MALI200_REG_VAL_CTRL_MGMT_STOP_BUS);

	for (i = 0; i < request_loop_count; i++)
	{
		if (mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_STATUS) & MALI200_REG_VAL_STATUS_BUS_STOPPED)
		{
			break;
		}
		_mali_osk_time_ubusydelay(10);
	}

	if (request_loop_count == i)
	{
		MALI_PRINT_ERROR(("Mali PP: Failed to stop bus for core %s, unable to recover\n", core->hw_core.description));
		return _MALI_OSK_ERR_FAULT ;
	}

	
	mali_pp_hard_reset(core);

#elif defined(USING_MALI400)

	

	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_CLEAR, MALI400PP_REG_VAL_IRQ_RESET_COMPLETED);
	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_CTRL_MGMT, MALI400PP_REG_VAL_CTRL_MGMT_SOFT_RESET);

	for (i = 0; i < request_loop_count; i++)
	{
		if (mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_RAWSTAT) & MALI400PP_REG_VAL_IRQ_RESET_COMPLETED)
		{
			break;
		}
		_mali_osk_time_ubusydelay(10);
	}

	if (request_loop_count == i)
	{
		MALI_DEBUG_PRINT(2, ("Mali PP: Failed to reset core %s, Status: 0x%08x\n", core->hw_core.description, mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_STATUS)));
		return _MALI_OSK_ERR_FAULT;
	}
#else
#error "no supported mali core defined"
#endif

	
	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_CLEAR, MALI200_REG_VAL_IRQ_MASK_ALL);
	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_MASK, MALI200_REG_VAL_IRQ_MASK_USED);

	return _MALI_OSK_ERR_OK;
}

void mali_pp_job_start(struct mali_pp_core *core, struct mali_pp_job *job, u32 sub_job)
{
	u32 *frame_registers = mali_pp_job_get_frame_registers(job);
	u32 *wb0_registers = mali_pp_job_get_wb0_registers(job);
	u32 *wb1_registers = mali_pp_job_get_wb1_registers(job);
	u32 *wb2_registers = mali_pp_job_get_wb2_registers(job);
	core->counter_src0_used = core->counter_src0;
	core->counter_src1_used = core->counter_src1;

	MALI_DEBUG_ASSERT_POINTER(core);
	MALI_ASSERT_GROUP_LOCKED(core->group);

	mali_hw_core_register_write_array_relaxed(&core->hw_core, MALI200_REG_ADDR_FRAME, frame_registers, MALI200_NUM_REGS_FRAME);
	if (0 != sub_job)
	{
		mali_hw_core_register_write_relaxed(&core->hw_core, MALI200_REG_ADDR_FRAME, mali_pp_job_get_addr_frame(job, sub_job));
		mali_hw_core_register_write_relaxed(&core->hw_core, MALI200_REG_ADDR_STACK, mali_pp_job_get_addr_stack(job, sub_job));
	}

	if (wb0_registers[0]) 
	{
		mali_hw_core_register_write_array_relaxed(&core->hw_core, MALI200_REG_ADDR_WB0, wb0_registers, MALI200_NUM_REGS_WBx);
	}

	if (wb1_registers[0]) 
	{
		mali_hw_core_register_write_array_relaxed(&core->hw_core, MALI200_REG_ADDR_WB1, wb1_registers, MALI200_NUM_REGS_WBx);
	}

	if (wb2_registers[0]) 
	{
		mali_hw_core_register_write_array_relaxed(&core->hw_core, MALI200_REG_ADDR_WB2, wb2_registers, MALI200_NUM_REGS_WBx);
	}

	
	if (MALI_HW_CORE_NO_COUNTER != core->counter_src0_used || MALI_HW_CORE_NO_COUNTER != core->counter_src1_used)
	{
		
		if (MALI_HW_CORE_NO_COUNTER != core->counter_src0_used)
		{
			mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_0_SRC, core->counter_src0_used);
			mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_0_ENABLE, MALI200_REG_VAL_PERF_CNT_ENABLE);
		}
		if (MALI_HW_CORE_NO_COUNTER != core->counter_src1_used)
		{
			mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_1_SRC, core->counter_src1_used);
			mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_1_ENABLE, MALI200_REG_VAL_PERF_CNT_ENABLE);
		}
	}
	else
	{
		
		u32 perf_counter_flag = mali_pp_job_get_perf_counter_flag(job);
		if (0 != perf_counter_flag)
		{
			if (perf_counter_flag & _MALI_PERFORMANCE_COUNTER_FLAG_SRC0_ENABLE)
			{
				core->counter_src0_used = mali_pp_job_get_perf_counter_src0(job);
				mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_0_SRC, core->counter_src0_used);
				mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_0_ENABLE, MALI200_REG_VAL_PERF_CNT_ENABLE);
			}

			if (perf_counter_flag & _MALI_PERFORMANCE_COUNTER_FLAG_SRC1_ENABLE)
			{
				core->counter_src1_used = mali_pp_job_get_perf_counter_src1(job);
				mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_1_SRC, core->counter_src1_used);
				mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_1_ENABLE, MALI200_REG_VAL_PERF_CNT_ENABLE);
			}
		}
	}

	MALI_DEBUG_PRINT(3, ("Mali PP: Starting job 0x%08X part %u/%u on PP core %s\n", job, sub_job + 1, mali_pp_job_get_sub_job_count(job), core->hw_core.description));

	
	_mali_osk_write_mem_barrier();

	
	mali_hw_core_register_write_relaxed(&core->hw_core, MALI200_REG_ADDR_MGMT_CTRL_MGMT, MALI200_REG_VAL_CTRL_MGMT_START_RENDERING);

	
	_mali_osk_write_mem_barrier();

	
	_mali_osk_timer_add(core->timeout_timer, _mali_osk_time_mstoticks(mali_max_job_runtime));
	core->timeout_job_id = mali_pp_job_get_id(job);

#if MALI_TIMELINE_PROFILING_ENABLED
	_mali_osk_profiling_add_event(MALI_PROFILING_EVENT_TYPE_SINGLE | MALI_PROFILING_MAKE_EVENT_CHANNEL_PP(core->core_id) | MALI_PROFILING_EVENT_REASON_SINGLE_HW_FLUSH, job->frame_builder_id, job->flush_id, 0, 0, 0);
	_mali_osk_profiling_add_event(MALI_PROFILING_EVENT_TYPE_START|MALI_PROFILING_MAKE_EVENT_CHANNEL_PP(core->core_id), job->pid, job->tid, 0, 0, 0);
#endif

	core->running_job = job;
	core->running_sub_job = sub_job;
}

u32 mali_pp_core_get_version(struct mali_pp_core *core)
{
	MALI_DEBUG_ASSERT_POINTER(core);
	return mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_VERSION);
}

u32 mali_pp_core_get_id(struct mali_pp_core *core)
{
	MALI_DEBUG_ASSERT_POINTER(core);
	return core->core_id;
}

mali_bool mali_pp_core_set_counter_src0(struct mali_pp_core *core, u32 counter)
{
	MALI_DEBUG_ASSERT_POINTER(core);

	core->counter_src0 = counter;
	return MALI_TRUE;
}

mali_bool mali_pp_core_set_counter_src1(struct mali_pp_core *core, u32 counter)
{
	MALI_DEBUG_ASSERT_POINTER(core);

	core->counter_src1 = counter;
	return MALI_TRUE;
}

u32 mali_pp_core_get_counter_src0(struct mali_pp_core *core)
{
	MALI_DEBUG_ASSERT_POINTER(core);
	return core->counter_src0;
}

u32 mali_pp_core_get_counter_src1(struct mali_pp_core *core)
{
	MALI_DEBUG_ASSERT_POINTER(core);
	return core->counter_src1;
}

struct mali_pp_core* mali_pp_get_global_pp_core(u32 index)
{
	if (MALI_MAX_NUMBER_OF_PP_CORES > index)
	{
		return mali_global_pp_cores[index];
	}

	return NULL;
}

u32 mali_pp_get_glob_num_pp_cores(void)
{
	return mali_global_num_pp_cores;
}

u32 mali_pp_get_max_num_pp_cores(void)
{
	return MALI_MAX_NUMBER_OF_PP_CORES;
}

static _mali_osk_errcode_t mali_pp_upper_half(void *data)
{
	struct mali_pp_core *core = (struct mali_pp_core *)data;
	u32 irq_readout = 0;

#if MALI_SHARED_INTERRUPTS
	mali_pm_lock();
	if (MALI_TRUE == mali_pm_is_powered_on())
	{
#endif
		irq_readout = mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_STATUS);
#if MALI_SHARED_INTERRUPTS
	}
	mali_pm_unlock();
#endif

	if (MALI200_REG_VAL_IRQ_MASK_NONE != irq_readout)
	{
		
		mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_MASK, MALI200_REG_VAL_IRQ_MASK_NONE);

#if MALI_TIMELINE_PROFILING_ENABLED
		_mali_osk_profiling_add_event(MALI_PROFILING_EVENT_TYPE_SINGLE|MALI_PROFILING_MAKE_EVENT_CHANNEL_PP(core->core_id)|MALI_PROFILING_EVENT_REASON_SINGLE_HW_INTERRUPT, irq_readout, 0, 0, 0, 0);
#endif

		
		_mali_osk_irq_schedulework(core->irq);
		return _MALI_OSK_ERR_OK;
	}

	return _MALI_OSK_ERR_FAULT;
}

static void mali_pp_bottom_half(void *data)
{
	struct mali_pp_core *core = (struct mali_pp_core *)data;
	u32 irq_readout;
	u32 irq_errors;

#if MALI_TIMELINE_PROFILING_ENABLED
#if 0  
	_mali_osk_profiling_add_event( MALI_PROFILING_EVENT_TYPE_START| MALI_PROFILING_EVENT_CHANNEL_SOFTWARE ,  _mali_osk_get_pid(), _mali_osk_get_tid(), 0, 0, 0);
#endif
#endif

	mali_group_lock(core->group); 

	if ( MALI_FALSE == mali_group_power_is_on(core->group) )
	{
		MALI_PRINT_ERROR(("Interrupt bottom half of %s when core is OFF.", core->hw_core.description));
		mali_group_unlock(core->group);
		return;
	}

	irq_readout = mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_RAWSTAT) & MALI200_REG_VAL_IRQ_MASK_USED;

	MALI_DEBUG_PRINT(4, ("Mali PP: Bottom half IRQ 0x%08X from core %s\n", irq_readout, core->hw_core.description));

	if (irq_readout & MALI200_REG_VAL_IRQ_END_OF_FRAME)
	{
		mali_pp_post_process_job(core);
		MALI_DEBUG_PRINT(3, ("Mali PP: Job completed, calling group handler\n"));
		mali_group_bottom_half(core->group, GROUP_EVENT_PP_JOB_COMPLETED); 
		return;
	}

	irq_errors = irq_readout & ~(MALI200_REG_VAL_IRQ_END_OF_FRAME|MALI200_REG_VAL_IRQ_HANG);
	if (0 != irq_errors)
	{
		mali_pp_post_process_job(core);
		MALI_PRINT_ERROR(("Mali PP: Unknown interrupt 0x%08X from core %s, aborting job\n",
		                  irq_readout, core->hw_core.description));
		mali_group_bottom_half(core->group, GROUP_EVENT_PP_JOB_FAILED); 
		return;
	}
	else if (MALI_TRUE == core->core_timed_out) 
	{
		if (core->timeout_job_id == mali_pp_job_get_id(core->running_job))
		{
			mali_pp_post_process_job(core);
			MALI_DEBUG_PRINT(2, ("Mali PP: Job %d timed out on core %s\n",
			                 mali_pp_job_get_id(core->running_job), core->hw_core.description));
			mali_group_bottom_half(core->group, GROUP_EVENT_PP_JOB_TIMED_OUT); 
		}
		else
		{
			mali_group_unlock(core->group);
		}
		core->core_timed_out = MALI_FALSE;
		return;
	}
	else if (irq_readout & MALI200_REG_VAL_IRQ_HANG)
	{
		
		mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_CLEAR, MALI200_REG_VAL_IRQ_HANG);
	}

	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_MASK, MALI200_REG_VAL_IRQ_MASK_USED);
	mali_group_unlock(core->group);

#if MALI_TIMELINE_PROFILING_ENABLED
#if 0   
	_mali_osk_profiling_add_event( MALI_PROFILING_EVENT_TYPE_STOP| MALI_PROFILING_EVENT_CHANNEL_SOFTWARE ,  _mali_osk_get_pid(), _mali_osk_get_tid(), 0, 0, 0);
#endif
#endif
}

static void mali_pp_irq_probe_trigger(void *data)
{
	struct mali_pp_core *core = (struct mali_pp_core *)data;
	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_MASK, MALI200_REG_VAL_IRQ_MASK_USED);     
	mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_RAWSTAT, MALI200_REG_VAL_IRQ_FORCE_HANG);
	_mali_osk_mem_barrier();
}

static _mali_osk_errcode_t mali_pp_irq_probe_ack(void *data)
{
	struct mali_pp_core *core = (struct mali_pp_core *)data;
	u32 irq_readout;

	irq_readout = mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_STATUS);
	if (MALI200_REG_VAL_IRQ_FORCE_HANG & irq_readout)
	{
		mali_hw_core_register_write(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_CLEAR, MALI200_REG_VAL_IRQ_FORCE_HANG);
		_mali_osk_mem_barrier();
		return _MALI_OSK_ERR_OK;
	}

	return _MALI_OSK_ERR_FAULT;
}


static void mali_pp_post_process_job(struct mali_pp_core *core)
{
	MALI_ASSERT_GROUP_LOCKED(core->group);

	if (NULL != core->running_job)
	{
		u32 val0 = 0;
		u32 val1 = 0;
#if MALI_TIMELINE_PROFILING_ENABLED
		int counter_index = COUNTER_FP0_C0 + (2 * core->core_id);
#endif

		if (MALI_HW_CORE_NO_COUNTER != core->counter_src0_used)
		{
			val0 = mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_0_VALUE);
			if (mali_pp_job_get_perf_counter_flag(core->running_job) &&
			    _MALI_PERFORMANCE_COUNTER_FLAG_SRC0_ENABLE && mali_pp_job_get_perf_counter_src0(core->running_job) == core->counter_src0_used)
			{
				
				mali_pp_job_set_perf_counter_value0(core->running_job, core->running_sub_job, val0);
			}
			else
			{
				
				mali_pp_job_set_perf_counter_value0(core->running_job, core->running_sub_job, MALI_HW_CORE_INVALID_VALUE);
			}

#if MALI_TIMELINE_PROFILING_ENABLED
			_mali_osk_profiling_report_hw_counter(counter_index, val0);
#endif
		}

		if (MALI_HW_CORE_NO_COUNTER != core->counter_src1_used)
		{
			val1 = mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_1_VALUE);
			if (mali_pp_job_get_perf_counter_flag(core->running_job) &&
			    _MALI_PERFORMANCE_COUNTER_FLAG_SRC1_ENABLE && mali_pp_job_get_perf_counter_src1(core->running_job) == core->counter_src1_used)
			{
				
				mali_pp_job_set_perf_counter_value1(core->running_job, core->running_sub_job, val1);
			}
			else
			{
				
				mali_pp_job_set_perf_counter_value1(core->running_job, core->running_sub_job, MALI_HW_CORE_INVALID_VALUE);
			}

#if MALI_TIMELINE_PROFILING_ENABLED
			_mali_osk_profiling_report_hw_counter(counter_index + 1, val1);
#endif
		}

#if MALI_TIMELINE_PROFILING_ENABLED
		_mali_osk_profiling_add_event(MALI_PROFILING_EVENT_TYPE_STOP|MALI_PROFILING_MAKE_EVENT_CHANNEL_PP(core->core_id),
		                          val0, val1, core->counter_src0_used | (core->counter_src1_used << 8), 0, 0);
#endif

		
		core->running_job = NULL;
		_mali_osk_timer_del(core->timeout_timer);
	}
}

static void mali_pp_timeout(void *data)
{
	struct mali_pp_core * core = ((struct mali_pp_core *)data);

	MALI_DEBUG_PRINT(3, ("Mali PP: TIMEOUT callback \n"));
	core->core_timed_out = MALI_TRUE;
	_mali_osk_irq_schedulework(core->irq);
}

#if 0
static void mali_pp_print_registers(struct mali_pp_core *core)
{
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_VERSION = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_VERSION)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_CURRENT_REND_LIST_ADDR = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_CURRENT_REND_LIST_ADDR)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_STATUS = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_STATUS)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_INT_RAWSTAT = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_RAWSTAT)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_INT_MASK = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_MASK)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_INT_STATUS = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_INT_STATUS)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_BUS_ERROR_STATUS = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_BUS_ERROR_STATUS)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_PERF_CNT_0_ENABLE = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_0_ENABLE)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_PERF_CNT_0_SRC = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_0_SRC)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_PERF_CNT_0_VALUE = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_0_VALUE)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_PERF_CNT_1_ENABLE = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_1_ENABLE)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_PERF_CNT_1_SRC = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_1_SRC)));
	MALI_DEBUG_PRINT(2, ("Mali PP: Register MALI200_REG_ADDR_MGMT_PERF_CNT_1_VALUE = 0x%08X\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_PERF_CNT_1_VALUE)));
}
#endif

#if 0
void mali_pp_print_state(struct mali_pp_core *core)
{
	MALI_DEBUG_PRINT(2, ("Mali PP: State: 0x%08x\n", mali_hw_core_register_read(&core->hw_core, MALI200_REG_ADDR_MGMT_STATUS) ));
}
#endif

#if MALI_STATE_TRACKING
u32 mali_pp_dump_state(struct mali_pp_core *core, char *buf, u32 size)
{
	int n = 0;

	n += _mali_osk_snprintf(buf + n, size - n, "\tPP #%d: %s\n", core->core_id, core->hw_core.description);

	return n;
}
#endif
