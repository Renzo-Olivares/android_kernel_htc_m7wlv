/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef __MALI_PLATFORM_H__
#define __MALI_PLATFORM_H__

#include "mali_osk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum mali_power_mode_tag
{
	MALI_POWER_MODE_ON,           
	MALI_POWER_MODE_LIGHT_SLEEP,  
	MALI_POWER_MODE_DEEP_SLEEP,   
} mali_power_mode;

_mali_osk_errcode_t mali_platform_init(void);

_mali_osk_errcode_t mali_platform_deinit(void);

_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode);


void mali_gpu_utilization_handler(u32 utilization);

void set_mali_parent_power_domain(void* dev);

#ifdef __cplusplus
}
#endif
#endif
