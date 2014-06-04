/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <linux/version.h>

#include <linux/spinlock.h>
#include <linux/rwsem.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <linux/semaphore.h>
#else 
#include <asm/semaphore.h>
#endif

#include <linux/slab.h>
#include "mali_osk.h"
#include "mali_kernel_common.h"

typedef enum
{
	_MALI_OSK_INTERNAL_LOCKTYPE_SPIN,            
	_MALI_OSK_INTERNAL_LOCKTYPE_SPIN_IRQ,        
	_MALI_OSK_INTERNAL_LOCKTYPE_MUTEX,           
	_MALI_OSK_INTERNAL_LOCKTYPE_MUTEX_NONINT,    
	_MALI_OSK_INTERNAL_LOCKTYPE_MUTEX_NONINT_RW, 



} _mali_osk_internal_locktype;

struct _mali_osk_lock_t_struct
{
    _mali_osk_internal_locktype type;
	unsigned long flags;
    union
    {
        spinlock_t spinlock;
        struct semaphore sema;
        struct rw_semaphore rw_sema;
    } obj;
	MALI_DEBUG_CODE(
				  
				  _mali_osk_lock_flags_t orig_flags;

				  u32 owner;
				  u32 nOwners;
				  
				  _mali_osk_lock_mode_t mode;
	); 
};

_mali_osk_lock_t *_mali_osk_lock_init( _mali_osk_lock_flags_t flags, u32 initial, u32 order )
{
    _mali_osk_lock_t *lock = NULL;

	
	
	MALI_DEBUG_ASSERT( 0 == ( flags & ~(_MALI_OSK_LOCKFLAG_SPINLOCK
                                      | _MALI_OSK_LOCKFLAG_SPINLOCK_IRQ
                                      | _MALI_OSK_LOCKFLAG_NONINTERRUPTABLE
                                      | _MALI_OSK_LOCKFLAG_READERWRITER
                                      | _MALI_OSK_LOCKFLAG_ORDERED
                                      | _MALI_OSK_LOCKFLAG_ONELOCK )) );
	
	MALI_DEBUG_ASSERT( (((flags & _MALI_OSK_LOCKFLAG_SPINLOCK) || (flags & _MALI_OSK_LOCKFLAG_SPINLOCK_IRQ)) && (flags & _MALI_OSK_LOCKFLAG_NONINTERRUPTABLE))
					 || !(flags & _MALI_OSK_LOCKFLAG_SPINLOCK));
	
	MALI_DEBUG_ASSERT( 0 == initial );

	lock = kmalloc(sizeof(_mali_osk_lock_t), GFP_KERNEL);

	if ( NULL == lock )
	{
		return lock;
	}

	
    

	if ( (flags & _MALI_OSK_LOCKFLAG_SPINLOCK) )
	{
		
		lock->type = _MALI_OSK_INTERNAL_LOCKTYPE_SPIN;
		spin_lock_init( &lock->obj.spinlock );
	}
	else if ( (flags & _MALI_OSK_LOCKFLAG_SPINLOCK_IRQ ) )
	{
		lock->type = _MALI_OSK_INTERNAL_LOCKTYPE_SPIN_IRQ;
		lock->flags = 0;
		spin_lock_init( &lock->obj.spinlock );
	}
#if 0
	else if ( (flags & _MALI_OSK_LOCKFLAG_NONINTERRUPTABLE)
			  && (flags & _MALI_OSK_LOCKFLAG_READERWRITER) )
	{
		lock->type = _MALI_OSK_INTERNAL_LOCKTYPE_MUTEX_NONINT_RW;
		init_rwsem( &lock->obj.rw_sema );
	}
#endif
	else
	{
		
		if ( (flags & _MALI_OSK_LOCKFLAG_NONINTERRUPTABLE) )
		{
			lock->type = _MALI_OSK_INTERNAL_LOCKTYPE_MUTEX_NONINT;
		}
		else
		{
			lock->type = _MALI_OSK_INTERNAL_LOCKTYPE_MUTEX;
		}

		
		sema_init( &lock->obj.sema, 1 );
	}

#ifdef DEBUG
	
	lock->orig_flags = flags;

	
	lock->owner = 0;
	lock->nOwners = 0;
#endif 

    return lock;
}

#ifdef DEBUG
u32 _mali_osk_lock_get_owner( _mali_osk_lock_t *lock )
{
	return lock->owner;
}

u32 _mali_osk_lock_get_number_owners( _mali_osk_lock_t *lock )
{
	return lock->nOwners;
}

u32 _mali_osk_lock_get_mode( _mali_osk_lock_t *lock )
{
	return lock->mode;
}
#endif 

_mali_osk_errcode_t _mali_osk_lock_wait( _mali_osk_lock_t *lock, _mali_osk_lock_mode_t mode)
{
    _mali_osk_errcode_t err = _MALI_OSK_ERR_OK;

	
	MALI_DEBUG_ASSERT_POINTER( lock );

	MALI_DEBUG_ASSERT( _MALI_OSK_LOCKMODE_RW == mode
					 || _MALI_OSK_LOCKMODE_RO == mode );

	MALI_DEBUG_ASSERT( _MALI_OSK_LOCKMODE_RW == mode
					 || (_MALI_OSK_LOCKMODE_RO == mode && (_MALI_OSK_LOCKFLAG_READERWRITER & lock->orig_flags)) );

	switch ( lock->type )
	{
	case _MALI_OSK_INTERNAL_LOCKTYPE_SPIN:
		spin_lock(&lock->obj.spinlock);
		break;
	case _MALI_OSK_INTERNAL_LOCKTYPE_SPIN_IRQ:
		{
			unsigned long tmp_flags;
			spin_lock_irqsave(&lock->obj.spinlock, tmp_flags);
			lock->flags = tmp_flags;
		}
		break;

	case _MALI_OSK_INTERNAL_LOCKTYPE_MUTEX:
		if ( down_interruptible(&lock->obj.sema) )
		{
			MALI_PRINT_ERROR(("Can not lock mutex\n"));
			err = _MALI_OSK_ERR_RESTARTSYSCALL;
		}
		break;

	case _MALI_OSK_INTERNAL_LOCKTYPE_MUTEX_NONINT:
		down(&lock->obj.sema);
		break;

	case _MALI_OSK_INTERNAL_LOCKTYPE_MUTEX_NONINT_RW:
		if (mode == _MALI_OSK_LOCKMODE_RO)
        {
            down_read(&lock->obj.rw_sema);
        }
        else
        {
            down_write(&lock->obj.rw_sema);
        }
		break;

	default:
		MALI_DEBUG_PRINT_ERROR( ("Invalid internal lock type: %.8X", lock->type ) );
		break;
	}

#ifdef DEBUG
	
	if (_MALI_OSK_ERR_OK == err)
	{
		if (mode == _MALI_OSK_LOCKMODE_RW)
		{
			
			if (0 != lock->owner)
			{
				printk(KERN_ERR "%d: ERROR: Lock %p already has owner %d\n", _mali_osk_get_tid(), lock, lock->owner);
				dump_stack();
			}
			lock->owner = _mali_osk_get_tid();
			lock->mode = mode;
			++lock->nOwners;
		}
		else 
		{
			lock->owner |= _mali_osk_get_tid();
			lock->mode = mode;
			++lock->nOwners;
		}
	}
#endif

    return err;
}

void _mali_osk_lock_signal( _mali_osk_lock_t *lock, _mali_osk_lock_mode_t mode )
{
	
	MALI_DEBUG_ASSERT_POINTER( lock );

	MALI_DEBUG_ASSERT( _MALI_OSK_LOCKMODE_RW == mode
					 || _MALI_OSK_LOCKMODE_RO == mode );

	MALI_DEBUG_ASSERT( _MALI_OSK_LOCKMODE_RW == mode
					 || (_MALI_OSK_LOCKMODE_RO == mode && (_MALI_OSK_LOCKFLAG_READERWRITER & lock->orig_flags)) );

#ifdef DEBUG
	
	if (mode == _MALI_OSK_LOCKMODE_RW)
	{
		
		if (_mali_osk_get_tid() != lock->owner)
		{
			printk(KERN_ERR "%d: ERROR: Lock %p owner was %d\n", _mali_osk_get_tid(), lock, lock->owner);
			dump_stack();
		}
		
		lock->owner = 0;
		--lock->nOwners;
	}
	else 
	{
		if ((_mali_osk_get_tid() & lock->owner) != _mali_osk_get_tid())
		{
			printk(KERN_ERR "%d: ERROR: Not an owner of %p lock.\n", _mali_osk_get_tid(), lock);
			dump_stack();
		}

		if (0 == --lock->nOwners)
		{
			lock->owner = 0;
		}
	}
#endif 

	switch ( lock->type )
	{
	case _MALI_OSK_INTERNAL_LOCKTYPE_SPIN:
		spin_unlock(&lock->obj.spinlock);
		break;
	case _MALI_OSK_INTERNAL_LOCKTYPE_SPIN_IRQ:
		spin_unlock_irqrestore(&lock->obj.spinlock, lock->flags);
		break;

	case _MALI_OSK_INTERNAL_LOCKTYPE_MUTEX:
		
	case _MALI_OSK_INTERNAL_LOCKTYPE_MUTEX_NONINT:
		up(&lock->obj.sema);
		break;

	case _MALI_OSK_INTERNAL_LOCKTYPE_MUTEX_NONINT_RW:
		if (mode == _MALI_OSK_LOCKMODE_RO)
        {
            up_read(&lock->obj.rw_sema);
        }
        else
        {
            up_write(&lock->obj.rw_sema);
        }
		break;

	default:
		MALI_DEBUG_PRINT_ERROR( ("Invalid internal lock type: %.8X", lock->type ) );
		break;
	}
}

void _mali_osk_lock_term( _mali_osk_lock_t *lock )
{
	
	MALI_DEBUG_ASSERT_POINTER( lock );

	
    kfree(lock);
}
