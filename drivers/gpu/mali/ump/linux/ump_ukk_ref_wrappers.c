/*
 * Copyright (C) 2010, 2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */



#include <asm/uaccess.h>             

#include "ump_osk.h"
#include "ump_uk_types.h"
#include "ump_ukk.h"
#include "ump_kernel_common.h"

int ump_allocate_wrapper(u32 __user * argument, struct ump_session_data  * session_data)
{
	_ump_uk_allocate_s user_interaction;
	_mali_osk_errcode_t err;

	
	if (NULL == argument || NULL == session_data)
	{
		MSG_ERR(("NULL parameter in ump_ioctl_allocate()\n"));
		return -ENOTTY;
	}

	
	if (0 != copy_from_user(&user_interaction, argument, sizeof(user_interaction)))
	{
		MSG_ERR(("copy_from_user() in ump_ioctl_allocate()\n"));
		return -EFAULT;
	}

	user_interaction.ctx = (void *) session_data;

	err = _ump_ukk_allocate( &user_interaction );
	if( _MALI_OSK_ERR_OK != err )
	{
		DBG_MSG(1, ("_ump_ukk_allocate() failed in ump_ioctl_allocate()\n"));
		return map_errcode(err);
	}
	user_interaction.ctx = NULL;

	if (0 != copy_to_user(argument, &user_interaction, sizeof(user_interaction)))
	{
		
		_ump_uk_release_s release_args;

		MSG_ERR(("copy_to_user() failed in ump_ioctl_allocate()\n"));

		release_args.ctx = (void *) session_data;
		release_args.secure_id = user_interaction.secure_id;

		err = _ump_ukk_release( &release_args );
		if(_MALI_OSK_ERR_OK != err)
		{
			MSG_ERR(("_ump_ukk_release() also failed when trying to release newly allocated memory in ump_ioctl_allocate()\n"));
		}

		return -EFAULT;
	}

	return 0; 
}

int ump_secure_id_from_phys_blocks_wrapper(u32 __user * argument, struct ump_session_data * session_data)
{   
    _ump_uk_secure_id_from_phys_blocks_s user_interaction;

	
	if (NULL == argument || NULL == session_data)
	{
		MSG_ERR(("NULL parameter in ump_ioctl_handle_from_phys_blocks()\n"));
		return -ENOTTY;
	}

	
	if (0 != copy_from_user(&user_interaction, argument, sizeof(user_interaction)))
	{
		MSG_ERR(("copy_from_user() in ump_ioctl_handle_from_phys_blocks()\n"));
		return -EFAULT;
    }

	user_interaction.ctx = (void *) session_data;

    if (_MALI_OSK_ERR_OK != _ump_ukk_secure_id_from_phys_blocks(&user_interaction))
    {
		MSG_ERR(("_ump_ukk_secure_id_from_phys_blocks() failed in ump_ioctl_handle_from_phys_blocks()\n"));
		return -EFAULT;
    }

    user_interaction.ctx = NULL; 

	
	if (0 != copy_to_user(argument, &user_interaction, sizeof(user_interaction)))
	{
		
		_ump_uk_release_s release_args;

		MSG_ERR(("copy_to_user() failed in ump_ioctl_handle_from_phys_blocks()\n"));

		release_args.ctx = (void *) session_data;
		release_args.secure_id = user_interaction.secure_id;

		if(_MALI_OSK_ERR_OK != _ump_ukk_release( &release_args ))
		{
			MSG_ERR(("_ump_ukk_release() also failed when trying to release newly mapped memory in ump_secure_id_from_phys_blocks_wrapper()\n"));
		}

		return -EFAULT;
	}

	return 0; 
}
