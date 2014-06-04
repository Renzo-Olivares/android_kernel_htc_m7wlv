/* =========================================================================
 * $File: //dwh/usb_iip/dev/software/dwc_common_port_2/dwc_os.h $
 * $Revision: #14 $
 * $Date: 2010/11/04 $
 * $Change: 1621695 $
 *
 * Synopsys Portability Library Software and documentation
 * (hereinafter, "Software") is an Unsupported proprietary work of
 * Synopsys, Inc. unless otherwise expressly agreed to in writing
 * between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product
 * under any End User Software License Agreement or Agreement for
 * Licensed Product with Synopsys or any supplement thereto. You are
 * permitted to use and redistribute this Software in source and binary
 * forms, with or without modification, provided that redistributions
 * of source code must retain this notice. You may not view, use,
 * disclose, copy or distribute this file or any information contained
 * herein except pursuant to this license grant from Synopsys. If you
 * do not agree with this notice, including the disclaimer below, then
 * you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS"
 * BASIS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL
 * SYNOPSYS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================= */
#ifndef _DWC_OS_H_
#define _DWC_OS_H_

#ifdef __cplusplus
extern "C" {
#endif



#ifdef DWC_LINUX
# include <linux/types.h>
# ifdef CONFIG_DEBUG_MUTEXES
#  include <linux/mutex.h>
# endif
# include <linux/errno.h>
# include <stdarg.h>
#endif

#if defined(DWC_FREEBSD) || defined(DWC_NETBSD)
# include <os_dep.h>
#endif



typedef uint8_t dwc_bool_t;
#define YES  1
#define NO   0

#ifdef DWC_LINUX

#define DWC_E_INVALID		EINVAL
#define DWC_E_NO_MEMORY		ENOMEM
#define DWC_E_NO_DEVICE		ENODEV
#define DWC_E_NOT_SUPPORTED	EOPNOTSUPP
#define DWC_E_TIMEOUT		ETIMEDOUT
#define DWC_E_BUSY		EBUSY
#define DWC_E_AGAIN		EAGAIN
#define DWC_E_RESTART		ERESTART
#define DWC_E_ABORT		ECONNABORTED
#define DWC_E_SHUTDOWN		ESHUTDOWN
#define DWC_E_NO_DATA		ENODATA
#define DWC_E_DISCONNECT	ECONNRESET
#define DWC_E_UNKNOWN		EINVAL
#define DWC_E_NO_STREAM_RES	ENOSR
#define DWC_E_COMMUNICATION	ECOMM
#define DWC_E_OVERFLOW		EOVERFLOW
#define DWC_E_PROTOCOL		EPROTO
#define DWC_E_IN_PROGRESS	EINPROGRESS
#define DWC_E_PIPE		EPIPE
#define DWC_E_IO		EIO
#define DWC_E_NO_SPACE		ENOSPC

#else

#define DWC_E_INVALID		1001
#define DWC_E_NO_MEMORY		1002
#define DWC_E_NO_DEVICE		1003
#define DWC_E_NOT_SUPPORTED	1004
#define DWC_E_TIMEOUT		1005
#define DWC_E_BUSY		1006
#define DWC_E_AGAIN		1007
#define DWC_E_RESTART		1008
#define DWC_E_ABORT		1009
#define DWC_E_SHUTDOWN		1010
#define DWC_E_NO_DATA		1011
#define DWC_E_DISCONNECT	2000
#define DWC_E_UNKNOWN		3000
#define DWC_E_NO_STREAM_RES	4001
#define DWC_E_COMMUNICATION	4002
#define DWC_E_OVERFLOW		4003
#define DWC_E_PROTOCOL		4004
#define DWC_E_IN_PROGRESS	4005
#define DWC_E_PIPE		4006
#define DWC_E_IO		4007
#define DWC_E_NO_SPACE		4008

#endif



extern dwc_bool_t DWC_IN_IRQ(void);
#define dwc_in_irq DWC_IN_IRQ

static inline char *dwc_irq(void) {
	return DWC_IN_IRQ() ? "IRQ" : "";
}

extern dwc_bool_t DWC_IN_BH(void);
#define dwc_in_bh DWC_IN_BH

static inline char *dwc_bh(void) {
	return DWC_IN_BH() ? "BH" : "";
}

extern void DWC_VPRINTF(char *format, va_list args);
#define dwc_vprintf DWC_VPRINTF

extern int DWC_VSNPRINTF(char *str, int size, char *format, va_list args);
#define dwc_vsnprintf DWC_VSNPRINTF

extern void DWC_PRINTF(char *format, ...)
#ifdef __GNUC__
	__attribute__ ((format(printf, 1, 2)));
#else
	;
#endif
#define dwc_printf DWC_PRINTF

extern int DWC_SPRINTF(char *string, char *format, ...)
#ifdef __GNUC__
	__attribute__ ((format(printf, 2, 3)));
#else
	;
#endif
#define dwc_sprintf DWC_SPRINTF

extern int DWC_SNPRINTF(char *string, int size, char *format, ...)
#ifdef __GNUC__
	__attribute__ ((format(printf, 3, 4)));
#else
	;
#endif
#define dwc_snprintf DWC_SNPRINTF

extern void __DWC_WARN(char *format, ...)
#ifdef __GNUC__
	__attribute__ ((format(printf, 1, 2)));
#else
	;
#endif

extern void __DWC_ERROR(char *format, ...)
#ifdef __GNUC__
	__attribute__ ((format(printf, 1, 2)));
#else
	;
#endif

extern void DWC_EXCEPTION(char *format, ...)
#ifdef __GNUC__
	__attribute__ ((format(printf, 1, 2)));
#else
	;
#endif
#define dwc_exception DWC_EXCEPTION

#ifdef DEBUG
extern void __DWC_DEBUG(char *format, ...)
#ifdef __GNUC__
	__attribute__ ((format(printf, 1, 2)));
#else
	;
#endif
#else
#define __DWC_DEBUG(...)
#endif

#define DWC_DEBUG(_format, _args...) __DWC_DEBUG("DEBUG:%s:%s: " _format "\n", \
						 __func__, dwc_irq(), ## _args)
#define dwc_debug DWC_DEBUG
#define DWC_INFO(_format, _args...) DWC_PRINTF("INFO:%s: " _format "\n", \
					       dwc_irq(), ## _args)
#define dwc_info DWC_INFO
#define DWC_WARN(_format, _args...) __DWC_WARN("WARN:%s:%s:%d: " _format "\n", \
					dwc_irq(), __func__, __LINE__, ## _args)
#define dwc_warn DWC_WARN
#define DWC_ERROR(_format, _args...) __DWC_ERROR("ERROR:%s:%s:%d: " _format "\n", \
					dwc_irq(), __func__, __LINE__, ## _args)
#define dwc_error DWC_ERROR

#define DWC_PROTO_ERROR(_format, _args...) __DWC_WARN("ERROR:%s:%s:%d: " _format "\n", \
						dwc_irq(), __func__, __LINE__, ## _args)
#define dwc_proto_error DWC_PROTO_ERROR

#ifdef DEBUG
#define DWC_ASSERT(_expr, _format, _args...) do { \
	if (!(_expr)) { DWC_EXCEPTION("%s:%s:%d: " _format "\n", dwc_irq(), \
				      __FILE__, __LINE__, ## _args); } \
	} while (0)
#else
#define DWC_ASSERT(_x...)
#endif
#define dwc_assert DWC_ASSERT



extern uint32_t DWC_CPU_TO_LE32(uint32_t *p);
#define dwc_cpu_to_le32 DWC_CPU_TO_LE32

extern uint32_t DWC_CPU_TO_BE32(uint32_t *p);
#define dwc_cpu_to_be32 DWC_CPU_TO_BE32

extern uint32_t DWC_LE32_TO_CPU(uint32_t *p);
#define dwc_le32_to_cpu DWC_LE32_TO_CPU

extern uint32_t DWC_BE32_TO_CPU(uint32_t *p);
#define dwc_be32_to_cpu DWC_BE32_TO_CPU

extern uint16_t DWC_CPU_TO_LE16(uint16_t *p);
#define dwc_cpu_to_le16 DWC_CPU_TO_LE16

extern uint16_t DWC_CPU_TO_BE16(uint16_t *p);
#define dwc_cpu_to_be16 DWC_CPU_TO_BE16

extern uint16_t DWC_LE16_TO_CPU(uint16_t *p);
#define dwc_le16_to_cpu DWC_LE16_TO_CPU

extern uint16_t DWC_BE16_TO_CPU(uint16_t *p);
#define dwc_be16_to_cpu DWC_BE16_TO_CPU



#ifdef DWC_LINUX
extern uint32_t DWC_READ_REG32(uint32_t volatile *reg);
#define dwc_read_reg32(_reg_) DWC_READ_REG32(_reg_)

extern uint64_t DWC_READ_REG64(uint64_t volatile *reg);
#define dwc_read_reg64(_reg_) DWC_READ_REG64(_reg_)

extern void DWC_WRITE_REG32(uint32_t volatile *reg, uint32_t value);
#define dwc_write_reg32(_reg_,_val_) DWC_WRITE_REG32(_reg_, _val_)

extern void DWC_WRITE_REG64(uint64_t volatile *reg, uint64_t value);
#define dwc_write_reg64(_reg_,_val_) DWC_WRITE_REG64(_reg_, _val_)

extern void DWC_MODIFY_REG32(uint32_t volatile *reg, uint32_t clear_mask, uint32_t set_mask);
#define dwc_modify_reg32(_reg_,_cmsk_,_smsk_) DWC_MODIFY_REG32(_reg_,_cmsk_,_smsk_)
extern void DWC_MODIFY_REG64(uint64_t volatile *reg, uint64_t clear_mask, uint64_t set_mask);
#define dwc_modify_reg64(_reg_,_cmsk_,_smsk_) DWC_MODIFY_REG64(_reg_,_cmsk_,_smsk_)

#endif	

#if defined(DWC_FREEBSD) || defined(DWC_NETBSD)
typedef struct dwc_ioctx {
	struct device *dev;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
} dwc_ioctx_t;

extern uint32_t DWC_READ_REG32(void *io_ctx, uint32_t volatile *reg);
#define dwc_read_reg32 DWC_READ_REG32

extern uint64_t DWC_READ_REG64(void *io_ctx, uint64_t volatile *reg);
#define dwc_read_reg64 DWC_READ_REG64

extern void DWC_WRITE_REG32(void *io_ctx, uint32_t volatile *reg, uint32_t value);
#define dwc_write_reg32 DWC_WRITE_REG32

extern void DWC_WRITE_REG64(void *io_ctx, uint64_t volatile *reg, uint64_t value);
#define dwc_write_reg64 DWC_WRITE_REG64

extern void DWC_MODIFY_REG32(void *io_ctx, uint32_t volatile *reg, uint32_t clear_mask, uint32_t set_mask);
#define dwc_modify_reg32 DWC_MODIFY_REG32
extern void DWC_MODIFY_REG64(void *io_ctx, uint64_t volatile *reg, uint64_t clear_mask, uint64_t set_mask);
#define dwc_modify_reg64 DWC_MODIFY_REG64

#endif	



#ifdef DWC_LINUX

# ifdef DWC_DEBUG_REGS

#define dwc_define_read_write_reg_n(_reg,_container_type) \
static inline uint32_t dwc_read_##_reg##_n(_container_type *container, int num) { \
	return DWC_READ_REG32(&container->regs->_reg[num]); \
} \
static inline void dwc_write_##_reg##_n(_container_type *container, int num, uint32_t data) { \
	DWC_DEBUG("WRITING %8s[%d]: %p: %08x", #_reg, num, \
		  &(((uint32_t*)container->regs->_reg)[num]), data); \
	DWC_WRITE_REG32(&(((uint32_t*)container->regs->_reg)[num]), data); \
}

#define dwc_define_read_write_reg(_reg,_container_type) \
static inline uint32_t dwc_read_##_reg(_container_type *container) { \
	return DWC_READ_REG32(&container->regs->_reg); \
} \
static inline void dwc_write_##_reg(_container_type *container, uint32_t data) { \
	DWC_DEBUG("WRITING %11s: %p: %08x", #_reg, &container->regs->_reg, data); \
	DWC_WRITE_REG32(&container->regs->_reg, data); \
}

# else	

#define dwc_define_read_write_reg_n(_reg,_container_type) \
static inline uint32_t dwc_read_##_reg##_n(_container_type *container, int num) { \
	return DWC_READ_REG32(&container->regs->_reg[num]); \
} \
static inline void dwc_write_##_reg##_n(_container_type *container, int num, uint32_t data) { \
	DWC_WRITE_REG32(&(((uint32_t*)container->regs->_reg)[num]), data); \
}

#define dwc_define_read_write_reg(_reg,_container_type) \
static inline uint32_t dwc_read_##_reg(_container_type *container) { \
	return DWC_READ_REG32(&container->regs->_reg); \
} \
static inline void dwc_write_##_reg(_container_type *container, uint32_t data) { \
	DWC_WRITE_REG32(&container->regs->_reg, data); \
}

# endif	

#endif	

#if defined(DWC_FREEBSD) || defined(DWC_NETBSD)

# ifdef DWC_DEBUG_REGS

#define dwc_define_read_write_reg_n(_reg,_container_type) \
static inline uint32_t dwc_read_##_reg##_n(void *io_ctx, _container_type *container, int num) { \
	return DWC_READ_REG32(io_ctx, &container->regs->_reg[num]); \
} \
static inline void dwc_write_##_reg##_n(void *io_ctx, _container_type *container, int num, uint32_t data) { \
	DWC_DEBUG("WRITING %8s[%d]: %p: %08x", #_reg, num, \
		  &(((uint32_t*)container->regs->_reg)[num]), data); \
	DWC_WRITE_REG32(io_ctx, &(((uint32_t*)container->regs->_reg)[num]), data); \
}

#define dwc_define_read_write_reg(_reg,_container_type) \
static inline uint32_t dwc_read_##_reg(void *io_ctx, _container_type *container) { \
	return DWC_READ_REG32(io_ctx, &container->regs->_reg); \
} \
static inline void dwc_write_##_reg(void *io_ctx, _container_type *container, uint32_t data) { \
	DWC_DEBUG("WRITING %11s: %p: %08x", #_reg, &container->regs->_reg, data); \
	DWC_WRITE_REG32(io_ctx, &container->regs->_reg, data); \
}

# else	

#define dwc_define_read_write_reg_n(_reg,_container_type) \
static inline uint32_t dwc_read_##_reg##_n(void *io_ctx, _container_type *container, int num) { \
	return DWC_READ_REG32(io_ctx, &container->regs->_reg[num]); \
} \
static inline void dwc_write_##_reg##_n(void *io_ctx, _container_type *container, int num, uint32_t data) { \
	DWC_WRITE_REG32(io_ctx, &(((uint32_t*)container->regs->_reg)[num]), data); \
}

#define dwc_define_read_write_reg(_reg,_container_type) \
static inline uint32_t dwc_read_##_reg(void *io_ctx, _container_type *container) { \
	return DWC_READ_REG32(io_ctx, &container->regs->_reg); \
} \
static inline void dwc_write_##_reg(void *io_ctx, _container_type *container, uint32_t data) { \
	DWC_WRITE_REG32(io_ctx, &container->regs->_reg, data); \
}

# endif	

#endif	



#ifdef DWC_CRYPTOLIB

extern int DWC_AES_CBC(uint8_t *message, uint32_t messagelen, uint8_t *key, uint32_t keylen, uint8_t iv[16], uint8_t *out);
#define dwc_aes_cbc DWC_AES_CBC

extern void DWC_RANDOM_BYTES(uint8_t *buffer, uint32_t length);
#define dwc_random_bytes DWC_RANDOM_BYTES

extern int DWC_SHA256(uint8_t *message, uint32_t len, uint8_t *out);
#define dwc_sha256 DWC_SHA256

extern int DWC_HMAC_SHA256(uint8_t *message, uint32_t messagelen, uint8_t *key, uint32_t keylen, uint8_t *out);
#define dwc_hmac_sha256 DWC_HMAC_SHA256

#endif	



#define DWC_PAGE_SIZE 4096
#define DWC_PAGE_OFFSET(addr) (((uint32_t)addr) & 0xfff)
#define DWC_PAGE_ALIGNED(addr) ((((uint32_t)addr) & 0xfff) == 0)

#define DWC_INVALID_DMA_ADDR 0x0

#ifdef DWC_LINUX
typedef dma_addr_t dwc_dma_t;
#endif

#if defined(DWC_FREEBSD) || defined(DWC_NETBSD)
typedef bus_addr_t dwc_dma_t;
#endif

#ifdef DWC_FREEBSD
typedef struct dwc_dmactx {
	struct device *dev;
	bus_dma_tag_t dma_tag;
	bus_dmamap_t dma_map;
	bus_addr_t dma_paddr;
	void *dma_vaddr;
} dwc_dmactx_t;
#endif

#ifdef DWC_NETBSD
typedef struct dwc_dmactx {
	struct device *dev;
	bus_dma_tag_t dma_tag;
	bus_dmamap_t dma_map;
	bus_dma_segment_t segs[1];
	int nsegs;
	bus_addr_t dma_paddr;
	void *dma_vaddr;
} dwc_dmactx_t;
#endif

#if 0
extern dwc_pool_t *DWC_DMA_POOL_CREATE(uint32_t size, uint32_t align, uint32_t boundary);

extern void DWC_DMA_POOL_DESTROY(dwc_pool_t *pool);

extern void *DWC_DMA_POOL_ALLOC(dwc_pool_t *pool, uint64_t *dma_addr);

extern void DWC_DMA_POOL_FREE(dwc_pool_t *pool, void *vaddr, void *daddr);
#endif

extern void *__DWC_DMA_ALLOC(void *dma_ctx, uint32_t size, dwc_dma_t *dma_addr);

extern void *__DWC_DMA_ALLOC_ATOMIC(void *dma_ctx, uint32_t size, dwc_dma_t *dma_addr);

extern void __DWC_DMA_FREE(void *dma_ctx, uint32_t size, void *virt_addr, dwc_dma_t dma_addr);

extern void *__DWC_ALLOC(void *mem_ctx, uint32_t size);

extern void *__DWC_ALLOC_ATOMIC(void *mem_ctx, uint32_t size);

extern void __DWC_FREE(void *mem_ctx, void *addr);

#ifndef DWC_DEBUG_MEMORY

#define DWC_ALLOC(_size_) __DWC_ALLOC(NULL, _size_)
#define DWC_ALLOC_ATOMIC(_size_) __DWC_ALLOC_ATOMIC(NULL, _size_)
#define DWC_FREE(_addr_) __DWC_FREE(NULL, _addr_)

# ifdef DWC_LINUX
#define DWC_DMA_ALLOC(_size_,_dma_) __DWC_DMA_ALLOC(NULL, _size_, _dma_)
#define DWC_DMA_ALLOC_ATOMIC(_size_,_dma_) __DWC_DMA_ALLOC_ATOMIC(NULL, _size_,_dma_)
#define DWC_DMA_FREE(_size_,_virt_,_dma_) __DWC_DMA_FREE(NULL, _size_, _virt_, _dma_)
# endif

# if defined(DWC_FREEBSD) || defined(DWC_NETBSD)
#define DWC_DMA_ALLOC __DWC_DMA_ALLOC
#define DWC_DMA_FREE __DWC_DMA_FREE
# endif

#else	

extern void *dwc_alloc_debug(void *mem_ctx, uint32_t size, char const *func, int line);
extern void *dwc_alloc_atomic_debug(void *mem_ctx, uint32_t size, char const *func, int line);
extern void dwc_free_debug(void *mem_ctx, void *addr, char const *func, int line);
extern void *dwc_dma_alloc_debug(void *dma_ctx, uint32_t size, dwc_dma_t *dma_addr,
				 char const *func, int line);
extern void *dwc_dma_alloc_atomic_debug(void *dma_ctx, uint32_t size, dwc_dma_t *dma_addr,
				char const *func, int line);
extern void dwc_dma_free_debug(void *dma_ctx, uint32_t size, void *virt_addr,
			       dwc_dma_t dma_addr, char const *func, int line);

extern int dwc_memory_debug_start(void *mem_ctx);
extern void dwc_memory_debug_stop(void);
extern void dwc_memory_debug_report(void);

#define DWC_ALLOC(_size_) dwc_alloc_debug(NULL, _size_, __func__, __LINE__)
#define DWC_ALLOC_ATOMIC(_size_) dwc_alloc_atomic_debug(NULL, _size_, \
							__func__, __LINE__)
#define DWC_FREE(_addr_) dwc_free_debug(NULL, _addr_, __func__, __LINE__)

# ifdef DWC_LINUX
#define DWC_DMA_ALLOC(_size_,_dma_) dwc_dma_alloc_debug(NULL, _size_, \
						_dma_, __func__, __LINE__)
#define DWC_DMA_ALLOC_ATOMIC(_size_,_dma_) dwc_dma_alloc_atomic_debug(NULL, _size_, \
						_dma_, __func__, __LINE__)
#define DWC_DMA_FREE(_size_,_virt_,_dma_) dwc_dma_free_debug(NULL, _size_, \
						_virt_, _dma_, __func__, __LINE__)
# endif

# if defined(DWC_FREEBSD) || defined(DWC_NETBSD)
#define DWC_DMA_ALLOC(_ctx_,_size_,_dma_) dwc_dma_alloc_debug(_ctx_, _size_, \
						_dma_, __func__, __LINE__)
#define DWC_DMA_FREE(_ctx_,_size_,_virt_,_dma_) dwc_dma_free_debug(_ctx_, _size_, \
						 _virt_, _dma_, __func__, __LINE__)
# endif

#endif 

#define dwc_alloc(_size_) DWC_ALLOC(_size_)
#define dwc_alloc_atomic(_size_) DWC_ALLOC_ATOMIC(_size_)
#define dwc_free(_addr_) DWC_FREE(_addr_)

#ifdef DWC_LINUX
#define dwc_dma_alloc(_size_,_dma_) DWC_DMA_ALLOC(_size_, _dma_)
#define dwc_dma_alloc_atomic(_size_,_dma_) DWC_DMA_ALLOC_ATOMIC(_size_, _dma_)
#define dwc_dma_free(_size_,_virt_,_dma_) DWC_DMA_FREE(_size_, _virt_, _dma_)
#endif

#if defined(DWC_FREEBSD) || defined(DWC_NETBSD)
#define dwc_dma_alloc DWC_DMA_ALLOC
#define dwc_dma_free DWC_DMA_FREE
#endif



extern void *DWC_MEMSET(void *dest, uint8_t byte, uint32_t size);
#define dwc_memset DWC_MEMSET

extern void *DWC_MEMCPY(void *dest, void const *src, uint32_t size);
#define dwc_memcpy DWC_MEMCPY

extern void *DWC_MEMMOVE(void *dest, void *src, uint32_t size);
#define dwc_memmove DWC_MEMMOVE

extern int DWC_MEMCMP(void *m1, void *m2, uint32_t size);
#define dwc_memcmp DWC_MEMCMP

extern int DWC_STRCMP(void *s1, void *s2);
#define dwc_strcmp DWC_STRCMP

extern int DWC_STRNCMP(void *s1, void *s2, uint32_t size);
#define dwc_strncmp DWC_STRNCMP

extern int DWC_STRLEN(char const *str);
#define dwc_strlen DWC_STRLEN

extern char *DWC_STRCPY(char *to, const char *from);
#define dwc_strcpy DWC_STRCPY

extern char *DWC_STRDUP(char const *str);
#define dwc_strdup(_ctx_,_str_) DWC_STRDUP(_str_)

extern int DWC_ATOI(const char *str, int32_t *value);
#define dwc_atoi DWC_ATOI

extern int DWC_ATOUI(const char *str, uint32_t *value);
#define dwc_atoui DWC_ATOUI

#ifdef DWC_UTFLIB
extern int DWC_UTF8_TO_UTF16LE(uint8_t const *utf8string, uint16_t *utf16string, unsigned len);
#define dwc_utf8_to_utf16le DWC_UTF8_TO_UTF16LE
#endif


struct dwc_waitq;

typedef struct dwc_waitq dwc_waitq_t;

typedef int (*dwc_waitq_condition_t)(void *data);

extern dwc_waitq_t *DWC_WAITQ_ALLOC(void);
#define dwc_waitq_alloc(_ctx_) DWC_WAITQ_ALLOC()

extern void DWC_WAITQ_FREE(dwc_waitq_t *wq);
#define dwc_waitq_free DWC_WAITQ_FREE

extern int32_t DWC_WAITQ_WAIT(dwc_waitq_t *wq, dwc_waitq_condition_t cond, void *data);
#define dwc_waitq_wait DWC_WAITQ_WAIT

extern int32_t DWC_WAITQ_WAIT_TIMEOUT(dwc_waitq_t *wq, dwc_waitq_condition_t cond,
				      void *data, int32_t msecs);
#define dwc_waitq_wait_timeout DWC_WAITQ_WAIT_TIMEOUT

extern void DWC_WAITQ_TRIGGER(dwc_waitq_t *wq);
#define dwc_waitq_trigger DWC_WAITQ_TRIGGER

extern void DWC_WAITQ_ABORT(dwc_waitq_t *wq);
#define dwc_waitq_abort DWC_WAITQ_ABORT



struct dwc_thread;

typedef struct dwc_thread dwc_thread_t;

typedef int (*dwc_thread_function_t)(void *data);

extern dwc_thread_t *DWC_THREAD_RUN(dwc_thread_function_t func, char *name, void *data);
#define dwc_thread_run(_ctx_,_func_,_name_,_data_) DWC_THREAD_RUN(_func_, _name_, _data_)

extern int DWC_THREAD_STOP(dwc_thread_t *thread);
#define dwc_thread_stop DWC_THREAD_STOP

#ifdef DWC_LINUX
extern dwc_bool_t DWC_THREAD_SHOULD_STOP(void);
#define dwc_thread_should_stop(_thrd_) DWC_THREAD_SHOULD_STOP()

#define dwc_thread_exit(_thrd_)
#endif

#if defined(DWC_FREEBSD) || defined(DWC_NETBSD)
extern dwc_bool_t DWC_THREAD_SHOULD_STOP(dwc_thread_t *thread);
#define dwc_thread_should_stop DWC_THREAD_SHOULD_STOP

extern void DWC_THREAD_EXIT(dwc_thread_t *thread);
#define dwc_thread_exit DWC_THREAD_EXIT
#endif


struct dwc_workq;

typedef struct dwc_workq dwc_workq_t;

typedef void (*dwc_work_callback_t)(void *data);

extern dwc_workq_t *DWC_WORKQ_ALLOC(char *name);
#define dwc_workq_alloc(_ctx_,_name_) DWC_WORKQ_ALLOC(_name_)

extern void DWC_WORKQ_FREE(dwc_workq_t *workq);
#define dwc_workq_free DWC_WORKQ_FREE

extern void DWC_WORKQ_SCHEDULE(dwc_workq_t *workq, dwc_work_callback_t cb,
			       void *data, char *format, ...)
#ifdef __GNUC__
	__attribute__ ((format(printf, 4, 5)));
#else
	;
#endif
#define dwc_workq_schedule DWC_WORKQ_SCHEDULE

extern void DWC_WORKQ_SCHEDULE_DELAYED(dwc_workq_t *workq, dwc_work_callback_t cb,
				       void *data, uint32_t time, char *format, ...)
#ifdef __GNUC__
	__attribute__ ((format(printf, 5, 6)));
#else
	;
#endif
#define dwc_workq_schedule_delayed DWC_WORKQ_SCHEDULE_DELAYED

extern int DWC_WORKQ_PENDING(dwc_workq_t *workq);
#define dwc_workq_pending DWC_WORKQ_PENDING

extern int DWC_WORKQ_WAIT_WORK_DONE(dwc_workq_t *workq, int timeout);
#define dwc_workq_wait_work_done DWC_WORKQ_WAIT_WORK_DONE


struct dwc_tasklet;

typedef struct dwc_tasklet dwc_tasklet_t;

typedef void (*dwc_tasklet_callback_t)(void *data);

extern dwc_tasklet_t *DWC_TASK_ALLOC(char *name, dwc_tasklet_callback_t cb, void *data);
#define dwc_task_alloc(_ctx_,_name_,_cb_,_data_) DWC_TASK_ALLOC(_name_, _cb_, _data_)

extern void DWC_TASK_FREE(dwc_tasklet_t *task);
#define dwc_task_free DWC_TASK_FREE

extern void DWC_TASK_SCHEDULE(dwc_tasklet_t *task);
#define dwc_task_schedule DWC_TASK_SCHEDULE


struct dwc_timer;

typedef struct dwc_timer dwc_timer_t;

typedef void (*dwc_timer_callback_t)(void *data);

extern dwc_timer_t *DWC_TIMER_ALLOC(char *name, dwc_timer_callback_t cb, void *data);
#define dwc_timer_alloc(_ctx_,_name_,_cb_,_data_) DWC_TIMER_ALLOC(_name_,_cb_,_data_)

extern void DWC_TIMER_FREE(dwc_timer_t *timer);
#define dwc_timer_free DWC_TIMER_FREE

extern void DWC_TIMER_SCHEDULE(dwc_timer_t *timer, uint32_t time);
#define dwc_timer_schedule DWC_TIMER_SCHEDULE

extern void DWC_TIMER_CANCEL(dwc_timer_t *timer);
#define dwc_timer_cancel DWC_TIMER_CANCEL



struct dwc_spinlock;

typedef struct dwc_spinlock dwc_spinlock_t;

typedef unsigned long dwc_irqflags_t;

extern dwc_spinlock_t *DWC_SPINLOCK_ALLOC(void);
#define dwc_spinlock_alloc(_ctx_) DWC_SPINLOCK_ALLOC()

extern void DWC_SPINLOCK_FREE(dwc_spinlock_t *lock);
#define dwc_spinlock_free(_ctx_,_lock_) DWC_SPINLOCK_FREE(_lock_)

extern void DWC_SPINLOCK_IRQSAVE(dwc_spinlock_t *lock, dwc_irqflags_t *flags);
#define dwc_spinlock_irqsave DWC_SPINLOCK_IRQSAVE

extern void DWC_SPINUNLOCK_IRQRESTORE(dwc_spinlock_t *lock, dwc_irqflags_t flags);
#define dwc_spinunlock_irqrestore DWC_SPINUNLOCK_IRQRESTORE

extern void DWC_SPINLOCK(dwc_spinlock_t *lock);
#define dwc_spinlock DWC_SPINLOCK

extern void DWC_SPINUNLOCK(dwc_spinlock_t *lock);
#define dwc_spinunlock DWC_SPINUNLOCK



struct dwc_mutex;

typedef struct dwc_mutex dwc_mutex_t;

#if defined(DWC_LINUX) && defined(CONFIG_DEBUG_MUTEXES)
#define DWC_MUTEX_ALLOC_LINUX_DEBUG(__mutexp) ({ \
	__mutexp = (dwc_mutex_t *)DWC_ALLOC(sizeof(struct mutex)); \
	mutex_init((struct mutex *)__mutexp); \
})
#endif

extern dwc_mutex_t *DWC_MUTEX_ALLOC(void);
#define dwc_mutex_alloc(_ctx_) DWC_MUTEX_ALLOC()

#if defined(DWC_LINUX) && defined(CONFIG_DEBUG_MUTEXES)
#define DWC_MUTEX_FREE(__mutexp) do { \
	mutex_destroy((struct mutex *)__mutexp); \
	DWC_FREE(__mutexp); \
} while(0)
#else
extern void DWC_MUTEX_FREE(dwc_mutex_t *mutex);
#define dwc_mutex_free(_ctx_,_mutex_) DWC_MUTEX_FREE(_mutex_)
#endif

extern void DWC_MUTEX_LOCK(dwc_mutex_t *mutex);
#define dwc_mutex_lock DWC_MUTEX_LOCK

extern int DWC_MUTEX_TRYLOCK(dwc_mutex_t *mutex);
#define dwc_mutex_trylock DWC_MUTEX_TRYLOCK

extern void DWC_MUTEX_UNLOCK(dwc_mutex_t *mutex);
#define dwc_mutex_unlock DWC_MUTEX_UNLOCK



extern void DWC_UDELAY(uint32_t usecs);
#define dwc_udelay DWC_UDELAY

extern void DWC_MDELAY(uint32_t msecs);
#define dwc_mdelay DWC_MDELAY

extern void DWC_MSLEEP(uint32_t msecs);
#define dwc_msleep DWC_MSLEEP

extern uint32_t DWC_TIME(void);
#define dwc_time DWC_TIME





#ifdef __cplusplus
}
#endif

#endif 
