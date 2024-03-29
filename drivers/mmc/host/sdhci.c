/*
 *  linux/drivers/mmc/host/sdhci.c - Secure Digital Host Controller Interface driver
 *
 *  Copyright (C) 2005-2008 Pierre Ossman, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * Thanks to the following companies for their support:
 *
 *     - JMicron (hardware and technical support)
 */

#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>

#include <linux/leds.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/delay.h>

#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/pm_runtime.h>
#include <linux/mmc/card.h>
#ifdef CONFIG_MMC_SDHCI_SCX35
#include <mach/hardware.h>
#endif

#include "sdhci.h"
#include <mach/arch_misc.h>

#include <mach/adi.h>
#include <mach/sci_glb_regs.h>

#if 0 
#ifndef	ANA_REG_GET
#define	ANA_REG_GET(_r)		sci_adi_read(_r)
#endif
#endif 

#define DRIVER_NAME "sdhci"

#define DBG(f, x...) \
	pr_debug(DRIVER_NAME " [%s()]: " f, __func__,## x)

#if 0
#if defined(CONFIG_LEDS_CLASS) || (defined(CONFIG_LEDS_CLASS_MODULE) && \
	defined(CONFIG_MMC_SDHCI_MODULE))
#define SDHCI_USE_LEDS_CLASS
#endif
#endif

#define MAX_TUNING_LOOP 40

static unsigned int debug_quirks = 0;
static unsigned int debug_quirks2;

#ifdef CONFIG_MMC_CARD_HOTPLUG
static struct wake_lock sdhci_detect_lock;
#endif

static void sdhci_finish_data(struct sdhci_host *);

static void sdhci_send_command(struct sdhci_host *, struct mmc_command *);
static void sdhci_finish_command(struct sdhci_host *);
static int sdhci_execute_tuning(struct mmc_host *mmc, u32 opcode);
static void sdhci_tuning_timer(unsigned long data);

extern int is_wifi_slot(struct sdhci_host *host);
extern int is_cbp_slot(struct sdhci_host *host);

#ifdef CONFIG_PM_RUNTIME
static int sdhci_runtime_pm_get(struct sdhci_host *host);
static int sdhci_runtime_pm_put(struct sdhci_host *host);
#else
static inline int sdhci_runtime_pm_get(struct sdhci_host *host)
{
	return 0;
}
static inline int sdhci_runtime_pm_put(struct sdhci_host *host)
{
	return 0;
}
#endif

#ifdef CONFIG_MMC_CARD_HOTPLUG
int sdcard_present(struct sdhci_host *host)
{
	int gpio;
	struct sprd_host_data *host_data;
	int irq;

	host_data = sdhci_priv(host);
	irq = host_data->detect_irq;
	if(irq > 0){
		gpio = irq_to_gpio(irq);

		if (gpio_get_value(gpio))
			return 0;
		else
			return 1;
	} else {
		pr_err("%s, %s:please check detect irq\n",
				mmc_hostname(host->mmc), __func__ );
	}
	return 1;
}

irqreturn_t sd_detect_irq(int irq, void *dev_id)
{
	struct sdhci_host* host = dev_id;
	msleep(200);

	if (sdcard_present(host))
		irq_set_irq_type(irq,IRQF_TRIGGER_HIGH);
	else
		irq_set_irq_type(irq,IRQF_TRIGGER_LOW);

	tasklet_schedule(&host->card_tasklet);
	return IRQ_HANDLED;
}
#endif

void sdhci_dumpregs(struct sdhci_host *host)
{
	unsigned int i, regAddr;
	printk(KERN_ERR DRIVER_NAME ": =========== REGISTER DUMP (%s)===========\n",
		mmc_hostname(host->mmc));

	regAddr = SDHCI_DMA_ADDRESS;
	for (i = 0; i < 0x08; i++) {
		printk(KERN_ERR DRIVER_NAME ": 0x%08x | 0x%08x | 0x%08x | 0x%08x\n\r",
			sdhci_readl(host, (regAddr + 16 *i)), sdhci_readl(host, (regAddr + 4+16*i)),
			sdhci_readl(host, (regAddr + 8 + 16* i)), sdhci_readl(host, (regAddr + 12 + 16* i)));
	}

	printk(KERN_ERR DRIVER_NAME ": 0x%08x | 0x%08x | 0x%08x\n\r",
			sdhci_readl(host, 0x80), sdhci_readl(host, 0x84),
			sdhci_readl(host, 0x88));

	if (host->flags & SDHCI_USE_ADMA)
		printk(KERN_ERR DRIVER_NAME ": ADMA Err: 0x%08x | ADMA Ptr: 0x%08x\n",
		       readl(host->ioaddr + SDHCI_ADMA_ERROR),
		       readl(host->ioaddr + SDHCI_ADMA_ADDRESS));

#ifdef CONFIG_MMC_SDHCI_SCX35
	printk(KERN_ERR DRIVER_NAME ": INTC1[0x71500008] : 0x%x (emmc is bit28)\n\r",
			readl(SPRD_INTC1_BASE + 0x08));
	printk(KERN_ERR DRIVER_NAME ": AHB_EN[0x20D00000] : 0x%x (emmc is bit11)\n\r",
			readl(SPRD_AHB_BASE));
	printk(KERN_ERR DRIVER_NAME ": GIC[0x12001100] : 0x%x \n\r",
			readl(CORE_GIC_DIS_VA + 0x100 + (60/32)*4));
#if 0 
	printk(KERN_ERR DRIVER_NAME ": ANA_REG_GLB_LDO_DCDC_PD_RTCSET : 0x%x \n\r",
			ANA_REG_GET(ANA_REG_GLB_LDO_DCDC_PD_RTCSET));
	printk(KERN_ERR DRIVER_NAME ": ANA_REG_GLB_LDO_PD_CTRL : 0x%x \n\r",
			ANA_REG_GET(ANA_REG_GLB_LDO_PD_CTRL));
#endif 
	printk(KERN_ERR DRIVER_NAME ": REG_PMU_APB_PLL_DIV_EN1 : 0x%x \n\r",
			readl(REG_PMU_APB_PLL_DIV_EN1));
	printk(KERN_ERR DRIVER_NAME ": REG_PMU_APB_CGM_AP_EN : 0x%x \n\r",
			readl(REG_PMU_APB_CGM_AP_EN));
	printk(KERN_ERR DRIVER_NAME ": REG_PMU_APB_TDPLL_REL_CFG : 0x%x \n\r",
			readl(REG_PMU_APB_TDPLL_REL_CFG));
	printk(KERN_ERR DRIVER_NAME ": REG_PMU_APB_CGM_AP_AUTO_GATE_EN : 0x%x \n\r",
			readl(REG_PMU_APB_CGM_AP_AUTO_GATE_EN)); 
#endif

	printk(KERN_ERR DRIVER_NAME ": ===========================================\n");
}

EXPORT_SYMBOL_GPL(sdhci_dumpregs);



static void sdhci_clear_set_irqs(struct sdhci_host *host, u32 clear, u32 set)
{
	u32 ier;

	ier = sdhci_readl(host, SDHCI_INT_ENABLE);
	ier &= ~clear;
	ier |= set;
	sdhci_writel(host, ier, SDHCI_INT_ENABLE);
	sdhci_writel(host, ier, SDHCI_SIGNAL_ENABLE);
}

static void sdhci_unmask_irqs(struct sdhci_host *host, u32 irqs)
{
	sdhci_clear_set_irqs(host, 0, irqs);
}

static void sdhci_mask_irqs(struct sdhci_host *host, u32 irqs)
{
	sdhci_clear_set_irqs(host, irqs, 0);
}

static void sdhci_set_card_detection(struct sdhci_host *host, bool enable)
{
#if 0
	u32 present, irqs;

	if ((host->quirks & SDHCI_QUIRK_BROKEN_CARD_DETECTION) ||
	    (host->mmc->caps & MMC_CAP_NONREMOVABLE))
		return;

	present = sdhci_readl(host, SDHCI_PRESENT_STATE) &
			      SDHCI_CARD_PRESENT;
	irqs = present ? SDHCI_INT_CARD_REMOVE : SDHCI_INT_CARD_INSERT;

	if (enable)
		sdhci_unmask_irqs(host, irqs);
	else
		sdhci_mask_irqs(host, irqs);
#endif
#ifdef CONFIG_MMC_CARD_HOTPLUG
	int irq;
	struct sprd_host_data *host_data;

	host_data = sdhci_priv(host);
	irq = host_data->detect_irq;
	if(irq > 0){
		if(!enable) {
			irq_set_irq_type(irq,IRQF_TRIGGER_NONE);
			return;
		}

		if(sdcard_present(host)){
			irq_set_irq_type(irq,IRQF_TRIGGER_HIGH);
		}else{
			irq_set_irq_type(irq,IRQF_TRIGGER_LOW);
		}
	}
#endif
}

static void sdhci_enable_card_detection(struct sdhci_host *host)
{
	sdhci_set_card_detection(host, true);
}

static __used void sdhci_disable_card_detection(struct sdhci_host *host)
{
	sdhci_set_card_detection(host, false);
}

static void sdhci_reset(struct sdhci_host *host, u8 mask)
{
	unsigned long timeout;
	u32 uninitialized_var(ier);

	if (host->quirks & SDHCI_QUIRK_NO_CARD_NO_RESET) {
		if (!(sdhci_readl(host, SDHCI_PRESENT_STATE) &
			SDHCI_CARD_PRESENT))
			return;
	}

	if (host->quirks & SDHCI_QUIRK_RESTORE_IRQS_AFTER_RESET)
		ier = sdhci_readl(host, SDHCI_INT_ENABLE);

	if (host->ops->platform_reset_enter)
		host->ops->platform_reset_enter(host, mask);
	if (mask & SDHCI_RESET_ALL){
		sdhci_sdclk_enable(host, 0);
	}
#if defined( CONFIG_MMC_SDHCI_SC8825 ) || defined (CONFIG_MMC_SDHCI_SCX35)
	sdhci_writeb(host, mask | SDHCI_HW_RESET_CARD, SDHCI_SOFTWARE_RESET);
#else
	sdhci_writeb(host, mask, SDHCI_SOFTWARE_RESET);
#endif
	if (mask & SDHCI_RESET_ALL)
		host->clock = 0;

	
	timeout = 100;

	
	while (sdhci_readb(host, SDHCI_SOFTWARE_RESET) & mask) {
		if (timeout == 0) {
			pr_err("%s: Reset 0x%x never completed.\n",
				mmc_hostname(host->mmc), (int)mask);
			sdhci_dumpregs(host);
			return;
		}
		timeout--;
		mdelay(1);
	}

#ifdef CONFIG_MMC_SDHCI_SCX35
	if (mask & SDHCI_RESET_ALL) {
		if ((0 == strcmp(host->hw_name, "sprd-sdio1"))
				|| (0 == strcmp(host->hw_name, "sprd-sdio2"))) {
			unsigned int tmp_val;
			tmp_val = sdhci_readw(host, SDHCI_TRANSFER_MODE);
			tmp_val &= 0xFFFFF8FF;		
			sdhci_writew(host, tmp_val, SDHCI_TRANSFER_MODE);
		}
	}
#endif
	if (host->ops->platform_reset_exit)
		host->ops->platform_reset_exit(host, mask);

	if (host->quirks & SDHCI_QUIRK_RESTORE_IRQS_AFTER_RESET)
		sdhci_clear_set_irqs(host, SDHCI_INT_ALL_MASK, ier);

	if (host->flags & (SDHCI_USE_SDMA | SDHCI_USE_ADMA)) {
		if ((host->ops->enable_dma) && (mask & SDHCI_RESET_ALL))
			host->ops->enable_dma(host);
	}
}

static void sdhci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios);

static void sdhci_init(struct sdhci_host *host, int soft)
{
	if (soft)
		sdhci_reset(host, SDHCI_RESET_CMD|SDHCI_RESET_DATA);
	else
		sdhci_reset(host, SDHCI_RESET_ALL);

	sdhci_clear_set_irqs(host, SDHCI_INT_ALL_MASK,
		SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
		SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
		SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
		SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE);
	if (soft) {
		
		host->clock = 0;
		sdhci_set_ios(host->mmc, &host->mmc->ios);
	}
}

void sdhci_reinit(struct sdhci_host *host)
{
	sdhci_init(host, 0);
#ifdef CONFIG_MMC_CARD_HOTPLUG
	sdhci_enable_card_detection(host);
#endif
}

#if 0
static void sdhci_activate_led(struct sdhci_host *host)
{
	u8 ctrl;

	ctrl = sdhci_readb(host, SDHCI_HOST_CONTROL);
	ctrl |= SDHCI_CTRL_LED;
	sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);
}

static void sdhci_deactivate_led(struct sdhci_host *host)
{
	u8 ctrl;

	ctrl = sdhci_readb(host, SDHCI_HOST_CONTROL);
	ctrl &= ~SDHCI_CTRL_LED;
	sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);
}
#endif

#ifdef SDHCI_USE_LEDS_CLASS
static void sdhci_led_control(struct led_classdev *led,
	enum led_brightness brightness)
{
	struct sdhci_host *host = container_of(led, struct sdhci_host, led);
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);

	if (host->runtime_suspended)
		goto out;

	if (brightness == LED_OFF)
		sdhci_deactivate_led(host);
	else
		sdhci_activate_led(host);
out:
	spin_unlock_irqrestore(&host->lock, flags);
}
#endif


static void sdhci_read_block_pio(struct sdhci_host *host)
{
	unsigned long flags;
	size_t blksize, len, chunk;
	u32 uninitialized_var(scratch);
	u8 *buf;

	DBG("PIO reading\n");

	blksize = host->data->blksz;
	chunk = 0;

	local_irq_save(flags);

	while (blksize) {
		if (!sg_miter_next(&host->sg_miter))
			BUG();

		len = min(host->sg_miter.length, blksize);

		blksize -= len;
		host->sg_miter.consumed = len;

		buf = host->sg_miter.addr;

		while (len) {
			if (chunk == 0) {
				scratch = sdhci_readl(host, SDHCI_BUFFER);
				chunk = 4;
			}

			*buf = scratch & 0xFF;

			buf++;
			scratch >>= 8;
			chunk--;
			len--;
		}
	}

	sg_miter_stop(&host->sg_miter);

	local_irq_restore(flags);
}

static void sdhci_write_block_pio(struct sdhci_host *host)
{
	unsigned long flags;
	size_t blksize, len, chunk;
	u32 scratch;
	u8 *buf;

	DBG("PIO writing\n");

	blksize = host->data->blksz;
	chunk = 0;
	scratch = 0;

	local_irq_save(flags);

	while (blksize) {
		if (!sg_miter_next(&host->sg_miter))
			BUG();

		len = min(host->sg_miter.length, blksize);

		blksize -= len;
		host->sg_miter.consumed = len;

		buf = host->sg_miter.addr;

		while (len) {
			scratch |= (u32)*buf << (chunk * 8);

			buf++;
			chunk++;
			len--;

			if ((chunk == 4) || ((len == 0) && (blksize == 0))) {
				sdhci_writel(host, scratch, SDHCI_BUFFER);
				chunk = 0;
				scratch = 0;
			}
		}
	}

	sg_miter_stop(&host->sg_miter);

	local_irq_restore(flags);
}

static void sdhci_transfer_pio(struct sdhci_host *host)
{
	u32 mask;

	BUG_ON(!host->data);

	if (host->blocks == 0)
		return;

	if (host->data->flags & MMC_DATA_READ)
		mask = SDHCI_DATA_AVAILABLE;
	else
		mask = SDHCI_SPACE_AVAILABLE;

	if ((host->quirks & SDHCI_QUIRK_BROKEN_SMALL_PIO) &&
		(host->data->blocks == 1))
		mask = ~0;

	while (sdhci_readl(host, SDHCI_PRESENT_STATE) & mask) {
		if (host->quirks & SDHCI_QUIRK_PIO_NEEDS_DELAY)
			udelay(100);

		if (host->data->flags & MMC_DATA_READ)
			sdhci_read_block_pio(host);
		else
			sdhci_write_block_pio(host);

		host->blocks--;
		if (host->blocks == 0)
			break;
	}

	DBG("PIO transfer complete.\n");
}

static char *sdhci_kmap_atomic(struct scatterlist *sg, unsigned long *flags)
{
	local_irq_save(*flags);
	return kmap_atomic(sg_page(sg)) + sg->offset;
}

static void sdhci_kunmap_atomic(void *buffer, unsigned long *flags)
{
	kunmap_atomic(buffer);
	local_irq_restore(*flags);
}

static void sdhci_set_adma_desc(u8 *desc, u32 addr, int len, unsigned cmd)
{
	__le32 *dataddr = (__le32 __force *)(desc + 4);
	__le16 *cmdlen = (__le16 __force *)desc;


	cmdlen[0] = cpu_to_le16(cmd);
	cmdlen[1] = cpu_to_le16(len);

	dataddr[0] = cpu_to_le32(addr);
}

static int sdhci_adma_table_pre(struct sdhci_host *host,
	struct mmc_data *data)
{
	int direction;

	u8 *desc;
	u8 *align;
	dma_addr_t addr;
	dma_addr_t align_addr;
	int len, offset;

	struct scatterlist *sg;
	int i;
	char *buffer;
	unsigned long flags;


	if (data->flags & MMC_DATA_READ)
		direction = DMA_FROM_DEVICE;
	else
		direction = DMA_TO_DEVICE;


	host->align_addr = dma_map_single(mmc_dev(host->mmc),
		host->align_buffer, 128 * 4, direction);
	if (dma_mapping_error(mmc_dev(host->mmc), host->align_addr))
		goto fail;
	BUG_ON(host->align_addr & 0x3);

	host->sg_count = dma_map_sg(mmc_dev(host->mmc),
		data->sg, data->sg_len, direction);
	if (host->sg_count == 0)
		goto unmap_align;

	desc = host->adma_desc;
	align = host->align_buffer;

	align_addr = host->align_addr;

	for_each_sg(data->sg, sg, host->sg_count, i) {
		addr = sg_dma_address(sg);
		len = sg_dma_len(sg);

		offset = (4 - (addr & 0x3)) & 0x3;
		if (offset) {
			if (data->flags & MMC_DATA_WRITE) {
				buffer = sdhci_kmap_atomic(sg, &flags);
				WARN_ON(((long)buffer & PAGE_MASK) > (PAGE_SIZE - 3));
				memcpy(align, buffer, offset);
				sdhci_kunmap_atomic(buffer, &flags);
			}

			
			sdhci_set_adma_desc(desc, align_addr, offset, 0x21);

			BUG_ON(offset > 65536);

			align += 4;
			align_addr += 4;

			desc += 8;

			addr += offset;
			len -= offset;
		}

		BUG_ON(len > 65536);

		
		sdhci_set_adma_desc(desc, addr, len, 0x21);
		desc += 8;

		WARN_ON((desc - host->adma_desc) > (128 * 2 + 1) * 4);
	}

	if (host->quirks & SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC) {
		if (desc != host->adma_desc) {
			desc -= 8;
			desc[0] |= 0x2; 
		}
	} else {

		
		sdhci_set_adma_desc(desc, 0, 0, 0x3);
	}

	if (data->flags & MMC_DATA_WRITE) {
		dma_sync_single_for_device(mmc_dev(host->mmc),
			host->align_addr, 128 * 4, direction);
	}

	host->adma_addr = dma_map_single(mmc_dev(host->mmc),
		host->adma_desc, (128 * 2 + 1) * 4, DMA_TO_DEVICE);
	if (dma_mapping_error(mmc_dev(host->mmc), host->adma_addr))
		goto unmap_entries;
	BUG_ON(host->adma_addr & 0x3);

	return 0;

unmap_entries:
	dma_unmap_sg(mmc_dev(host->mmc), data->sg,
		data->sg_len, direction);
unmap_align:
	dma_unmap_single(mmc_dev(host->mmc), host->align_addr,
		128 * 4, direction);
fail:
	return -EINVAL;
}

static void sdhci_adma_table_post(struct sdhci_host *host,
	struct mmc_data *data)
{
	int direction;

	struct scatterlist *sg;
	int i, size;
	u8 *align;
	char *buffer;
	unsigned long flags;

	if (data->flags & MMC_DATA_READ)
		direction = DMA_FROM_DEVICE;
	else
		direction = DMA_TO_DEVICE;

	dma_unmap_single(mmc_dev(host->mmc), host->adma_addr,
		(128 * 2 + 1) * 4, DMA_TO_DEVICE);

	dma_unmap_single(mmc_dev(host->mmc), host->align_addr,
		128 * 4, direction);

	if (data->flags & MMC_DATA_READ) {
		dma_sync_sg_for_cpu(mmc_dev(host->mmc), data->sg,
			data->sg_len, direction);

		align = host->align_buffer;

		for_each_sg(data->sg, sg, host->sg_count, i) {
			if (sg_dma_address(sg) & 0x3) {
				size = 4 - (sg_dma_address(sg) & 0x3);

				buffer = sdhci_kmap_atomic(sg, &flags);
				WARN_ON(((long)buffer & PAGE_MASK) > (PAGE_SIZE - 3));
				memcpy(buffer, align, size);
				sdhci_kunmap_atomic(buffer, &flags);

				align += 4;
			}
		}
	}

	dma_unmap_sg(mmc_dev(host->mmc), data->sg,
		data->sg_len, direction);
}

static u8 sdhci_calc_timeout(struct sdhci_host *host, struct mmc_command *cmd)
{
	u8 count;
	struct mmc_data *data = cmd->data;
	unsigned target_timeout, current_timeout;

	if (host->quirks & SDHCI_QUIRK_BROKEN_TIMEOUT_VAL)
		return 0xE;

	
	if (!data && !cmd->cmd_timeout_ms)
		return 0xE;

	
	if (!data)
		target_timeout = cmd->cmd_timeout_ms * 1000;
	else {
		target_timeout = data->timeout_ns / 1000;
		if (host->clock)
			target_timeout += data->timeout_clks / host->clock;
	}

	count = 0;
	current_timeout = (1 << 13) * 1000 / host->timeout_clk;
	while (current_timeout < target_timeout) {
		count++;
		current_timeout <<= 1;
		if (count >= 0xF)
			break;
	}

	if (count >= 0xF)
		count = 0xE;

	return count;
}

static void sdhci_set_transfer_irqs(struct sdhci_host *host)
{
	u32 pio_irqs = SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL;
	u32 dma_irqs = SDHCI_INT_DMA_END | SDHCI_INT_ADMA_ERROR;

	if (host->flags & SDHCI_REQ_USE_DMA)
		sdhci_clear_set_irqs(host, pio_irqs, dma_irqs);
	else
		sdhci_clear_set_irqs(host, dma_irqs, pio_irqs);
}

static void sdhci_prepare_data(struct sdhci_host *host, struct mmc_command *cmd)
{
	u8 count;
	u8 ctrl;
	struct mmc_data *data = cmd->data;
	int ret;

	WARN_ON(host->data);

	if (data || (cmd->flags & MMC_RSP_BUSY)) {
		count = sdhci_calc_timeout(host, cmd);
		sdhci_writeb(host, count, SDHCI_TIMEOUT_CONTROL);
	}

	if (!data)
		return;

	
	BUG_ON(data->blksz * data->blocks > 524288);
	BUG_ON(data->blksz > host->mmc->max_blk_size);
	BUG_ON(data->blocks > 65535);

	host->data = data;
	host->data_early = 0;
	host->data->bytes_xfered = 0;

	if (host->flags & (SDHCI_USE_SDMA | SDHCI_USE_ADMA))
		host->flags |= SDHCI_REQ_USE_DMA;

	if (host->flags & SDHCI_REQ_USE_DMA) {
		int broken, i;
		struct scatterlist *sg;

		broken = 0;
		if (host->flags & SDHCI_USE_ADMA) {
			if (host->quirks & SDHCI_QUIRK_32BIT_ADMA_SIZE)
				broken = 1;
		} else {
			if (host->quirks & SDHCI_QUIRK_32BIT_DMA_SIZE)
				broken = 1;
		}

		if (unlikely(broken)) {
			for_each_sg(data->sg, sg, data->sg_len, i) {
				if (sg->length & 0x3) {
					DBG("Reverting to PIO because of "
						"transfer size (%d)\n",
						sg->length);
					host->flags &= ~SDHCI_REQ_USE_DMA;
					break;
				}
			}
		}
	}

	if (host->flags & SDHCI_REQ_USE_DMA) {
		int broken, i;
		struct scatterlist *sg;

		broken = 0;
		if (host->flags & SDHCI_USE_ADMA) {
			if (host->quirks & SDHCI_QUIRK_32BIT_ADMA_SIZE)
				broken = 1;
		} else {
			if (host->quirks & SDHCI_QUIRK_32BIT_DMA_ADDR)
				broken = 1;
		}

		if (unlikely(broken)) {
			for_each_sg(data->sg, sg, data->sg_len, i) {
				if (sg->offset & 0x3) {
					DBG("Reverting to PIO because of "
						"bad alignment\n");
					host->flags &= ~SDHCI_REQ_USE_DMA;
					break;
				}
			}
		}
	}

	if (host->flags & SDHCI_REQ_USE_DMA) {
		if (host->flags & SDHCI_USE_ADMA) {
			ret = sdhci_adma_table_pre(host, data);
			if (ret) {
				WARN_ON(1);
				host->flags &= ~SDHCI_REQ_USE_DMA;
			} else {
				sdhci_writel(host, host->adma_addr,
					SDHCI_ADMA_ADDRESS);
			}
		} else {
			int sg_cnt;

			sg_cnt = dma_map_sg(mmc_dev(host->mmc),
					data->sg, data->sg_len,
					(data->flags & MMC_DATA_READ) ?
						DMA_FROM_DEVICE :
						DMA_TO_DEVICE);
			if (sg_cnt == 0) {
				WARN_ON(1);
				host->flags &= ~SDHCI_REQ_USE_DMA;
			} else {
				WARN_ON(sg_cnt != 1);
				sdhci_writel(host, sg_dma_address(data->sg),
					SDHCI_DMA_ADDRESS);
			}
		}
	}

	if (host->version >= SDHCI_SPEC_200) {
		ctrl = sdhci_readb(host, SDHCI_HOST_CONTROL);
		ctrl &= ~SDHCI_CTRL_DMA_MASK;
		if ((host->flags & SDHCI_REQ_USE_DMA) &&
			(host->flags & SDHCI_USE_ADMA))
			ctrl |= SDHCI_CTRL_ADMA32;
		else
			ctrl |= SDHCI_CTRL_SDMA;
		sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);
	}

	if (!(host->flags & SDHCI_REQ_USE_DMA)) {
		int flags;

		flags = SG_MITER_ATOMIC;
		if (host->data->flags & MMC_DATA_READ)
			flags |= SG_MITER_TO_SG;
		else
			flags |= SG_MITER_FROM_SG;
		sg_miter_start(&host->sg_miter, data->sg, data->sg_len, flags);
		host->blocks = data->blocks;
	}

	sdhci_set_transfer_irqs(host);

	
	sdhci_writew(host, SDHCI_MAKE_BLKSZ(SDHCI_DEFAULT_BOUNDARY_ARG,
		data->blksz), SDHCI_BLOCK_SIZE);
	sdhci_writew(host, data->blocks, SDHCI_BLOCK_COUNT);
}

static void sdhci_set_transfer_mode(struct sdhci_host *host,
	struct mmc_command *cmd)
{
	u16 mode;
	struct mmc_data *data = cmd->data;

	if (data == NULL)
		return;

	WARN_ON(!host->data);

	mode = SDHCI_TRNS_BLK_CNT_EN;
	if (mmc_op_multi(cmd->opcode) || data->blocks > 1) {
		mode |= SDHCI_TRNS_MULTI;
		if (!host->mrq->sbc && (host->flags & SDHCI_AUTO_CMD12))
			mode |= SDHCI_TRNS_AUTO_CMD12;
		else if (host->mrq->sbc && (host->flags & SDHCI_AUTO_CMD23)) {
			mode |= SDHCI_TRNS_AUTO_CMD23;
			sdhci_writel(host, host->mrq->sbc->arg, SDHCI_ARGUMENT2);
		}
	}

	if (data->flags & MMC_DATA_READ)
		mode |= SDHCI_TRNS_READ;
	if (host->flags & SDHCI_REQ_USE_DMA)
		mode |= SDHCI_TRNS_DMA;

	sdhci_writew(host, mode, SDHCI_TRANSFER_MODE);
}

static void sdhci_finish_data(struct sdhci_host *host)
{
	struct mmc_data *data;

	BUG_ON(!host->data);

	data = host->data;
	host->data = NULL;

	if (host->flags & SDHCI_REQ_USE_DMA) {
		if (host->flags & SDHCI_USE_ADMA)
			sdhci_adma_table_post(host, data);
		else {
			dma_unmap_sg(mmc_dev(host->mmc), data->sg,
				data->sg_len, (data->flags & MMC_DATA_READ) ?
					DMA_FROM_DEVICE : DMA_TO_DEVICE);
		}
	}

	if (data->error)
		data->bytes_xfered = 0;
	else
		data->bytes_xfered = data->blksz * data->blocks;

	if (data->stop &&
	    (data->error ||
	     !host->mrq->sbc)) {

		if (data->error) {
			sdhci_reset(host, SDHCI_RESET_CMD);
			sdhci_reset(host, SDHCI_RESET_DATA);
		}

		sdhci_send_command(host, data->stop);
	} else
		tasklet_schedule(&host->finish_tasklet);
}

static void sdhci_send_command(struct sdhci_host *host, struct mmc_command *cmd)
{
	int flags;
	u32 mask;
	unsigned long timeout;

	WARN_ON(host->cmd);

	
	timeout = 10;

	mask = SDHCI_CMD_INHIBIT;
	if ((cmd->data != NULL) || (cmd->flags & MMC_RSP_BUSY))
		mask |= SDHCI_DATA_INHIBIT;

	if (host->mrq->data && (cmd == host->mrq->data->stop))
		mask &= ~SDHCI_DATA_INHIBIT;

	while (sdhci_readl(host, SDHCI_PRESENT_STATE) & mask) {
		if (timeout == 0) {
			pr_err("%s: Controller never released "
				"inhibit bit(s).\n", mmc_hostname(host->mmc));
			sdhci_dumpregs(host);
			cmd->error = -EIO;
			tasklet_schedule(&host->finish_tasklet);
			return;
		}
		timeout--;
		mdelay(1);
	}

	if(host->suspending){
		mod_timer(&host->timer, jiffies + HZ / 5);
	}else{
		mod_timer(&host->timer, jiffies + 10 * HZ);
	}

	host->cmd = cmd;

	sdhci_prepare_data(host, cmd);

	sdhci_writel(host, cmd->arg, SDHCI_ARGUMENT);

	sdhci_set_transfer_mode(host, cmd);

	if ((cmd->flags & MMC_RSP_136) && (cmd->flags & MMC_RSP_BUSY)) {
		pr_err("%s: Unsupported response type!\n",
			mmc_hostname(host->mmc));
		cmd->error = -EINVAL;
		tasklet_schedule(&host->finish_tasklet);
		return;
	}

	if (!(cmd->flags & MMC_RSP_PRESENT))
		flags = SDHCI_CMD_RESP_NONE;
	else if (cmd->flags & MMC_RSP_136)
		flags = SDHCI_CMD_RESP_LONG;
	else if (cmd->flags & MMC_RSP_BUSY)
		flags = SDHCI_CMD_RESP_SHORT_BUSY;
	else
		flags = SDHCI_CMD_RESP_SHORT;

	if (cmd->flags & MMC_RSP_CRC)
		flags |= SDHCI_CMD_CRC;
	if (cmd->flags & MMC_RSP_OPCODE)
		flags |= SDHCI_CMD_INDEX;

	
	if (cmd->data || cmd->opcode == MMC_SEND_TUNING_BLOCK ||
	    cmd->opcode == MMC_SEND_TUNING_BLOCK_HS200)
		flags |= SDHCI_CMD_DATA;

	sdhci_writew(host, SDHCI_MAKE_CMD(cmd->opcode, flags), SDHCI_COMMAND);
}

static void sdhci_finish_command(struct sdhci_host *host)
{
	int i;

	BUG_ON(host->cmd == NULL);

	if (host->cmd->flags & MMC_RSP_PRESENT) {
		if (host->cmd->flags & MMC_RSP_136) {
			
			for (i = 0;i < 4;i++) {
				host->cmd->resp[i] = sdhci_readl(host,
					SDHCI_RESPONSE + (3-i)*4) << 8;
				if (i != 3)
					host->cmd->resp[i] |=
						sdhci_readb(host,
						SDHCI_RESPONSE + (3-i)*4-1);
			}
		} else {
			host->cmd->resp[0] = sdhci_readl(host, SDHCI_RESPONSE);
		}
	}

	host->cmd->error = 0;

	
	if (host->cmd == host->mrq->sbc) {
		host->cmd = NULL;
		sdhci_send_command(host, host->mrq->cmd);
	} else {

		
		if (host->data && host->data_early)
			sdhci_finish_data(host);

		if (!host->cmd->data)
			tasklet_schedule(&host->finish_tasklet);

		host->cmd = NULL;
	}
}

static void sdhci_set_clock(struct sdhci_host *host, unsigned int clock)
{
	int div = 0; 
	int real_div = div, clk_mul = 1;
	u16 clk = 0;
	

	unsigned long timeout;
	if(clock){
		host->mmc->max_discard_to = (1 << 27) /( clock/1000);
		printk(KERN_INFO "--------max_discard_to %d,%d",clock ,host->mmc->max_discard_to);
	}
	if (clock && clock == host->clock)
		return;

	host->mmc->actual_clock = 0;

	if (host->ops->set_clock) {
		host->ops->set_clock(host, clock);
		if (host->quirks & SDHCI_QUIRK_NONSTANDARD_CLOCK)
			return;
	}

	if (clock == host->clock)
		return;

	sdhci_sdclk_enable(host, 0);

	if (clock == 0)
		goto out;

	if (host->version >= SDHCI_SPEC_300) {
		if (host->clk_mul) {
			u16 ctrl;

			ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
			if (!(ctrl & SDHCI_CTRL_PRESET_VAL_ENABLE)) {
				for (div = 1; div <= 1024; div++) {
					if (((host->max_clk * host->clk_mul) /
					      div) <= clock)
						break;
				}
				clk = SDHCI_PROG_CLOCK_MODE;
				real_div = div;
				clk_mul = host->clk_mul;
				div--;
			}
		} else {
			
			if (host->max_clk <= clock)
				div = 1;
			else {
				for (div = 2; div < SDHCI_MAX_DIV_SPEC_300;
				     div += 2) {
					if ((host->max_clk / div) <= clock)
						break;
				}
			}
			real_div = div;
			div >>= 1;
			if ((real_div % 2) != 0){
				div++;
			}
#if defined(CONFIG_ARCH_SC8825) || defined(CONFIG_ARCH_SCX35)
			if(div > 1) {
				div --;
			}
#endif
		}
	} else {
		
#if 0 
		for (div = 1; div < SDHCI_MAX_DIV_SPEC_200; div *= 2) {
			if ((host->max_clk / div) <= clock)
				break;
		}
		real_div = div;
		div >>= 1;
#if defined(CONFIG_ARCH_SCX35)
		if ((real_div % 2) != 0){
			div++;
		}
		if(div > 1) {
			div --;
		}
#endif
#else 
		for(div=0;div<=SDHCI_MAX_DIV_SPEC_200-1;div++)
		{
			if ((host->max_clk / ((div+1)*2)) <= clock)
				break;
		}
		real_div = (div+1)*2;
#endif
	}

	if (real_div)
		host->mmc->actual_clock = (host->max_clk * clk_mul) / real_div;

	clk |= (div & SDHCI_DIV_MASK) << SDHCI_DIVIDER_SHIFT;
	clk |= ((div & SDHCI_DIV_HI_MASK) >> SDHCI_DIV_MASK_LEN)
		<< SDHCI_DIVIDER_HI_SHIFT;

	clk |= SDHCI_CLOCK_INT_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
	udelay(200);
	
	timeout = 20;
	while (!((clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL))
		& SDHCI_CLOCK_INT_STABLE)) {
		if (timeout == 0) {
			pr_err("%s: Internal clock never "
				"stabilised.\n", mmc_hostname(host->mmc));
			sdhci_dumpregs(host);
			return;
		}
		timeout--;
		mdelay(1);
	}

	clk |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
out:
	host->clock = clock;
}

static int sdhci_set_power(struct sdhci_host *host, unsigned short power)
{
	u8 pwr = 0;

	if (power != (unsigned short)-1) {
		switch (1 << power) {
		case MMC_VDD_165_195:
			pwr = SDHCI_POWER_180;
			break;
		case MMC_VDD_29_30:
		case MMC_VDD_30_31:
			pwr = SDHCI_POWER_300;
			break;
		case MMC_VDD_32_33:
		case MMC_VDD_33_34:
			pwr = SDHCI_POWER_330;
			break;
		default:
			BUG();
		}
	}

	if (host->pwr == pwr)
		return -1;

	if (pwr == 0) {
		sdhci_writeb(host, 0, SDHCI_POWER_CONTROL);

		if (host->ops->set_power)
			host->ops->set_power(host, pwr);
		host->pwr = pwr;
		return 0;
	}

	if (!(host->quirks & SDHCI_QUIRK_SINGLE_POWER_WRITE))
		sdhci_writeb(host, 0, SDHCI_POWER_CONTROL);

	if (host->quirks & SDHCI_QUIRK_NO_SIMULT_VDD_AND_POWER)
		sdhci_writeb(host, pwr, SDHCI_POWER_CONTROL);

	if (host->ops->set_power){
		host->ops->set_power(host, pwr);
		host->pwr = pwr;
        }
	pwr |= SDHCI_POWER_ON;

	sdhci_writeb(host, pwr, SDHCI_POWER_CONTROL);

	if (host->quirks & SDHCI_QUIRK_DELAY_AFTER_POWER)
		mdelay(10);

	return power;
}

static void sdhci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct sdhci_host *host;
	bool present;
	unsigned long flags;

	host = mmc_priv(mmc);

	sdhci_runtime_pm_get(host);

	spin_lock_irqsave(&host->lock, flags);

	WARN_ON(host->mrq != NULL);

#ifdef SDHCI_USE_LEDS_CLASS
	sdhci_activate_led(host);
#endif

	if (!mrq->sbc && (host->flags & SDHCI_AUTO_CMD12)) {
		if (mrq->stop) {
			mrq->data->stop = NULL;
			mrq->stop = NULL;
		}
	}

	host->mrq = mrq;

	
	if (host->quirks & SDHCI_QUIRK_BROKEN_CARD_DETECTION)
		present = true;
	else
		present = sdhci_readl(host, SDHCI_PRESENT_STATE) &
				SDHCI_CARD_PRESENT;

	if (!present || host->flags & SDHCI_DEVICE_DEAD) {
		host->mrq->cmd->error = -ENOMEDIUM;
		tasklet_schedule(&host->finish_tasklet);
	} else {
		u32 present_state;

		present_state = sdhci_readl(host, SDHCI_PRESENT_STATE);
		if ((host->flags & SDHCI_NEEDS_RETUNING) &&
		    !(present_state & (SDHCI_DOING_WRITE | SDHCI_DOING_READ))) {
			spin_unlock_irqrestore(&host->lock, flags);
			sdhci_execute_tuning(mmc, mrq->cmd->opcode);
			spin_lock_irqsave(&host->lock, flags);

			
			host->mrq = mrq;
		}

		if (mrq->sbc && !(host->flags & SDHCI_AUTO_CMD23))
			sdhci_send_command(host, mrq->sbc);
		else
			sdhci_send_command(host, mrq->cmd);
	}

	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);
}

static void sdhci_do_set_ios(struct sdhci_host *host, struct mmc_ios *ios)
{
	unsigned long flags;
	int vdd_bit = -1;
	u8 ctrl;
        struct sprd_host_data *host_data = sdhci_priv(host);

	spin_lock_irqsave(&host->lock, flags);

	if (host->flags & SDHCI_DEVICE_DEAD) {
		spin_unlock_irqrestore(&host->lock, flags);
		if (host->vmmc && ios->power_mode == MMC_POWER_OFF)
			mmc_regulator_set_ocr(host->mmc, host->vmmc, 0);
		return;
	}

	if (ios->power_mode == MMC_POWER_OFF) {
		sdhci_writel(host, 0, SDHCI_SIGNAL_ENABLE);
		sdhci_reinit(host);
	}

	sdhci_set_clock(host, ios->clock);

	if (ios->power_mode == MMC_POWER_OFF)
		vdd_bit = sdhci_set_power(host, -1);
	else
		vdd_bit = sdhci_set_power(host, ios->vdd);

	if (host->vmmc && vdd_bit != -1) {
		spin_unlock_irqrestore(&host->lock, flags);
		mmc_regulator_set_ocr(host->mmc, host->vmmc, vdd_bit);
		spin_lock_irqsave(&host->lock, flags);
	}

	if (host->ops->platform_send_init_74_clocks)
		host->ops->platform_send_init_74_clocks(host, ios->power_mode);

	if (host->ops->platform_8bit_width)
		host->ops->platform_8bit_width(host, ios->bus_width);
	else {
		ctrl = sdhci_readb(host, SDHCI_HOST_CONTROL);
		if (ios->bus_width == MMC_BUS_WIDTH_8) {
			ctrl &= ~SDHCI_CTRL_4BITBUS;
			if (host->version >= SDHCI_SPEC_300)
				ctrl |= SDHCI_CTRL_8BITBUS;
		} else {
			if (host->version >= SDHCI_SPEC_300)
				ctrl &= ~SDHCI_CTRL_8BITBUS;
			if (ios->bus_width == MMC_BUS_WIDTH_4)
				ctrl |= SDHCI_CTRL_4BITBUS;
			else
				ctrl &= ~SDHCI_CTRL_4BITBUS;
		}
		sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);
	}

	ctrl = sdhci_readb(host, SDHCI_HOST_CONTROL);

	if ((ios->timing == MMC_TIMING_SD_HS ||
	     ios->timing == MMC_TIMING_MMC_HS)
	    && !(host->quirks & SDHCI_QUIRK_NO_HISPD_BIT))
		ctrl |= SDHCI_CTRL_HISPD;
	else
		ctrl &= ~SDHCI_CTRL_HISPD;

	
	if (((0 == strcmp(host_data->platdata->hw_name, "sprd-sdio1"))
		|| (0 == strcmp(host_data->platdata->hw_name, "sprd-sdio2")))
		&& ( soc_is_scx35_v0())){
		ctrl &= ~SDHCI_CTRL_HISPD;
	}

	if (host->version >= SDHCI_SPEC_300) {
#if defined( CONFIG_MMC_SDHCI_SC8825 ) || defined (CONFIG_MMC_SDHCI_SCX35)
		u32 ctrl_2;
#else
		u16 clk, ctrl_2;
#endif
		unsigned int clock;

		
		ctrl &= ~SDHCI_CTRL_HISPD;
		sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);
		
		ctrl_2 = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		if (!(ctrl_2 & SDHCI_CTRL_PRESET_VAL_ENABLE)) {
#if defined( CONFIG_MMC_SDHCI_SC8825 ) || defined (CONFIG_MMC_SDHCI_SCX35)
			
#else
			ctrl_2 &= ~SDHCI_CTRL_DRV_TYPE_MASK;
			if (ios->drv_type == MMC_SET_DRIVER_TYPE_A)
				ctrl_2 |= SDHCI_CTRL_DRV_TYPE_A;
			else if (ios->drv_type == MMC_SET_DRIVER_TYPE_C)
				ctrl_2 |= SDHCI_CTRL_DRV_TYPE_C;

			sdhci_writew(host, ctrl_2, SDHCI_HOST_CONTROL2);
#endif
		} else {

			
			
			
			

			
			clock = host->clock;
			host->clock = 0;
			sdhci_set_clock(host, clock);
		}


		
		sdhci_sdclk_enable(host, 0);

		if (host->ops->set_uhs_signaling)
			host->ops->set_uhs_signaling(host, ios->timing);
		else {
#if defined( CONFIG_MMC_SDHCI_SC8825 ) || defined (CONFIG_MMC_SDHCI_SCX35)
			ctrl_2 = sdhci_readl(host, SDHCI_HOST_CONTROL2 & (~0x3));
			
			ctrl_2 &= ~(SDHCI_CTRL_UHS_MASK << 16);
			if (ios->timing == MMC_TIMING_UHS_SDR12)
				ctrl_2 |= (SDHCI_CTRL_UHS_SDR12 << 16);
			else if (ios->timing == MMC_TIMING_UHS_SDR25)
				ctrl_2 |= (SDHCI_CTRL_UHS_SDR25 << 16);
			else if (ios->timing == MMC_TIMING_UHS_SDR50)
				ctrl_2 |= (SDHCI_CTRL_UHS_SDR50 << 16);
			else if (ios->timing == MMC_TIMING_UHS_SDR104)
				ctrl_2 |= (SDHCI_CTRL_UHS_SDR104 << 16);
			else if (ios->timing == MMC_TIMING_UHS_DDR50){
				ctrl_2 |= (SDHCI_CTRL_UHS_DDR50 << 16);
				
			#if defined( CONFIG_MMC_SDHCI_SC8825 )
				sdhci_writel(host, 0x18 , 0x0080);
				sdhci_writel(host, 0x07 , 0x0084);
				sdhci_writel(host, 0x05 , 0x0088);
			#elif defined (CONFIG_MMC_SDHCI_SCX35)
				sdhci_writel(host, 0x20 , 0x0080);
				sdhci_writel(host, 0x07 , 0x0084);
				sdhci_writel(host, 0x05 , 0x0088);
			#endif
			}
			sdhci_writel(host, ctrl_2, SDHCI_HOST_CONTROL2 & (~0x3));
#else
			ctrl_2 = sdhci_readw(host, SDHCI_HOST_CONTROL2);
			
			ctrl_2 &= ~SDHCI_CTRL_UHS_MASK;
			if (ios->timing == MMC_TIMING_MMC_HS200)
				ctrl_2 |= SDHCI_CTRL_HS_SDR200;
			else if (ios->timing == MMC_TIMING_UHS_SDR12)
				ctrl_2 |= SDHCI_CTRL_UHS_SDR12;
			else if (ios->timing == MMC_TIMING_UHS_SDR25)
				ctrl_2 |= SDHCI_CTRL_UHS_SDR25;
			else if (ios->timing == MMC_TIMING_UHS_SDR50)
				ctrl_2 |= SDHCI_CTRL_UHS_SDR50;
			else if (ios->timing == MMC_TIMING_UHS_SDR104)
				ctrl_2 |= SDHCI_CTRL_UHS_SDR104;
			else if (ios->timing == MMC_TIMING_UHS_DDR50)
				ctrl_2 |= SDHCI_CTRL_UHS_DDR50;
			sdhci_writew(host, ctrl_2, SDHCI_HOST_CONTROL2);
#endif
		}

		
		sdhci_sdclk_enable(host, 1);

		
		
		
		
	} else
		sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);

	if(host->quirks & SDHCI_QUIRK_RESET_CMD_DATA_ON_IOS)
		sdhci_reset(host, SDHCI_RESET_CMD | SDHCI_RESET_DATA);

	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);
}

static void sdhci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct sdhci_host *host = mmc_priv(mmc);

	sdhci_runtime_pm_get(host);
	sdhci_do_set_ios(host, ios);
	sdhci_runtime_pm_put(host);
}

static int sdhci_check_ro(struct sdhci_host *host)
{
	unsigned long flags;
	int is_readonly;

	spin_lock_irqsave(&host->lock, flags);

	if (host->flags & SDHCI_DEVICE_DEAD)
		is_readonly = 0;
	else if (host->ops->get_ro)
		is_readonly = host->ops->get_ro(host);
	else
		is_readonly = !(sdhci_readl(host, SDHCI_PRESENT_STATE)
				& SDHCI_WRITE_PROTECT);

	spin_unlock_irqrestore(&host->lock, flags);

	
	return host->quirks & SDHCI_QUIRK_INVERTED_WRITE_PROTECT ?
		!is_readonly : is_readonly;
}

#define SAMPLE_COUNT	5

static int sdhci_do_get_ro(struct sdhci_host *host)
{
	int i, ro_count;

	if (!(host->quirks & SDHCI_QUIRK_UNSTABLE_RO_DETECT))
		return sdhci_check_ro(host);

	ro_count = 0;
	for (i = 0; i < SAMPLE_COUNT; i++) {
		if (sdhci_check_ro(host)) {
			if (++ro_count > SAMPLE_COUNT / 2)
				return 1;
		}
		msleep(30);
	}
	return 0;
}

static void sdhci_hw_reset(struct mmc_host *mmc)
{
	struct sdhci_host *host = mmc_priv(mmc);
	
	mmc_power_off(mmc); 
	msleep(300);
	
	mmc_power_up(mmc); 
	sdhci_reset(host, SDHCI_RESET_CMD|SDHCI_RESET_DATA);
	printk("%s, ****************** %s ***********\n", mmc_hostname(mmc), __func__ );
}

static int sdhci_get_ro(struct mmc_host *mmc)
{
	struct sdhci_host *host = mmc_priv(mmc);
	int ret;

	sdhci_runtime_pm_get(host);
	ret = sdhci_do_get_ro(host);
	sdhci_runtime_pm_put(host);
	return ret;
}

static void sdhci_enable_sdio_irq_nolock(struct sdhci_host *host, int enable)
{
	if (host->flags & SDHCI_DEVICE_DEAD)
		goto out;

	if (enable)
		host->flags |= SDHCI_SDIO_IRQ_ENABLED;
	else
		host->flags &= ~SDHCI_SDIO_IRQ_ENABLED;

	
	if (host->runtime_suspended)
		goto out;

	if (enable)
		sdhci_unmask_irqs(host, SDHCI_INT_CARD_INT);
	else
		sdhci_mask_irqs(host, SDHCI_INT_CARD_INT);
out:
	mmiowb();
}

static void sdhci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct sdhci_host *host = mmc_priv(mmc);
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	sdhci_enable_sdio_irq_nolock(host, enable);
	spin_unlock_irqrestore(&host->lock, flags);
}

static int sdhci_do_start_signal_voltage_switch(struct sdhci_host *host,
						struct mmc_ios *ios)
{
	u8 pwr;
	u16 ctrl;
	u32 present_state;

	if (host->version < SDHCI_SPEC_300)
		return 0;

	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_330) {
		
		ctrl &= ~SDHCI_CTRL_VDD_180;
		sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);

		
		usleep_range(5000, 5500);

		
		ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		if (!(ctrl & SDHCI_CTRL_VDD_180))
			return 0;
		else {
			pr_info(DRIVER_NAME ": Switching to 3.3V "
				"signalling voltage failed\n");
			return -EIO;
		}
	} else if (!(ctrl & SDHCI_CTRL_VDD_180) &&
		  (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_180)) {
		
		sdhci_sdclk_enable(host, 0);

		
		present_state = sdhci_readl(host, SDHCI_PRESENT_STATE);
		if (!((present_state & SDHCI_DATA_LVL_MASK) >>
		       SDHCI_DATA_LVL_SHIFT)) {
			ctrl |= SDHCI_CTRL_VDD_180;
			sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);

			
			usleep_range(5000, 5500);

			ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
			if (ctrl & SDHCI_CTRL_VDD_180) {
				
				sdhci_sdclk_enable(host, 1);
				usleep_range(1000, 1500);

				present_state = sdhci_readl(host,
							SDHCI_PRESENT_STATE);
				if ((present_state & SDHCI_DATA_LVL_MASK) ==
				     SDHCI_DATA_LVL_MASK)
					return 0;
			}
		}

		pwr = sdhci_readb(host, SDHCI_POWER_CONTROL);
		pwr &= ~SDHCI_POWER_ON;
		sdhci_writeb(host, pwr, SDHCI_POWER_CONTROL);

		
		usleep_range(1000, 1500);
		pwr |= SDHCI_POWER_ON;
		sdhci_writeb(host, pwr, SDHCI_POWER_CONTROL);

		pr_info(DRIVER_NAME ": Switching to 1.8V signalling "
			"voltage failed, retrying with S18R set to 0\n");
		return -EAGAIN;
	} else
		
		return 0;
}

static int sdhci_start_signal_voltage_switch(struct mmc_host *mmc,
	struct mmc_ios *ios)
{
	struct sdhci_host *host = mmc_priv(mmc);
	int err;

	if (host->version < SDHCI_SPEC_300)
		return 0;
	sdhci_runtime_pm_get(host);
	err = sdhci_do_start_signal_voltage_switch(host, ios);
	sdhci_runtime_pm_put(host);
	return err;
}

#if defined( CONFIG_MMC_SDHCI_SC8825 ) || defined (CONFIG_MMC_SDHCI_SCX35)
static int sdhci_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	return 0;
}
#else
static int sdhci_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct sdhci_host *host;
	u16 ctrl;
	u32 ier;
	int tuning_loop_counter = MAX_TUNING_LOOP;
	unsigned long timeout;
	int err = 0;
	bool requires_tuning_nonuhs = false;

	host = mmc_priv(mmc);

	sdhci_runtime_pm_get(host);
	disable_irq(host->irq);
	spin_lock(&host->lock);

	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);

	if (((ctrl & SDHCI_CTRL_UHS_MASK) == SDHCI_CTRL_UHS_SDR50) &&
	    (host->flags & SDHCI_SDR50_NEEDS_TUNING ||
	     host->flags & SDHCI_HS200_NEEDS_TUNING))
		requires_tuning_nonuhs = true;

	if (((ctrl & SDHCI_CTRL_UHS_MASK) == SDHCI_CTRL_UHS_SDR104) ||
	    requires_tuning_nonuhs)
		ctrl |= SDHCI_CTRL_EXEC_TUNING;
	else {
		spin_unlock(&host->lock);
		enable_irq(host->irq);
		sdhci_runtime_pm_put(host);
		return 0;
	}

	sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);

	ier = sdhci_readl(host, SDHCI_INT_ENABLE);
	sdhci_clear_set_irqs(host, ier, SDHCI_INT_DATA_AVAIL);

	timeout = 150;
	do {
		struct mmc_command cmd = {0};
		struct mmc_request mrq = {NULL};

		if (!tuning_loop_counter && !timeout)
			break;

		cmd.opcode = opcode;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
		cmd.retries = 0;
		cmd.data = NULL;
		cmd.error = 0;

		mrq.cmd = &cmd;
		host->mrq = &mrq;

		if (cmd.opcode == MMC_SEND_TUNING_BLOCK_HS200) {
			if (mmc->ios.bus_width == MMC_BUS_WIDTH_8)
				sdhci_writew(host, SDHCI_MAKE_BLKSZ(7, 128),
					     SDHCI_BLOCK_SIZE);
			else if (mmc->ios.bus_width == MMC_BUS_WIDTH_4)
				sdhci_writew(host, SDHCI_MAKE_BLKSZ(7, 64),
					     SDHCI_BLOCK_SIZE);
		} else {
			sdhci_writew(host, SDHCI_MAKE_BLKSZ(7, 64),
				     SDHCI_BLOCK_SIZE);
		}

		sdhci_writew(host, SDHCI_TRNS_READ, SDHCI_TRANSFER_MODE);

		sdhci_send_command(host, &cmd);

		host->cmd = NULL;
		host->mrq = NULL;

		spin_unlock(&host->lock);
		enable_irq(host->irq);

		
		wait_event_interruptible_timeout(host->buf_ready_int,
					(host->tuning_done == 1),
					msecs_to_jiffies(50));
		disable_irq(host->irq);
		spin_lock(&host->lock);

		if (!host->tuning_done) {
			pr_info(DRIVER_NAME ": Timeout waiting for "
				"Buffer Read Ready interrupt during tuning "
				"procedure, falling back to fixed sampling "
				"clock\n");
			ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
			ctrl &= ~SDHCI_CTRL_TUNED_CLK;
			ctrl &= ~SDHCI_CTRL_EXEC_TUNING;
			sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);

			err = -EIO;
			goto out;
		}

		host->tuning_done = 0;

		ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		tuning_loop_counter--;
		timeout--;
		mdelay(1);
	} while (ctrl & SDHCI_CTRL_EXEC_TUNING);

	if (!tuning_loop_counter || !timeout) {
		ctrl &= ~SDHCI_CTRL_TUNED_CLK;
		sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);
	} else {
		if (!(ctrl & SDHCI_CTRL_TUNED_CLK)) {
			pr_info(DRIVER_NAME ": Tuning procedure"
				" failed, falling back to fixed sampling"
				" clock\n");
			err = -EIO;
		}
	}

out:
	if (!(host->flags & SDHCI_NEEDS_RETUNING) && host->tuning_count &&
	    (host->tuning_mode == SDHCI_TUNING_MODE_1)) {
		mod_timer(&host->tuning_timer, jiffies +
			host->tuning_count * HZ);
		
		mmc->max_blk_count = (4 * 1024 * 1024) / mmc->max_blk_size;
	} else {
		host->flags &= ~SDHCI_NEEDS_RETUNING;
		
		if (host->tuning_mode == SDHCI_TUNING_MODE_1)
			mod_timer(&host->tuning_timer, jiffies +
				host->tuning_count * HZ);
	}

	if (err && host->tuning_count &&
	    host->tuning_mode == SDHCI_TUNING_MODE_1)
		err = 0;

	sdhci_clear_set_irqs(host, SDHCI_INT_DATA_AVAIL, ier);
	spin_unlock(&host->lock);
	enable_irq(host->irq);
	sdhci_runtime_pm_put(host);

	return err;
}
#endif

static void sdhci_do_enable_preset_value(struct sdhci_host *host, bool enable)
{
#if defined( CONFIG_MMC_SDHCI_SC8825 ) || defined (CONFIG_MMC_SDHCI_SCX35)
	u32 ctrl;
#else
	u16 ctrl;
#endif
	unsigned long flags;

	
	if (host->version < SDHCI_SPEC_300)
		return;

	spin_lock_irqsave(&host->lock, flags);

	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);

	if (enable && !(ctrl & SDHCI_CTRL_PRESET_VAL_ENABLE)) {
#if defined( CONFIG_MMC_SDHCI_SC8825 ) || defined (CONFIG_MMC_SDHCI_SCX35)
		ctrl |= SDHCI_CTRL_PRESET_VAL_ENABLE << 16;
		sdhci_writel(host, ctrl, SDHCI_HOST_CONTROL2 & (~0x3));
#else
		ctrl |= SDHCI_CTRL_PRESET_VAL_ENABLE;
		sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);
#endif
	} else if (!enable && (ctrl & SDHCI_CTRL_PRESET_VAL_ENABLE)) {
#if defined( CONFIG_MMC_SDHCI_SC8825 ) || defined (CONFIG_MMC_SDHCI_SCX35)
		ctrl &= ~(SDHCI_CTRL_PRESET_VAL_ENABLE << 16);
		sdhci_writel(host, ctrl, SDHCI_HOST_CONTROL2 & (~0x3));
#else
		ctrl &= ~SDHCI_CTRL_PRESET_VAL_ENABLE;
		sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);
#endif
	}

	spin_unlock_irqrestore(&host->lock, flags);
}

static void sdhci_enable_preset_value(struct mmc_host *mmc, bool enable)
{
	struct sdhci_host *host = mmc_priv(mmc);

	sdhci_runtime_pm_get(host);
	sdhci_do_enable_preset_value(host, enable);
	sdhci_runtime_pm_put(host);
}

static const struct mmc_host_ops sdhci_ops = {
	.request			= sdhci_request,
	.set_ios			= sdhci_set_ios,
	.get_ro				= sdhci_get_ro,
#ifndef CONFIG_MACH_SPX35FPGA
	.hw_reset			= sdhci_hw_reset,
#endif
	.enable_sdio_irq		= sdhci_enable_sdio_irq,
	.start_signal_voltage_switch	= sdhci_start_signal_voltage_switch,
	
	.enable_preset_value		= sdhci_enable_preset_value,
};


#ifdef CONFIG_MMC_CARD_HOTPLUG
static void sdhci_tasklet_card(unsigned long param)
{
	struct sdhci_host *host;
	unsigned long flags;

	host = (struct sdhci_host*)param;

	spin_lock_irqsave(&host->lock, flags);

	
	if (host->mrq &&
	    !(sdhci_readl(host, SDHCI_PRESENT_STATE) & SDHCI_CARD_PRESENT)) {
		pr_err("%s: Card removed during transfer!\n",
			mmc_hostname(host->mmc));
		pr_err("%s: Resetting controller.\n",
			mmc_hostname(host->mmc));

		sdhci_reset(host, SDHCI_RESET_CMD);
		sdhci_reset(host, SDHCI_RESET_DATA);

		host->mrq->cmd->error = -ENOMEDIUM;
		tasklet_schedule(&host->finish_tasklet);
	}

	spin_unlock_irqrestore(&host->lock, flags);

	wake_lock_timeout(&sdhci_detect_lock, 5*HZ);

	mmc_detect_change(host->mmc, msecs_to_jiffies(200));
}
#endif

static void sdhci_tasklet_finish(unsigned long param)
{
	struct sdhci_host *host;
	unsigned long flags;
	struct mmc_request *mrq;

	host = (struct sdhci_host*)param;

	spin_lock_irqsave(&host->lock, flags);

	if (!host->mrq) {
		spin_unlock_irqrestore(&host->lock, flags);
		return;
	}

	del_timer(&host->timer);

	mrq = host->mrq;

	if(mrq){
	if (!(host->flags & SDHCI_DEVICE_DEAD) &&
	    ((mrq->cmd && mrq->cmd->error) ||
		 (mrq->data && (mrq->data->error ||
		  (mrq->data->stop && mrq->data->stop->error))) ||
		   (host->quirks & SDHCI_QUIRK_RESET_AFTER_REQUEST))) {

		
		if (host->quirks & SDHCI_QUIRK_CLOCK_BEFORE_RESET) {
			unsigned int clock;

			
			clock = host->clock;
			host->clock = 0;
			sdhci_set_clock(host, clock);
		}

		sdhci_reset(host, SDHCI_RESET_CMD);
		sdhci_reset(host, SDHCI_RESET_DATA);
	}

	}
	host->mrq = NULL;
	host->cmd = NULL;
	host->data = NULL;

#ifdef SDHCI_USE_LEDS_CLASS
	sdhci_deactivate_led(host);
#endif

	sdhci_reset(host, SDHCI_RESET_CMD);
	sdhci_reset(host, SDHCI_RESET_DATA);
	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);

	mmc_request_done(host->mmc, mrq);
	sdhci_runtime_pm_put(host);
}

static void sdhci_timeout_timer(unsigned long data)
{
	struct sdhci_host *host;
	unsigned long flags;

	host = (struct sdhci_host*)data;

	spin_lock_irqsave(&host->lock, flags);
	pr_err("!!!! %s: %s timeout !!!!\n", mmc_hostname(host->mmc),
				host->suspending?"supsend":"command");
	sdhci_dumpregs(host);
	if (host->mrq) {
		pr_err("%s: Timeout waiting for hardware "
			"interrupt.\n", mmc_hostname(host->mmc));
		if(host->cmd){
			printk(KERN_ERR "%s, cmd:%d timeout\n",
					mmc_hostname(host->mmc), host->cmd->opcode);
		}
		if (host->data) {
			host->data->error = -ETIMEDOUT;
			sdhci_finish_data(host);
		} else {
			if (host->cmd)
				host->cmd->error = -ETIMEDOUT;
			else
				host->mrq->cmd->error = -ETIMEDOUT;

			tasklet_schedule(&host->finish_tasklet);
		}
	}

	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);
}

static void sdhci_tuning_timer(unsigned long data)
{
	struct sdhci_host *host;
	unsigned long flags;

	host = (struct sdhci_host *)data;

	spin_lock_irqsave(&host->lock, flags);

	host->flags |= SDHCI_NEEDS_RETUNING;

	spin_unlock_irqrestore(&host->lock, flags);
}


static void sdhci_cmd_irq(struct sdhci_host *host, u32 intmask)
{
	BUG_ON(intmask == 0);

	if (!host->cmd) {
		pr_err("%s: Got command interrupt 0x%08x even "
			"though no command operation was in progress.\n",
			mmc_hostname(host->mmc), (unsigned)intmask);
		sdhci_dumpregs(host);
		return;
	}

	if (intmask & SDHCI_INT_TIMEOUT)
		host->cmd->error = -ETIMEDOUT;
	else if (intmask & (SDHCI_INT_CRC | SDHCI_INT_END_BIT |
			SDHCI_INT_INDEX))
		host->cmd->error = -EILSEQ;

	if (host->cmd->error) {
		if(host->mmc->card){
			pr_err("%s: !!!!! error in sending cmd:%d, int:0x%x, err:%d \n",
				mmc_hostname(host->mmc), host->cmd->opcode, intmask,
								host->cmd->error);
			sdhci_dumpregs(host);
		}
		tasklet_schedule(&host->finish_tasklet);
		return;
	}

	if (host->cmd->flags & MMC_RSP_BUSY) {
		if (host->cmd->data)
			DBG("Cannot wait for busy signal when also "
				"doing a data transfer");
		else if (!(host->quirks & SDHCI_QUIRK_NO_BUSY_IRQ))
			return;

	}

	if (intmask & SDHCI_INT_RESPONSE)
		sdhci_finish_command(host);
}

static void sdhci_show_adma_error(struct sdhci_host *host) { }

static void sdhci_data_irq(struct sdhci_host *host, u32 intmask)
{
	u32 command;
	BUG_ON(intmask == 0);

	
	if (intmask & SDHCI_INT_DATA_AVAIL) {
		command = SDHCI_GET_CMD(sdhci_readw(host, SDHCI_COMMAND));
		if (command == MMC_SEND_TUNING_BLOCK ||
		    command == MMC_SEND_TUNING_BLOCK_HS200) {
			host->tuning_done = 1;
			wake_up(&host->buf_ready_int);
			return;
		}
	}

	if (!host->data) {
		if (host->cmd && (host->cmd->flags & MMC_RSP_BUSY)) {
			if (intmask & SDHCI_INT_DATA_END) {
				sdhci_finish_command(host);
				return;
			}
		}

		pr_err("%s: Got data interrupt 0x%08x even "
			"though no data operation was in progress.\n",
			mmc_hostname(host->mmc), (unsigned)intmask);
		sdhci_dumpregs(host);

		return;
	}

	if (intmask & SDHCI_INT_DATA_TIMEOUT) {
		if ((intmask & SDHCI_INT_DATA_END) == 0)
			host->data->error = -ETIMEDOUT;
	}
	else if (intmask & SDHCI_INT_DATA_END_BIT)
		host->data->error = -EILSEQ;
	else if ((intmask & SDHCI_INT_DATA_CRC) &&
		SDHCI_GET_CMD(sdhci_readw(host, SDHCI_COMMAND))
			!= MMC_BUS_TEST_R)
		host->data->error = -EILSEQ;
	else if (intmask & SDHCI_INT_ADMA_ERROR) {
		pr_err("%s: ADMA error\n", mmc_hostname(host->mmc));
		sdhci_show_adma_error(host);
		host->data->error = -EIO;
	}

	if (host->data->error){
		printk("%s: !!!!! error in sending data, int:0x%x, err:%d \n",
				mmc_hostname(host->mmc), intmask, host->data->error);
		sdhci_dumpregs(host);
		sdhci_finish_data(host);
	}
	else {
		if (intmask & (SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL))
			sdhci_transfer_pio(host);

		if (intmask & SDHCI_INT_DMA_END) {
			u32 dmastart, dmanow;
			dmastart = sg_dma_address(host->data->sg);
			dmanow = dmastart + host->data->bytes_xfered;
			dmanow = (dmanow &
				~(SDHCI_DEFAULT_BOUNDARY_SIZE - 1)) +
				SDHCI_DEFAULT_BOUNDARY_SIZE;
			host->data->bytes_xfered = dmanow - dmastart;
			DBG("%s: DMA base 0x%08x, transferred 0x%06x bytes,"
				" next 0x%08x\n",
				mmc_hostname(host->mmc), dmastart,
				host->data->bytes_xfered, dmanow);
			sdhci_writel(host, dmanow, SDHCI_DMA_ADDRESS);
		}

		if (intmask & SDHCI_INT_DATA_END) {
			if (host->cmd) {
				host->data_early = 1;
			} else {
				sdhci_finish_data(host);
			}
		}
	}
}

static irqreturn_t sdhci_irq(int irq, void *dev_id)
{
	irqreturn_t result;
	struct sdhci_host *host = dev_id;
	u32 intmask, unexpected = 0;
	int cardint = 0, max_loops = 16;

	spin_lock(&host->lock);

	if (host->runtime_suspended) {
		spin_unlock(&host->lock);
		pr_warning("%s: got irq while runtime suspended\n",
		       mmc_hostname(host->mmc));
		return IRQ_HANDLED;
	}

	intmask = sdhci_readl(host, SDHCI_INT_STATUS);

	if (!intmask || intmask == 0xffffffff) {
		result = IRQ_NONE;
		goto out;
	}

again:
#if 0
	DBG("*** %s got interrupt: 0x%08x\n",
		mmc_hostname(host->mmc), intmask);


	if (intmask & (SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_REMOVE)) {
		u32 present = sdhci_readl(host, SDHCI_PRESENT_STATE) &
			      SDHCI_CARD_PRESENT;

		sdhci_mask_irqs(host, present ? SDHCI_INT_CARD_INSERT :
						SDHCI_INT_CARD_REMOVE);
		sdhci_unmask_irqs(host, present ? SDHCI_INT_CARD_REMOVE :
						  SDHCI_INT_CARD_INSERT);

		sdhci_writel(host, intmask & (SDHCI_INT_CARD_INSERT |
			     SDHCI_INT_CARD_REMOVE), SDHCI_INT_STATUS);
		intmask &= ~(SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_REMOVE);
		tasklet_schedule(&host->card_tasklet);
	}
#endif
	intmask &= ~(SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_REMOVE);

	if (intmask & SDHCI_INT_CMD_MASK) {
		sdhci_writel(host, intmask & SDHCI_INT_CMD_MASK,
			SDHCI_INT_STATUS);
		sdhci_cmd_irq(host, intmask & SDHCI_INT_CMD_MASK);
	}

	if (intmask & SDHCI_INT_DATA_MASK) {
		sdhci_writel(host, intmask & SDHCI_INT_DATA_MASK,
			SDHCI_INT_STATUS);
		sdhci_data_irq(host, intmask & SDHCI_INT_DATA_MASK);
	}

	intmask &= ~(SDHCI_INT_CMD_MASK | SDHCI_INT_DATA_MASK);

	intmask &= ~SDHCI_INT_ERROR;

	if (intmask & SDHCI_INT_BUS_POWER) {
		pr_err("%s: Card is consuming too much power!\n",
			mmc_hostname(host->mmc));
		sdhci_writel(host, SDHCI_INT_BUS_POWER, SDHCI_INT_STATUS);
	}

	intmask &= ~SDHCI_INT_BUS_POWER;

	if (intmask & SDHCI_INT_CARD_INT)
		cardint = 1;

	intmask &= ~SDHCI_INT_CARD_INT;

	if (intmask) {
		unexpected |= intmask;
		sdhci_writel(host, intmask, SDHCI_INT_STATUS);
	}

	result = IRQ_HANDLED;

	intmask = sdhci_readl(host, SDHCI_INT_STATUS);
	if (intmask && --max_loops)
		goto again;
out:
	spin_unlock(&host->lock);

	if (unexpected) {
		pr_err("%s: Unexpected interrupt 0x%08x.\n",
			   mmc_hostname(host->mmc), unexpected);
		sdhci_dumpregs(host);
	}
	if (cardint)
		mmc_signal_sdio_irq(host->mmc);

	return result;
}


#ifdef CONFIG_PM

int sdhci_suspend_host(struct sdhci_host *host, pm_message_t state)
{
	int ret = 0;
#ifdef CONFIG_MMC_CARD_HOTPLUG
	sdhci_disable_card_detection(host);
#endif
	
	if (host->flags & SDHCI_USING_RETUNING_TIMER) {
		del_timer_sync(&host->tuning_timer);
		host->flags &= ~SDHCI_NEEDS_RETUNING;
	}

	
	host->suspending = 1;
	if(!is_wifi_slot(host) && !is_cbp_slot(host))
	{
		ret = mmc_suspend_host(host->mmc);
	}
	if (ret){
		printk("=== wow~ %s suspend error:%d ===\n",
					mmc_hostname(host->mmc), ret);

		if (host->flags & SDHCI_USING_RETUNING_TIMER) {
			host->flags |= SDHCI_NEEDS_RETUNING;
			mod_timer(&host->tuning_timer, jiffies +
					host->tuning_count * HZ);
		}

		sdhci_enable_card_detection(host);

		return ret;
	}

	free_irq(host->irq, host);

#if 0
	if (host->vmmc)
		ret = regulator_disable(host->vmmc);
#endif
	return ret;
}

EXPORT_SYMBOL_GPL(sdhci_suspend_host);

int sdhci_resume_host(struct sdhci_host *host)
{
	int ret = 0;
#if 0
	if (host->vmmc ) {
		int ret = regulator_enable(host->vmmc);
		if (ret)
			return ret;
	}
#endif
	host->suspending = 0;

	if (host->flags & (SDHCI_USE_SDMA | SDHCI_USE_ADMA)) {
		if (host->ops->enable_dma)
			host->ops->enable_dma(host);
	}

	sdhci_init(host, (host->mmc->pm_flags & MMC_PM_KEEP_POWER));
	if(is_cbp_slot(host) && (host->flags & SDHCI_SDIO_IRQ_ENABLED)) {
		sdhci_enable_sdio_irq_nolock(host, true);
	}
	mmiowb();

	ret = request_irq(host->irq, sdhci_irq, IRQF_SHARED,
					  mmc_hostname(host->mmc), host);
	if (ret)
		return ret;
	
	if(!is_wifi_slot(host) && !is_cbp_slot(host))
	{
		ret = mmc_resume_host(host->mmc);
		if (ret){
			return ret;
		}
	}

	if(!(host->mmc->card)){
		
		sdhci_set_power(host, -1);
	}
#ifdef CONFIG_MMC_CARD_HOTPLUG
	sdhci_enable_card_detection(host);
#endif

	
	if (host->flags & SDHCI_USING_RETUNING_TIMER)
		host->flags |= SDHCI_NEEDS_RETUNING;

	return ret;
}

EXPORT_SYMBOL_GPL(sdhci_resume_host);

void sdhci_enable_irq_wakeups(struct sdhci_host *host)
{
	u8 val;
	val = sdhci_readb(host, SDHCI_WAKE_UP_CONTROL);
	val |= SDHCI_WAKE_ON_INT;
	sdhci_writeb(host, val, SDHCI_WAKE_UP_CONTROL);
}

EXPORT_SYMBOL_GPL(sdhci_enable_irq_wakeups);

#endif 

#ifdef CONFIG_PM_RUNTIME

static int sdhci_runtime_pm_get(struct sdhci_host *host)
{
	return pm_runtime_get_sync(host->mmc->parent);
}

static int sdhci_runtime_pm_put(struct sdhci_host *host)
{
	pm_runtime_mark_last_busy(host->mmc->parent);
	return pm_runtime_put_autosuspend(host->mmc->parent);
}

int sdhci_runtime_suspend_host(struct sdhci_host *host)
{
	unsigned long flags;
	int ret = 0;

	
	if (host->flags & SDHCI_USING_RETUNING_TIMER) {
		del_timer_sync(&host->tuning_timer);
		host->flags &= ~SDHCI_NEEDS_RETUNING;
	}

	spin_lock_irqsave(&host->lock, flags);
	sdhci_mask_irqs(host, SDHCI_INT_ALL_MASK);
	spin_unlock_irqrestore(&host->lock, flags);

	synchronize_irq(host->irq);

	spin_lock_irqsave(&host->lock, flags);
	host->runtime_suspended = true;
	spin_unlock_irqrestore(&host->lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(sdhci_runtime_suspend_host);

int sdhci_runtime_resume_host(struct sdhci_host *host)
{
	unsigned long flags;
	int ret = 0, host_flags = host->flags;

	if (host_flags & (SDHCI_USE_SDMA | SDHCI_USE_ADMA)) {
		if (host->ops->enable_dma)
			host->ops->enable_dma(host);
	}

	
	if (host->mmc->pm_flags & MMC_PM_DISABLE_TIMEOUT_IRQ) {
		sdhci_clear_set_irqs(host, SDHCI_INT_ALL_MASK,
			SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
			SDHCI_INT_DATA_CRC | SDHCI_INT_INDEX |
			SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
			SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE);
	}
	else {
		sdhci_clear_set_irqs(host, SDHCI_INT_ALL_MASK,
			SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
			SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
			SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
			SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE);
	}
	#if 0  
	
	host->pwr = 0;
	host->clock = 0;
	sdhci_do_set_ios(host, &host->mmc->ios);
	#endif
	sdhci_do_start_signal_voltage_switch(host, &host->mmc->ios);
	if (host_flags & SDHCI_PV_ENABLED)
		sdhci_do_enable_preset_value(host, true);

	
	if (host->flags & SDHCI_USING_RETUNING_TIMER)
		host->flags |= SDHCI_NEEDS_RETUNING;

	spin_lock_irqsave(&host->lock, flags);

	host->runtime_suspended = false;

	
	if ((host->flags & SDHCI_SDIO_IRQ_ENABLED))
		sdhci_enable_sdio_irq_nolock(host, true);

	
	sdhci_enable_card_detection(host);

	spin_unlock_irqrestore(&host->lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(sdhci_runtime_resume_host);

#endif 



struct sdhci_host *sdhci_alloc_host(struct device *dev,
	size_t priv_size)
{
	struct mmc_host *mmc;
	struct sdhci_host *host;

	WARN_ON(dev == NULL);

	mmc = mmc_alloc_host(sizeof(struct sdhci_host) + priv_size, dev);
	if (!mmc)
		return ERR_PTR(-ENOMEM);

	host = mmc_priv(mmc);
	host->mmc = mmc;

	return host;
}

EXPORT_SYMBOL_GPL(sdhci_alloc_host);

int sdhci_add_host(struct sdhci_host *host)
{
	struct mmc_host *mmc;
	u32 caps[2];
	u32 max_current_caps;
	unsigned int ocr_avail;
	struct sprd_host_data *host_data;
#ifdef CONFIG_MMC_CARD_HOTPLUG
	int detect_irq;
#endif
	int ret;

	WARN_ON(host == NULL);
	if (host == NULL)
		return -EINVAL;

	mmc = host->mmc;

	if (debug_quirks)
		host->quirks = debug_quirks;
	if (debug_quirks2)
		host->quirks2 = debug_quirks2;

	sdhci_reset(host, SDHCI_RESET_ALL);

	host->version = sdhci_readw(host, SDHCI_HOST_VERSION);
	host->version = (host->version & SDHCI_SPEC_VER_MASK)
				>> SDHCI_SPEC_VER_SHIFT;
	if (host->version > SDHCI_SPEC_300) {
		pr_err("%s: Unknown controller version (%d). "
			"You may experience problems.\n", mmc_hostname(mmc),
			host->version);
	}

	caps[0] = (host->quirks & SDHCI_QUIRK_MISSING_CAPS) ? host->caps :
		sdhci_readl(host, SDHCI_CAPABILITIES);

	caps[1] = (host->version >= SDHCI_SPEC_300) ?
		sdhci_readl(host, SDHCI_CAPABILITIES_1) : 0;

	if (host->quirks & SDHCI_QUIRK_FORCE_DMA)
		host->flags |= SDHCI_USE_SDMA;
	else if (!(caps[0] & SDHCI_CAN_DO_SDMA))
		DBG("Controller doesn't have SDMA capability\n");
	else
		host->flags |= SDHCI_USE_SDMA;

	if ((host->quirks & SDHCI_QUIRK_BROKEN_DMA) &&
		(host->flags & SDHCI_USE_SDMA)) {
		DBG("Disabling DMA as it is marked broken\n");
		host->flags &= ~SDHCI_USE_SDMA;
	}

	if ((host->version >= SDHCI_SPEC_200) &&
		(caps[0] & SDHCI_CAN_DO_ADMA2))
		host->flags |= SDHCI_USE_ADMA;

	if ((host->quirks & SDHCI_QUIRK_BROKEN_ADMA) &&
		(host->flags & SDHCI_USE_ADMA)) {
		DBG("Disabling ADMA as it is marked broken\n");
		host->flags &= ~SDHCI_USE_ADMA;
	}

	if (host->flags & (SDHCI_USE_SDMA | SDHCI_USE_ADMA)) {
		if (host->ops->enable_dma) {
			if (host->ops->enable_dma(host)) {
				pr_warning("%s: No suitable DMA "
					"available. Falling back to PIO.\n",
					mmc_hostname(mmc));
				host->flags &=
					~(SDHCI_USE_SDMA | SDHCI_USE_ADMA);
			}
		}
	}

	if (host->flags & SDHCI_USE_ADMA) {
		host->adma_desc = kmalloc((128 * 2 + 1) * 4, GFP_KERNEL);
		host->align_buffer = kmalloc(128 * 4, GFP_KERNEL);
		if (!host->adma_desc || !host->align_buffer) {
			kfree(host->adma_desc);
			kfree(host->align_buffer);
			pr_warning("%s: Unable to allocate ADMA "
				"buffers. Falling back to standard DMA.\n",
				mmc_hostname(mmc));
			host->flags &= ~SDHCI_USE_ADMA;
		}
	}

	if (!(host->flags & (SDHCI_USE_SDMA | SDHCI_USE_ADMA))) {
		host->dma_mask = DMA_BIT_MASK(64);
		mmc_dev(host->mmc)->dma_mask = &host->dma_mask;
	}

	if (host->version >= SDHCI_SPEC_300)
		host->max_clk = (caps[0] & SDHCI_CLOCK_V3_BASE_MASK)
			>> SDHCI_CLOCK_BASE_SHIFT;
	else
		host->max_clk = (caps[0] & SDHCI_CLOCK_BASE_MASK)
			>> SDHCI_CLOCK_BASE_SHIFT;

	host->max_clk *= 1000000;
	if (host->max_clk == 0 || host->quirks &
			SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN) {
		if (!host->ops->get_max_clock) {
			pr_err("%s: Hardware doesn't specify base clock "
			       "frequency.\n", mmc_hostname(mmc));
			return -ENODEV;
		}
		host->max_clk = host->ops->get_max_clock(host);
	}

	host->clk_mul = (caps[1] & SDHCI_CLOCK_MUL_MASK) >>
			SDHCI_CLOCK_MUL_SHIFT;

	if (host->clk_mul)
		host->clk_mul += 1;

	mmc->ops = &sdhci_ops;
	mmc->f_max = host->max_clk;
	if (host->ops->get_min_clock)
		mmc->f_min = host->ops->get_min_clock(host);
	else if (host->version >= SDHCI_SPEC_300) {
		if (host->clk_mul) {
			mmc->f_min = (host->max_clk * host->clk_mul) / 1024;
			mmc->f_max = host->max_clk * host->clk_mul;
		} else
			mmc->f_min = host->max_clk / SDHCI_MAX_DIV_SPEC_300;
	} else
		mmc->f_min = host->max_clk / SDHCI_MAX_DIV_SPEC_200;

	host->timeout_clk =
		(caps[0] & SDHCI_TIMEOUT_CLK_MASK) >> SDHCI_TIMEOUT_CLK_SHIFT;
	if (host->timeout_clk == 0) {
		if (host->ops->get_timeout_clock) {
			host->timeout_clk = host->ops->get_timeout_clock(host);
		} else if (!(host->quirks &
				SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK)) {
			pr_err("%s: Hardware doesn't specify timeout clock "
			       "frequency.\n", mmc_hostname(mmc));
			return -ENODEV;
		}
	}
	if (caps[0] & SDHCI_TIMEOUT_CLK_UNIT)
		host->timeout_clk *= 1000;

	if (host->quirks & SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK)
		host->timeout_clk = mmc->f_max / 1000;

 	mmc->max_discard_to = (1 << 27) / host->timeout_clk;
	printk(KERN_INFO "--------max_discard_to init %d",mmc->max_discard_to);

	mmc->caps |= MMC_CAP_SDIO_IRQ | MMC_CAP_ERASE | MMC_CAP_CMD23;

	if (host->quirks & SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12)
		host->flags |= SDHCI_AUTO_CMD12;

	
	if ((host->version >= SDHCI_SPEC_300) &&
	    ((host->flags & SDHCI_USE_ADMA) ||
	     !(host->flags & SDHCI_USE_SDMA))) {
		host->flags |= SDHCI_AUTO_CMD23;
		DBG("%s: Auto-CMD23 available\n", mmc_hostname(mmc));
	} else {
		DBG("%s: Auto-CMD23 unavailable\n", mmc_hostname(mmc));
	}

	if (!(host->quirks & SDHCI_QUIRK_FORCE_1_BIT_DATA))
		mmc->caps |= MMC_CAP_4_BIT_DATA;

	if (caps[0] & SDHCI_CAN_DO_HISPD)
		mmc->caps |= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED;

	if ((host->quirks & SDHCI_QUIRK_BROKEN_CARD_DETECTION) &&
	    mmc_card_is_removable(mmc))
		mmc->caps |= MMC_CAP_NEEDS_POLL;

	
	if (caps[1] & (SDHCI_SUPPORT_SDR104 | SDHCI_SUPPORT_SDR50 |
		       SDHCI_SUPPORT_DDR50))
		mmc->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25;

	
	if (caps[1] & SDHCI_SUPPORT_SDR104)
		mmc->caps |= MMC_CAP_UHS_SDR104 | MMC_CAP_UHS_SDR50;
	else if (caps[1] & SDHCI_SUPPORT_SDR50)
		mmc->caps |= MMC_CAP_UHS_SDR50;

	if (caps[1] & SDHCI_SUPPORT_DDR50)
		mmc->caps |= MMC_CAP_UHS_DDR50;

	
	if (caps[1] & SDHCI_USE_SDR50_TUNING)
		host->flags |= SDHCI_SDR50_NEEDS_TUNING;

	
	if (mmc->caps2 & MMC_CAP2_HS200)
		host->flags |= SDHCI_HS200_NEEDS_TUNING;

	
	if (caps[1] & SDHCI_DRIVER_TYPE_A)
		mmc->caps |= MMC_CAP_DRIVER_TYPE_A;
	if (caps[1] & SDHCI_DRIVER_TYPE_C)
		mmc->caps |= MMC_CAP_DRIVER_TYPE_C;
	if (caps[1] & SDHCI_DRIVER_TYPE_D)
		mmc->caps |= MMC_CAP_DRIVER_TYPE_D;

	if (mmc->caps2 & MMC_CAP2_POWEROFF_NOTIFY)
		mmc->power_notify_type = MMC_HOST_PW_NOTIFY_SHORT;
	else
		mmc->power_notify_type = MMC_HOST_PW_NOTIFY_NONE;

	
	host->tuning_count = (caps[1] & SDHCI_RETUNING_TIMER_COUNT_MASK) >>
			      SDHCI_RETUNING_TIMER_COUNT_SHIFT;

	if (host->tuning_count)
		host->tuning_count = 1 << (host->tuning_count - 1);

	
	host->tuning_mode = (caps[1] & SDHCI_RETUNING_MODE_MASK) >>
			     SDHCI_RETUNING_MODE_SHIFT;

	ocr_avail = 0;
	max_current_caps = sdhci_readl(host, SDHCI_MAX_CURRENT);

	if (caps[0] & SDHCI_CAN_VDD_330) {
		int max_current_330;

		ocr_avail |= MMC_VDD_32_33 | MMC_VDD_33_34;

		max_current_330 = ((max_current_caps &
				   SDHCI_MAX_CURRENT_330_MASK) >>
				   SDHCI_MAX_CURRENT_330_SHIFT) *
				   SDHCI_MAX_CURRENT_MULTIPLIER;

		if (max_current_330 > 150)
			mmc->caps |= MMC_CAP_SET_XPC_330;
	}
	if (caps[0] & SDHCI_CAN_VDD_300) {
		int max_current_300;

		ocr_avail |= MMC_VDD_29_30 | MMC_VDD_30_31;

		max_current_300 = ((max_current_caps &
				   SDHCI_MAX_CURRENT_300_MASK) >>
				   SDHCI_MAX_CURRENT_300_SHIFT) *
				   SDHCI_MAX_CURRENT_MULTIPLIER;

		if (max_current_300 > 150)
			mmc->caps |= MMC_CAP_SET_XPC_300;
	}
	if (caps[0] & SDHCI_CAN_VDD_180) {
		int max_current_180;

		ocr_avail |= MMC_VDD_165_195;

		max_current_180 = ((max_current_caps &
				   SDHCI_MAX_CURRENT_180_MASK) >>
				   SDHCI_MAX_CURRENT_180_SHIFT) *
				   SDHCI_MAX_CURRENT_MULTIPLIER;

		if (max_current_180 > 150)
			mmc->caps |= MMC_CAP_SET_XPC_180;

		
		if (max_current_180 >= 800)
			mmc->caps |= MMC_CAP_MAX_CURRENT_800;
		else if (max_current_180 >= 600)
			mmc->caps |= MMC_CAP_MAX_CURRENT_600;
		else if (max_current_180 >= 400)
			mmc->caps |= MMC_CAP_MAX_CURRENT_400;
		else
			mmc->caps |= MMC_CAP_MAX_CURRENT_200;
	}

	mmc->ocr_avail = ocr_avail;
	mmc->ocr_avail_sdio = ocr_avail;
	if (host->ocr_avail_sdio)
		mmc->ocr_avail_sdio &= host->ocr_avail_sdio;
	mmc->ocr_avail_sd = ocr_avail;
	if (host->ocr_avail_sd)
		mmc->ocr_avail_sd &= host->ocr_avail_sd;
	else 
		mmc->ocr_avail_sd &= ~MMC_VDD_165_195;
	mmc->ocr_avail_mmc = ocr_avail;
	if (host->ocr_avail_mmc)
		mmc->ocr_avail_mmc &= host->ocr_avail_mmc;

	if (mmc->ocr_avail == 0) {
		pr_err("%s: Hardware doesn't report any "
			"support voltages.\n", mmc_hostname(mmc));
		return -ENODEV;
	}

	spin_lock_init(&host->lock);

	if (host->flags & SDHCI_USE_ADMA)
		mmc->max_segs = 128;
	else if (host->flags & SDHCI_USE_SDMA)
		mmc->max_segs = 1;
	else 
		mmc->max_segs = 128;

	mmc->max_req_size = 524288;

	if (host->flags & SDHCI_USE_ADMA) {
		if (host->quirks & SDHCI_QUIRK_BROKEN_ADMA_ZEROLEN_DESC)
			mmc->max_seg_size = 65535;
		else
			mmc->max_seg_size = 65536;
	} else {
		mmc->max_seg_size = mmc->max_req_size;
	}

	if (host->quirks & SDHCI_QUIRK_FORCE_BLK_SZ_2048) {
		mmc->max_blk_size = 2;
	} else {
		mmc->max_blk_size = (caps[0] & SDHCI_MAX_BLOCK_MASK) >>
				SDHCI_MAX_BLOCK_SHIFT;
		if (mmc->max_blk_size >= 3) {
			pr_warning("%s: Invalid maximum block size, "
				"assuming 512 bytes\n", mmc_hostname(mmc));
			mmc->max_blk_size = 0;
		}
	}

	mmc->max_blk_size = 512 << mmc->max_blk_size;

	mmc->max_blk_count = (host->quirks & SDHCI_QUIRK_NO_MULTIBLOCK) ? 1 : 65535;

#ifdef CONFIG_MMC_CARD_HOTPLUG
	tasklet_init(&host->card_tasklet,
		sdhci_tasklet_card, (unsigned long)host);
#endif
	tasklet_init(&host->finish_tasklet,
		sdhci_tasklet_finish, (unsigned long)host);

	setup_timer(&host->timer, sdhci_timeout_timer, (unsigned long)host);

	if (host->version >= SDHCI_SPEC_300) {
		init_waitqueue_head(&host->buf_ready_int);

		
		init_timer(&host->tuning_timer);
		host->tuning_timer.data = (unsigned long)host;
		host->tuning_timer.function = sdhci_tuning_timer;
	}

	ret = request_irq(host->irq, sdhci_irq, IRQF_SHARED,
		mmc_hostname(mmc), host);
	if (ret)
		goto untasklet;

	host_data = sdhci_priv(host);
#ifdef CONFIG_MMC_CARD_HOTPLUG
	detect_irq = host_data->detect_irq;
	if (detect_irq > 0) {
	if (sdcard_present(host)){
		ret = request_threaded_irq(detect_irq, NULL, sd_detect_irq,
			IRQF_TRIGGER_HIGH | IRQF_ONESHOT | IRQF_NO_SUSPEND, "sd card detect", host);
	} else {
		ret = request_threaded_irq(detect_irq, NULL, sd_detect_irq,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT | IRQF_NO_SUSPEND, "sd card detect", host);
	}

	}
	if (ret)
		goto untasklet;
#endif
	sdhci_init(host, 0);

#ifdef SDHCI_USE_LEDS_CLASS
	snprintf(host->led_name, sizeof(host->led_name),
		"%s::", mmc_hostname(mmc));
	host->led.name = host->led_name;
	host->led.brightness = LED_OFF;
	host->led.default_trigger = mmc_hostname(mmc);
	host->led.brightness_set = sdhci_led_control;

	ret = led_classdev_register(mmc_dev(mmc), &host->led);
	if (ret)
		goto reset;
#endif

	mmiowb();

	mmc_add_host(mmc);

	pr_info("%s: SDHCI controller on %s [%s] using %s\n",
		mmc_hostname(mmc), host->hw_name, dev_name(mmc_dev(mmc)),
		(host->flags & SDHCI_USE_ADMA) ? "ADMA" :
		(host->flags & SDHCI_USE_SDMA) ? "DMA" : "PIO");

#ifdef CONFIG_MMC_CARD_HOTPLUG
	sdhci_enable_card_detection(host);
#endif

	
	host->suspending = 0;
	return 0;

#ifdef SDHCI_USE_LEDS_CLASS
reset:
	sdhci_reset(host, SDHCI_RESET_ALL);
	free_irq(host->irq, host);
#ifdef CONFIG_MMC_CARD_HOTPLUG
	free_irq(detect_irq, host);
#endif
#endif
untasklet:
#ifdef CONFIG_MMC_CARD_HOTPLUG
	tasklet_kill(&host->card_tasklet);
#endif
	tasklet_kill(&host->finish_tasklet);

	return ret;
}

EXPORT_SYMBOL_GPL(sdhci_add_host);

void sdhci_remove_host(struct sdhci_host *host, int dead)
{
	unsigned long flags;
	struct sprd_host_data *host_data;

	if (dead) {
		spin_lock_irqsave(&host->lock, flags);

		host->flags |= SDHCI_DEVICE_DEAD;

		if (host->mrq) {
			pr_err("%s: Controller removed during "
				" transfer!\n", mmc_hostname(host->mmc));

			host->mrq->cmd->error = -ENOMEDIUM;
			tasklet_schedule(&host->finish_tasklet);
		}

		spin_unlock_irqrestore(&host->lock, flags);
	}

#ifdef CONFIG_MMC_CARD_HOTPLUG
	sdhci_disable_card_detection(host);
#endif

	mmc_remove_host(host->mmc);

#ifdef SDHCI_USE_LEDS_CLASS
	led_classdev_unregister(&host->led);
#endif

	if (!dead)
		sdhci_reset(host, SDHCI_RESET_ALL);

	free_irq(host->irq, host);

	host_data = sdhci_priv(host);
#ifdef CONFIG_MMC_CARD_HOTPLUG
	free_irq(host_data->detect_irq, host);
#endif
	del_timer_sync(&host->timer);
	if (host->version >= SDHCI_SPEC_300)
		del_timer_sync(&host->tuning_timer);

#ifdef CONFIG_MMC_CARD_HOTPLUG
	tasklet_kill(&host->card_tasklet);
#endif
	tasklet_kill(&host->finish_tasklet);

	if (host->vmmc)
		regulator_put(host->vmmc);

	kfree(host->adma_desc);
	kfree(host->align_buffer);

	host->adma_desc = NULL;
	host->align_buffer = NULL;
}

EXPORT_SYMBOL_GPL(sdhci_remove_host);

void sdhci_free_host(struct sdhci_host *host)
{
	mmc_free_host(host->mmc);
}

EXPORT_SYMBOL_GPL(sdhci_free_host);


static int __init sdhci_drv_init(void)
{
	pr_info(DRIVER_NAME
		": Secure Digital Host Controller Interface driver\n");
	pr_info(DRIVER_NAME ": Copyright(c) Pierre Ossman\n");

#ifdef CONFIG_MMC_CARD_HOTPLUG
	wake_lock_init(&sdhci_detect_lock, WAKE_LOCK_SUSPEND, "mmc_detect_detect");
#endif

	return 0;
}

static void __exit sdhci_drv_exit(void)
{
#ifdef CONFIG_MMC_CARD_HOTPLUG
	wake_lock_destroy(&sdhci_detect_lock);
#endif
}

module_init(sdhci_drv_init);
module_exit(sdhci_drv_exit);

module_param(debug_quirks, uint, 0444);
module_param(debug_quirks2, uint, 0444);

MODULE_AUTHOR("Pierre Ossman <pierre@ossman.eu>");
MODULE_DESCRIPTION("Secure Digital Host Controller Interface core driver");
MODULE_LICENSE("GPL");

MODULE_PARM_DESC(debug_quirks, "Force certain quirks.");
MODULE_PARM_DESC(debug_quirks2, "Force certain other quirks.");
