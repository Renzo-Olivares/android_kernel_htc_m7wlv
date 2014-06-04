/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef __MALI_UKK_H__
#define __MALI_UKK_H__

#include "mali_osk.h"
#include "mali_uk_types.h"

#ifdef __cplusplus
extern "C"
{
#endif




/** @defgroup _mali_uk_context U/K Context management
 *
 * These functions allow for initialisation of the user-kernel interface once per process.
 *
 * Generally the context will store the OS specific object to communicate with the kernel device driver and further
 * state information required by the specific implementation. The context is shareable among all threads in the caller process.
 *
 * On IOCTL systems, this is likely to be a file descriptor as a result of opening the kernel device driver.
 *
 * On a bare-metal/RTOS system with no distinction between kernel and
 * user-space, the U/K interface simply calls the _mali_ukk variant of the
 * function by direct function call. In this case, the context returned is the
 * mali_session_data from _mali_ukk_open().
 *
 * The kernel side implementations of the U/K interface expect the first member of the argument structure to
 * be the context created by _mali_uku_open(). On some OS implementations, the meaning of this context
 * will be different between user-side and kernel-side. In which case, the kernel-side will need to replace this context
 * with the kernel-side equivalent, because user-side will not have access to kernel-side data. The context parameter
 * in the argument structure therefore has to be of type input/output.
 *
 * It should be noted that the caller cannot reuse the \c ctx member of U/K
 * argument structure after a U/K call, because it may be overwritten. Instead,
 * the context handle must always be stored  elsewhere, and copied into
 * the appropriate U/K argument structure for each user-side call to
 * the U/K interface. This is not usually a problem, since U/K argument
 * structures are usually placed on the stack.
 *
 * @{ */

_mali_osk_errcode_t _mali_ukk_open( void **context );

_mali_osk_errcode_t _mali_ukk_close( void **context );

 



_mali_osk_errcode_t _mali_ukk_wait_for_notification( _mali_uk_wait_for_notification_s *args );

_mali_osk_errcode_t _mali_ukk_post_notification( _mali_uk_post_notification_s *args );

_mali_osk_errcode_t _mali_ukk_get_api_version( _mali_uk_get_api_version_s *args );

_mali_osk_errcode_t _mali_ukk_get_user_settings(_mali_uk_get_user_settings_s *args);

_mali_osk_errcode_t _mali_ukk_get_user_setting(_mali_uk_get_user_setting_s *args);

 



_mali_osk_errcode_t _mali_ukk_init_mem( _mali_uk_init_mem_s *args );

_mali_osk_errcode_t _mali_ukk_term_mem( _mali_uk_term_mem_s *args );

_mali_osk_errcode_t _mali_ukk_mem_mmap( _mali_uk_mem_mmap_s *args );

_mali_osk_errcode_t _mali_ukk_mem_munmap( _mali_uk_mem_munmap_s *args );

_mali_osk_errcode_t _mali_ukk_query_mmu_page_table_dump_size( _mali_uk_query_mmu_page_table_dump_size_s *args );
_mali_osk_errcode_t _mali_ukk_dump_mmu_page_table( _mali_uk_dump_mmu_page_table_s * args );

_mali_osk_errcode_t _mali_ukk_map_external_mem( _mali_uk_map_external_mem_s *args );

_mali_osk_errcode_t _mali_ukk_unmap_external_mem( _mali_uk_unmap_external_mem_s *args );

#if MALI_USE_UNIFIED_MEMORY_PROVIDER != 0
_mali_osk_errcode_t _mali_ukk_attach_ump_mem( _mali_uk_attach_ump_mem_s *args );
_mali_osk_errcode_t _mali_ukk_release_ump_mem( _mali_uk_release_ump_mem_s *args );
#endif 

/** @brief Determine virtual-to-physical mapping of a contiguous memory range
 * (optional)
 *
 * This allows the user-side to do a virtual-to-physical address translation.
 * In conjunction with _mali_uku_map_external_mem, this can be used to do
 * direct rendering.
 *
 * This function will only succeed on a virtual range that is mapped into the
 * current process, and that is contigious.
 *
 * If va is not page-aligned, then it is rounded down to the next page
 * boundary. The remainer is added to size, such that ((u32)va)+size before
 * rounding is equal to ((u32)va)+size after rounding. The rounded modified
 * va and size will be written out into args on success.
 *
 * If the supplied size is zero, or not a multiple of the system's PAGE_SIZE,
 * then size will be rounded up to the next multiple of PAGE_SIZE before
 * translation occurs. The rounded up size will be written out into args on
 * success.
 *
 * On most OSs, virtual-to-physical address translation is a priveledged
 * function. Therefore, the implementer must validate the range supplied, to
 * ensure they are not providing arbitrary virtual-to-physical address
 * translations. While it is unlikely such a mechanism could be used to
 * compromise the security of a system on its own, it is possible it could be
 * combined with another small security risk to cause a much larger security
 * risk.
 *
 * @note This is an optional part of the interface, and is only used by certain
 * implementations of libEGL. If the platform layer in your libEGL
 * implementation does not require Virtual-to-Physical address translation,
 * then this function need not be implemented. A stub implementation should not
 * be required either, as it would only be removed by the compiler's dead code
 * elimination.
 *
 * @note if implemented, this function is entirely platform-dependant, and does
 * not exist in common code.
 *
 * @param args see _mali_uk_va_to_mali_pa_s in "mali_utgard_uk_types.h"
 * @return _MALI_OSK_ERR_OK on success, otherwise a suitable _mali_osk_errcode_t on failure.
 */
_mali_osk_errcode_t _mali_ukk_va_to_mali_pa( _mali_uk_va_to_mali_pa_s * args );

 



_mali_osk_errcode_t _mali_ukk_pp_start_job( _mali_uk_pp_start_job_s *args );

_mali_osk_errcode_t _mali_ukk_get_pp_number_of_cores( _mali_uk_get_pp_number_of_cores_s *args );

_mali_osk_errcode_t _mali_ukk_get_pp_core_version( _mali_uk_get_pp_core_version_s *args );

void _mali_ukk_pp_job_disable_wb(_mali_uk_pp_disable_wb_s *args);


 



_mali_osk_errcode_t _mali_ukk_gp_start_job( _mali_uk_gp_start_job_s *args );

_mali_osk_errcode_t _mali_ukk_get_gp_number_of_cores( _mali_uk_get_gp_number_of_cores_s *args );

_mali_osk_errcode_t _mali_ukk_get_gp_core_version( _mali_uk_get_gp_core_version_s *args );

_mali_osk_errcode_t _mali_ukk_gp_suspend_response( _mali_uk_gp_suspend_response_s *args );

 

#if MALI_TIMELINE_PROFILING_ENABLED

_mali_osk_errcode_t _mali_ukk_profiling_start(_mali_uk_profiling_start_s *args);

_mali_osk_errcode_t _mali_ukk_profiling_add_event(_mali_uk_profiling_add_event_s *args);

_mali_osk_errcode_t _mali_ukk_profiling_stop(_mali_uk_profiling_stop_s *args);

_mali_osk_errcode_t _mali_ukk_profiling_get_event(_mali_uk_profiling_get_event_s *args);

_mali_osk_errcode_t _mali_ukk_profiling_clear(_mali_uk_profiling_clear_s *args);

 
#endif


_mali_osk_errcode_t _mali_ukk_vsync_event_report(_mali_uk_vsync_event_report_s *args);

 


_mali_osk_errcode_t _mali_ukk_sw_counters_report(_mali_uk_sw_counters_report_s *args);

 

 

 

u32 _mali_ukk_report_memory_usage(void);

#ifdef __cplusplus
}
#endif

#endif 
