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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <../../../drivers/staging/android/timed_output.h>
#include <linux/sched.h>
#include <mach/hardware.h>
#include <mach/adi.h>
#include <mach/adc.h>
#include <mach/sci_glb_regs.h>

#define SPRD_ANA_BASE           (SPRD_MISC_BASE + 0x600)
#define ANA_REG_BASE            (SPRD_ANA_BASE)
#define ANA_VIBR_WR_PROT        (ANA_REG_BASE + 0x90)
#define ANA_VIBRATOR_CTRL0      (ANA_REG_BASE + 0x74)
#define ANA_VIBRATOR_CTRL1      (ANA_REG_BASE + 0x78)

#if defined(CONFIG_ARCH_SC8825)
#define VIBRATOR_REG_UNLOCK     (0xA1B2)
#elif defined(CONFIG_ARCH_SCX35)
#define VIBRATOR_REG_UNLOCK (0x1A2B)
#endif
#define VIBRATOR_REG_LOCK       ((~VIBRATOR_REG_UNLOCK) & 0xffff)
#define VIBRATOR_STABLE_LEVEL   (8)
#define VIBRATOR_INIT_LEVEL     (11)
#define VIBRATOR_INIT_STATE_CNT (10)

#define VIBR_STABLE_V_SHIFT     (12)
#define VIBR_STABLE_V_MSK       (0x0f << VIBR_STABLE_V_SHIFT)
#define VIBR_INIT_V_SHIFT       (8)
#define VIBR_INIT_V_MSK         (0x0f << VIBR_INIT_V_SHIFT)
#define VIBR_V_BP_SHIFT         (4)
#define VIBR_V_BP_MSK           (0x0f << VIBR_V_BP_SHIFT)
#define VIBR_PD_RST             (1 << 3)
#define VIBR_PD_SET             (1 << 2)
#define VIBR_BP_EN              (1 << 1)
#define VIBR_RTC_EN             (1 << 0)

static struct hrtimer vibe_timer;


#if defined(CONFIG_ARCH_SC8825)
static void set_vibrator(int on)
{

	
	sci_adi_write(ANA_VIBR_WR_PROT, VIBRATOR_REG_UNLOCK, 0xffff);
	sci_adi_clr(ANA_VIBRATOR_CTRL0, VIBR_PD_SET | VIBR_PD_RST);
	if (on)
		sci_adi_set(ANA_VIBRATOR_CTRL0, VIBR_PD_RST);
	else
		sci_adi_set(ANA_VIBRATOR_CTRL0, VIBR_PD_SET);
	
	sci_adi_write(ANA_VIBR_WR_PROT, VIBRATOR_REG_LOCK, 0xffff);
}
#elif defined(CONFIG_ARCH_SCX35)
static void set_vibrator(int on)
{
	

	sci_adi_write(ANA_REG_GLB_VIBR_WR_PROT_VALUE, VIBRATOR_REG_UNLOCK, 0xffff); 
	if(on == 0)
	{
		sci_adi_clr (ANA_REG_GLB_VIBR_CTRL0, BIT_VIBR_PON);
	}
	else
	{
		sci_adi_set (ANA_REG_GLB_VIBR_CTRL0, BIT_VIBR_PON);
	}
	sci_adi_write(ANA_REG_GLB_VIBR_WR_PROT_VALUE, VIBRATOR_REG_LOCK, 0xffff);   
}
#endif

#if defined(CONFIG_ARCH_SC8825)
static void vibrator_hw_init(void)
{
	sci_adi_write(ANA_VIBR_WR_PROT, VIBRATOR_REG_UNLOCK, 0xffff);
	sci_adi_set(ANA_VIBRATOR_CTRL0, VIBR_RTC_EN);
	sci_adi_clr(ANA_VIBRATOR_CTRL0, VIBR_BP_EN);
	
	sci_adi_write(ANA_VIBRATOR_CTRL0,
		      (VIBRATOR_INIT_LEVEL << VIBR_INIT_V_SHIFT),
		      VIBR_INIT_V_MSK);
	sci_adi_write(ANA_VIBRATOR_CTRL0,
		      (VIBRATOR_STABLE_LEVEL << VIBR_STABLE_V_SHIFT),
		      VIBR_STABLE_V_MSK);
	
	sci_adi_write(ANA_VIBRATOR_CTRL1, VIBRATOR_INIT_STATE_CNT, 0xffff);
	sci_adi_write(ANA_VIBR_WR_PROT, VIBRATOR_REG_LOCK, 0xffff);
}
#elif defined(CONFIG_ARCH_SCX35)
static void vibrator_hw_init(void)
{
	sci_adi_write(ANA_REG_GLB_VIBR_WR_PROT_VALUE, VIBRATOR_REG_UNLOCK, 0xffff); 
	sci_adi_set(ANA_REG_GLB_RTC_CLK_EN, BIT_RTC_VIBR_EN);
	
	
	sci_adi_write(ANA_REG_GLB_VIBR_CTRL0, (VIBRATOR_INIT_LEVEL << VIBR_INIT_V_SHIFT), VIBR_INIT_V_MSK); 
	sci_adi_write(ANA_REG_GLB_VIBR_CTRL0, (VIBRATOR_STABLE_LEVEL << VIBR_STABLE_V_SHIFT), VIBR_STABLE_V_MSK); 
	sci_adi_write(ANA_REG_GLB_VIBR_CTRL1, VIBRATOR_INIT_STATE_CNT, 0xffff);   
	sci_adi_write(ANA_REG_GLB_VIBR_WR_PROT_VALUE, VIBRATOR_REG_LOCK, 0xffff);   
}
#endif
static void vibrator_enable(struct timed_output_dev *dev, int value)
{

	hrtimer_cancel(&vibe_timer);

	if (value == 0){
		
		
		set_vibrator(0);
	}else {
		value = (value > 15000) ? 15000 : value;
		
		
		set_vibrator(1);
		hrtimer_start(&vibe_timer,
			      ktime_set(value / 1000, (value % 1000) * 1000000),
			      HRTIMER_MODE_REL);
	}
	printk(KERN_INFO "[VIB] Start vibrating for %d ms:%s(parent:%s), tgid=%d\n",
		value, current->comm, current->parent->comm, current->tgid);
	
	
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	ktime_t re;

	if (hrtimer_active(&vibe_timer)) {
		re = hrtimer_get_remaining(&vibe_timer);
		return ktime_to_ns(re);
	} else
		return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	
	
	
	set_vibrator(0);

	return HRTIMER_NORESTART;
}

static struct timed_output_dev sprd_vibrator = {
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

static int __init sprd_init_vibrator(void)
{
	vibrator_hw_init();
	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	timed_output_dev_register(&sprd_vibrator);

	return 0;
}

module_init(sprd_init_vibrator);
MODULE_DESCRIPTION("vibrator driver for spreadtrum Processors");
MODULE_LICENSE("GPL");
