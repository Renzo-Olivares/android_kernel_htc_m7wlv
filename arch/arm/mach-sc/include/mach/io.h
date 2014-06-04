/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H


#define IO_SPACE_LIMIT		0xffffffff

#define __io(a)			((void __iomem *)(a))
#define __mem_pci(a)		(a)

#ifdef CONFIG_HTC_DBG_LAST_IO
extern void htc_debug_last_io_save(void *pc, void __iomem *vaddr);
extern void htc_debug_last_io_done(void __iomem *vaddr);

static inline void __iomem *__save_io_addr(void __iomem *p)
{
	void *pc;

	__asm__("mov %0, r15" : "=r" (pc));
	htc_debug_last_io_save(pc, p);
	return p;
}

static inline void __save_io_flag(void __iomem *p)
{
	htc_debug_last_io_done(p);
}

#define __mem_sio(a)		__save_io_addr((void __iomem*)(a))
#define __io_done(a)		__save_io_flag((void __iomem*)(a))
#endif

#endif
