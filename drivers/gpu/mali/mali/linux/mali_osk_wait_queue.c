/*
 * Copyright (C) 2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include "mali_osk.h"
#include "mali_kernel_common.h"

struct _mali_osk_wait_queue_t_struct
{
    wait_queue_head_t wait_queue;
};

_mali_osk_wait_queue_t* _mali_osk_wait_queue_init( void )
{
    _mali_osk_wait_queue_t* ret = NULL;

    ret = kmalloc(sizeof(_mali_osk_wait_queue_t), GFP_KERNEL);

    if (NULL == ret)
    {
        return ret;
    }

    init_waitqueue_head(&ret->wait_queue);
    MALI_DEBUG_ASSERT(!waitqueue_active(&ret->wait_queue));

    return ret;
}

void _mali_osk_wait_queue_wait_event( _mali_osk_wait_queue_t *queue, mali_bool (*condition)(void) )
{
    MALI_DEBUG_ASSERT_POINTER( queue );
    MALI_DEBUG_PRINT(6, ("Adding to wait queue %p\n", queue));
    wait_event(queue->wait_queue, condition());
}

void _mali_osk_wait_queue_wake_up( _mali_osk_wait_queue_t *queue )
{
    MALI_DEBUG_ASSERT_POINTER( queue );

    
    if (!waitqueue_active(&queue->wait_queue)) return;

    MALI_DEBUG_PRINT(6, ("Waking up elements in wait queue %p ....\n", queue));

    wake_up_all(&queue->wait_queue);

    MALI_DEBUG_PRINT(6, ("... elements in wait queue %p woken up\n", queue));
}

void _mali_osk_wait_queue_term( _mali_osk_wait_queue_t *queue )
{
	
	MALI_DEBUG_ASSERT_POINTER( queue );

	
    kfree(queue);
}
