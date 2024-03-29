/* * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ADI_H__
#define __ADI_H__

int sci_adi_init(void);

int sci_adi_read(u32 reg);

int sci_is_adi_vaddr(u32 vaddr);

int sci_adi_p2v(u32 paddr, u32 *vaddr);


int sci_adi_write_fast(u32 reg, u16 val, u32 sync);
int sci_adi_write(u32 reg, u16 or_val, u16 clear_msk);

static inline int sci_adi_raw_write(u32 reg, u16 val)
{
	return sci_adi_write_fast(reg, val, 1);
}

static inline int sci_adi_set(u32 reg, u16 bits)
{
	return sci_adi_write(reg, bits, 0);
}

static inline int sci_adi_clr(u32 reg, u16 bits)
{
	return sci_adi_write(reg, 0, bits);
}

int adi_xtl_get_analog(void);
int adi_xtl_put_analog(void);
int adi_xtl_get_digital(void);
int adi_xtl_put_digital(void);

#endif
