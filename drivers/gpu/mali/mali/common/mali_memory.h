/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MALI_MEMORY_H__
#define __MALI_MEMORY_H__

#include "mali_osk.h"
#include "mali_session.h"

struct mali_cluster;
struct mali_group;

_mali_osk_errcode_t mali_memory_initialize(void);

void mali_memory_terminate(void);

_mali_osk_errcode_t mali_memory_session_begin(struct mali_session_data *mali_session_data);

void mali_memory_session_end(struct mali_session_data *mali_session_data);

_mali_osk_errcode_t mali_mmu_get_table_page(u32 *table_page, mali_io_address *mapping);

void mali_mmu_release_table_page(u32 pa);


_mali_osk_errcode_t mali_memory_core_resource_os_memory(_mali_osk_resource_t * resource);

_mali_osk_errcode_t mali_memory_core_resource_dedicated_memory(_mali_osk_resource_t * resource);

mali_allocation_engine mali_mem_get_memory_engine(void);

#endif 
