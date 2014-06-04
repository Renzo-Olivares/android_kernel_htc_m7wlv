/* linux/arch/arm/mach-sc/htc-debug-last-io.c
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

#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <mach/hardware.h>
#include <mach/io.h>
#include <mach/board_htc.h>

struct htc_debug_last_io {
	void *pc;
	void __iomem *vaddr;
	u64 jiffies;
	u32 state;
} ____cacheline_aligned;

static struct htc_debug_last_io *htc_last_io;
static void __iomem *io_sci_base;
static void __iomem *io_sci_end;
static dma_addr_t htc_last_io_phys;
static int initial_state = 0;

void htc_debug_last_io_save(void *pc, void __iomem *vaddr)
{
	int index;

	if (!initial_state)
		return;

	
	if (io_sci_base && io_sci_end &&
		(((unsigned long)io_sci_base > (unsigned long)vaddr) ||
		((unsigned long)io_sci_end < (unsigned long)vaddr)))
		return;

	index = smp_processor_id();
	htc_last_io[index].pc = pc;
	htc_last_io[index].vaddr = vaddr;
	
	htc_last_io[index].jiffies = jiffies_64;
	htc_last_io[index].state = 0; 
}

void htc_debug_last_io_done(void __iomem *vaddr)
{
	int index;
	
	if (!initial_state)
		return;

	
	if (io_sci_base && io_sci_end &&
		(((unsigned long)io_sci_base > (unsigned long)vaddr) ||
		((unsigned long)io_sci_end < (unsigned long)vaddr)))
		return;

	index = smp_processor_id();
	htc_last_io[index].state = 1; 
}

static int __init htc_debug_last_io_init(void)
{
	size_t size;
	int cpu_num;

	if (!(htc_get_config(HTC_DBG_FLAG_SUPERMAN) &
		SUPERMAN_FLAG_LAST_IO)) {
		printk(KERN_INFO "[k] Disable last io function! \n");
		return -EPERM;
	}

	cpu_num = num_possible_cpus();
	printk(KERN_INFO "[k] Initialize %d buffers for last io ...\n", cpu_num);
	size = sizeof(struct htc_debug_last_io) * cpu_num;

	htc_last_io = dma_alloc_coherent(NULL, size, &htc_last_io_phys,
								GFP_KERNEL);
	if (!htc_last_io) {
		printk(KERN_ERR"%s: Failed to allocate memory\n", __func__);
		return -ENOMEM;
	}

	if (htc_get_config(HTC_DBG_FLAG_SUPERMAN) &
		SUPERMAN_FLAG_HW_LAST_IO) {
		printk(KERN_INFO "Record HW IO only.\n");
		io_sci_base = __io(SCI_IOMAP_BASE);
		io_sci_end  = __io(SPRD_ADISLAVE_BASE + SPRD_ADISLAVE_SIZE);
	}

	initial_state = 1;
	printk(KERN_INFO "[k] Debug IO superman is ready.\n");

	return 0;
}

EXPORT_SYMBOL(htc_debug_last_io_save);
EXPORT_SYMBOL(htc_debug_last_io_done);
arch_initcall(htc_debug_last_io_init);
