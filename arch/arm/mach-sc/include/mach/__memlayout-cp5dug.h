/* linux/arch/arm/mach-sc/include/mach/__memlayout-cp5dug.h
 * Copyright (C) 2013 HTC Corporation.
 * Author: Yili Xie <yili_xie@htc.com>
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


#ifndef __MEMLAYOUT_CP5DUG_H__
#define __MEMLAYOUT_CP5DUG_H__



#include <asm/sizes.h>

#define PROJECT_SPECIAL_LAYOUT

#define LINUX_BANK_NUM            3

#define LINUX_BASE_ADDR1		0x80000000
#define LINUX_SIZE_ADDR1		0x07A00000  
#define LINUX_VIRT_ADDR1		(PAGE_OFFSET)

#define TD_BASE_ADDR			0x87A00000
#define TD_SIZE_ADDR			0x01C00000  
#define TD_VIRT_ADDR			(LINUX_VIRT_ADDR1 + LINUX_SIZE_ADDR1)

#define LINUX_BASE_ADDR2		0x89600000
#define LINUX_SIZE_ADDR2		0x06800000  
#define LINUX_VIRT_ADDR2		(TD_VIRT_ADDR + TD_SIZE_ADDR)

#define W_BASE_ADDR				0x8FE00000
#define W_SIZE_ADDR				0x02E00000  
#define W_VIRT_ADDR				(LINUX_VIRT_ADDR2 + LINUX_SIZE_ADDR2)

#define LINUX_BASE_ADDR3		0x92C00000
#define LINUX_SIZE_ADDR3		0x25100000  
#define LINUX_VIRT_ADDR3		(W_VIRT_ADDR + W_SIZE_ADDR)

#define TDS_BASE_ADDR			0x87A00000
#define TDS_SIZE_ADDR			0x02B00000  
#define TDS_VIRT_ADDR			(LINUX_VIRT_ADDR1 + LINUX_SIZE_ADDR1)

#define LINUXS_BASE_ADDR2		0x8A500000
#define LINUXS_SIZE_ADDR2		0x05900000  
#define LINUXS_VIRT_ADDR2		(TDS_VIRT_ADDR + TDS_SIZE_ADDR)

#define WS_BASE_ADDR			0x8FE00000
#define WS_SIZE_ADDR			0x04000000  
#define WS_VIRT_ADDR			(LINUXS_VIRT_ADDR2 + LINUXS_SIZE_ADDR2)

#define LINUXS_BASE_ADDR3		0x93E00000
#define LINUXS_SIZE_ADDR3		0x23F00000  
#define LINUXS_VIRT_ADDR3		(WS_VIRT_ADDR + WS_SIZE_ADDR)

#define ION_BASE_ADDR			0xB7D00000
#define ION_SIZE_ADDR			0x07C00000  
#define ION_VIRT_ADDR			(LINUX_VIRT_ADDR3 + LINUX_SIZE_ADDR3)

#define FB_BASE_ADDR			0xBF900000
#define FB_SIZE_ADDR			0x00600000  
#define FB_VIRT_ADDR			(ION_VIRT_ADDR + ION_SIZE_ADDR)

#define RC_BASE_ADDR			0xBFF00000
#define RC_SIZE_ADDR			0x00100000  
#define RC_VIRT_ADDR			(FB_VIRT_ADDR + FB_SIZE_ADDR)

#define ELF_HEADER_ADDR			0xBF400000
#define ELF_HEADER_SIZE			0x00100000  

#ifdef CONFIG_ION
#define SPRD_ION_OVERLAY_SIZE		(CONFIG_SPRD_ION_OVERLAY_SIZE * SZ_1M)
#define SPRD_ION_SIZE				(ION_SIZE_ADDR - SPRD_ION_OVERLAY_SIZE)
#else
#define SPRD_ION_OVERLAY_SIZE		(0 * SZ_1M)
#define SPRD_ION_SIZE			(0 * SZ_1M)
#endif

#define SPRD_IO_MEM_SIZE		(ION_SIZE_ADDR)
#define SPRD_IO_MEM_BASE		(ION_BASE_ADDR)

#define SPRD_ION_BASE		(ION_BASE_ADDR)
#define SPRD_ION_OVERLAY_BASE		(ION_BASE_ADDR + SPRD_ION_SIZE)

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define SPRD_RAM_CONSOLE_SIZE		(RC_SIZE_ADDR)
#define SPRD_RAM_CONSOLE_START		(RC_BASE_ADDR)
#endif

#ifdef CONFIG_HTC_DBG_UNCACHE_FTRACE
#define FT_BASE_PHY (246 * SZ_1M + 0x80000000)
#define FT_SIZE_PHY (8 * SZ_1M)
#endif

#endif
