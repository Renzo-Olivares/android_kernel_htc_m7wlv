/*
 * Copyright (C) 2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mali_pp_scheduler.h"
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "mali_osk_list.h"
#include "mali_scheduler.h"
#include "mali_pp.h"
#include "mali_pp_job.h"
#include "mali_group.h"
#include "mali_cluster.h"

#define MALI_MAX_NUMBER_OF_PP_GROUPS 8

static mali_bool mali_pp_scheduler_is_suspended(void);

enum mali_pp_slot_state
{
	MALI_PP_SLOT_STATE_IDLE,
	MALI_PP_SLOT_STATE_WORKING,
};

struct mali_pp_slot
{
	struct mali_group *group;
	enum mali_pp_slot_state state;
	struct mali_session_data *session;
};

static u32 pp_version = 0;
static _MALI_OSK_LIST_HEAD(job_queue);                          
static struct mali_pp_slot slots[MALI_MAX_NUMBER_OF_PP_GROUPS];
static u32 num_slots = 0;
static u32 num_slots_idle = 0;

static _mali_osk_wait_queue_t *pp_scheduler_working_wait_queue = NULL;
static u32 pause_count = 0;

static _mali_osk_lock_t *pp_scheduler_lock = NULL;
MALI_DEBUG_CODE(static u32 pp_scheduler_lock_owner = 0);

_mali_osk_errcode_t mali_pp_scheduler_initialize(void)
{
	u32 i;

	_MALI_OSK_INIT_LIST_HEAD(&job_queue);

	pp_scheduler_lock = _mali_osk_lock_init(_MALI_OSK_LOCKFLAG_ORDERED |_MALI_OSK_LOCKFLAG_NONINTERRUPTABLE, 0, _MALI_OSK_LOCK_ORDER_SCHEDULER);
	if (NULL == pp_scheduler_lock)
	{
		return _MALI_OSK_ERR_NOMEM;
	}

	pp_scheduler_working_wait_queue = _mali_osk_wait_queue_init();
	if (NULL == pp_scheduler_working_wait_queue)
	{
		_mali_osk_lock_term(pp_scheduler_lock);
		return _MALI_OSK_ERR_NOMEM;
	}

	
	for (i = 0; i < mali_cluster_get_glob_num_clusters(); i++)
	{
		u32 group_id = 0;
		struct mali_cluster *curr_cluster = mali_cluster_get_global_cluster(i);
		struct mali_group *group = mali_cluster_get_group(curr_cluster, group_id);
		while (NULL != group)
		{
			struct mali_pp_core *pp_core = mali_group_get_pp_core(group);
			if (NULL != pp_core)
			{
				if (0 == pp_version)
				{
					
					pp_version = mali_pp_core_get_version(pp_core);
				}
				slots[num_slots].group = group;
				slots[num_slots].state = MALI_PP_SLOT_STATE_IDLE;
				slots[num_slots].session = NULL;
				num_slots++;
				num_slots_idle++;
			}
			group_id++;
			group = mali_cluster_get_group(curr_cluster, group_id);
		}
	}

	return _MALI_OSK_ERR_OK;
}

void mali_pp_scheduler_terminate(void)
{
	_mali_osk_wait_queue_term(pp_scheduler_working_wait_queue);
	_mali_osk_lock_term(pp_scheduler_lock);
}

MALI_STATIC_INLINE void mali_pp_scheduler_lock(void)
{
	if(_MALI_OSK_ERR_OK != _mali_osk_lock_wait(pp_scheduler_lock, _MALI_OSK_LOCKMODE_RW))
	{
		
		MALI_DEBUG_ASSERT(0);
	}
	MALI_DEBUG_PRINT(5, ("Mali PP scheduler: PP scheduler lock taken\n"));
	MALI_DEBUG_ASSERT(0 == pp_scheduler_lock_owner);
	MALI_DEBUG_CODE(pp_scheduler_lock_owner = _mali_osk_get_tid());
}

MALI_STATIC_INLINE void mali_pp_scheduler_unlock(void)
{
	MALI_DEBUG_PRINT(5, ("Mali PP scheduler: Releasing PP scheduler lock\n"));
	MALI_DEBUG_ASSERT(_mali_osk_get_tid() == pp_scheduler_lock_owner);
	MALI_DEBUG_CODE(pp_scheduler_lock_owner = 0);
	_mali_osk_lock_signal(pp_scheduler_lock, _MALI_OSK_LOCKMODE_RW);
}

#ifdef DEBUG
MALI_STATIC_INLINE void mali_pp_scheduler_assert_locked(void)
{
	MALI_DEBUG_ASSERT(_mali_osk_get_tid() == pp_scheduler_lock_owner);
}
#define MALI_ASSERT_PP_SCHEDULER_LOCKED() mali_pp_scheduler_assert_locked()
#else
#define MALI_ASSERT_PP_SCHEDULER_LOCKED()
#endif

static mali_bool mali_pp_scheduler_session_has_running_jobs(struct mali_session_data *session)
{
	u32 i;

	MALI_ASSERT_PP_SCHEDULER_LOCKED();

	if (num_slots_idle == num_slots)
	{
		return MALI_FALSE;
	}

	for (i = 0; i < num_slots; i++)
	{
		if (MALI_PP_SLOT_STATE_WORKING == slots[i].state)
		{
			if (slots[i].session == session)
			{
				return MALI_TRUE;
			}
		}
	}

	return MALI_FALSE;
}

static void mali_pp_scheduler_schedule(void)
{
	u32 i;
	struct mali_pp_job *job;
#if MALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP_BETWEEN_APPS
	struct mali_session_data * session;
#endif

	MALI_ASSERT_PP_SCHEDULER_LOCKED();

	if (0 < pause_count || 0 == num_slots_idle || _mali_osk_list_empty(&job_queue))
	{
		MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Nothing to schedule (paused=%u, idle slots=%u)\n",
		                     pause_count, num_slots_idle));
		return; 
	}


#if MALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP
	if ( num_slots_idle < num_slots )
	{
		MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Job not started, since only %d/%d cores are available\n", num_slots_idle,num_slots));
		return;
	}
#endif

#if MALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP_BETWEEN_APPS
	
	job = _MALI_OSK_LIST_ENTRY(job_queue.next, struct mali_pp_job, list);
	session = job->session;
	if ( num_slots != num_slots_idle )
	{
		for (i = 0; (i < num_slots) ; i++)
		{
			if ( slots[i].state == MALI_PP_SLOT_STATE_IDLE )
			{
				continue;
			}
			session = mali_group_get_session(slots[i].group);
			break;
		}
	}
#endif

	for (i = 0; (i < num_slots) && (0 < num_slots_idle); i++)
	{
		u32 sub_job;

		if (_mali_osk_list_empty(&job_queue)) 
		{
			break; 
		}

		if (MALI_PP_SLOT_STATE_IDLE != slots[i].state)
		{
			continue;
		}

		job = _MALI_OSK_LIST_ENTRY(job_queue.next, struct mali_pp_job, list);
		MALI_DEBUG_ASSERT(mali_pp_job_has_unstarted_sub_jobs(job)); 

		if (MALI_TRUE == mali_pp_job_has_active_barrier(job))
		{
			if (MALI_TRUE == mali_pp_scheduler_session_has_running_jobs(mali_pp_job_get_session(job)))
			{
				
				return;
			}
			else
			{
				
				mali_pp_job_barrier_enforced(job);
			}
		}

		#if MALI_PP_SCHEDULER_KEEP_SUB_JOB_STARTS_ALIGNED
		if ( (0==job->sub_jobs_started) && (num_slots_idle < num_slots) && (job->sub_job_count > num_slots_idle))
		{
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Job with %d subjobs not started, since only %d/%d cores are available\n", job->sub_job_count, num_slots_idle,num_slots));
			return;
		}
		#endif

		#if MALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP_BETWEEN_APPS
		if ( job->session != session )
		{
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Job not started since existing job is from another application\n"));
			return;
		}
		#endif

		sub_job = mali_pp_job_get_first_unstarted_sub_job(job);

		MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Starting job %u (0x%08X) part %u/%u\n", mali_pp_job_get_id(job), job, sub_job + 1, mali_pp_job_get_sub_job_count(job)));
		if (_MALI_OSK_ERR_OK == mali_group_start_pp_job(slots[i].group, job, sub_job))
		{
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Job %u (0x%08X) part %u/%u started\n", mali_pp_job_get_id(job), job, sub_job + 1, mali_pp_job_get_sub_job_count(job)));

			
			mali_pp_job_mark_sub_job_started(job, sub_job);

			
			slots[i].state = MALI_PP_SLOT_STATE_WORKING;
			slots[i].session =  mali_pp_job_get_session(job);
			num_slots_idle--;

			if (!mali_pp_job_has_unstarted_sub_jobs(job))
			{
				_mali_osk_list_del(&job->list);
#if MALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP
				MALI_DEBUG_PRINT(6, ("Mali PP scheduler: Skip scheduling more jobs when MALI_PP_SCHEDULER_FORCE_NO_JOB_OVERLAP is set.\n"));
				return;
#endif
			}
		}
		else
		{
			MALI_DEBUG_PRINT(3, ("Mali PP scheduler: Failed to start PP job\n"));
			return;
		}
	}
}

static void mali_pp_scheduler_return_job_to_user(struct mali_pp_job *job)
{
	if (MALI_FALSE == mali_pp_job_use_no_notification(job))
	{
		_mali_osk_notification_t *notobj = _mali_osk_notification_create(_MALI_NOTIFICATION_PP_FINISHED, sizeof(_mali_uk_pp_job_finished_s));
		if (NULL != notobj)
		{
			u32 i;
			u32 sub_jobs = mali_pp_job_get_sub_job_count(job);
			mali_bool success = mali_pp_job_was_success(job);

			_mali_uk_pp_job_finished_s *jobres = notobj->result_buffer;
			_mali_osk_memset(jobres, 0, sizeof(_mali_uk_pp_job_finished_s)); 
			jobres->user_job_ptr = mali_pp_job_get_user_id(job);
			if (MALI_TRUE == success)
			{
				jobres->status = _MALI_UK_JOB_STATUS_END_SUCCESS;
			}
			else
			{
				jobres->status = _MALI_UK_JOB_STATUS_END_UNKNOWN_ERR;
			}

			for (i = 0; i < sub_jobs; i++)
			{
				jobres->perf_counter0[i] = mali_pp_job_get_perf_counter_value0(job, i);
				jobres->perf_counter1[i] = mali_pp_job_get_perf_counter_value1(job, i);
			}

			mali_session_send_notification(mali_pp_job_get_session(job), notobj);
		}
		else
		{
			MALI_PRINT_ERROR(("Mali PP scheduler: Unable to allocate notification object\n"));
		}
	}

	mali_pp_job_delete(job);
}

void mali_pp_scheduler_do_schedule(void)
{
	mali_pp_scheduler_lock();

	mali_pp_scheduler_schedule();

	mali_pp_scheduler_unlock();
}

void mali_pp_scheduler_job_done(struct mali_group *group, struct mali_pp_job *job, u32 sub_job, mali_bool success)
{
	u32 i;
	mali_bool job_is_done;

	MALI_DEBUG_PRINT(3, ("Mali PP scheduler: Job %u (0x%08X) part %u/%u completed (%s)\n", mali_pp_job_get_id(job), job, sub_job + 1, mali_pp_job_get_sub_job_count(job), success ? "success" : "failure"));

	mali_pp_scheduler_lock();

	
	for (i = 0; i < num_slots; i++)
	{
		if (slots[i].group == group)
		{
			slots[i].state = MALI_PP_SLOT_STATE_IDLE;
			slots[i].session = NULL;
			num_slots_idle++;
			mali_pp_job_mark_sub_job_completed(job, success);
		}
	}

	
	if (pause_count > 0)
	{
		_mali_osk_wait_queue_wake_up(pp_scheduler_working_wait_queue);
	}
	else
	{
		mali_pp_scheduler_schedule();
	}

	job_is_done = mali_pp_job_is_complete(job);

	mali_pp_scheduler_unlock();

	if (job_is_done)
	{
		
		MALI_DEBUG_PRINT(4, ("Mali PP scheduler: All parts completed for job %u (0x%08X)\n", mali_pp_job_get_id(job), job));
		mali_pp_scheduler_return_job_to_user(job);
	}
}

void mali_pp_scheduler_suspend(void)
{
	mali_pp_scheduler_lock();
	pause_count++; 
	mali_pp_scheduler_unlock();

	
	
	

	_mali_osk_wait_queue_wait_event(pp_scheduler_working_wait_queue, mali_pp_scheduler_is_suspended);
}

void mali_pp_scheduler_resume(void)
{
	mali_pp_scheduler_lock();
	pause_count--; 
	if (0 == pause_count)
	{
		mali_pp_scheduler_schedule();
	}
	mali_pp_scheduler_unlock();
}

_mali_osk_errcode_t _mali_ukk_pp_start_job(_mali_uk_pp_start_job_s *args)
{
	struct mali_session_data *session;
	struct mali_pp_job *job;

	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_DEBUG_ASSERT_POINTER(args->ctx);

	session = (struct mali_session_data*)args->ctx;

	job = mali_pp_job_create(session, args, mali_scheduler_get_new_id());
	if (NULL == job)
	{
		return _MALI_OSK_ERR_NOMEM;
	}

	if (_MALI_OSK_ERR_OK != mali_pp_job_check(job))
	{
		
		mali_pp_job_mark_sub_job_completed(job, MALI_FALSE); 
		mali_pp_scheduler_return_job_to_user(job); 
		return _MALI_OSK_ERR_OK; 
	}

#if PROFILING_SKIP_PP_JOBS || PROFILING_SKIP_PP_AND_GP_JOBS
#warning PP jobs will not be executed
	mali_pp_scheduler_return_job_to_user(job);
	return _MALI_OSK_ERR_OK;
#endif

	mali_pp_scheduler_lock();

	_mali_osk_list_addtail(&job->list, &job_queue);

	MALI_DEBUG_PRINT(3, ("Mali PP scheduler: Job %u (0x%08X) with %u parts queued\n", mali_pp_job_get_id(job), job, mali_pp_job_get_sub_job_count(job)));

	mali_pp_scheduler_schedule();

	mali_pp_scheduler_unlock();

	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t _mali_ukk_get_pp_number_of_cores(_mali_uk_get_pp_number_of_cores_s *args)
{
	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_DEBUG_ASSERT_POINTER(args->ctx);
	args->number_of_cores = num_slots;
	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t _mali_ukk_get_pp_core_version(_mali_uk_get_pp_core_version_s *args)
{
	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_DEBUG_ASSERT_POINTER(args->ctx);
	args->version = pp_version;
	return _MALI_OSK_ERR_OK;
}

void _mali_ukk_pp_job_disable_wb(_mali_uk_pp_disable_wb_s *args)
{
	struct mali_session_data *session;
	struct mali_pp_job *job;
	struct mali_pp_job *tmp;

	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_DEBUG_ASSERT_POINTER(args->ctx);

	session = (struct mali_session_data*)args->ctx;

	mali_pp_scheduler_lock();

	
	_MALI_OSK_LIST_FOREACHENTRY(job, tmp, &job_queue, struct mali_pp_job, list)
	{
		if (mali_pp_job_get_session(job) == session &&
		    mali_pp_job_get_frame_builder_id(job) == (u32)args->fb_id &&
		    mali_pp_job_get_flush_id(job) == (u32)args->flush_id)
		{
			if (args->wbx & _MALI_UK_PP_JOB_WB0)
			{
				mali_pp_job_disable_wb0(job);
			}
			if (args->wbx & _MALI_UK_PP_JOB_WB1)
			{
				mali_pp_job_disable_wb1(job);
			}
			if (args->wbx & _MALI_UK_PP_JOB_WB2)
			{
				mali_pp_job_disable_wb2(job);
			}
			break;
		}
	}

	mali_pp_scheduler_unlock();
}

void mali_pp_scheduler_abort_session(struct mali_session_data *session)
{
	struct mali_pp_job *job, *tmp;
	int i;

	mali_pp_scheduler_lock();
	MALI_DEBUG_PRINT(3, ("Mali PP scheduler: Aborting all jobs from session 0x%08x\n", session));

	
	_MALI_OSK_LIST_FOREACHENTRY(job, tmp, &job_queue, struct mali_pp_job, list)
	{
		if (mali_pp_job_get_session(job) == session)
		{
			_mali_osk_list_del(&(job->list));

			if ( mali_pp_job_is_currently_rendering_and_if_so_abort_new_starts(job) )
			{
				
				
				MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Keeping partially started PP job 0x%08x in queue\n", job));
				continue;
			}
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Removing PP job 0x%08x from queue\n", job));
			mali_pp_job_delete(job);
		}
	}

	mali_pp_scheduler_unlock();

	
	for (i = 0; i < num_slots; i++)
	{
		struct mali_group *group = slots[i].group;

		MALI_DEBUG_PRINT(5, ("PP sched abort: Looking at group 0x%08x\n", group));

		if (MALI_PP_SLOT_STATE_WORKING == slots[i].state)
		{
			MALI_DEBUG_PRINT(4, ("Mali PP scheduler: Aborting session 0x%08x from group 0x%08x\n", session, group));

			mali_group_abort_session(group, session);
		}
	}
}

static mali_bool mali_pp_scheduler_is_suspended(void)
{
	mali_bool ret;

	mali_pp_scheduler_lock();
	ret = pause_count > 0 && num_slots == num_slots_idle;
	mali_pp_scheduler_unlock();

	return ret;
}

#if MALI_STATE_TRACKING
u32 mali_pp_scheduler_dump_state(char *buf, u32 size)
{
	int n = 0;
	int i;

	n += _mali_osk_snprintf(buf + n, size - n, "PP:\n");
	n += _mali_osk_snprintf(buf + n, size - n, "\tQueue is %s\n", _mali_osk_list_empty(&job_queue) ? "empty" : "not empty");
	n += _mali_osk_snprintf(buf + n, size - n, "\n");

	for (i = 0; i < num_slots; i++)
	{
		n += mali_group_dump_state(slots[i].group, buf + n, size - n);
		n += _mali_osk_snprintf(buf + n, size - n, "\t\tState: %d\n", slots[i].state);
	}

	return n;
}
#endif
