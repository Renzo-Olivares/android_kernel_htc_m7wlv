/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mali_osk.h"
#include "mali_osk_list.h"
#include "ump_osk.h"
#include "ump_uk_types.h"

#include "ump_kernel_interface_ref_drv.h"
#include "ump_kernel_common.h"
#include "ump_kernel_descriptor_mapping.h"

#define UMP_MINIMUM_SIZE         4096
#define UMP_MINIMUM_SIZE_MASK    (~(UMP_MINIMUM_SIZE-1))
#define UMP_SIZE_ALIGN(x)        (((x)+UMP_MINIMUM_SIZE-1)&UMP_MINIMUM_SIZE_MASK)
#define UMP_ADDR_ALIGN_OFFSET(x) ((x)&(UMP_MINIMUM_SIZE-1))
static void phys_blocks_release(void * ctx, struct ump_dd_mem * descriptor);

UMP_KERNEL_API_EXPORT ump_dd_handle ump_dd_handle_create_from_phys_blocks(ump_dd_physical_block * blocks, unsigned long num_blocks)
{
	ump_dd_mem * mem;
	unsigned long size_total = 0;
	int map_id;
	u32 i;

	
	for (i=0; i < num_blocks; i++)
	{
		unsigned long addr = blocks[i].addr;
		unsigned long size = blocks[i].size;

		DBG_MSG(5, ("Adding physical memory to new handle. Address: 0x%08lx, size: %lu\n", addr, size));
		size_total += blocks[i].size;

		if (0 != UMP_ADDR_ALIGN_OFFSET(addr))
		{
			MSG_ERR(("Trying to create UMP memory from unaligned physical address. Address: 0x%08lx\n", addr));
			return UMP_DD_HANDLE_INVALID;
		}

		if (0 != UMP_ADDR_ALIGN_OFFSET(size))
		{
			MSG_ERR(("Trying to create UMP memory with unaligned size. Size: %lu\n", size));
			return UMP_DD_HANDLE_INVALID;
		}
	}

	
	mem = _mali_osk_malloc(sizeof(*mem));
	if (NULL == mem)
	{
		DBG_MSG(1, ("Could not allocate ump_dd_mem in ump_dd_handle_create_from_phys_blocks()\n"));
		return UMP_DD_HANDLE_INVALID;
	}

	
	_mali_osk_lock_wait(device.secure_id_map_lock, _MALI_OSK_LOCKMODE_RW);
	map_id = ump_descriptor_mapping_allocate_mapping(device.secure_id_map, (void*) mem);

	if (map_id < 0)
	{
		_mali_osk_lock_signal(device.secure_id_map_lock, _MALI_OSK_LOCKMODE_RW);
		_mali_osk_free(mem);
		DBG_MSG(1, ("Failed to allocate secure ID in ump_dd_handle_create_from_phys_blocks()\n"));
		return UMP_DD_HANDLE_INVALID;
	}

	
	mem->block_array = _mali_osk_malloc(sizeof(ump_dd_physical_block)* num_blocks);
	if (NULL == mem->block_array)
	{
		ump_descriptor_mapping_free(device.secure_id_map, map_id);
		_mali_osk_lock_signal(device.secure_id_map_lock, _MALI_OSK_LOCKMODE_RW);
		_mali_osk_free(mem);
		DBG_MSG(1, ("Could not allocate a mem handle for function ump_dd_handle_create_from_phys_blocks().\n"));
		return UMP_DD_HANDLE_INVALID;
	}

	_mali_osk_memcpy(mem->block_array, blocks, sizeof(ump_dd_physical_block) * num_blocks);

	
	_mali_osk_atomic_init(&mem->ref_count, 1);
	mem->secure_id = (ump_secure_id)map_id;
	mem->size_bytes = size_total;
	mem->nr_blocks = num_blocks;
	mem->backend_info = NULL;
	mem->ctx = NULL;
	mem->release_func = phys_blocks_release;
	
	mem->is_cached = 1;
	mem->hw_device = _UMP_UK_USED_BY_CPU;
	mem->lock_usage = UMP_NOT_LOCKED;

	_mali_osk_lock_signal(device.secure_id_map_lock, _MALI_OSK_LOCKMODE_RW);
	DBG_MSG(3, ("UMP memory created. ID: %u, size: %lu\n", mem->secure_id, mem->size_bytes));

	return (ump_dd_handle)mem;
}

static void phys_blocks_release(void * ctx, struct ump_dd_mem * descriptor)
{
	_mali_osk_free(descriptor->block_array);
	descriptor->block_array = NULL;
}

_mali_osk_errcode_t _ump_ukk_allocate( _ump_uk_allocate_s *user_interaction )
{
	ump_session_data * session_data = NULL;
	ump_dd_mem *new_allocation = NULL;
	ump_session_memory_list_element * session_memory_element = NULL;
	int map_id;

	DEBUG_ASSERT_POINTER( user_interaction );
	DEBUG_ASSERT_POINTER( user_interaction->ctx );

	session_data = (ump_session_data *) user_interaction->ctx;

	session_memory_element = _mali_osk_calloc( 1, sizeof(ump_session_memory_list_element));
	if (NULL == session_memory_element)
	{
		DBG_MSG(1, ("Failed to allocate ump_session_memory_list_element in ump_ioctl_allocate()\n"));
		return _MALI_OSK_ERR_NOMEM;
	}


	new_allocation = _mali_osk_calloc( 1, sizeof(ump_dd_mem));
	if (NULL==new_allocation)
	{
		_mali_osk_free(session_memory_element);
		DBG_MSG(1, ("Failed to allocate ump_dd_mem in _ump_ukk_allocate()\n"));
		return _MALI_OSK_ERR_NOMEM;
	}

	
	_mali_osk_lock_wait(device.secure_id_map_lock, _MALI_OSK_LOCKMODE_RW);
	map_id = ump_descriptor_mapping_allocate_mapping(device.secure_id_map, (void*)new_allocation);

	if (map_id < 0)
	{
		_mali_osk_lock_signal(device.secure_id_map_lock, _MALI_OSK_LOCKMODE_RW);
		_mali_osk_free(session_memory_element);
		_mali_osk_free(new_allocation);
		DBG_MSG(1, ("Failed to allocate secure ID in ump_ioctl_allocate()\n"));
		return - _MALI_OSK_ERR_INVALID_FUNC;
	}

	
	new_allocation->secure_id = (ump_secure_id)map_id;
	_mali_osk_atomic_init(&new_allocation->ref_count,1);
	if ( 0==(UMP_REF_DRV_UK_CONSTRAINT_USE_CACHE & user_interaction->constraints) )
		 new_allocation->is_cached = 0;
	else new_allocation->is_cached = 1;

	
	if (0 == user_interaction->size)
	{
		user_interaction->size = 1; 
	}

	new_allocation->size_bytes = UMP_SIZE_ALIGN(user_interaction->size); 
	new_allocation->lock_usage = UMP_NOT_LOCKED;

	
	if (!device.backend->allocate( device.backend->ctx, new_allocation ) )
	{
		DBG_MSG(1, ("OOM: No more UMP memory left. Failed to allocate memory in ump_ioctl_allocate(). Size: %lu, requested size: %lu\n", new_allocation->size_bytes, (unsigned long)user_interaction->size));
		ump_descriptor_mapping_free(device.secure_id_map, map_id);
		_mali_osk_lock_signal(device.secure_id_map_lock, _MALI_OSK_LOCKMODE_RW);
		_mali_osk_free(new_allocation);
		_mali_osk_free(session_memory_element);
		return _MALI_OSK_ERR_INVALID_FUNC;
	}
	new_allocation->hw_device = _UMP_UK_USED_BY_CPU;
	new_allocation->ctx = device.backend->ctx;
	new_allocation->release_func = device.backend->release;

	_mali_osk_lock_signal(device.secure_id_map_lock, _MALI_OSK_LOCKMODE_RW);

	
	session_memory_element->mem = new_allocation;
	_mali_osk_lock_wait(session_data->lock, _MALI_OSK_LOCKMODE_RW);
	_mali_osk_list_add(&(session_memory_element->list), &(session_data->list_head_session_memory_list));
	_mali_osk_lock_signal(session_data->lock, _MALI_OSK_LOCKMODE_RW);

	user_interaction->secure_id = new_allocation->secure_id;
	user_interaction->size = new_allocation->size_bytes;
	DBG_MSG(3, ("UMP memory allocated. ID: %u, size: %lu\n", new_allocation->secure_id, new_allocation->size_bytes));

	return _MALI_OSK_ERR_OK;
}


_mali_osk_errcode_t _ump_ukk_secure_id_from_phys_blocks( _ump_uk_secure_id_from_phys_blocks_s *user_interaction )
{    
	ump_session_data * session_data = NULL;
	ump_dd_physical_block blocks;
	ump_dd_handle handle;
	ump_session_memory_list_element *session_memory_element;

	DEBUG_ASSERT_POINTER( user_interaction );
	DEBUG_ASSERT_POINTER( user_interaction->ctx );

	session_data = (ump_session_data *) user_interaction->ctx;

	session_memory_element = _mali_osk_calloc( 1, sizeof(ump_session_memory_list_element));
	if (NULL == session_memory_element)
	{
		DBG_MSG(1, ("Failed to allocate ump_session_memory_list_element in ump_secure_id_from_phys_blocks_wrapper()\n"));
		return _MALI_OSK_ERR_NOMEM;
	}

	blocks.addr = user_interaction->phys_addr;
	blocks.size = user_interaction->phys_size;

	handle = ump_dd_handle_create_from_phys_blocks(&blocks, 1);

	if( UMP_DD_HANDLE_INVALID == handle )
	{
		_mali_osk_free(session_memory_element);
		MSG_ERR(("ump_ioctl_handle_from_phys_blocks() failed in ump_dd_handle_create_from_phys_blocks()\n"));
        return _MALI_OSK_ERR_INVALID_FUNC;
	}

	user_interaction->secure_id = ump_dd_secure_id_get(handle);

	
	session_memory_element->mem = handle;
	_mali_osk_lock_wait(session_data->lock, _MALI_OSK_LOCKMODE_RW);
	_mali_osk_list_add(&(session_memory_element->list), &(session_data->list_head_session_memory_list));
	_mali_osk_lock_signal(session_data->lock, _MALI_OSK_LOCKMODE_RW);

    return _MALI_OSK_ERR_OK;
}
