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

struct mali_pmu_core;

struct mali_pmu_core *mali_pmu_create(_mali_osk_resource_t *resource, u32 number_of_pp_cores, u32 number_of_l2_caches);

void mali_pmu_delete(struct mali_pmu_core *pmu);

_mali_osk_errcode_t mali_pmu_reset(struct mali_pmu_core *pmu);

_mali_osk_errcode_t mali_pmu_powerdown_all(struct mali_pmu_core *pmu);


_mali_osk_errcode_t mali_pmu_powerup_all(struct mali_pmu_core *pmu);


struct mali_pmu_core *mali_pmu_get_global_pmu_core(void);
