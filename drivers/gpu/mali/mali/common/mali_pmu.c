/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mali_hw_core.h"
#include "mali_pmu.h"
#include "mali_pp.h"
#include "mali_kernel_common.h"
#include "mali_osk.h"

static u32 mali_pmu_detect_mask(u32 number_of_pp_cores, u32 number_of_l2_caches);

struct mali_pmu_core
{
	struct mali_hw_core hw_core;
	u32 mali_registered_cores_power_mask;
};

static struct mali_pmu_core *mali_global_pmu_core = NULL;

typedef enum {
	PMU_REG_ADDR_MGMT_POWER_UP                  = 0x00,     
	PMU_REG_ADDR_MGMT_POWER_DOWN                = 0x04,     
	PMU_REG_ADDR_MGMT_STATUS                    = 0x08,     
	PMU_REG_ADDR_MGMT_INT_MASK                  = 0x0C,     
	PMU_REGISTER_ADDRESS_SPACE_SIZE             = 0x10,     
} pmu_reg_addr_mgmt_addr;

struct mali_pmu_core *mali_pmu_create(_mali_osk_resource_t *resource, u32 number_of_pp_cores, u32 number_of_l2_caches)
{
	struct mali_pmu_core* pmu;

	MALI_DEBUG_ASSERT(NULL == mali_global_pmu_core);
	MALI_DEBUG_PRINT(2, ("Mali PMU: Creating Mali PMU core\n"));

	pmu = (struct mali_pmu_core *)_mali_osk_malloc(sizeof(struct mali_pmu_core));
	if (NULL != pmu)
	{
		pmu->mali_registered_cores_power_mask = mali_pmu_detect_mask(number_of_pp_cores, number_of_l2_caches);
		if (_MALI_OSK_ERR_OK == mali_hw_core_create(&pmu->hw_core, resource, PMU_REGISTER_ADDRESS_SPACE_SIZE))
		{
			if (_MALI_OSK_ERR_OK == mali_pmu_reset(pmu))
			{
				mali_global_pmu_core = pmu;
				return pmu;
			}
			mali_hw_core_delete(&pmu->hw_core);
		}
		_mali_osk_free(pmu);
	}

	return NULL;
}

void mali_pmu_delete(struct mali_pmu_core *pmu)
{
	MALI_DEBUG_ASSERT_POINTER(pmu);

	mali_hw_core_delete(&pmu->hw_core);
	_mali_osk_free(pmu);
	pmu = NULL;
}

_mali_osk_errcode_t mali_pmu_reset(struct mali_pmu_core *pmu)
{
	
	mali_hw_core_register_write(&pmu->hw_core, PMU_REG_ADDR_MGMT_INT_MASK, 0);
	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t mali_pmu_powerdown_all(struct mali_pmu_core *pmu)
{
	u32 stat;
	u32 timeout;

	MALI_DEBUG_ASSERT_POINTER(pmu);
	MALI_DEBUG_ASSERT( pmu->mali_registered_cores_power_mask != 0 );
	MALI_DEBUG_PRINT( 4, ("Mali PMU: power down (0x%08X)\n", pmu->mali_registered_cores_power_mask) );

	mali_hw_core_register_write(&pmu->hw_core, PMU_REG_ADDR_MGMT_POWER_DOWN, pmu->mali_registered_cores_power_mask);

	
	timeout = 100;
	do
	{
		
		stat = mali_hw_core_register_read(&pmu->hw_core, PMU_REG_ADDR_MGMT_STATUS);
		stat &= pmu->mali_registered_cores_power_mask;
		if( stat == pmu->mali_registered_cores_power_mask ) break; 
		_mali_osk_time_ubusydelay(100);
		timeout--;
	} while( timeout > 0 );

	if( timeout == 0 )
	{
		return _MALI_OSK_ERR_TIMEOUT;
	}

	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t mali_pmu_powerup_all(struct mali_pmu_core *pmu)
{
	u32 stat;
	u32 timeout;
       
	MALI_DEBUG_ASSERT_POINTER(pmu);
	MALI_DEBUG_ASSERT( pmu->mali_registered_cores_power_mask != 0 ); 
	MALI_DEBUG_PRINT( 4, ("Mali PMU: power up (0x%08X)\n", pmu->mali_registered_cores_power_mask) );

	mali_hw_core_register_write(&pmu->hw_core, PMU_REG_ADDR_MGMT_POWER_UP, pmu->mali_registered_cores_power_mask);

	
	timeout = 100;
	do
	{
		
		stat = mali_hw_core_register_read(&pmu->hw_core,PMU_REG_ADDR_MGMT_STATUS);
		stat &= pmu->mali_registered_cores_power_mask;
		if( stat == 0 ) break; 
		_mali_osk_time_ubusydelay(100);
		timeout--;
	} while( timeout > 0 );

	if( timeout == 0 )
	{
		return _MALI_OSK_ERR_TIMEOUT;
	}

	return _MALI_OSK_ERR_OK;
}

struct mali_pmu_core *mali_pmu_get_global_pmu_core(void)
{
	return mali_global_pmu_core;
}

static u32 mali_pmu_detect_mask(u32 number_of_pp_cores, u32 number_of_l2_caches)
{
	u32 mask = 0;

	if (number_of_l2_caches == 1)
	{
		
		u32 i;

		
		mask = 0x01;

		
		mask |= 0x01<<1;

		
		for (i = 0; i < number_of_pp_cores; i++)
		{
			mask |= 0x01<<(i+2);
		}
	}
	else if (number_of_l2_caches > 1)
	{
		

		
		mask = 0x01;

		
		mask |= 0x01<<1;

		
		if (number_of_pp_cores >= 2)
		{
			mask |= 0x01<<2;
		}

		
		if (number_of_pp_cores >= 5)
		{
			mask |= 0x01<<3;
		}
	}

	MALI_DEBUG_PRINT(4, ("Mali PMU: Power mask is 0x%08X (%u + %u)\n", mask, number_of_pp_cores, number_of_l2_caches));

	return mask;
}
