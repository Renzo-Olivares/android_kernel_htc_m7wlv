/*
 * Copyright (C) 2010, 2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef __UMP_KERNEL_PLATFORM_H__
#define __UMP_KERNEL_PLATFORM_H__



#if defined(_WIN32)

#if defined(UMP_BUILDING_UMP_LIBRARY)
#define UMP_KERNEL_API_EXPORT __declspec(dllexport)
#else
#define UMP_KERNEL_API_EXPORT __declspec(dllimport)
#endif

#else

#define UMP_KERNEL_API_EXPORT

#endif


 


#endif 
