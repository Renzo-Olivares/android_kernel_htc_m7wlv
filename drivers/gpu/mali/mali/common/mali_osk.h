/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef __MALI_OSK_H__
#define __MALI_OSK_H__

#ifdef __cplusplus
extern "C"
{
#endif




#ifndef __KERNEL__
	typedef unsigned char      u8;
	typedef signed char        s8;
	typedef unsigned short     u16;
	typedef signed short       s16;
	typedef unsigned int       u32;
	typedef signed int         s32;
	typedef unsigned long long u64;
	#define BITS_PER_LONG (sizeof(long)*8)
#else
	
	#include <linux/types.h>
#endif

	typedef unsigned long mali_bool;

#ifndef MALI_TRUE
	#define MALI_TRUE ((mali_bool)1)
#endif

#ifndef MALI_FALSE
	#define MALI_FALSE ((mali_bool)0)
#endif

typedef enum
{
    _MALI_OSK_ERR_OK = 0, 
    _MALI_OSK_ERR_FAULT = -1, 
    _MALI_OSK_ERR_INVALID_FUNC = -2, 
    _MALI_OSK_ERR_INVALID_ARGS = -3, 
    _MALI_OSK_ERR_NOMEM = -4, 
    _MALI_OSK_ERR_TIMEOUT = -5, 
    _MALI_OSK_ERR_RESTARTSYSCALL = -6, 
    _MALI_OSK_ERR_ITEM_NOT_FOUND = -7, 
    _MALI_OSK_ERR_BUSY = -8, 
	_MALI_OSK_ERR_UNSUPPORTED = -9, 
} _mali_osk_errcode_t;

 



typedef struct _mali_osk_irq_t_struct _mali_osk_irq_t;

typedef void  (*_mali_osk_irq_trigger_t)( void * arg );

typedef _mali_osk_errcode_t (*_mali_osk_irq_ack_t)( void * arg );

typedef _mali_osk_errcode_t  (*_mali_osk_irq_uhandler_t)( void * arg );

typedef void (*_mali_osk_irq_bhandler_t)( void * arg );
 



typedef struct
{
    union
    {
        u32 val;
        void *obj;
    } u;
} _mali_osk_atomic_t;
 




typedef enum
{
	_MALI_OSK_LOCK_ORDER_LAST = 0,

	_MALI_OSK_LOCK_ORDER_PM_EXECUTE,
	_MALI_OSK_LOCK_ORDER_UTILIZATION,
	_MALI_OSK_LOCK_ORDER_L2_COUNTER,
	_MALI_OSK_LOCK_ORDER_PROFILING,
	_MALI_OSK_LOCK_ORDER_L2_COMMAND,
	_MALI_OSK_LOCK_ORDER_PM_CORE_STATE,
	_MALI_OSK_LOCK_ORDER_GROUP,
	_MALI_OSK_LOCK_ORDER_SCHEDULER,

	_MALI_OSK_LOCK_ORDER_DESCRIPTOR_MAP,
	_MALI_OSK_LOCK_ORDER_MEM_PT_CACHE,
	_MALI_OSK_LOCK_ORDER_MEM_INFO,
	_MALI_OSK_LOCK_ORDER_MEM_SESSION,

	_MALI_OSK_LOCK_ORDER_SESSIONS,

	_MALI_OSK_LOCK_ORDER_FIRST
} _mali_osk_lock_order_t;


typedef enum
{
	_MALI_OSK_LOCKFLAG_SPINLOCK = 0x1,          
	_MALI_OSK_LOCKFLAG_NONINTERRUPTABLE = 0x2,  
	_MALI_OSK_LOCKFLAG_READERWRITER = 0x4,      
	_MALI_OSK_LOCKFLAG_ORDERED = 0x8,           
	_MALI_OSK_LOCKFLAG_ONELOCK = 0x10,          
	_MALI_OSK_LOCKFLAG_SPINLOCK_IRQ = 0x20,    

} _mali_osk_lock_flags_t;

typedef enum
{
	_MALI_OSK_LOCKMODE_UNDEF = -1,  
	_MALI_OSK_LOCKMODE_RW    = 0x0, 
	_MALI_OSK_LOCKMODE_RO,          
} _mali_osk_lock_mode_t;

typedef struct _mali_osk_lock_t_struct _mali_osk_lock_t;

#ifdef DEBUG
#define MALI_DEBUG_ASSERT_LOCK_HELD(l) MALI_DEBUG_ASSERT(_mali_osk_lock_get_owner(l) == _mali_osk_get_tid());

u32 _mali_osk_lock_get_owner( _mali_osk_lock_t *lock );
#endif

 


typedef struct _mali_io_address * mali_io_address;


#define _MALI_OSK_CPU_PAGE_ORDER ((u32)12)
#define _MALI_OSK_CPU_PAGE_SIZE (((u32)1) << (_MALI_OSK_CPU_PAGE_ORDER))
#define _MALI_OSK_CPU_PAGE_MASK (~((((u32)1) << (_MALI_OSK_CPU_PAGE_ORDER)) - ((u32)1)))
 


#define _MALI_OSK_MALI_PAGE_ORDER ((u32)12)
#define _MALI_OSK_MALI_PAGE_SIZE (((u32)1) << (_MALI_OSK_MALI_PAGE_ORDER))
#define _MALI_OSK_MALI_PAGE_MASK (~((((u32)1) << (_MALI_OSK_MALI_PAGE_ORDER)) - ((u32)1)))
 

typedef enum
{
	_MALI_OSK_MEM_MAPREGION_FLAG_OS_ALLOCATED_PHYSADDR = 0x1, 
} _mali_osk_mem_mapregion_flags_t;
 


typedef struct _mali_osk_notification_queue_t_struct _mali_osk_notification_queue_t;

typedef struct _mali_osk_notification_t_struct
{
	u32 notification_type;   
	u32 result_buffer_size; 
	void * result_buffer;   
} _mali_osk_notification_t;

 



typedef void (*_mali_osk_timer_callback_t)(void * arg );

typedef struct _mali_osk_timer_t_struct _mali_osk_timer_t;
 



typedef struct _mali_osk_list_s
{
	struct _mali_osk_list_s *next;
	struct _mali_osk_list_s *prev;
} _mali_osk_list_t;

#define _MALI_OSK_INIT_LIST_HEAD(exp) _mali_osk_list_init(exp)

#define _MALI_OSK_LIST_HEAD(exp)      _mali_osk_list_t exp

#define _MALI_OSK_CONTAINER_OF(ptr, type, member) \
             ((type *)( ((char *)ptr) - offsetof(type,member) ))

#define _MALI_OSK_LIST_ENTRY(ptr, type, member) \
            _MALI_OSK_CONTAINER_OF(ptr, type, member)

#define _MALI_OSK_LIST_FOREACHENTRY(ptr, tmp, list, type, member)         \
        for (ptr = _MALI_OSK_LIST_ENTRY((list)->next, type, member),      \
             tmp = _MALI_OSK_LIST_ENTRY(ptr->member.next, type, member); \
             &ptr->member != (list);                                    \
             ptr = tmp, tmp = _MALI_OSK_LIST_ENTRY(tmp->member.next, type, member))
 



typedef enum _mali_osk_resource_type
{
	RESOURCE_TYPE_FIRST =0,  

	MEMORY              =0,  
	OS_MEMORY           =1,  

	MALI_PP             =2,  
	MALI450PP           =2,  
	MALI400PP           =2,  
	MALI300PP           =2,  
	MALI200             =2,  
	
	MALI_GP             =3,  
	MALI450GP           =3,  
	MALI400GP           =3,  
	MALI300GP           =3,  
	MALIGP2             =3,  

	MMU                 =4,  

	FPGA_FRAMEWORK      =5,  

	MALI_L2             =6,  
	MALI450L2           =6,  
	MALI400L2           =6,  
	MALI300L2           =6,  

	MEM_VALIDATION      =7, 

	PMU                 =8, 

	RESOURCE_TYPE_COUNT      
} _mali_osk_resource_type_t;

typedef struct _mali_osk_resource
{
	_mali_osk_resource_type_t type; 
	const char * description;       
	u32 base;                       
	s32 cpu_usage_adjust;           
	u32 size;                       
	u32 irq;                        
	u32 flags;                      
	u32 mmu_id;                     
	u32 alloc_order;                
} _mali_osk_resource_t;
 


#include "mali_kernel_memory_engine.h"   


#define _MALI_OSK_IRQ_NUMBER_FAKE ((u32)0xFFFFFFF1)


#define _MALI_OSK_IRQ_NUMBER_PMM ((u32)0xFFFFFFF2)


/** @brief Initialize IRQ handling for a resource
 *
 * The _mali_osk_irq_t returned must be written into the resource-specific data
 * pointed to by data. This is so that the upper and lower handlers can call
 * _mali_osk_irq_schedulework().
 *
 * @note The caller must ensure that the resource does not generate an
 * interrupt after _mali_osk_irq_init() finishes, and before the
 * _mali_osk_irq_t is written into the resource-specific data. Otherwise,
 * the upper-half handler will fail to call _mali_osk_irq_schedulework().
 *
 * @param irqnum The IRQ number that the resource uses, as seen by the CPU.
 * The value -1 has a special meaning which indicates the use of probing, and trigger_func and ack_func must be
 * non-NULL.
 * @param uhandler The upper-half handler, corresponding to a ISR handler for
 * the resource
 * @param bhandler The lower-half handler, corresponding to an IST handler for
 * the resource
 * @param trigger_func Optional: a function to trigger the resource's irq, to
 * probe for the interrupt. Use NULL if irqnum != -1.
 * @param ack_func Optional: a function to acknowledge the resource's irq, to
 * probe for the interrupt. Use NULL if irqnum != -1.
 * @param data resource-specific data, which will be passed to uhandler,
 * bhandler and (if present) trigger_func and ack_funnc
 * @param description textual description of the IRQ resource.
 * @return on success, a pointer to a _mali_osk_irq_t object, which represents
 * the IRQ handling on this resource. NULL on failure.
 */
_mali_osk_irq_t *_mali_osk_irq_init( u32 irqnum, _mali_osk_irq_uhandler_t uhandler,	_mali_osk_irq_bhandler_t bhandler, _mali_osk_irq_trigger_t trigger_func, _mali_osk_irq_ack_t ack_func, void *data, const char *description );

void _mali_osk_irq_schedulework( _mali_osk_irq_t *irq );

void _mali_osk_irq_term( _mali_osk_irq_t *irq );

void _mali_osk_flush_workqueue( _mali_osk_irq_t *irq );

 



void _mali_osk_atomic_dec( _mali_osk_atomic_t *atom );

u32 _mali_osk_atomic_dec_return( _mali_osk_atomic_t *atom );

void _mali_osk_atomic_inc( _mali_osk_atomic_t *atom );

u32 _mali_osk_atomic_inc_return( _mali_osk_atomic_t *atom );

_mali_osk_errcode_t _mali_osk_atomic_init( _mali_osk_atomic_t *atom, u32 val );

u32 _mali_osk_atomic_read( _mali_osk_atomic_t *atom );

void _mali_osk_atomic_term( _mali_osk_atomic_t *atom );
  



void *_mali_osk_calloc( u32 n, u32 size );

void *_mali_osk_malloc( u32 size );

void _mali_osk_free( void *ptr );

void *_mali_osk_valloc( u32 size );

void _mali_osk_vfree( void *ptr );

void *_mali_osk_memcpy( void *dst, const void *src, u32 len );

void *_mali_osk_memset( void *s, u32 c, u32 n );
 


mali_bool _mali_osk_mem_check_allocated( u32 max_allocated );


_mali_osk_lock_t *_mali_osk_lock_init( _mali_osk_lock_flags_t flags, u32 initial, u32 order );

_mali_osk_errcode_t _mali_osk_lock_wait( _mali_osk_lock_t *lock, _mali_osk_lock_mode_t mode);


void _mali_osk_lock_signal( _mali_osk_lock_t *lock, _mali_osk_lock_mode_t mode );

void _mali_osk_lock_term( _mali_osk_lock_t *lock );
 



void _mali_osk_mem_barrier( void );

void _mali_osk_write_mem_barrier( void );

mali_io_address _mali_osk_mem_mapioregion( u32 phys, u32 size, const char *description );

void _mali_osk_mem_unmapioregion( u32 phys, u32 size, mali_io_address mapping );

mali_io_address _mali_osk_mem_allocioregion( u32 *phys, u32 size );

void _mali_osk_mem_freeioregion( u32 phys, u32 size, mali_io_address mapping );

_mali_osk_errcode_t _mali_osk_mem_reqregion( u32 phys, u32 size, const char *description );

void _mali_osk_mem_unreqregion( u32 phys, u32 size );

u32 _mali_osk_mem_ioread32( volatile mali_io_address mapping, u32 offset );

void _mali_osk_mem_iowrite32_relaxed( volatile mali_io_address addr, u32 offset, u32 val );

void _mali_osk_mem_iowrite32( volatile mali_io_address mapping, u32 offset, u32 val );

void _mali_osk_cache_flushall( void );

void _mali_osk_cache_ensure_uncached_range_flushed( void *uncached_mapping, u32 offset, u32 size );

 



_mali_osk_notification_t *_mali_osk_notification_create( u32 type, u32 size );

void _mali_osk_notification_delete( _mali_osk_notification_t *object );

_mali_osk_notification_queue_t *_mali_osk_notification_queue_init( void );

void _mali_osk_notification_queue_term( _mali_osk_notification_queue_t *queue );

void _mali_osk_notification_queue_send( _mali_osk_notification_queue_t *queue, _mali_osk_notification_t *object );

#if MALI_STATE_TRACKING
mali_bool _mali_osk_notification_queue_is_empty( _mali_osk_notification_queue_t *queue );
#endif

/** @brief Receive a notification from a queue
 *
 * Receives a single notification from the given queue.
 *
 * If no notifciations are ready the thread will sleep until one becomes ready.
 * Therefore, notifications may not be received into an
 * IRQ or 'atomic' context (that is, a context where sleeping is disallowed).
 *
 * @param queue The queue to receive from
 * @param result Pointer to storage of a pointer of type
 * \ref _mali_osk_notification_t*. \a result will be written to such that the
 * expression \a (*result) will evaluate to a pointer to a valid
 * \ref _mali_osk_notification_t object, or NULL if none were received.
 * @return _MALI_OSK_ERR_OK on success. _MALI_OSK_ERR_RESTARTSYSCALL if the sleep was interrupted.
 */
_mali_osk_errcode_t _mali_osk_notification_queue_receive( _mali_osk_notification_queue_t *queue, _mali_osk_notification_t **result );

/** @brief Dequeues a notification from a queue
 *
 * Receives a single notification from the given queue.
 *
 * If no notifciations are ready the function call will return an error code.
 *
 * @param queue The queue to receive from
 * @param result Pointer to storage of a pointer of type
 * \ref _mali_osk_notification_t*. \a result will be written to such that the
 * expression \a (*result) will evaluate to a pointer to a valid
 * \ref _mali_osk_notification_t object, or NULL if none were received.
 * @return _MALI_OSK_ERR_OK on success, _MALI_OSK_ERR_ITEM_NOT_FOUND if queue was empty.
 */
_mali_osk_errcode_t _mali_osk_notification_queue_dequeue( _mali_osk_notification_queue_t *queue, _mali_osk_notification_t **result );

 



_mali_osk_timer_t *_mali_osk_timer_init(void);

void _mali_osk_timer_add( _mali_osk_timer_t *tim, u32 ticks_to_expire );

void _mali_osk_timer_mod( _mali_osk_timer_t *tim, u32 expiry_tick);

void _mali_osk_timer_del( _mali_osk_timer_t *tim );

void _mali_osk_timer_setcallback( _mali_osk_timer_t *tim, _mali_osk_timer_callback_t callback, void *data );

void _mali_osk_timer_term( _mali_osk_timer_t *tim );
 



int	_mali_osk_time_after( u32 ticka, u32 tickb );

u32	_mali_osk_time_mstoticks( u32 ms );

u32	_mali_osk_time_tickstoms( u32 ticks );


u32	_mali_osk_time_tickcount( void );

void _mali_osk_time_ubusydelay( u32 usecs );

u64 _mali_osk_time_get_ns( void );


 


u32 _mali_osk_clz( u32 val );
 

typedef struct _mali_osk_wait_queue_t_struct _mali_osk_wait_queue_t;

_mali_osk_wait_queue_t* _mali_osk_wait_queue_init( void );

void _mali_osk_wait_queue_wait_event( _mali_osk_wait_queue_t *queue, mali_bool (*condition)(void) );

void _mali_osk_wait_queue_wake_up( _mali_osk_wait_queue_t *queue );

void _mali_osk_wait_queue_term( _mali_osk_wait_queue_t *queue );
 



void _mali_osk_dbgmsg( const char *fmt, ... );

/** @brief Print fmt into buf.
 *
 * The interpretation of \a fmt is the same as the \c format parameter in
 * _mali_osu_vsnprintf().
 *
 * @param buf a pointer to the result buffer
 * @param size the total number of bytes allowed to write to \a buf
 * @param fmt a _mali_osu_vsnprintf() style format string
 * @param ... a variable-number of parameters suitable for \a fmt
 * @return The number of bytes written to \a buf
 */
u32 _mali_osk_snprintf( char *buf, u32 size, const char *fmt, ... );

void _mali_osk_abort(void);

void _mali_osk_break(void);

u32 _mali_osk_get_pid(void);

u32 _mali_osk_get_tid(void);

void _mali_osk_pm_dev_enable(void);

_mali_osk_errcode_t _mali_osk_pm_dev_idle(void);

_mali_osk_errcode_t _mali_osk_pm_dev_activate(void);

 

 

 


#ifdef __cplusplus
}
#endif

#include "mali_osk_specific.h"           

#ifndef MALI_STATIC_INLINE
	#error MALI_STATIC_INLINE not defined on your OS
#endif

#ifndef MALI_NON_STATIC_INLINE
	#error MALI_NON_STATIC_INLINE not defined on your OS
#endif

#endif 
