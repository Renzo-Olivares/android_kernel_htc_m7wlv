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
#ifndef	_SPRD_VSP_H
#define _SPRD_VSP_H

#include <linux/ioctl.h>

#if defined(CONFIG_ARCH_SCX35)
#define SPRD_VSP_MAP_SIZE 0xA000

#define SPRD_VSP_IOCTL_MAGIC 'm'
#define VSP_CONFIG_FREQ _IOW(SPRD_VSP_IOCTL_MAGIC, 1, unsigned int)
#define VSP_GET_FREQ    _IOR(SPRD_VSP_IOCTL_MAGIC, 2, unsigned int)
#define VSP_ENABLE      _IO(SPRD_VSP_IOCTL_MAGIC, 3)
#define VSP_DISABLE     _IO(SPRD_VSP_IOCTL_MAGIC, 4)
#define VSP_ACQUAIRE    _IO(SPRD_VSP_IOCTL_MAGIC, 5)
#define VSP_RELEASE     _IO(SPRD_VSP_IOCTL_MAGIC, 6)
#define VSP_COMPLETE       _IO(SPRD_VSP_IOCTL_MAGIC, 7)
#define VSP_RESET       _IO(SPRD_VSP_IOCTL_MAGIC, 8)
#define VSP_HW_INFO     _IO(SPRD_VSP_IOCTL_MAGIC, 9)

enum sprd_vsp_frequency_e{ 
	VSP_FREQENCY_LEVEL_0 = 0,
	VSP_FREQENCY_LEVEL_1 = 1,
	VSP_FREQENCY_LEVEL_2 = 2,
	VSP_FREQENCY_LEVEL_3 = 3
};


#else	
#define SPRD_VSP_MAP_SIZE 0x13000

#define SPRD_VSP_IOCTL_MAGIC 'm'
#define VSP_CONFIG_FREQ _IOW(SPRD_VSP_IOCTL_MAGIC, 1, unsigned int)
#define VSP_GET_FREQ    _IOR(SPRD_VSP_IOCTL_MAGIC, 2, unsigned int)
#define VSP_ENABLE      _IO(SPRD_VSP_IOCTL_MAGIC, 3)
#define VSP_DISABLE     _IO(SPRD_VSP_IOCTL_MAGIC, 4)
#define VSP_ACQUAIRE    _IO(SPRD_VSP_IOCTL_MAGIC, 5)
#define VSP_RELEASE     _IO(SPRD_VSP_IOCTL_MAGIC, 6)
#define VSP_START       _IO(SPRD_VSP_IOCTL_MAGIC, 7)
#define VSP_RESET       _IO(SPRD_VSP_IOCTL_MAGIC, 8)
#define VSP_REG_IRQ       _IO(SPRD_VSP_IOCTL_MAGIC,9)
#define VSP_UNREG_IRQ       _IO(SPRD_VSP_IOCTL_MAGIC,10)
#define VSP_ACQUAIRE_MEA_DONE _IO(SPRD_VSP_IOCTL_MAGIC, 11)
#define VSP_ACQUAIRE_MP4ENC_DONE _IO(SPRD_VSP_IOCTL_MAGIC, 12)
enum sprd_vsp_frequency_e{ 
	VSP_FREQENCY_LEVEL_0 = 0,
	VSP_FREQENCY_LEVEL_1 = 1,
	VSP_FREQENCY_LEVEL_2 = 2,
	VSP_FREQENCY_LEVEL_3 = 3
};


#endif

#endif

