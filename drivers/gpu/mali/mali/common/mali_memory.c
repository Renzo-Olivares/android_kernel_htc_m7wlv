/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mali_kernel_common.h"
#include "mali_kernel_descriptor_mapping.h"
#include "mali_mem_validation.h"
#include "mali_memory.h"
#include "mali_mmu_page_directory.h"
#include "mali_kernel_memory_engine.h"
#include "mali_block_allocator.h"
#include "mali_kernel_mem_os.h"
#include "mali_session.h"
#include "mali_l2_cache.h"
#include "mali_cluster.h"
#include "mali_group.h"
#if MALI_USE_UNIFIED_MEMORY_PROVIDER != 0
#include "ump_kernel_interface.h"
#endif

#include "mali_osk.h"
#include "mali_osk_mali.h"
#include "mali_ukk.h"
#include "mali_osk_list.h"
#include "mali_osk_bitops.h"

#ifdef SPRD_MEM_OPT_UMP_DEFRAGMENTIZE
#include <linux/string.h>
#endif

#define MALI_MEM_DESCRIPTORS_INIT 64
#define MALI_MEM_DESCRIPTORS_MAX 65536

typedef struct dedicated_memory_info
{
	u32 base;
	u32 size;
	struct dedicated_memory_info * next;
} dedicated_memory_info;

#if MALI_USE_UNIFIED_MEMORY_PROVIDER != 0
typedef struct ump_mem_allocation
{
	mali_allocation_engine * engine;
	mali_memory_allocation * descriptor;
	u32 initial_offset;
	u32 size_allocated;
	ump_dd_handle ump_mem;
} ump_mem_allocation ;
#endif

typedef struct external_mem_allocation
{
	mali_allocation_engine * engine;
	mali_memory_allocation * descriptor;
	u32 initial_offset;
	u32 size;
} external_mem_allocation;

static _mali_osk_errcode_t _mali_ukk_mem_munmap_internal( _mali_uk_mem_munmap_s *args );

#if MALI_USE_UNIFIED_MEMORY_PROVIDER != 0
static void ump_memory_release(void * ctx, void * handle);
static mali_physical_memory_allocation_result ump_memory_commit(void* ctx, mali_allocation_engine * engine, mali_memory_allocation * descriptor, u32* offset, mali_physical_memory_allocation * alloc_info);
#endif 


static void external_memory_release(void * ctx, void * handle);
static mali_physical_memory_allocation_result external_memory_commit(void* ctx, mali_allocation_engine * engine, mali_memory_allocation * descriptor, u32* offset, mali_physical_memory_allocation * alloc_info);



static _mali_osk_errcode_t  mali_address_manager_allocate(mali_memory_allocation * descriptor); 
static _mali_osk_errcode_t  mali_address_manager_map(mali_memory_allocation * descriptor, u32 offset, u32 *phys_addr, u32 size);
static void mali_address_manager_release(mali_memory_allocation * descriptor);


typedef struct mali_mmu_page_table_allocation
{
	_mali_osk_list_t list;
	u32 * usage_map;
	u32 usage_count;
	u32 num_pages;
	mali_page_table_block pages;
} mali_mmu_page_table_allocation;

typedef struct mali_mmu_page_table_allocations
{
	_mali_osk_lock_t *lock;
	_mali_osk_list_t partial;
	_mali_osk_list_t full;
	
} mali_mmu_page_table_allocations;

static mali_kernel_mem_address_manager mali_address_manager =
{
	mali_address_manager_allocate, 
	mali_address_manager_release,  
	mali_address_manager_map,      
	NULL                           
};

static struct mali_mmu_page_table_allocations page_table_cache;


static mali_kernel_mem_address_manager process_address_manager =
{
	_mali_osk_mem_mapregion_init,  
	_mali_osk_mem_mapregion_term,  
	_mali_osk_mem_mapregion_map,   
	_mali_osk_mem_mapregion_unmap  
};

static _mali_osk_errcode_t mali_mmu_page_table_cache_create(void);
static void mali_mmu_page_table_cache_destroy(void);

static mali_allocation_engine memory_engine = NULL;
static mali_physical_memory_allocator * physical_memory_allocators = NULL;

static dedicated_memory_info * mem_region_registrations = NULL;

mali_allocation_engine mali_mem_get_memory_engine(void)
{
	return memory_engine;
}

_mali_osk_errcode_t mali_memory_initialize(void)
{
	_mali_osk_errcode_t err;

	MALI_DEBUG_PRINT(2, ("Memory system initializing\n"));

	err = mali_mmu_page_table_cache_create();
	if(_MALI_OSK_ERR_OK != err)
	{
		MALI_ERROR(err);
	}

	memory_engine = mali_allocation_engine_create(&mali_address_manager, &process_address_manager);
	MALI_CHECK_NON_NULL( memory_engine, _MALI_OSK_ERR_FAULT);

	MALI_SUCCESS;
}

void mali_memory_terminate(void)
{
	MALI_DEBUG_PRINT(2, ("Memory system terminating\n"));

	mali_mmu_page_table_cache_destroy();

	while ( NULL != mem_region_registrations)
	{
		dedicated_memory_info * m;
		m = mem_region_registrations;
		mem_region_registrations = m->next;
		_mali_osk_mem_unreqregion(m->base, m->size);
		_mali_osk_free(m);
	}

	while ( NULL != physical_memory_allocators)
	{
		mali_physical_memory_allocator * m;
		m = physical_memory_allocators;
		physical_memory_allocators = m->next;
		m->destroy(m);
	}

	if (NULL != memory_engine)
	{
		mali_allocation_engine_destroy(memory_engine);
		memory_engine = NULL;
	}
}

_mali_osk_errcode_t mali_memory_session_begin(struct mali_session_data * session_data)
{
	MALI_DEBUG_PRINT(5, ("Memory session begin\n"));

	
	session_data->descriptor_mapping = mali_descriptor_mapping_create(MALI_MEM_DESCRIPTORS_INIT, MALI_MEM_DESCRIPTORS_MAX);

	if (NULL == session_data->descriptor_mapping)
	{
		MALI_ERROR(_MALI_OSK_ERR_NOMEM);
	}

	session_data->memory_lock = _mali_osk_lock_init( _MALI_OSK_LOCKFLAG_ORDERED | _MALI_OSK_LOCKFLAG_ONELOCK
	                                | _MALI_OSK_LOCKFLAG_NONINTERRUPTABLE, 0, _MALI_OSK_LOCK_ORDER_MEM_SESSION);
	if (NULL == session_data->memory_lock)
	{
		mali_descriptor_mapping_destroy(session_data->descriptor_mapping);
		_mali_osk_free(session_data);
		MALI_ERROR(_MALI_OSK_ERR_FAULT);
	}

	
	_MALI_OSK_INIT_LIST_HEAD( &session_data->memory_head );

	MALI_DEBUG_PRINT(5, ("MMU session begin: success\n"));
	MALI_SUCCESS;
}

static void descriptor_table_cleanup_callback(int descriptor_id, void* map_target)
{
	mali_memory_allocation * descriptor;

	descriptor = (mali_memory_allocation*)map_target;

	MALI_DEBUG_PRINT(3, ("Cleanup of descriptor %d mapping to 0x%x in descriptor table\n", descriptor_id, map_target));
	MALI_DEBUG_ASSERT(descriptor);

	mali_allocation_engine_release_memory(memory_engine, descriptor);
	_mali_osk_free(descriptor);
}

void mali_memory_session_end(struct mali_session_data *session_data)
{
	_mali_osk_errcode_t err = _MALI_OSK_ERR_BUSY;

	MALI_DEBUG_PRINT(3, ("MMU session end\n"));

	if (NULL == session_data)
	{
		MALI_DEBUG_PRINT(1, ("No session data found during session end\n"));
		return;
	}

	while (err == _MALI_OSK_ERR_BUSY)
	{
		
		_mali_osk_lock_wait( session_data->memory_lock, _MALI_OSK_LOCKMODE_RW );
		err = _MALI_OSK_ERR_OK;

		
		if (0 == _mali_osk_list_empty(&session_data->memory_head))
		{
			mali_memory_allocation *descriptor;
			mali_memory_allocation *temp;
			_mali_uk_mem_munmap_s unmap_args;

			MALI_DEBUG_PRINT(1, ("Memory found on session usage list during session termination\n"));

			unmap_args.ctx = session_data;

			
			_MALI_OSK_LIST_FOREACHENTRY(descriptor, temp, &session_data->memory_head, mali_memory_allocation, list)
			{
				MALI_DEBUG_PRINT(4, ("Freeing block with mali address 0x%x size %d mapped in user space at 0x%x\n",
							descriptor->mali_address, descriptor->size, descriptor->size, descriptor->mapping)
						);
				
				MALI_DEBUG_ASSERT(  descriptor->lock == session_data->memory_lock );
				

				unmap_args.size = descriptor->size;
				unmap_args.mapping = descriptor->mapping;
				unmap_args.cookie = (u32)descriptor;

				err = _mali_ukk_mem_munmap_internal( &unmap_args );

				if (err == _MALI_OSK_ERR_BUSY)
				{
					_mali_osk_lock_signal( session_data->memory_lock, _MALI_OSK_LOCKMODE_RW );
					_mali_osk_time_ubusydelay(10);
					break; 
				}
			}
		}
	}
	
	MALI_DEBUG_ASSERT( _mali_osk_list_empty(&session_data->memory_head) );

	if (NULL != session_data->descriptor_mapping)
	{
		mali_descriptor_mapping_call_for_each(session_data->descriptor_mapping, descriptor_table_cleanup_callback);
		mali_descriptor_mapping_destroy(session_data->descriptor_mapping);
		session_data->descriptor_mapping = NULL;
	}

	_mali_osk_lock_signal( session_data->memory_lock, _MALI_OSK_LOCKMODE_RW );


	
	_mali_osk_lock_term( session_data->memory_lock );

	return;
}

_mali_osk_errcode_t mali_memory_core_resource_os_memory(_mali_osk_resource_t * resource)
{
	mali_physical_memory_allocator * allocator;
	mali_physical_memory_allocator ** next_allocator_list;

	u32 alloc_order = resource->alloc_order;

	allocator = mali_os_allocator_create(resource->size, resource->cpu_usage_adjust, resource->description);
	if (NULL == allocator)
	{
		MALI_DEBUG_PRINT(1, ("Failed to create OS memory allocator\n"));
		MALI_ERROR(_MALI_OSK_ERR_FAULT);
	}

	allocator->alloc_order = alloc_order;

	next_allocator_list = &physical_memory_allocators;

	while (NULL != *next_allocator_list &&
			(*next_allocator_list)->alloc_order < alloc_order )
	{
		next_allocator_list = &((*next_allocator_list)->next);
	}

	allocator->next = (*next_allocator_list);
	(*next_allocator_list) = allocator;

	MALI_SUCCESS;
}

_mali_osk_errcode_t mali_memory_core_resource_dedicated_memory(_mali_osk_resource_t * resource)
{
	mali_physical_memory_allocator * allocator;
	mali_physical_memory_allocator ** next_allocator_list;
	dedicated_memory_info * cleanup_data;

	u32 alloc_order = resource->alloc_order;

	

	
	if (_MALI_OSK_ERR_OK != _mali_osk_mem_reqregion(resource->base, resource->size, resource->description))
	{
		MALI_DEBUG_PRINT(1, ("Failed to request memory region %s (0x%08X - 0x%08X)\n", resource->description, resource->base, resource->base + resource->size - 1));
		MALI_ERROR(_MALI_OSK_ERR_FAULT);
	}

	
	allocator = mali_block_allocator_create(resource->base, resource->cpu_usage_adjust, resource->size, resource->description );

	if (NULL == allocator)
	{
		MALI_DEBUG_PRINT(1, ("Memory bank registration failed\n"));
		_mali_osk_mem_unreqregion(resource->base, resource->size);
		MALI_ERROR(_MALI_OSK_ERR_FAULT);
	}

	
	allocator->alloc_order = alloc_order;

	cleanup_data = _mali_osk_malloc(sizeof(dedicated_memory_info));

	if (NULL == cleanup_data)
	{
		_mali_osk_mem_unreqregion(resource->base, resource->size);
		allocator->destroy(allocator);
		MALI_ERROR(_MALI_OSK_ERR_FAULT);
	}

	cleanup_data->base = resource->base;
	cleanup_data->size = resource->size;

	cleanup_data->next = mem_region_registrations;
	mem_region_registrations = cleanup_data;

	next_allocator_list = &physical_memory_allocators;

	while ( NULL != *next_allocator_list &&
			(*next_allocator_list)->alloc_order < alloc_order )
	{
		next_allocator_list = &((*next_allocator_list)->next);
	}

	allocator->next = (*next_allocator_list);
	(*next_allocator_list) = allocator;

	MALI_SUCCESS;
}

#ifdef SPRD_MEM_OPT_UMP_DEFRAGMENTIZE
DEFINE_SPINLOCK(ump_block_buf_lock);
#define SPRD_UMP_BLOCK_BUF_LEN 16384
#define SPRD_UMP_BLOCK_BUF_NUM 3
static int ump_blocks_count[SPRD_UMP_BLOCK_BUF_NUM] = {0, 0, 0};
static unsigned char ump_blocks_buf[SPRD_UMP_BLOCK_BUF_NUM][SPRD_UMP_BLOCK_BUF_LEN];

void *_do_ump_block_alloc(u32 size)
{
	int i;
	unsigned char *ptr;

	if(size <= SPRD_UMP_BLOCK_BUF_LEN)
	{
		spin_lock(&ump_block_buf_lock);
		for(i = 0; i < SPRD_UMP_BLOCK_BUF_NUM; i++) {
			if(ump_blocks_count[i] == 0) {
				ump_blocks_count[i] = 1;
				ptr = (unsigned char *)ump_blocks_buf[i];
				break;
			}
		}
		spin_unlock(&ump_block_buf_lock);
	}
	else
		i = SPRD_UMP_BLOCK_BUF_NUM;

	if(i == SPRD_UMP_BLOCK_BUF_NUM)
		ptr = _mali_osk_calloc(1, size);
	else
		memset(ptr, 0, SPRD_UMP_BLOCK_BUF_LEN);

	return (void *)ptr;
}
void _do_ump_block_free(void *ptr)
{
	int i;

	spin_lock(&ump_block_buf_lock);
	for(i = 0; i < SPRD_UMP_BLOCK_BUF_NUM; i++) {
		if((unsigned long)ptr == (unsigned long)ump_blocks_buf[i]) {
			ump_blocks_count[i] = 0;
			break;
		}
	}
	spin_unlock(&ump_block_buf_lock);

	if(i == SPRD_UMP_BLOCK_BUF_NUM)
		_mali_osk_free(ptr);
}
#endif

#if MALI_USE_UNIFIED_MEMORY_PROVIDER != 0
static mali_physical_memory_allocation_result ump_memory_commit(void* ctx, mali_allocation_engine * engine, mali_memory_allocation * descriptor, u32* offset, mali_physical_memory_allocation * alloc_info)
{
	ump_dd_handle ump_mem;
	u32 nr_blocks;
	u32 i;
	ump_dd_physical_block * ump_blocks;
	ump_mem_allocation *ret_allocation;

	MALI_DEBUG_ASSERT_POINTER(ctx);
	MALI_DEBUG_ASSERT_POINTER(engine);
	MALI_DEBUG_ASSERT_POINTER(descriptor);
	MALI_DEBUG_ASSERT_POINTER(alloc_info);

	ret_allocation = _mali_osk_malloc( sizeof( ump_mem_allocation ) );
	if ( NULL==ret_allocation ) return MALI_MEM_ALLOC_INTERNAL_FAILURE;

	ump_mem = (ump_dd_handle)ctx;

	MALI_DEBUG_PRINT(4, ("In ump_memory_commit\n"));

	nr_blocks = ump_dd_phys_block_count_get(ump_mem);

	MALI_DEBUG_PRINT(4, ("Have %d blocks\n", nr_blocks));

	if (nr_blocks == 0)
	{
		MALI_DEBUG_PRINT(1, ("No block count\n"));
		_mali_osk_free( ret_allocation );
		return MALI_MEM_ALLOC_INTERNAL_FAILURE;
	}

#ifdef SPRD_MEM_OPT_UMP_DEFRAGMENTIZE
	ump_blocks = _do_ump_block_alloc(sizeof(*ump_blocks)*nr_blocks);
#else
	ump_blocks = _mali_osk_malloc(sizeof(*ump_blocks)*nr_blocks);
#endif
	if ( NULL==ump_blocks )
	{
		_mali_osk_free( ret_allocation );
		return MALI_MEM_ALLOC_INTERNAL_FAILURE;
	}

	if (UMP_DD_INVALID == ump_dd_phys_blocks_get(ump_mem, ump_blocks, nr_blocks))
	{
#ifdef SPRD_MEM_OPT_UMP_DEFRAGMENTIZE
		_do_ump_block_free(ump_blocks);
#else
		_mali_osk_free(ump_blocks);
#endif
		_mali_osk_free( ret_allocation );
		return MALI_MEM_ALLOC_INTERNAL_FAILURE;
	}

	
	ret_allocation->initial_offset = *offset;

	for(i=0; i<nr_blocks; ++i)
	{
		MALI_DEBUG_PRINT(4, ("Mapping in 0x%08x size %d\n", ump_blocks[i].addr , ump_blocks[i].size));
		if (_MALI_OSK_ERR_OK != mali_allocation_engine_map_physical(engine, descriptor, *offset, ump_blocks[i].addr , 0, ump_blocks[i].size ))
		{
			u32 size_allocated = *offset - ret_allocation->initial_offset;
			MALI_DEBUG_PRINT(1, ("Mapping of external memory failed\n"));

			
			mali_allocation_engine_unmap_physical(engine, descriptor, ret_allocation->initial_offset, size_allocated, (_mali_osk_mem_mapregion_flags_t)0 );

#ifdef SPRD_MEM_OPT_UMP_DEFRAGMENTIZE
			_do_ump_block_free(ump_blocks);
#else
			_mali_osk_free(ump_blocks);
#endif
			_mali_osk_free(ret_allocation);
			return MALI_MEM_ALLOC_INTERNAL_FAILURE;
		}
		*offset += ump_blocks[i].size;
	}

	if (descriptor->flags & MALI_MEMORY_ALLOCATION_FLAG_MAP_GUARD_PAGE)
	{
		
		MALI_DEBUG_PRINT(4, ("Mapping in extra guard page\n"));
		if (_MALI_OSK_ERR_OK != mali_allocation_engine_map_physical(engine, descriptor, *offset, ump_blocks[0].addr , 0, _MALI_OSK_MALI_PAGE_SIZE ))
		{
			u32 size_allocated = *offset - ret_allocation->initial_offset;
			MALI_DEBUG_PRINT(1, ("Mapping of external memory (guard page) failed\n"));

			
			mali_allocation_engine_unmap_physical(engine, descriptor, ret_allocation->initial_offset, size_allocated, (_mali_osk_mem_mapregion_flags_t)0 );

#ifdef SPRD_MEM_OPT_UMP_DEFRAGMENTIZE
			_do_ump_block_free(ump_blocks);
#else
			_mali_osk_free(ump_blocks);
#endif
			_mali_osk_free(ret_allocation);
			return MALI_MEM_ALLOC_INTERNAL_FAILURE;
		}
		*offset += _MALI_OSK_MALI_PAGE_SIZE;
	}

#ifdef SPRD_MEM_OPT_UMP_DEFRAGMENTIZE
	_do_ump_block_free(ump_blocks);
#else
	_mali_osk_free( ump_blocks );
#endif

	ret_allocation->engine = engine;
	ret_allocation->descriptor = descriptor;
	ret_allocation->ump_mem = ump_mem;
	ret_allocation->size_allocated = *offset - ret_allocation->initial_offset;

	alloc_info->ctx = NULL;
	alloc_info->handle = ret_allocation;
	alloc_info->next = NULL;
	alloc_info->release = ump_memory_release;

	return MALI_MEM_ALLOC_FINISHED;
}

static void ump_memory_release(void * ctx, void * handle)
{
	ump_dd_handle ump_mem;
	ump_mem_allocation *allocation;

	allocation = (ump_mem_allocation *)handle;

	MALI_DEBUG_ASSERT_POINTER( allocation );

	ump_mem = allocation->ump_mem;

	MALI_DEBUG_ASSERT(UMP_DD_HANDLE_INVALID!=ump_mem);

	mali_allocation_engine_unmap_physical( allocation->engine,
										   allocation->descriptor,
										   allocation->initial_offset,
										   allocation->size_allocated,
										   (_mali_osk_mem_mapregion_flags_t)0
										   );
	_mali_osk_free( allocation );


	ump_dd_reference_release(ump_mem) ;
	return;
}

_mali_osk_errcode_t _mali_ukk_attach_ump_mem( _mali_uk_attach_ump_mem_s *args )
{
	ump_dd_handle ump_mem;
	mali_physical_memory_allocator external_memory_allocator;
	struct mali_session_data *session_data;
	mali_memory_allocation * descriptor;
	int md;

  	MALI_DEBUG_ASSERT_POINTER(args);
  	MALI_CHECK_NON_NULL(args->ctx, _MALI_OSK_ERR_INVALID_ARGS);

	session_data = (struct mali_session_data *)args->ctx;
	MALI_CHECK_NON_NULL(session_data, _MALI_OSK_ERR_INVALID_ARGS);

	
	
	if ( ! args->size) MALI_ERROR(_MALI_OSK_ERR_INVALID_ARGS);

	
	if ( args->size % _MALI_OSK_MALI_PAGE_SIZE ) MALI_ERROR(_MALI_OSK_ERR_INVALID_ARGS);

	MALI_DEBUG_PRINT(3,
	                 ("Requested to map ump memory with secure id %d into virtual memory 0x%08X, size 0x%08X\n",
	                  args->secure_id, args->mali_address, args->size));

	ump_mem = ump_dd_handle_create_from_secure_id( (int)args->secure_id ) ;

	if ( UMP_DD_HANDLE_INVALID==ump_mem ) MALI_ERROR(_MALI_OSK_ERR_FAULT);

	descriptor = _mali_osk_calloc(1, sizeof(mali_memory_allocation));
	if (NULL == descriptor)
	{
		ump_dd_reference_release(ump_mem);
		MALI_ERROR(_MALI_OSK_ERR_NOMEM);
	}

	descriptor->size = args->size;
	descriptor->mapping = NULL;
	descriptor->mali_address = args->mali_address;
	descriptor->mali_addr_mapping_info = (void*)session_data;
	descriptor->process_addr_mapping_info = NULL; 
	descriptor->cache_settings = (u32) MALI_CACHE_STANDARD;
	descriptor->lock = session_data->memory_lock;

	if (args->flags & _MALI_MAP_EXTERNAL_MAP_GUARD_PAGE)
	{
		descriptor->flags = MALI_MEMORY_ALLOCATION_FLAG_MAP_GUARD_PAGE;
	}
	_mali_osk_list_init( &descriptor->list );

	if (_MALI_OSK_ERR_OK != mali_descriptor_mapping_allocate_mapping(session_data->descriptor_mapping, descriptor, &md))
	{
		ump_dd_reference_release(ump_mem);
		_mali_osk_free(descriptor);
		MALI_ERROR(_MALI_OSK_ERR_FAULT);
	}

	external_memory_allocator.allocate = ump_memory_commit;
	external_memory_allocator.allocate_page_table_block = NULL;
	external_memory_allocator.ctx = ump_mem;
	external_memory_allocator.name = "UMP Memory";
	external_memory_allocator.next = NULL;

	_mali_osk_lock_wait(session_data->memory_lock, _MALI_OSK_LOCKMODE_RW);

	if (_MALI_OSK_ERR_OK != mali_allocation_engine_allocate_memory(memory_engine, descriptor, &external_memory_allocator, NULL))
	{
		_mali_osk_lock_signal(session_data->memory_lock, _MALI_OSK_LOCKMODE_RW);
		mali_descriptor_mapping_free(session_data->descriptor_mapping, md);
		ump_dd_reference_release(ump_mem);
		_mali_osk_free(descriptor);
		MALI_ERROR(_MALI_OSK_ERR_NOMEM);
	}

	_mali_osk_lock_signal(session_data->memory_lock, _MALI_OSK_LOCKMODE_RW);

	args->cookie = md;

	MALI_DEBUG_PRINT(5,("Returning from UMP attach\n"));

	
	MALI_SUCCESS;
}


_mali_osk_errcode_t _mali_ukk_release_ump_mem( _mali_uk_release_ump_mem_s *args )
{
	mali_memory_allocation * descriptor;
	struct mali_session_data *session_data;

	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_CHECK_NON_NULL(args->ctx, _MALI_OSK_ERR_INVALID_ARGS);

	session_data = (struct mali_session_data *)args->ctx;
	MALI_CHECK_NON_NULL(session_data, _MALI_OSK_ERR_INVALID_ARGS);

	if (_MALI_OSK_ERR_OK != mali_descriptor_mapping_get(session_data->descriptor_mapping, args->cookie, (void**)&descriptor))
	{
		MALI_DEBUG_PRINT(1, ("Invalid memory descriptor %d used to release ump memory\n", args->cookie));
		MALI_ERROR(_MALI_OSK_ERR_FAULT);
	}

	descriptor = mali_descriptor_mapping_free(session_data->descriptor_mapping, args->cookie);

	if (NULL != descriptor)
	{
		_mali_osk_lock_wait( session_data->memory_lock, _MALI_OSK_LOCKMODE_RW );

		mali_allocation_engine_release_memory(memory_engine, descriptor);

		_mali_osk_lock_signal( session_data->memory_lock, _MALI_OSK_LOCKMODE_RW );

		_mali_osk_free(descriptor);
	}

	MALI_SUCCESS;

}
#endif 


static mali_physical_memory_allocation_result external_memory_commit(void* ctx, mali_allocation_engine * engine, mali_memory_allocation * descriptor, u32* offset, mali_physical_memory_allocation * alloc_info)
{
	u32 * data;
	external_mem_allocation * ret_allocation;

	MALI_DEBUG_ASSERT_POINTER(ctx);
	MALI_DEBUG_ASSERT_POINTER(engine);
	MALI_DEBUG_ASSERT_POINTER(descriptor);
	MALI_DEBUG_ASSERT_POINTER(alloc_info);

	ret_allocation = _mali_osk_malloc( sizeof(external_mem_allocation) );

	if ( NULL == ret_allocation )
	{
		return MALI_MEM_ALLOC_INTERNAL_FAILURE;
	}

	data = (u32*)ctx;

	ret_allocation->engine = engine;
	ret_allocation->descriptor = descriptor;
	ret_allocation->initial_offset = *offset;

	alloc_info->ctx = NULL;
	alloc_info->handle = ret_allocation;
	alloc_info->next = NULL;
	alloc_info->release = external_memory_release;

	MALI_DEBUG_PRINT(5, ("External map: mapping phys 0x%08X at mali virtual address 0x%08X staring at offset 0x%08X length 0x%08X\n", data[0], descriptor->mali_address, *offset, data[1]));

	if (_MALI_OSK_ERR_OK != mali_allocation_engine_map_physical(engine, descriptor, *offset, data[0], 0, data[1]))
	{
		MALI_DEBUG_PRINT(1, ("Mapping of external memory failed\n"));
		_mali_osk_free(ret_allocation);
		return MALI_MEM_ALLOC_INTERNAL_FAILURE;
	}
	*offset += data[1];

	if (descriptor->flags & MALI_MEMORY_ALLOCATION_FLAG_MAP_GUARD_PAGE)
	{
		
		MALI_DEBUG_PRINT(4, ("Mapping in extra guard page\n"));
		if (_MALI_OSK_ERR_OK != mali_allocation_engine_map_physical(engine, descriptor, *offset, data[0], 0, _MALI_OSK_MALI_PAGE_SIZE))
		{
			u32 size_allocated = *offset - ret_allocation->initial_offset;
			MALI_DEBUG_PRINT(1, ("Mapping of external memory (guard page) failed\n"));

			
			mali_allocation_engine_unmap_physical(engine, descriptor, ret_allocation->initial_offset, size_allocated, (_mali_osk_mem_mapregion_flags_t)0 );
			_mali_osk_free(ret_allocation);
			return MALI_MEM_ALLOC_INTERNAL_FAILURE;
		}
		*offset += _MALI_OSK_MALI_PAGE_SIZE;
	}

	ret_allocation->size = *offset - ret_allocation->initial_offset;

	return MALI_MEM_ALLOC_FINISHED;
}

static void external_memory_release(void * ctx, void * handle)
{
	external_mem_allocation * allocation;

	allocation = (external_mem_allocation *) handle;
	MALI_DEBUG_ASSERT_POINTER( allocation );


	mali_allocation_engine_unmap_physical( allocation->engine,
										   allocation->descriptor,
										   allocation->initial_offset,
										   allocation->size,
										   (_mali_osk_mem_mapregion_flags_t)0
										   );

	_mali_osk_free( allocation );

	return;
}

_mali_osk_errcode_t _mali_ukk_map_external_mem( _mali_uk_map_external_mem_s *args )
{
	mali_physical_memory_allocator external_memory_allocator;
	struct mali_session_data *session_data;
	u32 info[2];
	mali_memory_allocation * descriptor;
	int md;

	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_CHECK_NON_NULL(args->ctx, _MALI_OSK_ERR_INVALID_ARGS);

	session_data = (struct mali_session_data *)args->ctx;
	MALI_CHECK_NON_NULL(session_data, _MALI_OSK_ERR_INVALID_ARGS);

	external_memory_allocator.allocate = external_memory_commit;
	external_memory_allocator.allocate_page_table_block = NULL;
	external_memory_allocator.ctx = &info[0];
	external_memory_allocator.name = "External Memory";
	external_memory_allocator.next = NULL;

	
	
	if ( ! args->size) MALI_ERROR(_MALI_OSK_ERR_INVALID_ARGS);

	
	if ( args->size % _MALI_OSK_MALI_PAGE_SIZE ) MALI_ERROR(_MALI_OSK_ERR_INVALID_ARGS);

	MALI_DEBUG_PRINT(3,
	        ("Requested to map physical memory 0x%x-0x%x into virtual memory 0x%x\n",
	        (void*)args->phys_addr,
	        (void*)(args->phys_addr + args->size -1),
	        (void*)args->mali_address)
	);

	
	if (_MALI_OSK_ERR_OK != mali_mem_validation_check(args->phys_addr, args->size))
	{
		return _MALI_OSK_ERR_FAULT;
	}

	info[0] = args->phys_addr;
	info[1] = args->size;

	descriptor = _mali_osk_calloc(1, sizeof(mali_memory_allocation));
	if (NULL == descriptor) MALI_ERROR(_MALI_OSK_ERR_NOMEM);

	descriptor->size = args->size;
	descriptor->mapping = NULL;
	descriptor->mali_address = args->mali_address;
	descriptor->mali_addr_mapping_info = (void*)session_data;
	descriptor->process_addr_mapping_info = NULL; 
	descriptor->cache_settings = (u32)MALI_CACHE_STANDARD;
	descriptor->lock = session_data->memory_lock;
	if (args->flags & _MALI_MAP_EXTERNAL_MAP_GUARD_PAGE)
	{
		descriptor->flags = MALI_MEMORY_ALLOCATION_FLAG_MAP_GUARD_PAGE;
	}
	_mali_osk_list_init( &descriptor->list );

	_mali_osk_lock_wait(session_data->memory_lock, _MALI_OSK_LOCKMODE_RW);

	if (_MALI_OSK_ERR_OK != mali_allocation_engine_allocate_memory(memory_engine, descriptor, &external_memory_allocator, NULL))
	{
		_mali_osk_lock_signal(session_data->memory_lock, _MALI_OSK_LOCKMODE_RW);
		_mali_osk_free(descriptor);
		MALI_ERROR(_MALI_OSK_ERR_NOMEM);
	}

	_mali_osk_lock_signal(session_data->memory_lock, _MALI_OSK_LOCKMODE_RW);

	if (_MALI_OSK_ERR_OK != mali_descriptor_mapping_allocate_mapping(session_data->descriptor_mapping, descriptor, &md))
	{
		mali_allocation_engine_release_memory(memory_engine, descriptor);
		_mali_osk_free(descriptor);
		MALI_ERROR(_MALI_OSK_ERR_FAULT);
	}

	args->cookie = md;

	MALI_DEBUG_PRINT(5,("Returning from range_map_external_memory\n"));

	
	MALI_SUCCESS;
}


_mali_osk_errcode_t _mali_ukk_unmap_external_mem( _mali_uk_unmap_external_mem_s *args )
{
	mali_memory_allocation * descriptor;
	void* old_value;
	struct mali_session_data *session_data;

	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_CHECK_NON_NULL(args->ctx, _MALI_OSK_ERR_INVALID_ARGS);

	session_data = (struct mali_session_data *)args->ctx;
	MALI_CHECK_NON_NULL(session_data, _MALI_OSK_ERR_INVALID_ARGS);

	if (_MALI_OSK_ERR_OK != mali_descriptor_mapping_get(session_data->descriptor_mapping, args->cookie, (void**)&descriptor))
	{
		MALI_DEBUG_PRINT(1, ("Invalid memory descriptor %d used to unmap external memory\n", args->cookie));
		MALI_ERROR(_MALI_OSK_ERR_FAULT);
	}

	old_value = mali_descriptor_mapping_free(session_data->descriptor_mapping, args->cookie);

	if (NULL != old_value)
	{
		_mali_osk_lock_wait( session_data->memory_lock, _MALI_OSK_LOCKMODE_RW );

		mali_allocation_engine_release_memory(memory_engine, descriptor);

		_mali_osk_lock_signal( session_data->memory_lock, _MALI_OSK_LOCKMODE_RW );

		_mali_osk_free(descriptor);
	}

	MALI_SUCCESS;
}

_mali_osk_errcode_t _mali_ukk_init_mem( _mali_uk_init_mem_s *args )
{
	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_CHECK_NON_NULL(args->ctx, _MALI_OSK_ERR_INVALID_ARGS);
#ifdef SPRD_MEM_OPT_PAGE_TABLE_SHRINK
	args->memory_size       = ARCH_MALI_MEMORY_SIZE_DEFAULT;
	args->mali_address_base = ARCH_MALI_MEMORY_BASE_DEFAULT;
#else
	args->memory_size = 2 * 1024 * 1024 * 1024UL; 
	args->mali_address_base = 1 * 1024 * 1024 * 1024UL; 
#endif
	MALI_SUCCESS;
}

_mali_osk_errcode_t _mali_ukk_term_mem( _mali_uk_term_mem_s *args )
{
	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_CHECK_NON_NULL(args->ctx, _MALI_OSK_ERR_INVALID_ARGS);
	MALI_SUCCESS;
}

static _mali_osk_errcode_t mali_address_manager_allocate(mali_memory_allocation * descriptor)
{
	struct mali_session_data *session_data;
	u32 actual_size;

	MALI_DEBUG_ASSERT_POINTER(descriptor);

	session_data = (struct mali_session_data *)descriptor->mali_addr_mapping_info;

	actual_size = descriptor->size;

	if (descriptor->flags & MALI_MEMORY_ALLOCATION_FLAG_MAP_GUARD_PAGE)
	{
		actual_size += _MALI_OSK_MALI_PAGE_SIZE;
	}

	return mali_mmu_pagedir_map(session_data->page_directory, descriptor->mali_address, actual_size);
}

static void mali_address_manager_release(mali_memory_allocation * descriptor)
{
	const u32 illegal_mali_address = 0xffffffff;
	struct mali_session_data *session_data;
	MALI_DEBUG_ASSERT_POINTER(descriptor);

	if ( illegal_mali_address == descriptor->mali_address) return;

	session_data = (struct mali_session_data *)descriptor->mali_addr_mapping_info;
	mali_mmu_pagedir_unmap(session_data->page_directory, descriptor->mali_address, descriptor->size);

	descriptor->mali_address = illegal_mali_address ;
}

static _mali_osk_errcode_t mali_address_manager_map(mali_memory_allocation * descriptor, u32 offset, u32 *phys_addr, u32 size)
{
	struct mali_session_data *session_data;
	u32 mali_address;

	MALI_DEBUG_ASSERT_POINTER(descriptor);
	MALI_DEBUG_ASSERT_POINTER(phys_addr);

	session_data = (struct mali_session_data *)descriptor->mali_addr_mapping_info;
	MALI_DEBUG_ASSERT_POINTER(session_data);

	mali_address = descriptor->mali_address + offset;

	MALI_DEBUG_PRINT(7, ("Mali map: mapping 0x%08X to Mali address 0x%08X length 0x%08X\n", *phys_addr, mali_address, size));

	mali_mmu_pagedir_update(session_data->page_directory, mali_address, *phys_addr, size, descriptor->cache_settings);

	MALI_SUCCESS;
}

_mali_osk_errcode_t _mali_ukk_mem_mmap( _mali_uk_mem_mmap_s *args )
{
	struct mali_session_data *session_data;
	mali_memory_allocation * descriptor;

	
	if (NULL == args) { MALI_DEBUG_PRINT(3,("mali_ukk_mem_mmap: args was NULL\n")); MALI_ERROR(_MALI_OSK_ERR_INVALID_ARGS); }

	
	session_data = (struct mali_session_data *)args->ctx;

	
	if (NULL == session_data) { MALI_DEBUG_PRINT(3,("mali_ukk_mem_mmap: session data was NULL\n")); MALI_ERROR(_MALI_OSK_ERR_FAULT); }

	descriptor = (mali_memory_allocation*) _mali_osk_calloc( 1, sizeof(mali_memory_allocation) );
	if (NULL == descriptor) { MALI_DEBUG_PRINT(3,("mali_ukk_mem_mmap: descriptor was NULL\n")); MALI_ERROR(_MALI_OSK_ERR_NOMEM); }

	descriptor->size = args->size;
	descriptor->mali_address = args->phys_addr;
	descriptor->mali_addr_mapping_info = (void*)session_data;

	descriptor->process_addr_mapping_info = args->ukk_private; 
	descriptor->flags = MALI_MEMORY_ALLOCATION_FLAG_MAP_INTO_USERSPACE;
	descriptor->cache_settings = (u32) args->cache_settings ;
	descriptor->lock = session_data->memory_lock;
	_mali_osk_list_init( &descriptor->list );

	_mali_osk_lock_wait(session_data->memory_lock, _MALI_OSK_LOCKMODE_RW);

	if (0 == mali_allocation_engine_allocate_memory(memory_engine, descriptor, physical_memory_allocators, &session_data->memory_head))
	{
		
	   	_mali_osk_lock_signal(session_data->memory_lock, _MALI_OSK_LOCKMODE_RW);

		args->mapping = descriptor->mapping;
		args->cookie = (u32)descriptor;

		MALI_DEBUG_PRINT(7, ("MMAP OK\n"));
		MALI_SUCCESS;
	}
	else
	{
	   	_mali_osk_lock_signal(session_data->memory_lock, _MALI_OSK_LOCKMODE_RW);
		
		MALI_DEBUG_PRINT(4, ("Memory allocation failure, OOM\n"));
		_mali_osk_free(descriptor);
		
		MALI_ERROR(_MALI_OSK_ERR_FAULT);
	}
}

static _mali_osk_errcode_t _mali_ukk_mem_munmap_internal( _mali_uk_mem_munmap_s *args )
{
	struct mali_session_data *session_data;
	mali_memory_allocation * descriptor;

	u32 num_groups = mali_group_get_glob_num_groups();
	struct mali_group *group;
	u32 i;

	descriptor = (mali_memory_allocation *)args->cookie;
	MALI_DEBUG_ASSERT_POINTER(descriptor);

	

	session_data = (struct mali_session_data *)descriptor->mali_addr_mapping_info;
	MALI_DEBUG_ASSERT_POINTER(session_data);

	mali_allocation_engine_release_pt1_mali_pagetables_unmap(memory_engine, descriptor);

#ifdef MALI_UNMAP_FLUSH_ALL_MALI_L2
	{
		u32 number_of_clusters = mali_cluster_get_glob_num_clusters();
		for (i = 0; i < number_of_clusters; i++)
		{
			struct mali_cluster *cluster;
			cluster = mali_cluster_get_global_cluster(i);
			if( mali_cluster_power_is_enabled_get(cluster) )
			{
				mali_cluster_l2_cache_invalidate_all_force(cluster);
			}
		}
	}
#endif

	for (i = 0; i < num_groups; i++)
	{
		group = mali_group_get_glob_group(i);
		mali_group_lock(group);
		mali_group_remove_session_if_unused(group, session_data);
		if (mali_group_get_session(group) == session_data)
		{
			
			mali_bool zap_success = mali_mmu_zap_tlb(mali_group_get_mmu(group));
			if (MALI_TRUE != zap_success)
			{
				MALI_DEBUG_PRINT(2, ("Mali memory unmap failed. Doing pagefault handling.\n"));
				mali_group_bottom_half(group, GROUP_EVENT_MMU_PAGE_FAULT);
				
				continue;
			}
		}
		mali_group_unlock(group);
	}

	
	mali_allocation_engine_release_pt2_physical_memory_free(memory_engine, descriptor);

	_mali_osk_free(descriptor);

	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t _mali_ukk_mem_munmap( _mali_uk_mem_munmap_s *args )
{
	mali_memory_allocation * descriptor;
	_mali_osk_lock_t *descriptor_lock;
	_mali_osk_errcode_t err;

	descriptor = (mali_memory_allocation *)args->cookie;
	MALI_DEBUG_ASSERT_POINTER(descriptor);

	

	MALI_DEBUG_ASSERT_POINTER((struct mali_session_data *)descriptor->mali_addr_mapping_info);

	descriptor_lock = descriptor->lock; 

	err = _MALI_OSK_ERR_BUSY;
	while (err == _MALI_OSK_ERR_BUSY)
	{
		if (descriptor_lock)
		{
			_mali_osk_lock_wait( descriptor_lock, _MALI_OSK_LOCKMODE_RW );
		}

		err = _mali_ukk_mem_munmap_internal( args );

		if (descriptor_lock)
		{
			_mali_osk_lock_signal( descriptor_lock, _MALI_OSK_LOCKMODE_RW );
		}

		if (err == _MALI_OSK_ERR_BUSY)
		{
			_mali_osk_time_ubusydelay(10);
		}
	}

	return err;
}

u32 _mali_ukk_report_memory_usage(void)
{
	return mali_allocation_engine_memory_usage(physical_memory_allocators);
}

_mali_osk_errcode_t mali_mmu_get_table_page(u32 *table_page, mali_io_address *mapping)
{
	_mali_osk_lock_wait(page_table_cache.lock, _MALI_OSK_LOCKMODE_RW);

	if (0 == _mali_osk_list_empty(&page_table_cache.partial))
	{
		mali_mmu_page_table_allocation * alloc = _MALI_OSK_LIST_ENTRY(page_table_cache.partial.next, mali_mmu_page_table_allocation, list);
		int page_number = _mali_osk_find_first_zero_bit(alloc->usage_map, alloc->num_pages);
		MALI_DEBUG_PRINT(6, ("Partial page table allocation found, using page offset %d\n", page_number));
		_mali_osk_set_nonatomic_bit(page_number, alloc->usage_map);
		alloc->usage_count++;
		if (alloc->num_pages == alloc->usage_count)
		{
			
			_mali_osk_list_move(&alloc->list, &page_table_cache.full);
		}
		_mali_osk_lock_signal(page_table_cache.lock, _MALI_OSK_LOCKMODE_RW);

		*table_page = (MALI_MMU_PAGE_SIZE * page_number) + alloc->pages.phys_base;
		*mapping =  (mali_io_address)((MALI_MMU_PAGE_SIZE * page_number) + (u32)alloc->pages.mapping);
		MALI_DEBUG_PRINT(4, ("Page table allocated for VA=0x%08X, MaliPA=0x%08X\n", *mapping, *table_page ));
		MALI_SUCCESS;
	}
	else
	{
		mali_mmu_page_table_allocation * alloc;
		

		alloc = (mali_mmu_page_table_allocation *)_mali_osk_calloc(1, sizeof(mali_mmu_page_table_allocation));
		if (NULL == alloc)
		{
			_mali_osk_lock_signal(page_table_cache.lock, _MALI_OSK_LOCKMODE_RW);
			*table_page = MALI_INVALID_PAGE;
			MALI_ERROR(_MALI_OSK_ERR_NOMEM);
		}

		_MALI_OSK_INIT_LIST_HEAD(&alloc->list);

		if (_MALI_OSK_ERR_OK != mali_allocation_engine_allocate_page_tables(memory_engine, &alloc->pages, physical_memory_allocators))
		{
			MALI_DEBUG_PRINT(1, ("No more memory for page tables\n"));
			_mali_osk_free(alloc);
			_mali_osk_lock_signal(page_table_cache.lock, _MALI_OSK_LOCKMODE_RW);
			*table_page = MALI_INVALID_PAGE;
			*mapping = NULL;
			MALI_ERROR(_MALI_OSK_ERR_NOMEM);
		}

		
		alloc->num_pages = alloc->pages.size / MALI_MMU_PAGE_SIZE;
		alloc->usage_count = 1;
		MALI_DEBUG_PRINT(3, ("New page table cache expansion, %d pages in new cache allocation\n", alloc->num_pages));
		alloc->usage_map = _mali_osk_calloc(1, ((alloc->num_pages + BITS_PER_LONG - 1) & ~(BITS_PER_LONG-1) / BITS_PER_LONG) * sizeof(unsigned long));
		if (NULL == alloc->usage_map)
		{
			MALI_DEBUG_PRINT(1, ("Failed to allocate memory to describe MMU page table cache usage\n"));
			alloc->pages.release(&alloc->pages);
			_mali_osk_free(alloc);
			_mali_osk_lock_signal(page_table_cache.lock, _MALI_OSK_LOCKMODE_RW);
			*table_page = MALI_INVALID_PAGE;
			*mapping = NULL;
			MALI_ERROR(_MALI_OSK_ERR_NOMEM);
		}

		_mali_osk_set_nonatomic_bit(0, alloc->usage_map);

		if (alloc->num_pages > 1)
		{
			_mali_osk_list_add(&alloc->list, &page_table_cache.partial);
		}
		else
		{
			_mali_osk_list_add(&alloc->list, &page_table_cache.full);
		}

		_mali_osk_lock_signal(page_table_cache.lock, _MALI_OSK_LOCKMODE_RW);
		*table_page = alloc->pages.phys_base; 
		*mapping = alloc->pages.mapping; 
		MALI_DEBUG_PRINT(4, ("Page table allocated: VA=0x%08X, MaliPA=0x%08X\n", *mapping, *table_page ));
		MALI_SUCCESS;
	}
}

void mali_mmu_release_table_page(u32 pa)
{
	mali_mmu_page_table_allocation * alloc, * temp_alloc;

	MALI_DEBUG_PRINT_IF(1, pa & 4095, ("Bad page address 0x%x given to mali_mmu_release_table_page\n", (void*)pa));

	MALI_DEBUG_PRINT(4, ("Releasing table page 0x%08X to the cache\n", pa));

   	_mali_osk_lock_wait(page_table_cache.lock, _MALI_OSK_LOCKMODE_RW);

	
	
	_MALI_OSK_LIST_FOREACHENTRY(alloc, temp_alloc, &page_table_cache.partial, mali_mmu_page_table_allocation, list)
	{
		u32 start = alloc->pages.phys_base;
		u32 last = start + (alloc->num_pages - 1) * MALI_MMU_PAGE_SIZE;
		if (pa >= start && pa <= last)
		{
			MALI_DEBUG_ASSERT(0 != _mali_osk_test_bit((pa - start)/MALI_MMU_PAGE_SIZE, alloc->usage_map));
			_mali_osk_clear_nonatomic_bit((pa - start)/MALI_MMU_PAGE_SIZE, alloc->usage_map);
			alloc->usage_count--;

			_mali_osk_memset((void*)( ((u32)alloc->pages.mapping) + (pa - start) ), 0, MALI_MMU_PAGE_SIZE);

			if (0 == alloc->usage_count)
			{
				
				_mali_osk_list_del(&alloc->list);
				alloc->pages.release(&alloc->pages);
				_mali_osk_free(alloc->usage_map);
				_mali_osk_free(alloc);
			}
		   	_mali_osk_lock_signal(page_table_cache.lock, _MALI_OSK_LOCKMODE_RW);
			MALI_DEBUG_PRINT(4, ("(partial list)Released table page 0x%08X to the cache\n", pa));
			return;
		}
	}

	
	_MALI_OSK_LIST_FOREACHENTRY(alloc, temp_alloc, &page_table_cache.full, mali_mmu_page_table_allocation, list)
	{
		u32 start = alloc->pages.phys_base;
		u32 last = start + (alloc->num_pages - 1) * MALI_MMU_PAGE_SIZE;
		if (pa >= start && pa <= last)
		{
			_mali_osk_clear_nonatomic_bit((pa - start)/MALI_MMU_PAGE_SIZE, alloc->usage_map);
			alloc->usage_count--;

			_mali_osk_memset((void*)( ((u32)alloc->pages.mapping) + (pa - start) ), 0, MALI_MMU_PAGE_SIZE);


			if (0 == alloc->usage_count)
			{
				
				_mali_osk_list_del(&alloc->list);
				alloc->pages.release(&alloc->pages);
				_mali_osk_free(alloc->usage_map);
				_mali_osk_free(alloc);
			}
			else
			{
				
				_mali_osk_list_move(&alloc->list, &page_table_cache.partial);
			}

		   	_mali_osk_lock_signal(page_table_cache.lock, _MALI_OSK_LOCKMODE_RW);
			MALI_DEBUG_PRINT(4, ("(full list)Released table page 0x%08X to the cache\n", pa));
			return;
		}
	}

	MALI_DEBUG_PRINT(1, ("pa 0x%x not found in the page table cache\n", (void*)pa));

   	_mali_osk_lock_signal(page_table_cache.lock, _MALI_OSK_LOCKMODE_RW);
}

static _mali_osk_errcode_t mali_mmu_page_table_cache_create(void)
{
	page_table_cache.lock = _mali_osk_lock_init( _MALI_OSK_LOCKFLAG_ORDERED | _MALI_OSK_LOCKFLAG_ONELOCK
	                            | _MALI_OSK_LOCKFLAG_NONINTERRUPTABLE, 0, _MALI_OSK_LOCK_ORDER_MEM_PT_CACHE);
	MALI_CHECK_NON_NULL( page_table_cache.lock, _MALI_OSK_ERR_FAULT );
	_MALI_OSK_INIT_LIST_HEAD(&page_table_cache.partial);
	_MALI_OSK_INIT_LIST_HEAD(&page_table_cache.full);
	MALI_SUCCESS;
}

static void mali_mmu_page_table_cache_destroy(void)
{
	mali_mmu_page_table_allocation * alloc, *temp;

	_MALI_OSK_LIST_FOREACHENTRY(alloc, temp, &page_table_cache.partial, mali_mmu_page_table_allocation, list)
	{
		MALI_DEBUG_PRINT_IF(1, 0 != alloc->usage_count, ("Destroying page table cache while pages are tagged as in use. %d allocations still marked as in use.\n", alloc->usage_count));
		_mali_osk_list_del(&alloc->list);
		alloc->pages.release(&alloc->pages);
		_mali_osk_free(alloc->usage_map);
		_mali_osk_free(alloc);
	}

	MALI_DEBUG_PRINT_IF(1, 0 == _mali_osk_list_empty(&page_table_cache.full), ("Page table cache full list contains one or more elements \n"));

	_MALI_OSK_LIST_FOREACHENTRY(alloc, temp, &page_table_cache.full, mali_mmu_page_table_allocation, list)
	{
		MALI_DEBUG_PRINT(1, ("Destroy alloc 0x%08X with usage count %d\n", (u32)alloc, alloc->usage_count));
		_mali_osk_list_del(&alloc->list);
		alloc->pages.release(&alloc->pages);
		_mali_osk_free(alloc->usage_map);
		_mali_osk_free(alloc);
	}

	_mali_osk_lock_term(page_table_cache.lock);
}
