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

#ifndef __SBUF_H
#define __SBUF_H

#define SMSG_CMD_SBUF_INIT	0x0001
#define SMSG_DONE_SBUF_INIT	0x0002

#define SMSG_EVENT_SBUF_WRPTR	0x0001
#define SMSG_EVENT_SBUF_RDPTR	0x0002

struct sbuf_ring_header {
	
	uint32_t		txbuf_addr;
	uint32_t		txbuf_size;
	uint32_t		txbuf_rdptr;
	uint32_t		txbuf_wrptr;

	
	uint32_t		rxbuf_addr;
	uint32_t		rxbuf_size;
	uint32_t		rxbuf_rdptr;
	uint32_t		rxbuf_wrptr;
};

struct sbuf_smem_header {
	uint32_t		ringnr;

	struct sbuf_ring_header	headers[0];
};

struct sbuf_ring {
	
	volatile struct sbuf_ring_header	*header;

	void			*txbuf_virt;
	void			*rxbuf_virt;

	
	wait_queue_head_t	txwait;
	wait_queue_head_t	rxwait;

	
	struct mutex		txlock;
	struct mutex		rxlock;
};

#define SBUF_STATE_IDLE		0
#define SBUF_STATE_READY	1
#define SBUF_STATE_RESET	2

struct sbuf_mgr {
	uint8_t			dst;
	uint8_t			channel;
	volatile uint32_t	state;

	void			*smem_virt;
	uint32_t		smem_addr;
	uint32_t		smem_size;
	uint32_t		ringnr;
	struct sbuf_ring	*rings;
	struct task_struct	*thread;
	wait_queue_head_t	thread_wait;
	struct mutex		statelock;
};

#endif
