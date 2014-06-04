/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_hcd.h $
 * $Revision: #58 $
 * $Date: 2011/09/15 $
 * $Change: 1846647 $
 *
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */
#ifndef DWC_DEVICE_ONLY
#ifndef __DWC_HCD_H__
#define __DWC_HCD_H__

#include "dwc_otg_os_dep.h"
#include "usb.h"
#include "dwc_otg_hcd_if.h"
#include "dwc_otg_core_if.h"
#include "dwc_list.h"
#include "dwc_otg_cil.h"


struct dwc_otg_hcd_pipe_info {
	uint8_t dev_addr;
	uint8_t ep_num;
	uint8_t pipe_type;
	uint8_t pipe_dir;
	uint16_t mps;
};

struct dwc_otg_hcd_iso_packet_desc {
	uint32_t offset;
	uint32_t length;
	uint32_t actual_length;
	uint32_t status;
};

struct dwc_otg_qtd;

struct dwc_otg_hcd_urb {
	void *priv;
	struct dwc_otg_qtd *qtd;
	void *buf;
	dwc_dma_t dma;
	void *setup_packet;
	dwc_dma_t setup_dma;
	uint32_t length;
	uint32_t actual_length;
	uint32_t status;
	uint32_t error_count;
	uint32_t packet_count;
	uint32_t flags;
	uint16_t interval;
	struct dwc_otg_hcd_pipe_info pipe_info;
	struct dwc_otg_hcd_iso_packet_desc iso_descs[0];
};

static inline uint8_t dwc_otg_hcd_get_ep_num(struct dwc_otg_hcd_pipe_info *pipe)
{
	return pipe->ep_num;
}

static inline uint8_t dwc_otg_hcd_get_pipe_type(struct dwc_otg_hcd_pipe_info
						*pipe)
{
	return pipe->pipe_type;
}

static inline uint16_t dwc_otg_hcd_get_mps(struct dwc_otg_hcd_pipe_info *pipe)
{
	return pipe->mps;
}

static inline uint8_t dwc_otg_hcd_get_dev_addr(struct dwc_otg_hcd_pipe_info
					       *pipe)
{
	return pipe->dev_addr;
}

static inline uint8_t dwc_otg_hcd_is_pipe_isoc(struct dwc_otg_hcd_pipe_info
					       *pipe)
{
	return (pipe->pipe_type == UE_ISOCHRONOUS);
}

static inline uint8_t dwc_otg_hcd_is_pipe_int(struct dwc_otg_hcd_pipe_info
					      *pipe)
{
	return (pipe->pipe_type == UE_INTERRUPT);
}

static inline uint8_t dwc_otg_hcd_is_pipe_bulk(struct dwc_otg_hcd_pipe_info
					       *pipe)
{
	return (pipe->pipe_type == UE_BULK);
}

static inline uint8_t dwc_otg_hcd_is_pipe_control(struct dwc_otg_hcd_pipe_info
						  *pipe)
{
	return (pipe->pipe_type == UE_CONTROL);
}

static inline uint8_t dwc_otg_hcd_is_pipe_in(struct dwc_otg_hcd_pipe_info *pipe)
{
	return (pipe->pipe_dir == UE_DIR_IN);
}

static inline uint8_t dwc_otg_hcd_is_pipe_out(struct dwc_otg_hcd_pipe_info
					      *pipe)
{
	return (!dwc_otg_hcd_is_pipe_in(pipe));
}

static inline void dwc_otg_hcd_fill_pipe(struct dwc_otg_hcd_pipe_info *pipe,
					 uint8_t devaddr, uint8_t ep_num,
					 uint8_t pipe_type, uint8_t pipe_dir,
					 uint16_t mps)
{
	pipe->dev_addr = devaddr;
	pipe->ep_num = ep_num;
	pipe->pipe_type = pipe_type;
	pipe->pipe_dir = pipe_dir;
	pipe->mps = mps;
}

typedef enum dwc_otg_control_phase {
	DWC_OTG_CONTROL_SETUP,
	DWC_OTG_CONTROL_DATA,
	DWC_OTG_CONTROL_STATUS
} dwc_otg_control_phase_e;

typedef enum dwc_otg_transaction_type {
	DWC_OTG_TRANSACTION_NONE,
	DWC_OTG_TRANSACTION_PERIODIC,
	DWC_OTG_TRANSACTION_NON_PERIODIC,
	DWC_OTG_TRANSACTION_ALL
} dwc_otg_transaction_type_e;

struct dwc_otg_qh;

typedef struct dwc_otg_qtd {
	uint8_t data_toggle;

	
	dwc_otg_control_phase_e control_phase;

	uint8_t complete_split;

	
	uint32_t ssplit_out_xfer_count;

	uint8_t error_count;

	uint16_t isoc_frame_index;

	
	uint8_t isoc_split_pos;

	
	uint16_t isoc_split_offset;

	
	struct dwc_otg_hcd_urb *urb;

	struct dwc_otg_qh *qh;

	
	 DWC_CIRCLEQ_ENTRY(dwc_otg_qtd) qtd_list_entry;

	
	uint8_t in_process;

	
	uint8_t n_desc;

	uint16_t isoc_frame_index_last;

} dwc_otg_qtd_t;

DWC_CIRCLEQ_HEAD(dwc_otg_qtd_list, dwc_otg_qtd);

typedef struct dwc_otg_qh {
	uint8_t ep_type;
	uint8_t ep_is_in;

	
	uint16_t maxp;

	uint8_t dev_speed;

	uint8_t data_toggle;

	
	uint8_t ping_state;

	struct dwc_otg_qtd_list qtd_list;

	
	struct dwc_hc *channel;

	
	uint8_t do_split;

	
	

	
	uint16_t usecs;

	
	uint16_t interval;

	uint16_t sched_frame;

	
	uint16_t start_split_frame;

	

	uint8_t *dw_align_buf;
	dwc_dma_t dw_align_buf_dma;

	
	dwc_list_link_t qh_list_entry;

	
	

	
	dwc_otg_host_dma_desc_t *desc_list;

	
	dwc_dma_t desc_list_dma;

	uint32_t *n_bytes;

	
	uint16_t ntd;

	
	uint8_t td_first;
	
	uint8_t td_last;

	

} dwc_otg_qh_t;

DWC_CIRCLEQ_HEAD(hc_list, dwc_hc);

struct dwc_otg_hcd {
	
	struct dwc_otg_device *otg_dev;
	
	dwc_otg_core_if_t *core_if;

	
	struct dwc_otg_hcd_function_ops *fops;

	
	volatile union dwc_otg_hcd_internal_flags {
		uint32_t d32;
		struct {
			unsigned port_connect_status_change:1;
			unsigned port_connect_status:1;
			unsigned port_reset_change:1;
			unsigned port_enable_change:1;
			unsigned port_suspend_change:1;
			unsigned port_over_current_change:1;
			unsigned port_l1_change:1;
			unsigned reserved:26;
		} b;
	} flags;

	dwc_list_link_t non_periodic_sched_inactive;

	dwc_list_link_t non_periodic_sched_active;

	dwc_list_link_t *non_periodic_qh_ptr;

	dwc_list_link_t periodic_sched_inactive;

	dwc_list_link_t periodic_sched_ready;

	dwc_list_link_t periodic_sched_assigned;

	dwc_list_link_t periodic_sched_queued;

	uint16_t periodic_usecs;

	uint16_t frame_number;

	uint16_t periodic_qh_count;

	struct hc_list free_hc_list;
	int periodic_channels;

	int non_periodic_channels;

	struct dwc_hc *hc_ptr_array[MAX_EPS_CHANNELS];

	uint8_t *status_buf;

	dma_addr_t status_buf_dma;
#define DWC_OTG_HCD_STATUS_BUF_SIZE 64

	dwc_timer_t *conn_timer;

	
	dwc_tasklet_t *reset_tasklet;

	
	dwc_spinlock_t *lock;

	void *priv;

	uint8_t otg_port;

	
	uint32_t *frame_list;

	
	dma_addr_t frame_list_dma;

#ifdef DEBUG
	uint32_t frrem_samples;
	uint64_t frrem_accum;

	uint32_t hfnum_7_samples_a;
	uint64_t hfnum_7_frrem_accum_a;
	uint32_t hfnum_0_samples_a;
	uint64_t hfnum_0_frrem_accum_a;
	uint32_t hfnum_other_samples_a;
	uint64_t hfnum_other_frrem_accum_a;

	uint32_t hfnum_7_samples_b;
	uint64_t hfnum_7_frrem_accum_b;
	uint32_t hfnum_0_samples_b;
	uint64_t hfnum_0_frrem_accum_b;
	uint32_t hfnum_other_samples_b;
	uint64_t hfnum_other_frrem_accum_b;
#endif
};

extern dwc_otg_transaction_type_e dwc_otg_hcd_select_transactions(dwc_otg_hcd_t
								  * hcd);
extern void dwc_otg_hcd_queue_transactions(dwc_otg_hcd_t * hcd,
					   dwc_otg_transaction_type_e tr_type);


extern int32_t dwc_otg_hcd_handle_intr(dwc_otg_hcd_t * dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_sof_intr(dwc_otg_hcd_t * dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_rx_status_q_level_intr(dwc_otg_hcd_t *
							 dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_np_tx_fifo_empty_intr(dwc_otg_hcd_t *
							dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_perio_tx_fifo_empty_intr(dwc_otg_hcd_t *
							   dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_incomplete_periodic_intr(dwc_otg_hcd_t *
							   dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_port_intr(dwc_otg_hcd_t * dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_conn_id_status_change_intr(dwc_otg_hcd_t *
							     dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_disconnect_intr(dwc_otg_hcd_t * dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_hc_intr(dwc_otg_hcd_t * dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_hc_n_intr(dwc_otg_hcd_t * dwc_otg_hcd,
					    uint32_t num);
extern int32_t dwc_otg_hcd_handle_session_req_intr(dwc_otg_hcd_t * dwc_otg_hcd);
extern int32_t dwc_otg_hcd_handle_wakeup_detected_intr(dwc_otg_hcd_t *
						       dwc_otg_hcd);


extern dwc_otg_qh_t *dwc_otg_hcd_qh_create(dwc_otg_hcd_t * hcd,
					   dwc_otg_hcd_urb_t * urb, int atomic_alloc);
extern void dwc_otg_hcd_qh_free(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh);
extern int dwc_otg_hcd_qh_add(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh);
extern void dwc_otg_hcd_qh_remove(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh);
extern void dwc_otg_hcd_qh_deactivate(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh,
				      int sched_csplit);

static inline void dwc_otg_hcd_qh_remove_and_free(dwc_otg_hcd_t * hcd,
						  dwc_otg_qh_t * qh)
{
	dwc_irqflags_t flags;
	DWC_SPINLOCK_IRQSAVE(hcd->lock, &flags);
	dwc_otg_hcd_qh_remove(hcd, qh);
	DWC_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
	dwc_otg_hcd_qh_free(hcd, qh);
}

static inline dwc_otg_qh_t *dwc_otg_hcd_qh_alloc(int atomic_alloc)
{
	if (atomic_alloc)
		return (dwc_otg_qh_t *) DWC_ALLOC_ATOMIC(sizeof(dwc_otg_qh_t));
	else
		return (dwc_otg_qh_t *) DWC_ALLOC(sizeof(dwc_otg_qh_t));
}

extern dwc_otg_qtd_t *dwc_otg_hcd_qtd_create(dwc_otg_hcd_urb_t * urb,
					     int atomic_alloc);
extern void dwc_otg_hcd_qtd_init(dwc_otg_qtd_t * qtd, dwc_otg_hcd_urb_t * urb);
extern int dwc_otg_hcd_qtd_add(dwc_otg_qtd_t * qtd, dwc_otg_hcd_t * dwc_otg_hcd,
			       dwc_otg_qh_t ** qh, int atomic_alloc);

static inline dwc_otg_qtd_t *dwc_otg_hcd_qtd_alloc(int atomic_alloc)
{
	if (atomic_alloc)
		return (dwc_otg_qtd_t *) DWC_ALLOC_ATOMIC(sizeof(dwc_otg_qtd_t));
	else
		return (dwc_otg_qtd_t *) DWC_ALLOC(sizeof(dwc_otg_qtd_t));
}

static inline void dwc_otg_hcd_qtd_free(dwc_otg_qtd_t * qtd)
{
	DWC_FREE(qtd);
}

static inline void dwc_otg_hcd_qtd_remove(dwc_otg_hcd_t * hcd,
					  dwc_otg_qtd_t * qtd,
					  dwc_otg_qh_t * qh)
{
	DWC_CIRCLEQ_REMOVE(&qh->qtd_list, qtd, qtd_list_entry);
}

static inline void dwc_otg_hcd_qtd_remove_and_free(dwc_otg_hcd_t * hcd,
						   dwc_otg_qtd_t * qtd,
						   dwc_otg_qh_t * qh)
{
	dwc_otg_hcd_qtd_remove(hcd, qtd, qh);
	dwc_otg_hcd_qtd_free(qtd);
}



extern void dwc_otg_hcd_start_xfer_ddma(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh);
extern void dwc_otg_hcd_complete_xfer_ddma(dwc_otg_hcd_t * hcd,
					   dwc_hc_t * hc,
					   dwc_otg_hc_regs_t * hc_regs,
					   dwc_otg_halt_status_e halt_status);

extern int dwc_otg_hcd_qh_init_ddma(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh);
extern void dwc_otg_hcd_qh_free_ddma(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh);


dwc_otg_qh_t *dwc_urb_to_qh(dwc_otg_hcd_urb_t * urb);

#ifdef CONFIG_USB_DWC_OTG_LPM
extern int dwc_otg_hcd_get_hc_for_lpm_tran(dwc_otg_hcd_t * hcd,
					   uint8_t devaddr);
extern void dwc_otg_hcd_free_hc_from_lpm(dwc_otg_hcd_t * hcd);
#endif

#define dwc_list_to_qh(_list_head_ptr_) container_of(_list_head_ptr_, dwc_otg_qh_t, qh_list_entry)

#define dwc_list_to_qtd(_list_head_ptr_) container_of(_list_head_ptr_, dwc_otg_qtd_t, qtd_list_entry)

#define dwc_qh_is_non_per(_qh_ptr_) ((_qh_ptr_->ep_type == UE_BULK) || \
				     (_qh_ptr_->ep_type == UE_CONTROL))

#define dwc_hb_mult(wMaxPacketSize) (1 + (((wMaxPacketSize) >> 11) & 0x03))

#define dwc_max_packet(wMaxPacketSize) ((wMaxPacketSize) & 0x07ff)

static inline int dwc_frame_num_le(uint16_t frame1, uint16_t frame2)
{
	return ((frame2 - frame1) & DWC_HFNUM_MAX_FRNUM) <=
	    (DWC_HFNUM_MAX_FRNUM >> 1);
}

static inline int dwc_frame_num_gt(uint16_t frame1, uint16_t frame2)
{
	return (frame1 != frame2) &&
	    (((frame1 - frame2) & DWC_HFNUM_MAX_FRNUM) <
	     (DWC_HFNUM_MAX_FRNUM >> 1));
}

static inline uint16_t dwc_frame_num_inc(uint16_t frame, uint16_t inc)
{
	return (frame + inc) & DWC_HFNUM_MAX_FRNUM;
}

static inline uint16_t dwc_full_frame_num(uint16_t frame)
{
	return (frame & DWC_HFNUM_MAX_FRNUM) >> 3;
}

static inline uint16_t dwc_micro_frame_num(uint16_t frame)
{
	return frame & 0x7;
}

void dwc_otg_hcd_save_data_toggle(dwc_hc_t * hc,
				  dwc_otg_hc_regs_t * hc_regs,
				  dwc_otg_qtd_t * qtd);

#ifdef DEBUG
#define dwc_sample_frrem(_hcd, _qh, _letter) \
{ \
	hfnum_data_t hfnum; \
	dwc_otg_qtd_t *qtd; \
	qtd = list_entry(_qh->qtd_list.next, dwc_otg_qtd_t, qtd_list_entry); \
	if (usb_pipeint(qtd->urb->pipe) && _qh->start_split_frame != 0 && !qtd->complete_split) { \
		hfnum.d32 = DWC_READ_REG32(&_hcd->core_if->host_if->host_global_regs->hfnum); \
		switch (hfnum.b.frnum & 0x7) { \
		case 7: \
			_hcd->hfnum_7_samples_##_letter++; \
			_hcd->hfnum_7_frrem_accum_##_letter += hfnum.b.frrem; \
			break; \
		case 0: \
			_hcd->hfnum_0_samples_##_letter++; \
			_hcd->hfnum_0_frrem_accum_##_letter += hfnum.b.frrem; \
			break; \
		default: \
			_hcd->hfnum_other_samples_##_letter++; \
			_hcd->hfnum_other_frrem_accum_##_letter += hfnum.b.frrem; \
			break; \
		} \
	} \
}
#else
#define dwc_sample_frrem(_hcd, _qh, _letter)
#endif
#endif
#endif 
