/*
 * sound/soc/sprd/dai/sprd-pcm.h
 *
 * SpreadTrum DMA for the pcm stream.
 *
 * Copyright (C) 2012 SpreadTrum Ltd.
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
#ifndef __SPRD_PCM_H
#define __SPRD_PCM_H

#include <mach/dma.h>

#define VBC_PCM_FORMATS			SNDRV_PCM_FMTBIT_S16_LE
#define VBC_FIFO_FRAME_NUM	  	160
#define VBC_BUFFER_BYTES_MAX 		(128 * 1024)

#define I2S_BUFFER_BYTES_MAX 		(128 * 1024) 

#define AUDIO_BUFFER_BYTES_MAX 		(VBC_BUFFER_BYTES_MAX + I2S_BUFFER_BYTES_MAX)

#ifdef DMA_VER_R1P0
struct sprd_pcm_dma_params {
	char *name;		
	int channels[2];	
	int workmode;		
	int irq_type;		
	struct sprd_dma_channel_desc desc;	
	u32 dev_paddr[2];	
};

typedef struct sprd_dma_desc {
	volatile u32 cfg;
	volatile u32 tlen;	
	volatile u32 dsrc;	
	volatile u32 ddst;	
	volatile u32 llptr;	
	volatile u32 pmod;	
	volatile u32 sbm;	
	volatile u32 dbm;	
} sprd_dma_desc;

static inline int sprd_pcm_dma_get_addr(int dma_ch,
					struct snd_pcm_substream *substream)
{
	int offset;
	offset = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ?
	    DMA_CH_SRC_ADDR : DMA_CH_DEST_ADDR;
	return dma_get_reg(DMA_CHx_BASE(dma_ch) + offset);
}

#endif

#ifdef DMA_VER_R4P0
struct sprd_pcm_dma_params {
	char *name;		
	int channels[2];	
	int workmode;		
	int irq_type;		
	struct sci_dma_cfg desc;	
	u32 dev_paddr[2];	
};

typedef struct sci_dma_cfg  sprd_dma_desc;


static inline int sprd_pcm_dma_get_addr(int dma_ch,
					struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return sci_dma_get_src_addr(dma_ch);
	else
		return sci_dma_get_dst_addr(dma_ch);
}

#endif

#endif 
