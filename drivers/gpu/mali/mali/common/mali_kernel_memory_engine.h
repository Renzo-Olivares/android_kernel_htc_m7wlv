/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MALI_KERNEL_MEMORY_ENGINE_H__
#define  __MALI_KERNEL_MEMORY_ENGINE_H__

typedef void * mali_allocation_engine;

typedef enum { MALI_MEM_ALLOC_FINISHED, MALI_MEM_ALLOC_PARTIAL, MALI_MEM_ALLOC_NONE, MALI_MEM_ALLOC_INTERNAL_FAILURE } mali_physical_memory_allocation_result;

typedef struct mali_physical_memory_allocation
{
	void (*release)(void * ctx, void * handle); 
	void * ctx;
	void * handle;
	struct mali_physical_memory_allocation * next;
} mali_physical_memory_allocation;

struct mali_page_table_block;

typedef struct mali_page_table_block
{
	void (*release)(struct mali_page_table_block *page_table_block);
	void * ctx;
	void * handle;
	u32 size; 
	u32 phys_base; 
	mali_io_address mapping;
} mali_page_table_block;



typedef enum
{
	MALI_MEMORY_ALLOCATION_FLAG_MAP_INTO_USERSPACE = 0x1,
	MALI_MEMORY_ALLOCATION_FLAG_MAP_GUARD_PAGE     = 0x2,
} mali_memory_allocation_flag;

#define MALI_MEMORY_ALLOCATION_OS_ALLOCATED_PHYSADDR_MAGIC ((u32)(-1))

typedef struct mali_memory_allocation
{
	
	void * mapping; 
	u32 mali_address; 
	u32 size; 
	u32 permission; 
	mali_memory_allocation_flag flags;
	u32 cache_settings; 

	_mali_osk_lock_t * lock;

	
	void * mali_addr_mapping_info; 
	void * process_addr_mapping_info; 

	mali_physical_memory_allocation physical_allocation;

	_mali_osk_list_t list; 
} mali_memory_allocation;
 


typedef struct mali_physical_memory_allocator
{
	mali_physical_memory_allocation_result (*allocate)(void* ctx, mali_allocation_engine * engine, mali_memory_allocation * descriptor, u32* offset, mali_physical_memory_allocation * alloc_info);
	mali_physical_memory_allocation_result (*allocate_page_table_block)(void * ctx, mali_page_table_block * block); 
	void (*destroy)(struct mali_physical_memory_allocator * allocator);
	u32 (*stat)(struct mali_physical_memory_allocator * allocator);
	void * ctx;
	const char * name; 
	u32 alloc_order; 
	struct mali_physical_memory_allocator * next;
} mali_physical_memory_allocator;

typedef struct mali_kernel_mem_address_manager
{
	_mali_osk_errcode_t (*allocate)(mali_memory_allocation *); 
	void (*release)(mali_memory_allocation *); 

	 _mali_osk_errcode_t (*map_physical)(mali_memory_allocation * descriptor, u32 offset, u32 *phys_addr, u32 size);

	void (*unmap_physical)(mali_memory_allocation * descriptor, u32 offset, u32 size, _mali_osk_mem_mapregion_flags_t flags);

} mali_kernel_mem_address_manager;

mali_allocation_engine mali_allocation_engine_create(mali_kernel_mem_address_manager * mali_address_manager, mali_kernel_mem_address_manager * process_address_manager);

void mali_allocation_engine_destroy(mali_allocation_engine engine);

int mali_allocation_engine_allocate_memory(mali_allocation_engine engine, mali_memory_allocation * descriptor, mali_physical_memory_allocator * physical_provider, _mali_osk_list_t *tracking_list );
void mali_allocation_engine_release_memory(mali_allocation_engine engine, mali_memory_allocation * descriptor);

void mali_allocation_engine_release_pt1_mali_pagetables_unmap(mali_allocation_engine engine, mali_memory_allocation * descriptor);
void mali_allocation_engine_release_pt2_physical_memory_free(mali_allocation_engine engine, mali_memory_allocation * descriptor);

int mali_allocation_engine_map_physical(mali_allocation_engine engine, mali_memory_allocation * descriptor, u32 offset, u32 phys, u32 cpu_usage_adjust, u32 size);
void mali_allocation_engine_unmap_physical(mali_allocation_engine engine, mali_memory_allocation * descriptor, u32 offset, u32 size, _mali_osk_mem_mapregion_flags_t unmap_flags);

int mali_allocation_engine_allocate_page_tables(mali_allocation_engine, mali_page_table_block * descriptor, mali_physical_memory_allocator * physical_provider);

void mali_allocation_engine_report_allocators(mali_physical_memory_allocator * physical_provider);

u32 mali_allocation_engine_memory_usage(mali_physical_memory_allocator *allocator);

#endif 
