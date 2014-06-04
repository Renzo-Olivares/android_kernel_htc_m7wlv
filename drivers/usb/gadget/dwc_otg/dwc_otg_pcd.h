/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_pcd.h $
 * $Revision: #48 $
 * $Date: 2012/08/10 $
 * $Change: 2047372 $
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
#ifndef DWC_HOST_ONLY
#if !defined(__DWC_PCD_H__)
#define __DWC_PCD_H__

#include "dwc_otg_os_dep.h"
#include "usb.h"
#include "dwc_otg_cil.h"
#include "dwc_otg_pcd_if.h"
struct cfiobject;


#define DWC_DMA_ADDR_INVALID	(~(dwc_dma_t)0)

#define DDMA_MAX_TRANSFER_SIZE 65535

#define GET_CORE_IF( _pcd ) (_pcd->core_if)

typedef enum ep0_state {
	EP0_DISCONNECT,		
	EP0_IDLE,
	EP0_IN_DATA_PHASE,
	EP0_OUT_DATA_PHASE,
	EP0_IN_STATUS_PHASE,
	EP0_OUT_STATUS_PHASE,
	EP0_STALL,
} ep0state_e;

struct dwc_otg_pcd;

typedef struct usb_iso_request dwc_otg_pcd_iso_request_t;

#ifdef DWC_UTE_PER_IO

struct dwc_iso_pkt_desc_port {
	uint32_t offset;
	uint32_t length;	
	uint32_t actual_length;
	uint32_t status;
};

struct dwc_iso_xreq_port {
	
	uint32_t tr_sub_flags;
	
#define DWC_EREQ_TF_ASAP		0x00000002
	
#define DWC_EREQ_TF_ENQUEUE		0x00000004

	uint32_t pio_pkt_count;
	
	uint32_t pio_alloc_pkt_count;
	
	uint32_t error_count;
	
	uint32_t res;
	
	struct dwc_iso_pkt_desc_port *per_io_frame_descs;
};
#endif
typedef struct dwc_otg_pcd_request {
	void *priv;
	void *buf;
	dwc_dma_t dma;
	uint32_t length;
	uint32_t actual;

	unsigned sent_zlp:1;
	unsigned mapped:1;
	uint8_t *dw_align_buf;
	dwc_dma_t dw_align_buf_dma;

	 DWC_CIRCLEQ_ENTRY(dwc_otg_pcd_request) queue_entry;
#ifdef DWC_UTE_PER_IO
	struct dwc_iso_xreq_port ext_req;
	
#endif
} dwc_otg_pcd_request_t;

DWC_CIRCLEQ_HEAD(req_list, dwc_otg_pcd_request);

typedef struct dwc_otg_pcd_ep {
	
	const usb_endpoint_descriptor_t *desc;

	
	struct req_list queue;
	unsigned stopped:1;
	unsigned disabling:1;
	unsigned dma:1;
	unsigned queue_sof:1;

#ifdef DWC_EN_ISOC
	
	void *iso_req_handle;
#endif				

	
	dwc_ep_t dwc_ep;

	
	struct dwc_otg_pcd *pcd;

	void *priv;
} dwc_otg_pcd_ep_t;

struct dwc_otg_pcd {
	const struct dwc_otg_pcd_function_ops *fops;
	
	struct dwc_otg_device *otg_dev;
	
	dwc_otg_core_if_t *core_if;
	
	ep0state_e ep0state;
	
	unsigned ep0_pending:1;
	
	unsigned request_config:1;
	
	unsigned remote_wakeup_enable:1;
	
	unsigned b_hnp_enable:1;
	
	unsigned a_hnp_support:1;
	
	unsigned a_alt_hnp_support:1;
	
	unsigned request_pending;

	union {
		usb_device_request_t req;
		uint32_t d32[2];
	} *setup_pkt;

	dwc_dma_t setup_pkt_dma_handle;

	
	uint8_t *backup_buf;
	unsigned data_terminated;

	
	uint16_t *status_buf;
	dwc_dma_t status_buf_dma_handle;

	
	dwc_otg_pcd_ep_t ep0;

	
	dwc_otg_pcd_ep_t in_ep[MAX_EPS_CHANNELS - 1];
	
	dwc_otg_pcd_ep_t out_ep[MAX_EPS_CHANNELS - 1];
	
	dwc_spinlock_t *lock;

	dwc_tasklet_t *test_mode_tasklet;

	
	dwc_tasklet_t *start_xfer_tasklet;

	
	unsigned test_mode;
#ifdef DWC_UTE_CFI
	struct cfiobject *cfi;
#endif
	unsigned pcd_startup;

};

extern void dwc_otg_request_nuke(dwc_otg_pcd_ep_t * ep);
extern void dwc_otg_request_done(dwc_otg_pcd_ep_t * ep,
				 dwc_otg_pcd_request_t * req, int32_t status);

void dwc_otg_iso_buffer_done(dwc_otg_pcd_t * pcd, dwc_otg_pcd_ep_t * ep,
			     void *req_handle);

extern void do_test_mode(void *data);
#endif
#endif 
