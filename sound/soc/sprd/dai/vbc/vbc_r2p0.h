/*
 * sound/soc/sprd/dai/vbc/vbc_r2p0.h
 *
 * SPRD SoC CPU-DAI -- SpreadTrum SOC DAI with EQ&ALC and some loop.
 *
 * Copyright (C) 2012 SpreadTrum Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY ork FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __VBC_R2P0H
#define __VBC_R2P0H

#include <mach/sprd-audio.h>

#define VBC_VERSION	"vbc.r2p0"

#define VBC_EQ_FIRMWARE_MAGIC_LEN	(4)
#define VBC_EQ_FIRMWARE_MAGIC_ID	("VBEQ")
#define VBC_EQ_PROFILE_VERSION		(0x00000002)
#define VBC_EQ_PROFILE_CNT_MAX		(100)
#define VBC_EQ_PROFILE_NAME_MAX		(32)
#define VBC_DA_EFFECT_PARAS_LEN              (20+72*2)
#define VBC_AD_EFFECT_PARAS_LEN              (2+ 43*2)

enum {
	VBISADCK_INV = 9,
	VBISDACK_INV,
	VBLSB_EB,
	VBIIS_DLOOP = 13,
	VBPCM_MODE,
	VBIIS_LRCK,
};
enum {
	RAMSW_NUMB = 9,
	RAMSW_EN,
	VBAD0DMA_EN,
	VBAD1DMA_EN,
	VBDA0DMA_EN,
	VBDA1DMA_EN,
	VBENABLE,
};
enum {
	DAPATH_NO_MIX = 0,
	DAPATH_ADD_FM,
	DAPATH_SUB_FM,
};

enum {
	ST_INPUT_NORMAL = 0,
	ST_INPUT_INVERSE,
	ST_INPUT_ZERO,
};

enum {
	ADC0_DGMUX = 0,
	ADC1_DGMUX,
	ADC2_DGMUX,
	ADC3_DGMUX,
	ADC_DGMUX_MAX
};

enum vbc_chan {
	VBC_CHAN_DA = 0,
	VBC_CHAN_AD01,
	VBC_CHAN_AD23,
	VBC_CHAN_MAX,
};


#define PHYS_VBDA0		(VBC_PHY_BASE + 0x0000)	
#define PHYS_VBDA1		(VBC_PHY_BASE + 0x0004)	
#define PHYS_VBAD0		(VBC_PHY_BASE + 0x0008)	
#define PHYS_VBAD1		(VBC_PHY_BASE + 0x000C)	
#define PHYS_VBAD2		(VBC_PHY_BASE + 0x00C0)	
#define PHYS_VBAD3		(VBC_PHY_BASE + 0x00C4)	

#define VBC_CP2_PHY_BASE		0x02020000
#define CP2_PHYS_VBDA0		(VBC_CP2_PHY_BASE + 0x0000)	
#define CP2_PHYS_VBDA1		(VBC_CP2_PHY_BASE + 0x0004)	

#define ARM_VB_BASE		VBC_BASE
#define VBDA0			(ARM_VB_BASE + 0x0000)	
#define VBDA1			(ARM_VB_BASE + 0x0004)	
#define VBAD0			(ARM_VB_BASE + 0x0008)	
#define VBAD1			(ARM_VB_BASE + 0x000C)	
#define VBBUFFSIZE		(ARM_VB_BASE + 0x0010)	
#define VBADBUFFDTA		(ARM_VB_BASE + 0x0014)	
#define VBDABUFFDTA		(ARM_VB_BASE + 0x0018)	
#define VBADCNT			(ARM_VB_BASE + 0x001C)	
#define VBDACNT			(ARM_VB_BASE + 0x0020)	
#define VBAD23CNT		(ARM_VB_BASE + 0x0024)	
#define VBADDMA			(ARM_VB_BASE + 0x0028)	
#define VBBUFFAD23		(ARM_VB_BASE + 0x002C)	
#define VBINTTYPE		(ARM_VB_BASE + 0x0034)
#define VBDATASWT		(ARM_VB_BASE + 0x0038)
#define VBIISSEL			(ARM_VB_BASE + 0x003C)

#define DAPATCHCTL		(ARM_VB_BASE + 0x0040)
#define DADGCTL			(ARM_VB_BASE + 0x0044)
#define DAHPCTL			(ARM_VB_BASE + 0x0048)
#define DAALCCTL0		(ARM_VB_BASE + 0x004C)
#define DAALCCTL1		(ARM_VB_BASE + 0x0050)
#define DAALCCTL2		(ARM_VB_BASE + 0x0054)
#define DAALCCTL3		(ARM_VB_BASE + 0x0058)
#define DAALCCTL4		(ARM_VB_BASE + 0x005C)
#define DAALCCTL5		(ARM_VB_BASE + 0x0060)
#define DAALCCTL6		(ARM_VB_BASE + 0x0064)
#define DAALCCTL7		(ARM_VB_BASE + 0x0068)
#define DAALCCTL8		(ARM_VB_BASE + 0x006C)
#define DAALCCTL9		(ARM_VB_BASE + 0x0070)
#define DAALCCTL10		(ARM_VB_BASE + 0x0074)
#define STCTL0			(ARM_VB_BASE + 0x0078)
#define STCTL1			(ARM_VB_BASE + 0x007C)
#define ADPATCHCTL		(ARM_VB_BASE + 0x0080)
#define ADDG01CTL		(ARM_VB_BASE + 0x0084)
#define ADDG23CTL		(ARM_VB_BASE + 0x0088)
#define ADHPCTL			(ARM_VB_BASE + 0x008C)
#define ADCSRCCTL		(ARM_VB_BASE + 0x0090)
#define DACSRCCTL		(ARM_VB_BASE + 0x0094)
#define MIXERCTL			(ARM_VB_BASE + 0x0098)
#if 0
#define STFIFOLVL		(ARM_VB_BASE + 0x009C)
#define VBIRQEN			(ARM_VB_BASE + 0x00A0)
#define VBIRQCLR		(ARM_VB_BASE + 0x00A4)
#define VBIRQRAW		(ARM_VB_BASE + 0x00A8)
#define VBIRQSTS			(ARM_VB_BASE + 0x00AC)
#endif
#define VBNGCVTHD		(ARM_VB_BASE + 0x00B0)
#define VBNGCTTHD		(ARM_VB_BASE + 0x00B4)
#define VBNGCTL			(ARM_VB_BASE + 0x00B8)

#define HPCOEF0_H			(ARM_VB_BASE + 0x0100)
#define HPCOEF0_L			(ARM_VB_BASE + 0x0104)
#define HPCOEF42_H			(ARM_VB_BASE + 0x0250)
#define HPCOEF42_L			(ARM_VB_BASE + 0x0254)
#define HPCOEF43_H			(ARM_VB_BASE + 0x0258)
#define HPCOEF43_L			(ARM_VB_BASE + 0x025C)
#define HPCOEF71_H			(ARM_VB_BASE + 0x0338)
#define HPCOEF71_L			(ARM_VB_BASE + 0x033C)

#define AD01_HPCOEF0_H			(ARM_VB_BASE + 0x0400)
#define AD01_HPCOEF0_L			(ARM_VB_BASE + 0x0404)
#define AD01_HPCOEF42_H			(ARM_VB_BASE + 0x0550)
#define AD01_HPCOEF42_L			(ARM_VB_BASE + 0x0554)
#define AD23_HPCOEF0_H			(ARM_VB_BASE + 0x0600)
#define AD23_HPCOEF0_L			(ARM_VB_BASE + 0x0604)
#define AD23_HPCOEF42_H			(ARM_VB_BASE + 0x0750)
#define AD23_HPCOEF42_L			(ARM_VB_BASE + 0x0754)

#define ARM_VB_END		(ARM_VB_BASE + 0x0754)

#define VBADBUFFERSIZE_SHIFT	(0)
#define VBADBUFFERSIZE_MASK	(0xFF<<VBADBUFFERSIZE_SHIFT)
#define VBDABUFFERSIZE_SHIFT	(8)
#define VBDABUFFERSIZE_MASK	(0xFF<<VBDABUFFERSIZE_SHIFT)
#define VBAD23BUFFERSIZE_SHIFT	(0)
#define VBAD23BUFFERSIZE_MASK	(0xFF<<VBAD23BUFFERSIZE_SHIFT)

#define VBCHNEN			(ARM_VB_BASE + 0x00C8)
#define VBDA0_EN			(0)
#define VBDA1_EN			(1)
#define VBAD0_EN			(2)
#define VBAD1_EN			(3)
#define VBAD2_EN			(4)
#define VBAD3_EN			(5)

#define VBDACHEN_SHIFT  	(0)
#define VBADCHEN_SHIFT  	(2)
#define VBAD23CHEN_SHIFT  	(4)

#define VBAD2DMA_EN 	(0)
#define VBAD3DMA_EN	(1)

 
#define VBIISSEL_AD01_PORT_SHIFT	(0)
#define VBIISSEL_AD01_PORT_MASK	(0x7)
#define VBIISSEL_AD23_PORT_SHIFT	(3)
#define VBIISSEL_AD23_PORT_MASK	(0x7<<VBIISSEL_AD23_PORT_SHIFT)
#define VBIISSEL_DA_PORT_SHIFT	(6)
#define VBIISSEL_DA_PORT_MASK	(0x7<<VBIISSEL_DA_PORT_SHIFT)
#define VBIISSEL_DA_LRCK	(14)
#define VBIISSEL_AD01_LRCK	(13)
#define VBDATASWT_AD23_LRCK	(6)
     
#define VBDAPATH_DA0_ADDFM_SHIFT	(0)
#define VBDAPATH_DA0_ADDFM_MASK	(0x3<<VBDAPATH_DA0_ADDFM_SHIFT)
#define VBDAPATH_DA1_ADDFM_SHIFT	(2)
#define VBDAPATH_DA1_ADDFM_MASK	(0x3<<VBDAPATH_DA1_ADDFM_SHIFT)
     
#define VBADPATH_AD0_INMUX_SHIFT    (0)
#define VBADPATH_AD0_INMUX_MASK    (0x3<<VBADPATH_AD0_INMUX_SHIFT)
#define VBADPATH_AD1_INMUX_SHIFT    (2)
#define VBADPATH_AD1_INMUX_MASK    (0x3<<VBADPATH_AD1_INMUX_SHIFT)
#define VBADPATH_AD2_INMUX_SHIFT    (4)
#define VBADPATH_AD2_INMUX_MASK    (0x3<<VBADPATH_AD2_INMUX_SHIFT)
#define VBADPATH_AD3_INMUX_SHIFT    (6)
#define VBADPATH_AD3_INMUX_MASK    (0x3<<VBADPATH_AD3_INMUX_SHIFT)
#define VBADPATH_AD0_DGMUX_SHIFT    (8)
#define VBADPATH_AD0_DGMUX_MASK    (0x1<<VBADPATH_AD0_DGMUX_SHIFT)
#define VBADPATH_AD1_DGMUX_SHIFT    (9)
#define VBADPATH_AD1_DGMUX_MASK    (0x1<<VBADPATH_AD1_DGMUX_SHIFT)
#define VBADPATH_AD2_DGMUX_SHIFT    (10)
#define VBADPATH_AD2_DGMUX_MASK    (0x1<<VBADPATH_AD2_DGMUX_SHIFT)
#define VBADPATH_AD3_DGMUX_SHIFT    (11)
#define VBADPATH_AD3_DGMUX_MASK    (0x1<<VBADPATH_AD3_DGMUX_SHIFT)
#define VBADPATH_ST0_INMUX_SHIFT	(12)
#define VBADPATH_ST0_INMUX_MASK	(0x3<<VBADPATH_ST0_INMUX_SHIFT)
#define VBADPATH_ST1_INMUX_SHIFT	(14)
#define VBADPATH_ST1_INMUX_MASK	(0x3<<VBADPATH_ST1_INMUX_SHIFT)
     
#define 	VBDACSRC_EN		(0)
#define 	VBDACSRC_CLR		(1)
#define 	VBDACSRC_F1F2F3_BP	(3)
#define 	VBDACSRC_F1_SEL		(4)
#define	VBDACSRC_F0_BP		(5)
#define 	VBDACSRC_F0_SEL		(6)
     
#define 	VBADCSRC_EN_01		(0)
#define 	VBADCSRC_CLR_01		(1)
#define 	VBADCSRC_PAUSE_01	(2)
#define 	VBADCSRC_F1F2F3_BP_01	(3)
#define 	VBADCSRC_F1_SEL_01	(4)
#define 	VBADCSRC_EN_23		(8)
#define 	VBADCSRC_CLR_23		(9)
#define 	VBADCSRC_PAUSE_23	(10)
#define 	VBADCSRC_F1F2F3_BP_23	(11)
#define 	VBADCSRC_F1_SEL_23	(12)
    
#define     VBST_EN_0			(12)
#define 	 VBST_HPF_0		(11)
    
#define 	 VBST_HPF_1		(11)
#define     VBST_EN_1	                  (12)
#define 	VBST0_SEL_CHN		(13)
#define 	VBST1_SEL_CHN		(14)
#endif 
