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
#include "mali_session.h"
#include "mali_osk.h"
#include "mali_osk_mali.h"
#include "mali_ukk.h"
#include "mali_kernel_core.h"
#include "mali_memory.h"
#include "mali_mem_validation.h"
#include "mali_mmu.h"
#include "mali_mmu_page_directory.h"
#include "mali_dlbu.h"
#include "mali_gp.h"
#include "mali_pp.h"
#include "mali_gp_scheduler.h"
#include "mali_pp_scheduler.h"
#include "mali_cluster.h"
#include "mali_group.h"
#include "mali_pm.h"
#include "mali_pmu.h"
#include "mali_scheduler.h"
#ifdef CONFIG_MALI400_GPU_UTILIZATION
#include "mali_kernel_utilization.h"
#endif
#include "mali_l2_cache.h"
#if MALI_TIMELINE_PROFILING_ENABLED
#include "mali_osk_profiling.h"
#endif

static _mali_osk_resource_t *arch_configuration = NULL;

int mali_boot_profiling = 0;

static u32 num_resources;

static _mali_product_id_t global_product_id = _MALI_PRODUCT_ID_UNKNOWN;
static u32 global_gpu_base_address = 0;
static u32 global_gpu_major_version = 0;
static u32 global_gpu_minor_version = 0;

static u32 first_pp_offset = 0;

#define WATCHDOG_MSECS_DEFAULT 4000 

int mali_max_job_runtime = WATCHDOG_MSECS_DEFAULT;

static _mali_osk_resource_t *mali_find_resource(_mali_osk_resource_type_t type, u32 offset)
{
	int i;
	u32 addr = global_gpu_base_address + offset;

	for (i = 0; i < num_resources; i++)
	{
		if (type == arch_configuration[i].type && arch_configuration[i].base == addr)
		{
			return &(arch_configuration[i]);
		}
	}

	return NULL;
}

static u32 mali_count_resources(_mali_osk_resource_type_t type)
{
	int i;
	u32 retval = 0;

	for (i = 0; i < num_resources; i++)
	{
		if (type == arch_configuration[i].type)
		{
			retval++;
		}
	}

	return retval;
}


static _mali_osk_errcode_t mali_parse_gpu_base_and_first_pp_offset_address(void)
{
	int i;
	_mali_osk_resource_t *first_gp_resource = NULL;
	_mali_osk_resource_t *first_pp_resource = NULL;

	for (i = 0; i < num_resources; i++)
	{
		if (MALI_GP == arch_configuration[i].type)
		{
			if (NULL == first_gp_resource || first_gp_resource->base > arch_configuration[i].base)
			{
				first_gp_resource = &(arch_configuration[i]);
			}
		}
		if (MALI_PP == arch_configuration[i].type)
		{
			if (NULL == first_pp_resource || first_pp_resource->base > arch_configuration[i].base)
			{
				first_pp_resource = &(arch_configuration[i]);
			}
		}
	}

	if (NULL == first_gp_resource || NULL == first_pp_resource)
	{
		MALI_PRINT_ERROR(("No GP+PP core specified in config file\n"));
		return _MALI_OSK_ERR_FAULT;
	}

	if (first_gp_resource->base < first_pp_resource->base)
	{
		
		global_gpu_base_address = first_gp_resource->base;
		first_pp_offset = 0x8000;
	}
	else
	{
		
		global_gpu_base_address = first_pp_resource->base;
		first_pp_offset = 0x0;
	}
	MALI_SUCCESS;
}

static _mali_osk_errcode_t mali_parse_product_info(void)
{
	_mali_osk_resource_t *first_pp_resource = NULL;

	
	first_pp_resource = mali_find_resource(MALI_PP, first_pp_offset);
	if (NULL != first_pp_resource)
	{
		
		struct mali_group *group = mali_group_create(NULL, NULL);
		if (NULL != group)
		{
			
			struct mali_pp_core *pp_core = mali_pp_create(first_pp_resource, group);
			if (NULL != pp_core)
			{
				u32 pp_version = mali_pp_core_get_version(pp_core);
				mali_pp_delete(pp_core);
				mali_group_delete(group);

				global_gpu_major_version = (pp_version >> 8) & 0xFF;
				global_gpu_minor_version = pp_version & 0xFF;

				switch (pp_version >> 16)
				{
					case MALI200_PP_PRODUCT_ID:
						global_product_id = _MALI_PRODUCT_ID_MALI200;
						MALI_DEBUG_PRINT(2, ("Found Mali GPU Mali-200 r%up%u\n", global_gpu_major_version, global_gpu_minor_version));
						break;
					case MALI300_PP_PRODUCT_ID:
						global_product_id = _MALI_PRODUCT_ID_MALI300;
						MALI_DEBUG_PRINT(2, ("Found Mali GPU Mali-300 r%up%u\n", global_gpu_major_version, global_gpu_minor_version));
						break;
					case MALI400_PP_PRODUCT_ID:
						global_product_id = _MALI_PRODUCT_ID_MALI400;
						MALI_DEBUG_PRINT(2, ("Found Mali GPU Mali-400 MP r%up%u\n", global_gpu_major_version, global_gpu_minor_version));
						break;
					case MALI450_PP_PRODUCT_ID:
						global_product_id = _MALI_PRODUCT_ID_MALI450;
						MALI_DEBUG_PRINT(2, ("Found Mali GPU Mali-450 MP r%up%u\n", global_gpu_major_version, global_gpu_minor_version));
						break;
					default:
						MALI_DEBUG_PRINT(2, ("Found unknown Mali GPU GPU (r%up%u)\n", global_gpu_major_version, global_gpu_minor_version));
						return _MALI_OSK_ERR_FAULT;
				}

				return _MALI_OSK_ERR_OK;
			}
			else
			{
				MALI_PRINT_ERROR(("Failed to create initial PP object\n"));
			}
		}
		else
		{
			MALI_PRINT_ERROR(("Failed to create initial group object\n"));
		}
	}
	else
	{
		MALI_PRINT_ERROR(("First PP core not specified in config file\n"));
	}

	return _MALI_OSK_ERR_FAULT;
}

static void mali_delete_clusters(void)
{
	u32 i;
	u32 number_of_clusters = mali_cluster_get_glob_num_clusters();

	for (i = 0; i < number_of_clusters; i++)
	{
		mali_cluster_delete(mali_cluster_get_global_cluster(i));
	}
}

static _mali_osk_errcode_t mali_create_cluster(_mali_osk_resource_t *resource)
{
	if (NULL != resource)
	{
		struct mali_l2_cache_core *l2_cache;

		if (mali_l2_cache_core_get_glob_num_l2_cores() >= mali_l2_cache_core_get_max_num_l2_cores())
		{
			MALI_PRINT_ERROR(("Found too many L2 cache core objects, max %u is supported\n", mali_l2_cache_core_get_max_num_l2_cores()));
			return _MALI_OSK_ERR_FAULT;
		}

		MALI_DEBUG_PRINT(3, ("Found L2 cache %s, starting new cluster\n", resource->description));

		
		l2_cache = mali_l2_cache_create(resource);
		if (NULL == l2_cache)
		{
			MALI_PRINT_ERROR(("Failed to create L2 cache object\n"));
			return _MALI_OSK_ERR_FAULT;
		}

		if (NULL == mali_cluster_create(l2_cache))
		{
			MALI_PRINT_ERROR(("Failed to create cluster object\n"));
			mali_l2_cache_delete(l2_cache);
			return _MALI_OSK_ERR_FAULT;
		}
	}
	else
	{
		mali_cluster_create(NULL);
		if (NULL == mali_cluster_get_global_cluster(0))
		{
			MALI_PRINT_ERROR(("Failed to create cluster object\n"));
			return _MALI_OSK_ERR_FAULT;
		}
	}

	MALI_DEBUG_PRINT(3, ("Created cluster object\n"));
	return _MALI_OSK_ERR_OK;
}

static _mali_osk_errcode_t mali_parse_config_cluster(void)
{
	if (_MALI_PRODUCT_ID_MALI200 == global_product_id)
	{
		
		return mali_create_cluster(NULL);
	}
	else if (_MALI_PRODUCT_ID_MALI300 == global_product_id || _MALI_PRODUCT_ID_MALI400 == global_product_id)
	{
		_mali_osk_resource_t *l2_resource = mali_find_resource(MALI_L2, 0x1000);
		if (NULL == l2_resource)
		{
			MALI_DEBUG_PRINT(3, ("Did not find required Mali L2 cache in config file\n"));
			return _MALI_OSK_ERR_FAULT;
		}

		return mali_create_cluster(l2_resource);
	}
	else if (_MALI_PRODUCT_ID_MALI450 == global_product_id)
	{

		_mali_osk_resource_t *l2_gp_resource;
		_mali_osk_resource_t *l2_pp_grp0_resource;
		_mali_osk_resource_t *l2_pp_grp1_resource;

		
		l2_gp_resource = mali_find_resource(MALI_L2, 0x10000);
		if (NULL != l2_gp_resource)
		{
			_mali_osk_errcode_t ret;
			MALI_DEBUG_PRINT(3, ("Creating Mali-450 cluster for GP\n"));
			ret = mali_create_cluster(l2_gp_resource);
			if (_MALI_OSK_ERR_OK != ret)
			{
				return ret;
			}
		}
		else
		{
			MALI_DEBUG_PRINT(3, ("Did not find required Mali L2 cache for GP in config file\n"));
			return _MALI_OSK_ERR_FAULT;
		}

		
		l2_pp_grp0_resource = mali_find_resource(MALI_L2, 0x1000);
		if (NULL != l2_pp_grp0_resource)
		{
			_mali_osk_errcode_t ret;
			MALI_DEBUG_PRINT(3, ("Creating Mali-450 cluster for PP group 0\n"));
			ret = mali_create_cluster(l2_pp_grp0_resource);
			if (_MALI_OSK_ERR_OK != ret)
			{
				return ret;
			}
		}
		else
		{
			MALI_DEBUG_PRINT(3, ("Did not find required Mali L2 cache for PP group 0 in config file\n"));
			return _MALI_OSK_ERR_FAULT;
		}

		
		l2_pp_grp1_resource = mali_find_resource(MALI_L2, 0x11000);
		if (NULL != l2_pp_grp1_resource)
		{
			_mali_osk_errcode_t ret;
			MALI_DEBUG_PRINT(3, ("Creating Mali-450 cluster for PP group 0\n"));
			ret = mali_create_cluster(l2_pp_grp1_resource);
			if (_MALI_OSK_ERR_OK != ret)
			{
				return ret;
			}
		}
	}

	return _MALI_OSK_ERR_OK;
}

static _mali_osk_errcode_t mali_create_group(struct mali_cluster *cluster,
                                             _mali_osk_resource_t *resource_mmu,
                                             _mali_osk_resource_t *resource_gp,
                                             _mali_osk_resource_t *resource_pp)
{
	struct mali_mmu_core *mmu;
	struct mali_group *group;
	struct mali_pp_core *pp;

	MALI_DEBUG_PRINT(3, ("Starting new group for MMU %s\n", resource_mmu->description));

	
	mmu = mali_mmu_create(resource_mmu);
	if (NULL == mmu)
	{
		MALI_PRINT_ERROR(("Failed to create MMU object\n"));
		return _MALI_OSK_ERR_FAULT;
	}

	
	group = mali_group_create(cluster, mmu);
	if (NULL == group)
	{
		MALI_PRINT_ERROR(("Failed to create group object for MMU %s\n", resource_mmu->description));
		mali_mmu_delete(mmu);
		return _MALI_OSK_ERR_FAULT;
	}

	
	mali_mmu_set_group(mmu, group);

	
	mali_cluster_add_group(cluster, group);

	if (NULL != resource_gp)
	{
		
		
		if (NULL == mali_gp_create(resource_gp, group))
		{
			
			MALI_PRINT_ERROR(("Failed to create GP object\n"));
			return _MALI_OSK_ERR_FAULT;
		}

		
		MALI_DEBUG_PRINT(3, ("Adding GP %s to group\n", resource_gp->description));
		mali_group_add_gp_core(group, mali_gp_get_global_gp_core());
	}

	if (NULL != resource_pp)
	{
		
		pp = mali_pp_create(resource_pp, group);

		if (NULL == pp)
		{
			
			MALI_PRINT_ERROR(("Failed to create PP object\n"));
			return _MALI_OSK_ERR_FAULT;
		}

		
		MALI_DEBUG_PRINT(3, ("Adding PP %s to group\n", resource_pp->description));
		mali_group_add_pp_core(group, pp);
	}

	return _MALI_OSK_ERR_OK;
}

static _mali_osk_errcode_t mali_parse_config_groups(void)
{
	if (_MALI_PRODUCT_ID_MALI200 == global_product_id)
	{
		_mali_osk_resource_t *resource_gp;
		_mali_osk_resource_t *resource_pp;
		_mali_osk_resource_t *resource_mmu;

		MALI_DEBUG_ASSERT(1 == mali_cluster_get_glob_num_clusters());

		resource_gp  = mali_find_resource(MALI_GP, 0x02000);
		resource_pp  = mali_find_resource(MALI_PP, 0x00000);
		resource_mmu = mali_find_resource(MMU, 0x03000);

		if (NULL == resource_mmu || NULL == resource_gp || NULL == resource_pp)
		{
			
			return _MALI_OSK_ERR_FAULT;
		}

		
		return mali_create_group(mali_cluster_get_global_cluster(0), resource_mmu, resource_gp, resource_pp);
	}
	else if (_MALI_PRODUCT_ID_MALI300 == global_product_id ||
	         _MALI_PRODUCT_ID_MALI400 == global_product_id ||
	         _MALI_PRODUCT_ID_MALI450 == global_product_id)
	{
		_mali_osk_errcode_t err;
		int cluster_id_gp = 0;
		int cluster_id_pp_grp0 = 0;
		int cluster_id_pp_grp1 = 0;
		int i;
		_mali_osk_resource_t *resource_gp;
		_mali_osk_resource_t *resource_gp_mmu;
		_mali_osk_resource_t *resource_pp[mali_pp_get_max_num_pp_cores()];
		_mali_osk_resource_t *resource_pp_mmu[mali_pp_get_max_num_pp_cores()];
		u32 max_num_pp_cores = mali_pp_get_max_num_pp_cores();

		if (_MALI_PRODUCT_ID_MALI450 == global_product_id)
		{
			
			cluster_id_pp_grp0 = 1;
			cluster_id_pp_grp1 = 2;
		}

		resource_gp = mali_find_resource(MALI_GP, 0x00000);
		resource_gp_mmu = mali_find_resource(MMU, 0x03000);
		resource_pp[0] = mali_find_resource(MALI_PP, 0x08000);
		resource_pp[1] = mali_find_resource(MALI_PP, 0x0A000);
		resource_pp[2] = mali_find_resource(MALI_PP, 0x0C000);
		resource_pp[3] = mali_find_resource(MALI_PP, 0x0E000);
		resource_pp[4] = mali_find_resource(MALI_PP, 0x28000);
		resource_pp[5] = mali_find_resource(MALI_PP, 0x2A000);
		resource_pp[6] = mali_find_resource(MALI_PP, 0x2C000);
		resource_pp[7] = mali_find_resource(MALI_PP, 0x2E000);
		resource_pp_mmu[0] = mali_find_resource(MMU, 0x04000);
		resource_pp_mmu[1] = mali_find_resource(MMU, 0x05000);
		resource_pp_mmu[2] = mali_find_resource(MMU, 0x06000);
		resource_pp_mmu[3] = mali_find_resource(MMU, 0x07000);
		resource_pp_mmu[4] = mali_find_resource(MMU, 0x1C000);
		resource_pp_mmu[5] = mali_find_resource(MMU, 0x1D000);
		resource_pp_mmu[6] = mali_find_resource(MMU, 0x1E000);
		resource_pp_mmu[7] = mali_find_resource(MMU, 0x1F000);

		if (NULL == resource_gp || NULL == resource_gp_mmu || NULL == resource_pp[0] || NULL == resource_pp_mmu[0])
		{
			
			MALI_DEBUG_PRINT(2, ("Missing mandatory resource, need at least one GP and one PP, both with a separate MMU (0x%08X, 0x%08X, 0x%08X, 0x%08X)\n",
			                     resource_gp, resource_gp_mmu, resource_pp[0], resource_pp_mmu[0]));
			return _MALI_OSK_ERR_FAULT;
		}

		MALI_DEBUG_ASSERT(1 <= mali_cluster_get_glob_num_clusters());
		err = mali_create_group(mali_cluster_get_global_cluster(cluster_id_gp), resource_gp_mmu, resource_gp, NULL);
		if (err != _MALI_OSK_ERR_OK)
		{
			return err;
		}

		
		MALI_DEBUG_ASSERT(mali_cluster_get_glob_num_clusters() >= (cluster_id_pp_grp0 + 1)); 
		err = mali_create_group(mali_cluster_get_global_cluster(cluster_id_pp_grp0), resource_pp_mmu[0], NULL, resource_pp[0]);
		if (err != _MALI_OSK_ERR_OK)
		{
			return err;
		}

		
		for (i = 1; i < 4; i++) 
		{
			if (NULL != resource_pp[i])
			{
				err = mali_create_group(mali_cluster_get_global_cluster(cluster_id_pp_grp0), resource_pp_mmu[i], NULL, resource_pp[i]);
				if (err != _MALI_OSK_ERR_OK)
				{
					return err;
				}
			}
		}

		
		for (i = 4; i < max_num_pp_cores; i++) 
		{
			if (NULL != resource_pp[i])
			{
				MALI_DEBUG_ASSERT(mali_cluster_get_glob_num_clusters() >= 2); 
				err = mali_create_group(mali_cluster_get_global_cluster(cluster_id_pp_grp1), resource_pp_mmu[i], NULL, resource_pp[i]);
				if (err != _MALI_OSK_ERR_OK)
				{
					return err;
				}
			}
		}
	}

	return _MALI_OSK_ERR_OK;
}

static _mali_osk_errcode_t mali_parse_config_pmu(void)
{
	_mali_osk_errcode_t err = _MALI_OSK_ERR_OK;
	_mali_osk_resource_t *resource_pmu;
	u32 number_of_pp_cores;
	u32 number_of_l2_caches;

	resource_pmu = mali_find_resource(PMU, 0x02000);
	number_of_pp_cores = mali_count_resources(MALI_PP);
	number_of_l2_caches = mali_count_resources(MALI_L2);

	if (NULL != resource_pmu)
	{
		if (NULL == mali_pmu_create(resource_pmu, number_of_pp_cores, number_of_l2_caches))
		{
			err = _MALI_OSK_ERR_FAULT;
		}
	}
	return err;
}

static _mali_osk_errcode_t mali_parse_config_memory(void)
{
	int i;
	_mali_osk_errcode_t ret;

	for(i = 0; i < num_resources; i++)
	{
		switch(arch_configuration[i].type)
		{
			case OS_MEMORY:
				ret = mali_memory_core_resource_os_memory(&arch_configuration[i]);
				if (_MALI_OSK_ERR_OK != ret)
				{
					MALI_PRINT_ERROR(("Failed to register OS_MEMORY\n"));
					mali_memory_terminate();
					return ret;
				}
				break;
			case MEMORY:
				ret = mali_memory_core_resource_dedicated_memory(&arch_configuration[i]);
				if (_MALI_OSK_ERR_OK != ret)
				{
					MALI_PRINT_ERROR(("Failed to register MEMORY\n"));
					mali_memory_terminate();
					return ret;
				}
				break;
			case MEM_VALIDATION:
				ret = mali_mem_validation_add_range(&arch_configuration[i]);
				if (_MALI_OSK_ERR_OK != ret)
				{
					MALI_PRINT_ERROR(("Failed to register MEM_VALIDATION\n"));
					mali_memory_terminate();
					return ret;
				}
				break;
			default:
				break;
		}
	}
	return _MALI_OSK_ERR_OK;
}

_mali_osk_errcode_t mali_initialize_subsystems(void)
{
	_mali_osk_errcode_t err;
	mali_bool is_pmu_enabled;

	err = mali_session_initialize();
	if (_MALI_OSK_ERR_OK != err) goto session_init_failed;

#if MALI_TIMELINE_PROFILING_ENABLED
	err = _mali_osk_profiling_init(mali_boot_profiling ? MALI_TRUE : MALI_FALSE);
	if (_MALI_OSK_ERR_OK != err)
	{
		
		MALI_PRINT_ERROR(("Failed to initialize profiling, feature will be unavailable\n"));
	}
#endif

	
	err = _mali_osk_resources_init(&arch_configuration, &num_resources);
	if (_MALI_OSK_ERR_OK != err) goto osk_resources_init_failed;

	
	err = mali_memory_initialize();
	if (_MALI_OSK_ERR_OK != err) goto memory_init_failed;

	
	err = mali_parse_config_memory();
	if (_MALI_OSK_ERR_OK != err) goto parse_memory_config_failed;

	
	err = mali_parse_gpu_base_and_first_pp_offset_address();
	if (_MALI_OSK_ERR_OK != err) goto parse_gpu_base_address_failed;

	
	err = mali_parse_config_pmu();
	if (_MALI_OSK_ERR_OK != err) goto parse_pmu_config_failed;

	is_pmu_enabled = mali_pmu_get_global_pmu_core() != NULL ? MALI_TRUE : MALI_FALSE;

	
	err = mali_pm_initialize();
	if (_MALI_OSK_ERR_OK != err) goto pm_init_failed;

	
	mali_pm_always_on(MALI_TRUE);

	
	err = mali_parse_product_info();
	if (_MALI_OSK_ERR_OK != err) goto product_info_parsing_failed;

	

	
	err = mali_mmu_initialize();
	if (_MALI_OSK_ERR_OK != err) goto mmu_init_failed;

	
	if (_MALI_PRODUCT_ID_MALI450 == global_product_id)
	{
		err = mali_dlbu_initialize();
		if (_MALI_OSK_ERR_OK != err) goto dlbu_init_failed;
	}

	
	err = mali_parse_config_cluster();
	if (_MALI_OSK_ERR_OK != err) goto config_parsing_failed;
	err = mali_parse_config_groups();
	if (_MALI_OSK_ERR_OK != err) goto config_parsing_failed;

	
	err = mali_scheduler_initialize();
	if (_MALI_OSK_ERR_OK != err) goto scheduler_init_failed;
	err = mali_gp_scheduler_initialize();
	if (_MALI_OSK_ERR_OK != err) goto gp_scheduler_init_failed;
	err = mali_pp_scheduler_initialize();
	if (_MALI_OSK_ERR_OK != err) goto pp_scheduler_init_failed;

#ifdef CONFIG_MALI400_GPU_UTILIZATION
	
	err = mali_utilization_init();
	if (_MALI_OSK_ERR_OK != err) goto utilization_init_failed;
#endif

	
	mali_pm_always_on(MALI_FALSE);
	MALI_SUCCESS; 

	
#ifdef CONFIG_MALI400_GPU_UTILIZATION
utilization_init_failed:
	mali_pp_scheduler_terminate();
#endif
pp_scheduler_init_failed:
	mali_gp_scheduler_terminate();
gp_scheduler_init_failed:
	mali_scheduler_terminate();
scheduler_init_failed:
config_parsing_failed:
	mali_delete_clusters(); 
	if (_MALI_PRODUCT_ID_MALI450 == global_product_id)
	{
		mali_dlbu_terminate();
	}
dlbu_init_failed:
	mali_mmu_terminate();
mmu_init_failed:
	
product_info_parsing_failed:
	mali_pm_terminate();
pm_init_failed:
	if (is_pmu_enabled)
	{
		mali_pmu_delete(mali_pmu_get_global_pmu_core());
	}
parse_pmu_config_failed:
parse_gpu_base_address_failed:
parse_memory_config_failed:
	mali_memory_terminate();
memory_init_failed:
	_mali_osk_resources_term(&arch_configuration, num_resources);
osk_resources_init_failed:
#if MALI_TIMELINE_PROFILING_ENABLED
	_mali_osk_profiling_term();
#endif
	mali_session_terminate();
session_init_failed:
	return err;
}

void mali_terminate_subsystems(void)
{
	struct mali_pmu_core *pmu;

	MALI_DEBUG_PRINT(2, ("terminate_subsystems() called\n"));

	

	mali_pm_always_on(MALI_TRUE); 

#ifdef CONFIG_MALI400_GPU_UTILIZATION
	mali_utilization_term();
#endif

	mali_pp_scheduler_terminate();
	mali_gp_scheduler_terminate();
	mali_scheduler_terminate();

	mali_delete_clusters(); 

	if (_MALI_PRODUCT_ID_MALI450 == global_product_id)
	{
		mali_dlbu_terminate();
	}

	mali_mmu_terminate();

	pmu = mali_pmu_get_global_pmu_core();
	if (NULL != pmu)
	{
		mali_pmu_delete(pmu);
	}

	mali_pm_terminate();

	mali_memory_terminate();

	_mali_osk_resources_term(&arch_configuration, num_resources);

#if MALI_TIMELINE_PROFILING_ENABLED
	_mali_osk_profiling_term();
#endif

	mali_session_terminate();
}

_mali_product_id_t mali_kernel_core_get_product_id(void)
{
	return global_product_id;
}

void mali_kernel_core_wakeup(void)
{
	u32 i;
	u32 glob_num_clusters = mali_cluster_get_glob_num_clusters();
	struct mali_cluster *cluster;

	for (i = 0; i < glob_num_clusters; i++)
	{
		cluster = mali_cluster_get_global_cluster(i);
		mali_cluster_reset(cluster);
	}
}

_mali_osk_errcode_t _mali_ukk_get_api_version( _mali_uk_get_api_version_s *args )
{
	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_CHECK_NON_NULL(args->ctx, _MALI_OSK_ERR_INVALID_ARGS);

	
	if ( args->version == _MALI_UK_API_VERSION )
	{
		args->compatible = 1;
	}
	else
	{
		args->compatible = 0;
	}

	args->version = _MALI_UK_API_VERSION; 

	
	MALI_SUCCESS;
}

_mali_osk_errcode_t _mali_ukk_wait_for_notification( _mali_uk_wait_for_notification_s *args )
{
	_mali_osk_errcode_t err;
	_mali_osk_notification_t *notification;
	_mali_osk_notification_queue_t *queue;

	
	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_CHECK_NON_NULL(args->ctx, _MALI_OSK_ERR_INVALID_ARGS);

	queue = ((struct mali_session_data *)args->ctx)->ioctl_queue;

	
	if (NULL == queue)
	{
		MALI_DEBUG_PRINT(1, ("No notification queue registered with the session. Asking userspace to stop querying\n"));
		args->type = _MALI_NOTIFICATION_CORE_SHUTDOWN_IN_PROGRESS;
		MALI_SUCCESS;
	}

	
	err = _mali_osk_notification_queue_receive(queue, &notification);
	if (_MALI_OSK_ERR_OK != err)
	{
		MALI_ERROR(err); 
	}

	
	args->type = (_mali_uk_notification_type)notification->notification_type;
	_mali_osk_memcpy(&args->data, notification->result_buffer, notification->result_buffer_size);

	
	_mali_osk_notification_delete( notification );

	MALI_SUCCESS; 
}

_mali_osk_errcode_t _mali_ukk_post_notification( _mali_uk_post_notification_s *args )
{
	_mali_osk_notification_t * notification;
	_mali_osk_notification_queue_t *queue;

	
	MALI_DEBUG_ASSERT_POINTER(args);
	MALI_CHECK_NON_NULL(args->ctx, _MALI_OSK_ERR_INVALID_ARGS);

	queue = ((struct mali_session_data *)args->ctx)->ioctl_queue;

	
	if (NULL == queue)
	{
		MALI_DEBUG_PRINT(1, ("No notification queue registered with the session. Asking userspace to stop querying\n"));
		MALI_SUCCESS;
	}

	notification = _mali_osk_notification_create(args->type, 0);
	if ( NULL == notification)
	{
		MALI_PRINT_ERROR( ("Failed to create notification object\n"));
		return _MALI_OSK_ERR_NOMEM;
	}

	_mali_osk_notification_queue_send(queue, notification);

	MALI_SUCCESS; 
}

_mali_osk_errcode_t _mali_ukk_open(void **context)
{
	struct mali_session_data *session_data;

	
	session_data = (struct mali_session_data *)_mali_osk_calloc(1, sizeof(struct mali_session_data));
	MALI_CHECK_NON_NULL(session_data, _MALI_OSK_ERR_NOMEM);

	MALI_DEBUG_PRINT(2, ("Session starting\n"));

	
	session_data->ioctl_queue = _mali_osk_notification_queue_init();
	if (NULL == session_data->ioctl_queue)
	{
		_mali_osk_free(session_data);
		MALI_ERROR(_MALI_OSK_ERR_NOMEM);
	}

	session_data->page_directory = mali_mmu_pagedir_alloc();
	if (NULL == session_data->page_directory)
	{
		_mali_osk_notification_queue_term(session_data->ioctl_queue);
		_mali_osk_free(session_data);
		MALI_ERROR(_MALI_OSK_ERR_NOMEM);
	}

	if (_MALI_OSK_ERR_OK != mali_mmu_pagedir_map(session_data->page_directory, MALI_DLB_VIRT_ADDR, _MALI_OSK_MALI_PAGE_SIZE))
	{
		MALI_PRINT_ERROR(("Failed to map DLB page into session\n"));
		_mali_osk_notification_queue_term(session_data->ioctl_queue);
		_mali_osk_free(session_data);
		MALI_ERROR(_MALI_OSK_ERR_NOMEM);
	}

	if (0 != mali_dlbu_phys_addr)
	{
		mali_mmu_pagedir_update(session_data->page_directory, MALI_DLB_VIRT_ADDR, mali_dlbu_phys_addr, _MALI_OSK_MALI_PAGE_SIZE, MALI_CACHE_STANDARD);
	}

	if (_MALI_OSK_ERR_OK != mali_memory_session_begin(session_data))
	{
		mali_mmu_pagedir_free(session_data->page_directory);
		_mali_osk_notification_queue_term(session_data->ioctl_queue);
		_mali_osk_free(session_data);
		MALI_ERROR(_MALI_OSK_ERR_NOMEM);
	}

	*context = (void*)session_data;

	
	mali_session_add(session_data);

	MALI_DEBUG_PRINT(3, ("Session started\n"));
	MALI_SUCCESS;
}

_mali_osk_errcode_t _mali_ukk_close(void **context)
{
	struct mali_session_data *session;
	MALI_CHECK_NON_NULL(context, _MALI_OSK_ERR_INVALID_ARGS);
	session = (struct mali_session_data *)*context;

	MALI_DEBUG_PRINT(3, ("Session ending\n"));

	
	mali_session_remove(session);

	
	mali_gp_scheduler_abort_session(session);
	mali_pp_scheduler_abort_session(session);

	_mali_osk_flush_workqueue(NULL);

	
	mali_memory_session_end(session);

	
	mali_mmu_pagedir_free(session->page_directory);
	_mali_osk_notification_queue_term(session->ioctl_queue);
	_mali_osk_free(session);

	*context = NULL;

	MALI_DEBUG_PRINT(2, ("Session has ended\n"));

	MALI_SUCCESS;
}

#if MALI_STATE_TRACKING
u32 _mali_kernel_core_dump_state(char* buf, u32 size)
{
	int n = 0; /* Number of bytes written to buf */

	n += mali_gp_scheduler_dump_state(buf + n, size - n);
	n += mali_pp_scheduler_dump_state(buf + n, size - n);

	return n;
}
#endif
