/*
 * Copyright (C) 2010, 2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef __UMP_KERNEL_INTERFACE_H__
#define __UMP_KERNEL_INTERFACE_H__




#include "ump_kernel_platform.h"


#ifdef __cplusplus
extern "C"
{
#endif


typedef void * ump_dd_handle;

typedef unsigned int ump_secure_id;


#define UMP_DD_HANDLE_INVALID ((ump_dd_handle)0)


#define UMP_INVALID_SECURE_ID ((ump_secure_id)-1)


typedef enum
{
	UMP_DD_SUCCESS, 
	UMP_DD_INVALID, 
} ump_dd_status_code;


typedef struct ump_dd_physical_block
{
	unsigned long addr; 
	unsigned long size; 
} ump_dd_physical_block;


UMP_KERNEL_API_EXPORT ump_secure_id ump_dd_secure_id_get(ump_dd_handle mem);


UMP_KERNEL_API_EXPORT ump_dd_handle ump_dd_handle_create_from_secure_id(ump_secure_id secure_id);


UMP_KERNEL_API_EXPORT unsigned long ump_dd_phys_block_count_get(ump_dd_handle mem);


UMP_KERNEL_API_EXPORT ump_dd_status_code ump_dd_phys_blocks_get(ump_dd_handle mem, ump_dd_physical_block * blocks, unsigned long num_blocks);


UMP_KERNEL_API_EXPORT ump_dd_status_code ump_dd_phys_block_get(ump_dd_handle mem, unsigned long index, ump_dd_physical_block * block);


UMP_KERNEL_API_EXPORT unsigned long ump_dd_size_get(ump_dd_handle mem);


UMP_KERNEL_API_EXPORT void ump_dd_reference_add(ump_dd_handle mem);


UMP_KERNEL_API_EXPORT void ump_dd_reference_release(ump_dd_handle mem);


#ifdef __cplusplus
}
#endif


 


#endif  
