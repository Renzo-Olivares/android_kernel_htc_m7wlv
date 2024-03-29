/*
 * Copyright (C) 2011-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MALI_PP_JOB_H__
#define __MALI_PP_JOB_H__

#include "mali_osk.h"
#include "mali_osk_list.h"
#include "mali_uk_types.h"
#include "mali_session.h"
#include "mali_kernel_common.h"
#include "regs/mali_200_regs.h"

struct mali_pp_job
{
	_mali_osk_list_t list;                             
	struct mali_session_data *session;                 
	u32 id;                                            
	u32 user_id;                                       
	u32 frame_registers[_MALI_PP_MAX_FRAME_REGISTERS]; 
    u32 frame_registers_addr_frame[_MALI_PP_MAX_SUB_JOBS - 1]; 
    u32 frame_registers_addr_stack[_MALI_PP_MAX_SUB_JOBS - 1]; 
	u32 wb0_registers[_MALI_PP_MAX_WB_REGISTERS];      
	u32 wb1_registers[_MALI_PP_MAX_WB_REGISTERS];      
	u32 wb2_registers[_MALI_PP_MAX_WB_REGISTERS];      
	u32 perf_counter_flag;                             
	u32 perf_counter_src0;                             
	u32 perf_counter_src1;                             
	u32 perf_counter_value0[_MALI_PP_MAX_SUB_JOBS];    
	u32 perf_counter_value1[_MALI_PP_MAX_SUB_JOBS];     
	u32 sub_job_count;                                 
	u32 sub_jobs_started;                              
	u32 sub_jobs_completed;                            
	u32 sub_job_errors;                                
	u32 pid;                                           
	u32 tid;                                           
	u32 frame_builder_id;                              
	u32 flush_id;                                      
	mali_bool barrier;                                 
	mali_bool active_barrier;                          
	mali_bool no_notification;                         
};

struct mali_pp_job *mali_pp_job_create(struct mali_session_data *session, _mali_uk_pp_start_job_s *args, u32 id);
void mali_pp_job_delete(struct mali_pp_job *job);

_mali_osk_errcode_t mali_pp_job_check(struct mali_pp_job *job);


MALI_STATIC_INLINE u32 mali_pp_job_get_id(struct mali_pp_job *job)
{
	return (NULL == job) ? 0 : job->id;
}

MALI_STATIC_INLINE u32 mali_pp_job_get_user_id(struct mali_pp_job *job)
{
	return job->user_id;
}

MALI_STATIC_INLINE u32 mali_pp_job_get_frame_builder_id(struct mali_pp_job *job)
{
	return job->frame_builder_id;
}

MALI_STATIC_INLINE u32 mali_pp_job_get_flush_id(struct mali_pp_job *job)
{
	return job->flush_id;
}

MALI_STATIC_INLINE u32* mali_pp_job_get_frame_registers(struct mali_pp_job *job)
{
	return job->frame_registers;
}

MALI_STATIC_INLINE u32 mali_pp_job_get_addr_frame(struct mali_pp_job *job, u32 sub_job)
{
	if (sub_job == 0)
	{
		return job->frame_registers[MALI200_REG_ADDR_FRAME / sizeof(u32)];
	}
	else if (sub_job < _MALI_PP_MAX_SUB_JOBS)
	{
		return job->frame_registers_addr_frame[sub_job - 1];
	}

	return 0;
}

MALI_STATIC_INLINE u32 mali_pp_job_get_addr_stack(struct mali_pp_job *job, u32 sub_job)
{
	if (sub_job == 0)
	{
		return job->frame_registers[MALI200_REG_ADDR_STACK / sizeof(u32)];
	}
	else if (sub_job < _MALI_PP_MAX_SUB_JOBS)
	{
		return job->frame_registers_addr_stack[sub_job - 1];
	}

	return 0;
}

MALI_STATIC_INLINE u32* mali_pp_job_get_wb0_registers(struct mali_pp_job *job)
{
	return job->wb0_registers;
}

MALI_STATIC_INLINE u32* mali_pp_job_get_wb1_registers(struct mali_pp_job *job)
{
	return job->wb1_registers;
}

MALI_STATIC_INLINE u32* mali_pp_job_get_wb2_registers(struct mali_pp_job *job)
{
	return job->wb2_registers;
}

MALI_STATIC_INLINE void mali_pp_job_disable_wb0(struct mali_pp_job *job)
{
	job->wb0_registers[MALI200_REG_ADDR_WB_SOURCE_SELECT] = 0;
}

MALI_STATIC_INLINE void mali_pp_job_disable_wb1(struct mali_pp_job *job)
{
	job->wb1_registers[MALI200_REG_ADDR_WB_SOURCE_SELECT] = 0;
}

MALI_STATIC_INLINE void mali_pp_job_disable_wb2(struct mali_pp_job *job)
{
	job->wb2_registers[MALI200_REG_ADDR_WB_SOURCE_SELECT] = 0;
}

MALI_STATIC_INLINE struct mali_session_data *mali_pp_job_get_session(struct mali_pp_job *job)
{
	return job->session;
}

MALI_STATIC_INLINE mali_bool mali_pp_job_has_unstarted_sub_jobs(struct mali_pp_job *job)
{
	return (job->sub_jobs_started < job->sub_job_count) ? MALI_TRUE : MALI_FALSE;
}

MALI_STATIC_INLINE mali_bool mali_pp_job_is_currently_rendering_and_if_so_abort_new_starts(struct mali_pp_job *job)
{
	
	MALI_DEBUG_ASSERT( job->sub_jobs_started != job->sub_job_count );

	
	if (  (job->sub_jobs_started > 0)  )
	{
		
		if (job->sub_jobs_started > job->sub_jobs_completed )
		{
			u32 jobs_remaining = job->sub_job_count - job->sub_jobs_started;
			job->sub_jobs_started   += jobs_remaining;
			job->sub_jobs_completed += jobs_remaining;
			job->sub_job_errors     += jobs_remaining;
			
			return MALI_TRUE;
		}
	}
	
	return MALI_FALSE;
}

MALI_STATIC_INLINE mali_bool mali_pp_job_is_complete(struct mali_pp_job *job)
{
	return (job->sub_job_count == job->sub_jobs_completed) ? MALI_TRUE : MALI_FALSE;
}

MALI_STATIC_INLINE u32 mali_pp_job_get_first_unstarted_sub_job(struct mali_pp_job *job)
{
	return job->sub_jobs_started;
}

MALI_STATIC_INLINE u32 mali_pp_job_get_sub_job_count(struct mali_pp_job *job)
{
	return job->sub_job_count;
}

MALI_STATIC_INLINE void mali_pp_job_mark_sub_job_started(struct mali_pp_job *job, u32 sub_job)
{
	
	MALI_DEBUG_ASSERT(job->sub_jobs_started == sub_job);
	job->sub_jobs_started++;
}

MALI_STATIC_INLINE void mali_pp_job_mark_sub_job_completed(struct mali_pp_job *job, mali_bool success)
{
	job->sub_jobs_completed++;
	if ( MALI_FALSE == success )
	{
		job->sub_job_errors++;
	}
}

MALI_STATIC_INLINE mali_bool mali_pp_job_was_success(struct mali_pp_job *job)
{
	if ( 0 == job->sub_job_errors )
	{
		return MALI_TRUE;
	}
	return MALI_FALSE;
}

MALI_STATIC_INLINE mali_bool mali_pp_job_has_active_barrier(struct mali_pp_job *job)
{
	return job->active_barrier;
}

MALI_STATIC_INLINE void mali_pp_job_barrier_enforced(struct mali_pp_job *job)
{
	job->active_barrier = MALI_FALSE;
}

MALI_STATIC_INLINE mali_bool mali_pp_job_use_no_notification(struct mali_pp_job *job)
{
	return job->no_notification;
}

MALI_STATIC_INLINE u32 mali_pp_job_get_perf_counter_flag(struct mali_pp_job *job)
{
	return job->perf_counter_flag;
}

MALI_STATIC_INLINE u32 mali_pp_job_get_perf_counter_src0(struct mali_pp_job *job)
{
	return job->perf_counter_src0;
}

MALI_STATIC_INLINE u32 mali_pp_job_get_perf_counter_src1(struct mali_pp_job *job)
{
	return job->perf_counter_src1;
}

MALI_STATIC_INLINE u32 mali_pp_job_get_perf_counter_value0(struct mali_pp_job *job, u32 sub_job)
{
	return job->perf_counter_value0[sub_job];
}

MALI_STATIC_INLINE u32 mali_pp_job_get_perf_counter_value1(struct mali_pp_job *job, u32 sub_job)
{
	return job->perf_counter_value1[sub_job];
}

MALI_STATIC_INLINE void mali_pp_job_set_perf_counter_value0(struct mali_pp_job *job, u32 sub_job, u32 value)
{
	job->perf_counter_value0[sub_job] = value;
}

MALI_STATIC_INLINE void mali_pp_job_set_perf_counter_value1(struct mali_pp_job *job, u32 sub_job, u32 value)
{
	job->perf_counter_value1[sub_job] = value;
}

#endif 
