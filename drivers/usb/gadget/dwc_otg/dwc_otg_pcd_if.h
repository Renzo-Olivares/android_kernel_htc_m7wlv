/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_pcd_if.h $
 * $Revision: #11 $
 * $Date: 2011/10/26 $
 * $Change: 1873028 $
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

#if !defined(__DWC_PCD_IF_H__)
#define __DWC_PCD_IF_H__

#include "dwc_os.h"
#include "dwc_otg_core_if.h"


struct dwc_otg_pcd;
typedef struct dwc_otg_pcd dwc_otg_pcd_t;

#define MAX_EP0_SIZE	64
#define MAX_PACKET_SIZE 1024


typedef int (*dwc_completion_cb_t) (dwc_otg_pcd_t * pcd, void *ep_handle,
				    void *req_handle, int32_t status,
				    uint32_t actual);
typedef int (*dwc_isoc_completion_cb_t) (dwc_otg_pcd_t * pcd, void *ep_handle,
					 void *req_handle, int proc_buf_num);
typedef int (*dwc_setup_cb_t) (dwc_otg_pcd_t * pcd, uint8_t * bytes);
typedef int (*dwc_disconnect_cb_t) (dwc_otg_pcd_t * pcd);
typedef int (*dwc_connect_cb_t) (dwc_otg_pcd_t * pcd, int speed);
typedef int (*dwc_suspend_cb_t) (dwc_otg_pcd_t * pcd);
typedef int (*dwc_sleep_cb_t) (dwc_otg_pcd_t * pcd);
typedef int (*dwc_resume_cb_t) (dwc_otg_pcd_t * pcd);
typedef int (*dwc_hnp_params_changed_cb_t) (dwc_otg_pcd_t * pcd);
typedef int (*dwc_reset_cb_t) (dwc_otg_pcd_t * pcd);

typedef int (*cfi_setup_cb_t) (dwc_otg_pcd_t * pcd, void *ctrl_req_bytes);

typedef int (*xiso_completion_cb_t) (dwc_otg_pcd_t * pcd, void *ep_handle,
				     void *req_handle, int32_t status,
				     void *ereq_port);
struct dwc_otg_pcd_function_ops {
	dwc_connect_cb_t connect;
	dwc_disconnect_cb_t disconnect;
	dwc_setup_cb_t setup;
	dwc_completion_cb_t complete;
	dwc_isoc_completion_cb_t isoc_complete;
	dwc_suspend_cb_t suspend;
	dwc_sleep_cb_t sleep;
	dwc_resume_cb_t resume;
	dwc_reset_cb_t reset;
	dwc_hnp_params_changed_cb_t hnp_changed;
	cfi_setup_cb_t cfi_setup;
#ifdef DWC_UTE_PER_IO
	xiso_completion_cb_t xisoc_complete;
#endif
};


extern dwc_otg_pcd_t *dwc_otg_pcd_init(dwc_otg_core_if_t * core_if);

extern void dwc_otg_pcd_remove(dwc_otg_pcd_t * pcd);

extern void dwc_otg_pcd_start(dwc_otg_pcd_t * pcd,
			      const struct dwc_otg_pcd_function_ops *fops);

extern int dwc_otg_pcd_ep_enable(dwc_otg_pcd_t * pcd,
				 const uint8_t * ep_desc, void *usb_ep);

extern int dwc_otg_pcd_ep_disable(dwc_otg_pcd_t * pcd, void *ep_handle);

extern int dwc_otg_pcd_ep_queue(dwc_otg_pcd_t * pcd, void *ep_handle,
				uint8_t * buf, dwc_dma_t dma_buf,
				uint32_t buflen, int zero, void *req_handle,
				int atomic_alloc);
#ifdef DWC_UTE_PER_IO
extern int dwc_otg_pcd_xiso_ep_queue(dwc_otg_pcd_t * pcd, void *ep_handle,
				     uint8_t * buf, dwc_dma_t dma_buf,
				     uint32_t buflen, int zero,
				     void *req_handle, int atomic_alloc,
				     void *ereq_nonport);

#endif

extern int dwc_otg_pcd_ep_dequeue(dwc_otg_pcd_t * pcd, void *ep_handle,
				  void *req_handle);

extern int dwc_otg_pcd_ep_halt(dwc_otg_pcd_t * pcd, void *ep_handle, int value);

extern int32_t dwc_otg_pcd_handle_intr(dwc_otg_pcd_t * pcd);

extern int dwc_otg_pcd_get_frame_number(dwc_otg_pcd_t * pcd);

extern int dwc_otg_pcd_iso_ep_start(dwc_otg_pcd_t * pcd, void *ep_handle,
				    uint8_t * buf0, uint8_t * buf1,
				    dwc_dma_t dma0, dwc_dma_t dma1,
				    int sync_frame, int dp_frame,
				    int data_per_frame, int start_frame,
				    int buf_proc_intrvl, void *req_handle,
				    int atomic_alloc);

int dwc_otg_pcd_iso_ep_stop(dwc_otg_pcd_t * pcd, void *ep_handle,
			    void *req_handle);

extern void dwc_otg_pcd_get_iso_packet_params(dwc_otg_pcd_t * pcd,
					      void *ep_handle,
					      void *iso_req_handle, int packet,
					      int *status, int *actual,
					      int *offset);

extern int dwc_otg_pcd_get_iso_packet_count(dwc_otg_pcd_t * pcd,
					    void *ep_handle,
					    void *iso_req_handle);

extern int dwc_otg_pcd_wakeup(dwc_otg_pcd_t * pcd);

extern int dwc_otg_pcd_is_lpm_enabled(dwc_otg_pcd_t * pcd);

extern int dwc_otg_pcd_get_rmwkup_enable(dwc_otg_pcd_t * pcd);

extern void dwc_otg_pcd_initiate_srp(dwc_otg_pcd_t * pcd);

extern void dwc_otg_pcd_remote_wakeup(dwc_otg_pcd_t * pcd, int set);

extern void dwc_otg_pcd_disconnect_us(dwc_otg_pcd_t * pcd, int no_of_usecs);
extern uint32_t dwc_otg_pcd_is_dualspeed(dwc_otg_pcd_t * pcd);

extern uint32_t dwc_otg_pcd_is_otg(dwc_otg_pcd_t * pcd);

extern uint32_t get_b_hnp_enable(dwc_otg_pcd_t * pcd);
extern uint32_t get_a_hnp_support(dwc_otg_pcd_t * pcd);
extern uint32_t get_a_alt_hnp_support(dwc_otg_pcd_t * pcd);

extern uint8_t *cfiw_ep_alloc_buffer(dwc_otg_pcd_t * pcd, void *pep,
				     dwc_dma_t * addr, size_t buflen,
				     int flags);



#endif				

#endif				
