/*
 * Copyright (C) 2011-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MALI_GROUP_H__
#define __MALI_GROUP_H__

#include "linux/jiffies.h"
#include "mali_osk.h"
#include "mali_cluster.h"
#include "mali_mmu.h"
#include "mali_gp.h"
#include "mali_pp.h"
#include "mali_session.h"

#define MAX_RUNTIME 5000
#define MALI_MAX_NUMBER_OF_GROUPS 9

struct mali_group;

enum mali_group_event_t
{
	GROUP_EVENT_PP_JOB_COMPLETED,  
	GROUP_EVENT_PP_JOB_FAILED,     
	GROUP_EVENT_PP_JOB_TIMED_OUT,  
	GROUP_EVENT_GP_JOB_COMPLETED,  
	GROUP_EVENT_GP_JOB_FAILED,     
	GROUP_EVENT_GP_JOB_TIMED_OUT,  
	GROUP_EVENT_GP_OOM,            
	GROUP_EVENT_MMU_PAGE_FAULT,    
};

enum mali_group_core_state
{
	MALI_GROUP_CORE_STATE_IDLE,
	MALI_GROUP_CORE_STATE_WORKING,
	MALI_GROUP_CORE_STATE_OOM
};

struct mali_group *mali_group_create(struct mali_cluster *cluster, struct mali_mmu_core *mmu);
void mali_group_add_gp_core(struct mali_group *group, struct mali_gp_core* gp_core);
void mali_group_add_pp_core(struct mali_group *group, struct mali_pp_core* pp_core);
void mali_group_delete(struct mali_group *group);

void mali_group_reset(struct mali_group *group);

struct mali_gp_core* mali_group_get_gp_core(struct mali_group *group);

struct mali_pp_core* mali_group_get_pp_core(struct mali_group *group);

void mali_group_lock(struct mali_group *group);

void mali_group_unlock(struct mali_group *group);
#ifdef DEBUG
void mali_group_assert_locked(struct mali_group *group);
#define MALI_ASSERT_GROUP_LOCKED(group) mali_group_assert_locked(group)
#else
#define MALI_ASSERT_GROUP_LOCKED(group)
#endif

_mali_osk_errcode_t mali_group_start_gp_job(struct mali_group *group, struct mali_gp_job *job);
_mali_osk_errcode_t mali_group_start_pp_job(struct mali_group *group, struct mali_pp_job *job, u32 sub_job);

void mali_group_resume_gp_with_new_heap(struct mali_group *group, u32 job_id, u32 start_addr, u32 end_addr);
void mali_group_abort_gp_job(struct mali_group *group, u32 job_id);
void mali_group_abort_session(struct mali_group *group, struct mali_session_data *session);

enum mali_group_core_state mali_group_gp_state(struct mali_group *group);
enum mali_group_core_state mali_group_pp_state(struct mali_group *group);

void mali_group_bottom_half(struct mali_group *group, enum mali_group_event_t event);

struct mali_mmu_core *mali_group_get_mmu(struct mali_group *group);
struct mali_session_data *mali_group_get_session(struct mali_group *group);

void mali_group_remove_session_if_unused(struct mali_group *group, struct mali_session_data *session_data);

void mali_group_power_on(void);
void mali_group_power_off(void);
mali_bool mali_group_power_is_on(struct mali_group *group);

struct mali_group *mali_group_get_glob_group(u32 index);
u32 mali_group_get_glob_num_groups(void);

u32 mali_group_dump_state(struct mali_group *group, char *buf, u32 size);

#endif 
