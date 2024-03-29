/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <linux/timer.h>
#include <linux/slab.h>
#include "mali_osk.h"
#include "mali_kernel_common.h"

struct _mali_osk_timer_t_struct
{
    struct timer_list timer;
};

typedef void (*timer_timeout_function_t)(unsigned long);

_mali_osk_timer_t *_mali_osk_timer_init(void)
{
    _mali_osk_timer_t *t = (_mali_osk_timer_t*)kmalloc(sizeof(_mali_osk_timer_t), GFP_KERNEL);
    if (NULL != t) init_timer(&t->timer);
    return t;
}

void _mali_osk_timer_add( _mali_osk_timer_t *tim, u32 ticks_to_expire )
{
	MALI_DEBUG_ASSERT_POINTER(tim);
    tim->timer.expires = _mali_osk_time_tickcount() + ticks_to_expire;
    add_timer(&(tim->timer));
}

void _mali_osk_timer_mod( _mali_osk_timer_t *tim, u32 expiry_tick)
{
    MALI_DEBUG_ASSERT_POINTER(tim);
    mod_timer(&(tim->timer), expiry_tick);
}

void _mali_osk_timer_del( _mali_osk_timer_t *tim )
{
    MALI_DEBUG_ASSERT_POINTER(tim);
    del_timer_sync(&(tim->timer));
}

void _mali_osk_timer_setcallback( _mali_osk_timer_t *tim, _mali_osk_timer_callback_t callback, void *data )
{
    MALI_DEBUG_ASSERT_POINTER(tim);
    tim->timer.data = (unsigned long)data;
    tim->timer.function = (timer_timeout_function_t)callback;
}

void _mali_osk_timer_term( _mali_osk_timer_t *tim )
{
    MALI_DEBUG_ASSERT_POINTER(tim);
    kfree(tim);
}
