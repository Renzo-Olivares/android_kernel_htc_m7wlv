/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MALI_KERNEL_UTILIZATION_H__
#define __MALI_KERNEL_UTILIZATION_H__

#include "mali_osk.h"
#include "mali_pm.h"

_mali_osk_errcode_t mali_utilization_init(void);

void mali_utilization_term(void);

void mali_utilization_core_start(u64 time_now,enum mali_core_event core_event,u32 active_gps,u32 active_pps);

void mali_utilization_suspend(void);

void mali_utilization_core_end(u64 time_now,enum mali_core_event core_event,u32 active_gps,u32 active_pps);


#endif 
