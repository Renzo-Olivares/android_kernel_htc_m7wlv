/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_hcd_if.h $
 * $Revision: #12 $
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
#ifndef DWC_DEVICE_ONLY
#ifndef __DWC_HCD_IF_H__
#define __DWC_HCD_IF_H__

#include "dwc_otg_core_if.h"


struct dwc_otg_hcd;
typedef struct dwc_otg_hcd dwc_otg_hcd_t;

struct dwc_otg_hcd_urb;
typedef struct dwc_otg_hcd_urb dwc_otg_hcd_urb_t;


typedef int (*dwc_otg_hcd_start_cb_t) (dwc_otg_hcd_t * hcd);

typedef int (*dwc_otg_hcd_disconnect_cb_t) (dwc_otg_hcd_t * hcd);

typedef int (*dwc_otg_hcd_hub_info_from_urb_cb_t) (dwc_otg_hcd_t * hcd,
						   void *urb_handle,
						   uint32_t * hub_addr,
						   uint32_t * port_addr);
typedef int (*dwc_otg_hcd_speed_from_urb_cb_t) (dwc_otg_hcd_t * hcd,
						void *urb_handle);

typedef int (*dwc_otg_hcd_complete_urb_cb_t) (dwc_otg_hcd_t * hcd,
					      void *urb_handle,
					      dwc_otg_hcd_urb_t * dwc_otg_urb,
					      int32_t status);

typedef int (*dwc_otg_hcd_get_b_hnp_enable) (dwc_otg_hcd_t * hcd);

struct dwc_otg_hcd_function_ops {
	dwc_otg_hcd_start_cb_t start;
	dwc_otg_hcd_disconnect_cb_t disconnect;
	dwc_otg_hcd_hub_info_from_urb_cb_t hub_info;
	dwc_otg_hcd_speed_from_urb_cb_t speed;
	dwc_otg_hcd_complete_urb_cb_t complete;
	dwc_otg_hcd_get_b_hnp_enable get_b_hnp_enable;
};

extern dwc_otg_hcd_t *dwc_otg_hcd_alloc_hcd(void);

extern int dwc_otg_hcd_init(dwc_otg_hcd_t * hcd, dwc_otg_core_if_t * core_if);

extern void dwc_otg_hcd_remove(dwc_otg_hcd_t * hcd);

extern int32_t dwc_otg_hcd_handle_intr(dwc_otg_hcd_t * dwc_otg_hcd);

extern void *dwc_otg_hcd_get_priv_data(dwc_otg_hcd_t * hcd);

extern void dwc_otg_hcd_set_priv_data(dwc_otg_hcd_t * hcd, void *priv_data);

extern int dwc_otg_hcd_start(dwc_otg_hcd_t * hcd,
			     struct dwc_otg_hcd_function_ops *fops);

extern void dwc_otg_hcd_stop(dwc_otg_hcd_t * hcd);

extern int dwc_otg_hcd_hub_control(dwc_otg_hcd_t * dwc_otg_hcd,
				   uint16_t typeReq, uint16_t wValue,
				   uint16_t wIndex, uint8_t * buf,
				   uint16_t wLength);

extern uint32_t dwc_otg_hcd_otg_port(dwc_otg_hcd_t * hcd);

extern uint16_t dwc_otg_get_otg_version(dwc_otg_core_if_t * core_if);

extern uint32_t dwc_otg_hcd_is_b_host(dwc_otg_hcd_t * hcd);

extern int dwc_otg_hcd_get_frame_number(dwc_otg_hcd_t * hcd);

extern void dwc_otg_hcd_dump_state(dwc_otg_hcd_t * hcd);

extern void dwc_otg_hcd_dump_frrem(dwc_otg_hcd_t * hcd);

extern int dwc_otg_hcd_send_lpm(dwc_otg_hcd_t * hcd, uint8_t devaddr,
				uint8_t hird, uint8_t bRemoteWake);


extern dwc_otg_hcd_urb_t *dwc_otg_hcd_urb_alloc(dwc_otg_hcd_t * hcd,
						int iso_desc_count,
						int atomic_alloc);

extern void dwc_otg_hcd_urb_set_pipeinfo(dwc_otg_hcd_urb_t * hcd_urb,
					 uint8_t devaddr, uint8_t ep_num,
					 uint8_t ep_type, uint8_t ep_dir,
					 uint16_t mps);

#define URB_GIVEBACK_ASAP 0x1
#define URB_SEND_ZERO_PACKET 0x2

extern void dwc_otg_hcd_urb_set_params(dwc_otg_hcd_urb_t * urb,
				       void *urb_handle, void *buf,
				       dwc_dma_t dma, uint32_t buflen, void *sp,
				       dwc_dma_t sp_dma, uint32_t flags,
				       uint16_t interval);

extern uint32_t dwc_otg_hcd_urb_get_status(dwc_otg_hcd_urb_t * dwc_otg_urb);

extern uint32_t dwc_otg_hcd_urb_get_actual_length(dwc_otg_hcd_urb_t *
						  dwc_otg_urb);

extern uint32_t dwc_otg_hcd_urb_get_error_count(dwc_otg_hcd_urb_t *
						dwc_otg_urb);

extern void dwc_otg_hcd_urb_set_iso_desc_params(dwc_otg_hcd_urb_t * dwc_otg_urb,
						int desc_num, uint32_t offset,
						uint32_t length);

extern uint32_t dwc_otg_hcd_urb_get_iso_desc_status(dwc_otg_hcd_urb_t *
						    dwc_otg_urb, int desc_num);

extern uint32_t dwc_otg_hcd_urb_get_iso_desc_actual_length(dwc_otg_hcd_urb_t *
							   dwc_otg_urb,
							   int desc_num);

extern int dwc_otg_hcd_urb_enqueue(dwc_otg_hcd_t * dwc_otg_hcd,
				   dwc_otg_hcd_urb_t * dwc_otg_urb,
				   void **ep_handle, int atomic_alloc);

extern int dwc_otg_hcd_urb_dequeue(dwc_otg_hcd_t * dwc_otg_hcd,
				   dwc_otg_hcd_urb_t * dwc_otg_urb);

extern int dwc_otg_hcd_endpoint_disable(dwc_otg_hcd_t * hcd, void *ep_handle,
					int retry);

extern int dwc_otg_hcd_endpoint_reset(dwc_otg_hcd_t * hcd, void *ep_handle);

extern int dwc_otg_hcd_is_status_changed(dwc_otg_hcd_t * hcd, int port);

extern int dwc_otg_hcd_is_bandwidth_allocated(dwc_otg_hcd_t * hcd,
					      void *ep_handle);

extern int dwc_otg_hcd_is_bandwidth_freed(dwc_otg_hcd_t * hcd, void *ep_handle);

extern uint8_t dwc_otg_hcd_get_ep_bandwidth(dwc_otg_hcd_t * hcd,
					    void *ep_handle);


#endif 
#endif 
