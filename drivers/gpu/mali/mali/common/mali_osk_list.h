/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef __MALI_OSK_LIST_H__
#define __MALI_OSK_LIST_H__

#ifdef __cplusplus
extern "C"
{
#endif

MALI_STATIC_INLINE void __mali_osk_list_add(_mali_osk_list_t *new_entry, _mali_osk_list_t *prev, _mali_osk_list_t *next)
{
    next->prev = new_entry;
    new_entry->next = next;
    new_entry->prev = prev;
    prev->next = new_entry;
}

MALI_STATIC_INLINE void __mali_osk_list_del(_mali_osk_list_t *prev, _mali_osk_list_t *next)
{
    next->prev = prev;
    prev->next = next;
}



MALI_STATIC_INLINE void _mali_osk_list_init( _mali_osk_list_t *list )
{
    list->next = list;
    list->prev = list;
}

MALI_STATIC_INLINE void _mali_osk_list_add( _mali_osk_list_t *new_entry, _mali_osk_list_t *list )
{
    __mali_osk_list_add(new_entry, list, list->next);
}

MALI_STATIC_INLINE void _mali_osk_list_addtail( _mali_osk_list_t *new_entry, _mali_osk_list_t *list )
{
    __mali_osk_list_add(new_entry, list->prev, list);
}

MALI_STATIC_INLINE void _mali_osk_list_del( _mali_osk_list_t *list )
{
    __mali_osk_list_del(list->prev, list->next);
}

MALI_STATIC_INLINE void _mali_osk_list_delinit( _mali_osk_list_t *list )
{
    __mali_osk_list_del(list->prev, list->next);
    _mali_osk_list_init(list);
}

MALI_STATIC_INLINE int _mali_osk_list_empty( _mali_osk_list_t *list )
{
    return list->next == list;
}

MALI_STATIC_INLINE void _mali_osk_list_move( _mali_osk_list_t *move_entry, _mali_osk_list_t *list )
{
    __mali_osk_list_del(move_entry->prev, move_entry->next);
    _mali_osk_list_add(move_entry, list);
}

MALI_STATIC_INLINE void _mali_osk_list_splice( _mali_osk_list_t *list, _mali_osk_list_t *at )
{
    if (!_mali_osk_list_empty(list))
    {
        
        _mali_osk_list_t *first = list->next;
        _mali_osk_list_t *last = list->prev;
        _mali_osk_list_t *split = at->next;

        first->prev = at;
        at->next = first;

        last->next  = split;
        split->prev = last;
    }
}
 

#ifdef __cplusplus
}
#endif

#endif 
