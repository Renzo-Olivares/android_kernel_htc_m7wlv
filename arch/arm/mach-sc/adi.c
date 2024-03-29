/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 * Copyright (C) 2012 steve zhan
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/irqflags.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/hwspinlock.h>
#include <asm/delay.h>
#include <linux/spinlock.h>


#include <mach/hardware.h>
#include <mach/globalregs.h>
#include <mach/adi.h>
#include <mach/irqs.h>
#include <mach/sci.h>
#include <mach/sci_glb_regs.h>
#include <mach/arch_lock.h>

#define CTL_ADI_BASE			( SPRD_ADI_BASE )

#define REG_ADI_CTRL0					(CTL_ADI_BASE + 0x04)
#define REG_ADI_CHNL_PRI				(CTL_ADI_BASE + 0x08)
#define REG_ADI_INT_RAW					(CTL_ADI_BASE + 0x10)
#define REG_ADI_RD_CMD					(CTL_ADI_BASE + 0x24)
#define REG_ADI_RD_DATA					(CTL_ADI_BASE + 0x28)
#define REG_ADI_FIFO_STS				(CTL_ADI_BASE + 0x2c)

#define BIT_ARM_SCLK_EN                 ( BIT(1) )
#define BITS_CMMB_WR_PRI			( (1) << 4 & (BIT(4)|BIT(5)) )

#define BITS_PD_WR_PRI             ( (1) << 14 & (BIT(14)|BIT(15)) )
#define BITS_RFT_WR_PRI       	   ( (1) << 12 & (BIT(12)|BIT(13)) )
#define BITS_DSP_RD_PRI            ( (2) << 10 & (BIT(10)|BIT(11)) )
#define BITS_DSP_WR_PRI            ( (2) << 8 & (BIT(8)|BIT(9)) )
#define BITS_ARM_RD_PRI            ( (3) << 6 & (BIT(6)|BIT(7)) )
#define BITS_ARM_WR_PRI            ( (3) << 4 & (BIT(4)|BIT(5)) )
#define BITS_STC_WR_PRI            ( (1) << 2 & (BIT(2)|BIT(3)) )
#define BITS_INT_STEAL_PRI         ( (3) << 0 & (BIT(0)|BIT(1)) )

#define BIT_RD_CMD_BUSY                 ( BIT(31) )
#define SHIFT_RD_ADDR                   ( 16 )

#define SHIFT_RD_VALU                   ( 0 )
#define MASK_RD_VALU                    ( 0xFFFF )

#define BIT_FIFO_FULL                   ( BIT(11) )
#define FIFO_IS_FULL()	(__raw_readl(REG_ADI_FIFO_STS) & BIT_FIFO_FULL)

#define BIT_FIFO_EMPTY                  ( BIT(10) )

#define BIT_ADI_WR(_X_)                 ( (_X_) << 2 )
#define BITS_ADDR_BYTE_SEL(_X_)			( (_X_) << 0 & (BIT(0)|BIT(1)) )

#define VALUE_CH_PRI	(0x0)

#define REG_ADI_GSSI_CFG0					(CTL_ADI_BASE + 0x1C)
#define REG_ADI_GSSI_CFG1					(CTL_ADI_BASE + 0x20)


static u32 readback_addr_mak __read_mostly = 0;


struct adi_xtl_analog{
	unsigned int refcnt;
	spinlock_t lock;
} adi_xtl_analog;

struct adi_xtl_digital {
	unsigned int refcnt;
	spinlock_t lock;
} adi_xtl_digital;



static inline int __adi_ver(void)
{
#ifdef	CONFIG_ARCH_SC8825
	return 0;
#else
	return 1;
#endif
}

#define	TO_ADDR(_x_)		( ((_x_) >> SHIFT_RD_ADDR) & readback_addr_mak )

static inline int __adi_fifo_drain(void)
{
	int cnt = 1000;
	while (!(__raw_readl(REG_ADI_FIFO_STS) & BIT_FIFO_EMPTY) && cnt--) {
		udelay(1);
	}
	WARN(cnt == 0, "ADI WAIT timeout!!!");
	return 0;
}

#if defined(CONFIG_ARCH_SCX35)
#define ANA_VIRT_BASE			( SPRD_ADISLAVE_BASE )
#define ANA_PHYS_BASE			( SPRD_ADISLAVE_PHYS )
#else
#define ANA_VIRT_BASE			( SPRD_MISC_BASE )
#define ANA_PHYS_BASE			( SPRD_MISC_PHYS )
#endif

#define ANA_ADDR_SIZE			(SZ_4K)

int sci_is_adi_vaddr(u32 vaddr)
{
	return (vaddr >= ANA_VIRT_BASE && vaddr <= (ANA_VIRT_BASE + ANA_ADDR_SIZE));
}
EXPORT_SYMBOL(sci_is_adi_vaddr);

int sci_adi_p2v(u32 paddr, u32 *vaddr)
{
	if(paddr < ANA_PHYS_BASE || paddr > (ANA_PHYS_BASE + ANA_ADDR_SIZE)) {
		return -1;
	} else {
		*vaddr = paddr - ANA_PHYS_BASE + ANA_VIRT_BASE;
	}
	return 0;
}
EXPORT_SYMBOL(sci_adi_p2v);

static inline int __adi_addr_check(u32 vaddr)
{
	if(!sci_is_adi_vaddr(vaddr)) {
		WARN(1, "Maybe ADI vaddr is wrong?!!");
		return -1;
	}
	return 0;
}

static inline u32 __adi_translate_addr(u32 regvddr)
{
	regvddr = regvddr - ANA_VIRT_BASE + ANA_PHYS_BASE;
	return regvddr;
}

static inline int __adi_read(u32 regPddr, unsigned int *v)
{
	unsigned long val;
	int cnt = 2000;

	__raw_writel(regPddr, REG_ADI_RD_CMD);

	do {
		val = __raw_readl(REG_ADI_RD_DATA);
	} while ((val & BIT_RD_CMD_BUSY) && cnt--);

	WARN(cnt == 0, "ADI READ timeout!!!");
	
	if ((!v) || TO_ADDR(val) != (regPddr & readback_addr_mak)) {
		printk("val = 0x%lx, regPaddr = 0x%x, readback_addr_mak = 0x%x\n",val,regPddr, readback_addr_mak);
		return -1;
	}

	*v = val & MASK_RD_VALU;
	return 0;
}

int sci_adi_read(u32 reg)
{
	int val = 0;
	if (!__adi_addr_check(reg)) {
		unsigned long flags;
		int ret = 0;
		reg = __adi_translate_addr(reg);
		__arch_default_lock(HWLOCK_ADI, &flags);
		ret = __adi_read(reg, &val);
		__arch_default_unlock(HWLOCK_ADI, &flags);
		if (ret) {
			printk("read error: reg = 0x%x\n",reg);
			BUG_ON(1);
		}
	}
	return val;
}

EXPORT_SYMBOL(sci_adi_read);

#define CACHE_SIZE	(16)

static struct __data {
	u32 reg;
	u16 val;
} __data_array[CACHE_SIZE];

static struct __data *head_p = &__data_array[0];
static struct __data *tail_p = &__data_array[0];
static u32 data_in_cache = 0;

#define HEAD_ADD	(1)
#define TAIL_ADD	(0)
static inline void __p_add(struct __data **p, u32 isHead)
{
	if (++(*p) > &__data_array[CACHE_SIZE - 1])
		(*p) = &__data_array[0];
	if (head_p == tail_p) {
		if (isHead == HEAD_ADD) {
			data_in_cache = 0;
		} else {
			data_in_cache = CACHE_SIZE;
		}
	} else {
		data_in_cache = 2;
	}
}

static inline int __adi_write(u32 reg, u16 val, u32 sync)
{
	tail_p->reg = reg;
	tail_p->val = val;
	__p_add(&tail_p, TAIL_ADD);
	while (!FIFO_IS_FULL() && (data_in_cache != 0)) {
		__raw_writel(head_p->val, head_p->reg);
		__p_add(&head_p, HEAD_ADD);
	}

	if (sync || data_in_cache == CACHE_SIZE) {
		__adi_fifo_drain();
		while (data_in_cache != 0) {
			while (FIFO_IS_FULL()) {
				cpu_relax();
			}
			__raw_writel(head_p->val, head_p->reg);
			__p_add(&head_p, HEAD_ADD);
		}
		__adi_fifo_drain();
	}

	return 0;
}

int sci_adi_write_fast(u32 reg, u16 val, u32 sync)
{
	if (!__adi_addr_check(reg)) {
		unsigned long flags;
		__arch_default_lock(HWLOCK_ADI, &flags);
		__adi_write(reg, val, sync);
		__arch_default_unlock(HWLOCK_ADI, &flags);
	}
	return 0;
}

EXPORT_SYMBOL(sci_adi_write_fast);

int sci_adi_write(u32 reg, u16 or_val, u16 clear_msk)
{
	if (!__adi_addr_check(reg)) {
		unsigned long flags;
		int ret = 0, val = 0;
		__arch_default_lock(HWLOCK_ADI, &flags);
		ret = __adi_read(__adi_translate_addr(reg), &val);
		if (!ret)
			__adi_write(reg, (val & ~clear_msk) | or_val, 1);
		__arch_default_unlock(HWLOCK_ADI, &flags);
		if (ret)
			BUG_ON(1);
	}
	return 0;
}

EXPORT_SYMBOL(sci_adi_write);

static void __init __adi_init(void)
{
	uint32_t value;
	value = __raw_readl(REG_ADI_CTRL0);

	if (__adi_ver() == 0) {
		value &= ~BIT_ARM_SCLK_EN;
		value |= BITS_CMMB_WR_PRI;
		__raw_writel(value, REG_ADI_CTRL0);

		value = __raw_readl(REG_ADI_CHNL_PRI);
		value |= BITS_PD_WR_PRI | BITS_RFT_WR_PRI |
		    BITS_DSP_RD_PRI | BITS_DSP_WR_PRI |
		    BITS_ARM_RD_PRI | BITS_ARM_WR_PRI |
		    BITS_STC_WR_PRI | BITS_INT_STEAL_PRI;
		__raw_writel(value, REG_ADI_CHNL_PRI);

		readback_addr_mak = 0x7ff;
	} else if (__adi_ver() == 1) {
		if (value)
			WARN_ON(1);

		value = VALUE_CH_PRI;
		__raw_writel(value, REG_ADI_CHNL_PRI);

		value = __raw_readl(REG_ADI_GSSI_CFG0);
		readback_addr_mak = (value & 0x3f) - ((value >> 11) & 0x1f) - 1;
		readback_addr_mak = (1<<(readback_addr_mak + 2)) - 1;
	}
}

int __init sci_adi_init(void)
{
#if defined(CONFIG_ARCH_SC8825)
	
	sci_glb_set(REG_GLB_GEN0, BIT_ADI_EB);
	
	sci_glb_set(REG_GLB_SOFT_RST, BIT_ADI_RST);
	udelay(2);
	sci_glb_clr(REG_GLB_SOFT_RST, BIT_ADI_RST);

#elif defined(CONFIG_ARCH_SCX35)
	
	sci_glb_set(REG_AON_APB_APB_EB0, BIT_ADI_EB);
	
	sci_glb_set(REG_AON_APB_APB_RST0, BIT_ADI_SOFT_RST);
	udelay(2);
	sci_glb_clr(REG_AON_APB_APB_RST0, BIT_ADI_SOFT_RST);
#endif
	__adi_init();

	spin_lock_init(&adi_xtl_analog.lock);
	spin_lock_init(&adi_xtl_digital.lock);

	return 0;
}


int adi_xtl_get_analog(void)
{
	unsigned long flags;

	spin_lock_irqsave(&adi_xtl_analog.lock, flags);
	if (!adi_xtl_analog.refcnt++) {
		
		sci_adi_write(ANA_REG_GLB_XTL_WAIT_CTRL, BIT_XTL_EN, BIT_XTL_EN);
	}
	spin_unlock_irqrestore(&adi_xtl_analog.lock, flags);

	return adi_xtl_analog.refcnt;
}
EXPORT_SYMBOL_GPL(adi_xtl_get_analog);

int adi_xtl_get_digital(void)
{
	unsigned long flags;

	spin_lock_irqsave(&adi_xtl_digital.lock, flags);
	if (!adi_xtl_digital.refcnt++) {
		
		sci_glb_write(REG_AON_APB_SINDRV_CTRL,  (BIT_SINDRV_ENA |BIT_SINDRV_ENA_SQUARE), (BIT_SINDRV_ENA |BIT_SINDRV_ENA_SQUARE));
	}
	spin_unlock_irqrestore(&adi_xtl_digital.lock, flags);

	return adi_xtl_digital.refcnt;
}
EXPORT_SYMBOL_GPL(adi_xtl_get_digital);

int adi_xtl_put_analog(void)
{
	unsigned long flags;

	spin_lock_irqsave(&adi_xtl_analog.lock, flags);
	if (adi_xtl_analog.refcnt && !--adi_xtl_analog.refcnt) {
		
		sci_adi_write(ANA_REG_GLB_XTL_WAIT_CTRL, 0, BIT_XTL_EN);
	}
	spin_unlock_irqrestore(&adi_xtl_analog.lock, flags);

	return adi_xtl_analog.refcnt;
}
EXPORT_SYMBOL_GPL(adi_xtl_put_analog);

int adi_xtl_put_digital(void)
{
	unsigned long flags;

	spin_lock_irqsave(&adi_xtl_digital.lock, flags);
	if (adi_xtl_digital.refcnt && !--adi_xtl_digital.refcnt) {
		
		sci_glb_write(REG_AON_APB_SINDRV_CTRL,  0,  (BIT_SINDRV_ENA |BIT_SINDRV_ENA_SQUARE));
	}
	spin_unlock_irqrestore(&adi_xtl_digital.lock, flags);

	return adi_xtl_digital.refcnt;
}
EXPORT_SYMBOL_GPL(adi_xtl_put_digital);
