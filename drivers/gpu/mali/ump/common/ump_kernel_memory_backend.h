/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef __UMP_KERNEL_MEMORY_BACKEND_H__
#define __UMP_KERNEL_MEMORY_BACKEND_H__

#include "ump_kernel_interface.h"
#include "ump_kernel_types.h"


typedef struct ump_memory_allocation
{
	void                    * phys_addr;
	void                    * mapping;
	unsigned long             size;
	ump_dd_handle             handle;
	void                    * process_mapping_info;
	u32                       cookie;               
	struct ump_session_data * ump_session;          
	_mali_osk_list_t          list;                 
	u32 is_cached;
} ump_memory_allocation;

typedef struct ump_memory_backend
{
	int  (*allocate)(void* ctx, ump_dd_mem * descriptor);
	void (*release)(void* ctx, ump_dd_mem * descriptor);
	void (*shutdown)(struct ump_memory_backend * backend);
	u32  (*stat)(struct ump_memory_backend *backend);
	int  (*pre_allocate_physical_check)(void *ctx, u32 size);
	u32  (*adjust_to_mali_phys)(void *ctx, u32 cpu_phys);
	void * ctx;
} ump_memory_backend;

ump_memory_backend * ump_memory_backend_create ( void );
void ump_memory_backend_destroy( void );

#endif 

