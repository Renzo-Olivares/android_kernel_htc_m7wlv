/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_cil.h $
 * $Revision: #123 $
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

#if !defined(__DWC_CIL_H__)
#define __DWC_CIL_H__

#include "dwc_list.h"
#include "dwc_otg_dbg.h"
#include "dwc_otg_regs.h"

#include "dwc_otg_core_if.h"
#include "dwc_otg_adp.h"


#ifdef DWC_UTE_CFI

#define MAX_DMA_DESCS_PER_EP	256

typedef enum _data_buffer_mode {
	BM_STANDARD = 0,	
	BM_SG = 1,		
	BM_CONCAT = 2,		
	BM_CIRCULAR = 3,	
	BM_ALIGN = 4		
} data_buffer_mode_e;
#endif 


#define OTG_CORE_REV_2_60a	0x4F54260A
#define OTG_CORE_REV_2_71a	0x4F54271A
#define OTG_CORE_REV_2_72a	0x4F54272A
#define OTG_CORE_REV_2_80a	0x4F54280A
#define OTG_CORE_REV_2_81a	0x4F54281A
#define OTG_CORE_REV_2_90a	0x4F54290A
#define OTG_CORE_REV_2_91a	0x4F54291A
#define OTG_CORE_REV_2_92a	0x4F54292A
#define OTG_CORE_REV_2_93a	0x4F54293A
#define OTG_CORE_REV_2_94a	0x4F54294A
#define OTG_CORE_REV_3_00a	0x4F54300A

typedef struct iso_pkt_info {
	uint32_t offset;
	uint32_t length;
	int32_t status;
} iso_pkt_info_t;

typedef struct dwc_ep {
	
	uint8_t num;
	
	unsigned is_in:1;
	
	unsigned active:1;

	unsigned tx_fifo_num:4;
	
	unsigned type:2;
#define DWC_OTG_EP_TYPE_CONTROL	   0
#define DWC_OTG_EP_TYPE_ISOC	   1
#define DWC_OTG_EP_TYPE_BULK	   2
#define DWC_OTG_EP_TYPE_INTR	   3

	
	unsigned data_pid_start:1;
	
	unsigned even_odd_frame:1;
	
	unsigned maxpacket:11;

	
	uint32_t maxxfer;

	
	


	dwc_dma_t dma_addr;

	dwc_dma_t dma_desc_addr;
	dwc_otg_dev_dma_desc_t *desc_addr;

	uint8_t *start_xfer_buff;
	
	uint8_t *xfer_buff;
	
	unsigned xfer_len:19;
	
	unsigned xfer_count:19;
	
	unsigned sent_zlp:1;
	
	unsigned total_len:19;

	
	unsigned stall_clear_flag:1;

	
	unsigned stp_rollover;

#ifdef DWC_UTE_CFI
	
	data_buffer_mode_e buff_mode;

	dwc_otg_dma_desc_t *descs;

	
	dma_addr_t descs_dma_addr;
	
	uint32_t cfi_req_len;
#endif				

#define MAX_DMA_DESC_CNT 256
	
	uint32_t desc_cnt;

	
	uint32_t bInterval;
	
	uint32_t frame_num;
	
	uint8_t frm_overrun;

#ifdef DWC_UTE_PER_IO
	
	uint32_t xiso_frame_num;
	
	uint32_t xiso_bInterval;
	
	int xiso_active_xfers;
	int xiso_queued_xfers;
#endif
#ifdef DWC_EN_ISOC
	
	dwc_dma_t dma_addr0;
	dwc_dma_t dma_addr1;

	dwc_dma_t iso_dma_desc_addr;
	dwc_otg_dev_dma_desc_t *iso_desc_addr;

	
	uint8_t *xfer_buff0;
	uint8_t *xfer_buff1;

	
	uint32_t proc_buf_num;
	
	uint32_t buf_proc_intrvl;
	
	uint32_t data_per_frame;

	
	
	uint32_t data_pattern_frame;
	
	uint32_t sync_frame;

	
	uint32_t bInterval;
	
	uint32_t pkt_per_frm;
	
	uint32_t next_frame;
	
	uint32_t pkt_cnt;
	
	iso_pkt_info_t *pkt_info;
	
	uint32_t cur_pkt;
	
	uint8_t *cur_pkt_addr;
	
	uint32_t cur_pkt_dma_addr;
#endif				

} dwc_ep_t;

typedef enum dwc_otg_halt_status {
	DWC_OTG_HC_XFER_NO_HALT_STATUS,
	DWC_OTG_HC_XFER_COMPLETE,
	DWC_OTG_HC_XFER_URB_COMPLETE,
	DWC_OTG_HC_XFER_ACK,
	DWC_OTG_HC_XFER_NAK,
	DWC_OTG_HC_XFER_NYET,
	DWC_OTG_HC_XFER_STALL,
	DWC_OTG_HC_XFER_XACT_ERR,
	DWC_OTG_HC_XFER_FRAME_OVERRUN,
	DWC_OTG_HC_XFER_BABBLE_ERR,
	DWC_OTG_HC_XFER_DATA_TOGGLE_ERR,
	DWC_OTG_HC_XFER_AHB_ERR,
	DWC_OTG_HC_XFER_PERIODIC_INCOMPLETE,
	DWC_OTG_HC_XFER_URB_DEQUEUE
} dwc_otg_halt_status_e;

typedef struct dwc_hc {
	
	uint8_t hc_num;

	
	unsigned dev_addr:7;

	
	unsigned ep_num:4;

	
	unsigned ep_is_in:1;

	unsigned speed:2;
#define DWC_OTG_EP_SPEED_LOW	0
#define DWC_OTG_EP_SPEED_FULL	1
#define DWC_OTG_EP_SPEED_HIGH	2

	unsigned ep_type:2;

	
	unsigned max_packet:11;

	unsigned data_pid_start:2;
#define DWC_OTG_HC_PID_DATA0 0
#define DWC_OTG_HC_PID_DATA2 1
#define DWC_OTG_HC_PID_DATA1 2
#define DWC_OTG_HC_PID_MDATA 3
#define DWC_OTG_HC_PID_SETUP 3

	
	unsigned multi_count:2;

	
	

	
	uint8_t *xfer_buff;
	dwc_dma_t align_buff;
	
	uint32_t xfer_len;
	
	uint32_t xfer_count;
	
	uint16_t start_pkt_count;

	uint8_t xfer_started;

	uint8_t do_ping;

	uint8_t error_state;

	uint8_t halt_on_queue;

	uint8_t halt_pending;

	dwc_otg_halt_status_e halt_status;

	uint8_t do_split;		   
	uint8_t complete_split;	   
	uint8_t hub_addr;		   

	uint8_t port_addr;		   
	uint8_t xact_pos;

	
	uint8_t short_read;

	uint8_t requests;

	struct dwc_otg_qh *qh;

	

	
	 DWC_CIRCLEQ_ENTRY(dwc_hc) hc_list_entry;

	
	

	
	uint16_t ntd;

	
	dwc_dma_t desc_list_addr;

	
	uint8_t schinfo;

	
} dwc_hc_t;

typedef struct dwc_otg_core_params {
	int32_t opt;

	int32_t otg_cap;

	int32_t dma_enable;

	int32_t dma_desc_enable;
	int32_t dma_burst_size;	

	int32_t speed;
	int32_t host_support_fs_ls_low_power;

	int32_t host_ls_low_power_phy_clk;

	int32_t enable_dynamic_fifo;

	int32_t data_fifo_size;

	int32_t dev_rx_fifo_size;

	int32_t dev_nperio_tx_fifo_size;

	uint32_t dev_perio_tx_fifo_size[MAX_PERIO_FIFOS];

	int32_t host_rx_fifo_size;

	int32_t host_nperio_tx_fifo_size;

	int32_t host_perio_tx_fifo_size;

	int32_t max_transfer_size;

	int32_t max_packet_count;

	int32_t host_channels;

	int32_t dev_endpoints;

	int32_t phy_type;

	int32_t phy_utmi_width;

	int32_t phy_ulpi_ddr;

	int32_t phy_ulpi_ext_vbus;

	int32_t i2c_enable;

	int32_t ulpi_fs_ls;

	int32_t ts_dline;

	int32_t en_multiple_tx_fifo;

	uint32_t dev_tx_fifo_size[MAX_TX_FIFOS];

	uint32_t thr_ctl;

	uint32_t tx_thr_length;

	uint32_t rx_thr_length;

	int32_t lpm_enable;

	int32_t pti_enable;

	int32_t mpi_enable;

	int32_t ic_usb_cap;

	int32_t ahb_thr_ratio;

	int32_t adp_supp_enable;

	int32_t reload_ctl;

	int32_t dev_out_nak;

	int32_t cont_on_bna;

	int32_t ahb_single;

	int32_t power_down;

	int32_t otg_ver;

} dwc_otg_core_params_t;

#ifdef DEBUG
struct dwc_otg_core_if;
typedef struct hc_xfer_info {
	struct dwc_otg_core_if *core_if;
	dwc_hc_t *hc;
} hc_xfer_info_t;
#endif

typedef struct ep_xfer_info {
	struct dwc_otg_core_if *core_if;
	dwc_ep_t *ep;
	uint8_t state;
} ep_xfer_info_t;
typedef enum dwc_otg_lx_state {
	
	DWC_OTG_L0,
	
	DWC_OTG_L1,
	
	DWC_OTG_L2,
	
	DWC_OTG_L3
} dwc_otg_lx_state_e;

struct dwc_otg_global_regs_backup {
	uint32_t gotgctl_local;
	uint32_t gintmsk_local;
	uint32_t gahbcfg_local;
	uint32_t gusbcfg_local;
	uint32_t grxfsiz_local;
	uint32_t gnptxfsiz_local;
#ifdef CONFIG_USB_DWC_OTG_LPM
	uint32_t glpmcfg_local;
#endif
	uint32_t gi2cctl_local;
	uint32_t hptxfsiz_local;
	uint32_t pcgcctl_local;
	uint32_t gdfifocfg_local;
	uint32_t dtxfsiz_local[MAX_EPS_CHANNELS];
	uint32_t gpwrdn_local;
	uint32_t xhib_pcgcctl;
	uint32_t xhib_gpwrdn;
};

struct dwc_otg_host_regs_backup {
	uint32_t hcfg_local;
	uint32_t haintmsk_local;
	uint32_t hcintmsk_local[MAX_EPS_CHANNELS];
	uint32_t hprt0_local;
	uint32_t hfir_local;
};

struct dwc_otg_dev_regs_backup {
	uint32_t dcfg;
	uint32_t dctl;
	uint32_t daintmsk;
	uint32_t diepmsk;
	uint32_t doepmsk;
	uint32_t diepctl[MAX_EPS_CHANNELS];
	uint32_t dieptsiz[MAX_EPS_CHANNELS];
	uint32_t diepdma[MAX_EPS_CHANNELS];
};
struct dwc_otg_core_if {
	
	dwc_otg_core_params_t *core_params;

	
	dwc_otg_core_global_regs_t *core_global_regs;

	
	dwc_otg_dev_if_t *dev_if;
	
	dwc_otg_host_if_t *host_if;

	
	uint32_t snpsid;

	uint8_t phy_init_done;

	uint8_t srp_success;
	uint8_t srp_timer_started;
	dwc_timer_t *srp_timer;

#ifdef DWC_DEV_SRPCAP
	uint8_t pwron_timer_started;
	dwc_timer_t *pwron_timer;
#endif
	
	
	volatile uint32_t *pcgcctl;
#define DWC_OTG_PCGCCTL_OFFSET 0xE00

	
	uint32_t *data_fifo[MAX_EPS_CHANNELS];
#define DWC_OTG_DATA_FIFO_OFFSET 0x1000
#define DWC_OTG_DATA_FIFO_SIZE 0x1000

	
	uint16_t total_fifo_size;
	
	uint16_t rx_fifo_size;
	
	uint16_t nperio_tx_fifo_size;

	
	uint8_t dma_enable;

	
	uint8_t dma_desc_enable;

	
	uint8_t pti_enh_enable;

	
	uint8_t multiproc_int_enable;

	
	uint8_t en_multiple_tx_fifo;

	uint8_t queuing_high_bandwidth;

	
	hwcfg1_data_t hwcfg1;
	hwcfg2_data_t hwcfg2;
	hwcfg3_data_t hwcfg3;
	hwcfg4_data_t hwcfg4;
	fifosize_data_t hptxfsiz;

	
	hcfg_data_t hcfg;
	dcfg_data_t dcfg;

	uint8_t op_state;

	uint8_t restart_hcd_on_session_req;

	
	
#define A_HOST		(1)
	
#define A_SUSPEND	(2)
	
#define A_PERIPHERAL	(3)
	
#define B_PERIPHERAL	(4)
	
#define B_HOST		(5)

	
	struct dwc_otg_cil_callbacks *hcd_cb;
	
	struct dwc_otg_cil_callbacks *pcd_cb;

	
	uint32_t p_tx_msk;
	
	uint32_t tx_msk;

	
	dwc_workq_t *wq_otg;

	
	dwc_timer_t *wkp_timer;
	
	uint32_t start_doeptsiz_val[MAX_EPS_CHANNELS];
	ep_xfer_info_t ep_xfer_info[MAX_EPS_CHANNELS];
	dwc_timer_t *ep_xfer_timer[MAX_EPS_CHANNELS];
#ifdef DEBUG
	uint32_t start_hcchar_val[MAX_EPS_CHANNELS];

	hc_xfer_info_t hc_xfer_info[MAX_EPS_CHANNELS];
	dwc_timer_t *hc_xfer_timer[MAX_EPS_CHANNELS];

	uint32_t hfnum_7_samples;
	uint64_t hfnum_7_frrem_accum;
	uint32_t hfnum_0_samples;
	uint64_t hfnum_0_frrem_accum;
	uint32_t hfnum_other_samples;
	uint64_t hfnum_other_frrem_accum;
#endif

#ifdef DWC_UTE_CFI
	uint16_t pwron_rxfsiz;
	uint16_t pwron_gnptxfsiz;
	uint16_t pwron_txfsiz[15];

	uint16_t init_rxfsiz;
	uint16_t init_gnptxfsiz;
	uint16_t init_txfsiz[15];
#endif

	
	dwc_otg_lx_state_e lx_state;

	
	struct dwc_otg_global_regs_backup *gr_backup;
	
	struct dwc_otg_host_regs_backup *hr_backup;
	
	struct dwc_otg_dev_regs_backup *dr_backup;

	
	uint32_t power_down;

	
	uint32_t adp_enable;

	
	dwc_otg_adp_t adp;

	
	int hibernation_suspend;

	
	int xhib;

	
	uint32_t otg_ver;

	
	uint8_t otg_sts;

	
	dwc_spinlock_t *lock;

	uint8_t start_predict;

	uint8_t nextep_seq[MAX_EPS_CHANNELS];

	
	uint8_t first_in_nextep_seq;

	
	uint32_t frame_num;

};

#ifdef DEBUG
extern void hc_xfer_timeout(void *ptr);
#endif

extern void ep_xfer_timeout(void *ptr);

extern void w_conn_id_status_change(void *p);

extern void w_wakeup_detected(void *p);

extern int dwc_otg_save_global_regs(dwc_otg_core_if_t * core_if);
extern int dwc_otg_save_dev_regs(dwc_otg_core_if_t * core_if);
extern int dwc_otg_save_host_regs(dwc_otg_core_if_t * core_if);
extern int dwc_otg_restore_global_regs(dwc_otg_core_if_t * core_if);
extern int dwc_otg_restore_host_regs(dwc_otg_core_if_t * core_if, int reset);
extern int dwc_otg_restore_dev_regs(dwc_otg_core_if_t * core_if,
				    int rem_wakeup);
extern int restore_lpm_i2c_regs(dwc_otg_core_if_t * core_if);
extern int restore_essential_regs(dwc_otg_core_if_t * core_if, int rmode,
				  int is_host);

extern int dwc_otg_host_hibernation_restore(dwc_otg_core_if_t * core_if,
					    int restore_mode, int reset);
extern int dwc_otg_device_hibernation_restore(dwc_otg_core_if_t * core_if,
					      int rem_wakeup, int reset);

extern void dwc_otg_core_host_init(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_core_dev_init(dwc_otg_core_if_t * _core_if);

extern void dwc_otg_wakeup(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_read_setup_packet(dwc_otg_core_if_t * _core_if,
				      uint32_t * _dest);
extern uint32_t dwc_otg_get_frame_number(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_ep0_activate(dwc_otg_core_if_t * _core_if, dwc_ep_t * _ep);
extern void dwc_otg_ep_activate(dwc_otg_core_if_t * _core_if, dwc_ep_t * _ep);
extern void dwc_otg_ep_deactivate(dwc_otg_core_if_t * _core_if, dwc_ep_t * _ep);
extern void dwc_otg_ep_start_transfer(dwc_otg_core_if_t * _core_if,
				      dwc_ep_t * _ep);
extern void dwc_otg_ep_start_zl_transfer(dwc_otg_core_if_t * _core_if,
					 dwc_ep_t * _ep);
extern void dwc_otg_ep0_start_transfer(dwc_otg_core_if_t * _core_if,
				       dwc_ep_t * _ep);
extern void dwc_otg_ep0_continue_transfer(dwc_otg_core_if_t * _core_if,
					  dwc_ep_t * _ep);
extern void dwc_otg_ep_write_packet(dwc_otg_core_if_t * _core_if,
				    dwc_ep_t * _ep, int _dma);
extern void dwc_otg_ep_set_stall(dwc_otg_core_if_t * _core_if, dwc_ep_t * _ep);
extern void dwc_otg_ep_clear_stall(dwc_otg_core_if_t * _core_if,
				   dwc_ep_t * _ep);
extern void dwc_otg_enable_device_interrupts(dwc_otg_core_if_t * _core_if);

#ifdef DWC_EN_ISOC
extern void dwc_otg_iso_ep_start_frm_transfer(dwc_otg_core_if_t * core_if,
					      dwc_ep_t * ep);
extern void dwc_otg_iso_ep_start_buf_transfer(dwc_otg_core_if_t * core_if,
					      dwc_ep_t * ep);
#endif 

extern void dwc_otg_hc_init(dwc_otg_core_if_t * _core_if, dwc_hc_t * _hc);
extern void dwc_otg_hc_halt(dwc_otg_core_if_t * _core_if,
			    dwc_hc_t * _hc, dwc_otg_halt_status_e _halt_status);
extern void dwc_otg_hc_cleanup(dwc_otg_core_if_t * _core_if, dwc_hc_t * _hc);
extern void dwc_otg_hc_start_transfer(dwc_otg_core_if_t * _core_if,
				      dwc_hc_t * _hc);
extern int dwc_otg_hc_continue_transfer(dwc_otg_core_if_t * _core_if,
					dwc_hc_t * _hc);
extern void dwc_otg_hc_do_ping(dwc_otg_core_if_t * _core_if, dwc_hc_t * _hc);
extern void dwc_otg_hc_write_packet(dwc_otg_core_if_t * _core_if,
				    dwc_hc_t * _hc);
extern void dwc_otg_enable_host_interrupts(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_disable_host_interrupts(dwc_otg_core_if_t * _core_if);

extern void dwc_otg_hc_start_transfer_ddma(dwc_otg_core_if_t * core_if,
					   dwc_hc_t * hc);

extern uint32_t calc_frame_interval(dwc_otg_core_if_t * core_if);

#define clear_hc_int(_hc_regs_, _intr_) \
do { \
	hcint_data_t hcint_clear = {.d32 = 0}; \
	hcint_clear.b._intr_ = 1; \
	DWC_WRITE_REG32(&(_hc_regs_)->hcint, hcint_clear.d32); \
} while (0)

#define disable_hc_int(_hc_regs_, _intr_) \
do { \
	hcintmsk_data_t hcintmsk = {.d32 = 0}; \
	hcintmsk.b._intr_ = 1; \
	DWC_MODIFY_REG32(&(_hc_regs_)->hcintmsk, hcintmsk.d32, 0); \
} while (0)

static inline uint32_t dwc_otg_read_hprt0(dwc_otg_core_if_t * _core_if)
{
	hprt0_data_t hprt0;
	hprt0.d32 = DWC_READ_REG32(_core_if->host_if->hprt0);
	hprt0.b.prtena = 0;
	hprt0.b.prtconndet = 0;
	hprt0.b.prtenchng = 0;
	hprt0.b.prtovrcurrchng = 0;
	return hprt0.d32;
}



extern void dwc_otg_read_packet(dwc_otg_core_if_t * core_if,
				uint8_t * dest, uint16_t bytes);

extern void dwc_otg_flush_tx_fifo(dwc_otg_core_if_t * _core_if, const int _num);
extern void dwc_otg_flush_rx_fifo(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_core_reset(dwc_otg_core_if_t * _core_if);

static inline uint32_t dwc_otg_read_core_intr(dwc_otg_core_if_t * core_if)
{
	return (DWC_READ_REG32(&core_if->core_global_regs->gintsts) &
		DWC_READ_REG32(&core_if->core_global_regs->gintmsk));
}

static inline uint32_t dwc_otg_read_otg_intr(dwc_otg_core_if_t * core_if)
{
	return (DWC_READ_REG32(&core_if->core_global_regs->gotgint));
}

static inline uint32_t dwc_otg_read_dev_all_in_ep_intr(dwc_otg_core_if_t *
						       core_if)
{

	uint32_t v;

	if (core_if->multiproc_int_enable) {
		v = DWC_READ_REG32(&core_if->dev_if->
				   dev_global_regs->deachint) &
		    DWC_READ_REG32(&core_if->
				   dev_if->dev_global_regs->deachintmsk);
	} else {
		v = DWC_READ_REG32(&core_if->dev_if->dev_global_regs->daint) &
		    DWC_READ_REG32(&core_if->dev_if->dev_global_regs->daintmsk);
	}
	return (v & 0xffff);
}

static inline uint32_t dwc_otg_read_dev_all_out_ep_intr(dwc_otg_core_if_t *
							core_if)
{
	uint32_t v;

	if (core_if->multiproc_int_enable) {
		v = DWC_READ_REG32(&core_if->dev_if->
				   dev_global_regs->deachint) &
		    DWC_READ_REG32(&core_if->
				   dev_if->dev_global_regs->deachintmsk);
	} else {
		v = DWC_READ_REG32(&core_if->dev_if->dev_global_regs->daint) &
		    DWC_READ_REG32(&core_if->dev_if->dev_global_regs->daintmsk);
	}

	return ((v & 0xffff0000) >> 16);
}

static inline uint32_t dwc_otg_read_dev_in_ep_intr(dwc_otg_core_if_t * core_if,
						   dwc_ep_t * ep)
{
	dwc_otg_dev_if_t *dev_if = core_if->dev_if;
	uint32_t v, msk, emp;

	if (core_if->multiproc_int_enable) {
		msk =
		    DWC_READ_REG32(&dev_if->
				   dev_global_regs->diepeachintmsk[ep->num]);
		emp =
		    DWC_READ_REG32(&dev_if->
				   dev_global_regs->dtknqr4_fifoemptymsk);
		msk |= ((emp >> ep->num) & 0x1) << 7;
		v = DWC_READ_REG32(&dev_if->in_ep_regs[ep->num]->diepint) & msk;
	} else {
		msk = DWC_READ_REG32(&dev_if->dev_global_regs->diepmsk);
		emp =
		    DWC_READ_REG32(&dev_if->
				   dev_global_regs->dtknqr4_fifoemptymsk);
		msk |= ((emp >> ep->num) & 0x1) << 7;
		v = DWC_READ_REG32(&dev_if->in_ep_regs[ep->num]->diepint) & msk;
	}

	return v;
}

static inline uint32_t dwc_otg_read_dev_out_ep_intr(dwc_otg_core_if_t *
						    _core_if, dwc_ep_t * _ep)
{
	dwc_otg_dev_if_t *dev_if = _core_if->dev_if;
	uint32_t v;
	doepmsk_data_t msk = {.d32 = 0 };

	if (_core_if->multiproc_int_enable) {
		msk.d32 =
		    DWC_READ_REG32(&dev_if->
				   dev_global_regs->doepeachintmsk[_ep->num]);
		if (_core_if->pti_enh_enable) {
			msk.b.pktdrpsts = 1;
		}
		v = DWC_READ_REG32(&dev_if->
				   out_ep_regs[_ep->num]->doepint) & msk.d32;
	} else {
		msk.d32 = DWC_READ_REG32(&dev_if->dev_global_regs->doepmsk);
		if (_core_if->pti_enh_enable) {
			msk.b.pktdrpsts = 1;
		}
		v = DWC_READ_REG32(&dev_if->
				   out_ep_regs[_ep->num]->doepint) & msk.d32;
	}
	return v;
}

static inline uint32_t dwc_otg_read_host_all_channels_intr(dwc_otg_core_if_t *
							   _core_if)
{
	return (DWC_READ_REG32(&_core_if->host_if->host_global_regs->haint));
}

static inline uint32_t dwc_otg_read_host_channel_intr(dwc_otg_core_if_t *
						      _core_if, dwc_hc_t * _hc)
{
	return (DWC_READ_REG32
		(&_core_if->host_if->hc_regs[_hc->hc_num]->hcint));
}

static inline uint32_t dwc_otg_mode(dwc_otg_core_if_t * _core_if)
{
	return (DWC_READ_REG32(&_core_if->core_global_regs->gintsts) & 0x1);
}


typedef struct dwc_otg_cil_callbacks {
	
	int (*start) (void *_p);
	
	int (*stop) (void *_p, int mute_disconnect);
	
	int (*disconnect) (void *_p);
	
	int (*resume_wakeup) (void *_p);
	
	int (*suspend) (void *_p);
	
	int (*session_start) (void *_p);
#ifdef CONFIG_USB_DWC_OTG_LPM
	
	int (*sleep) (void *_p);
#endif
	
	void *p;
} dwc_otg_cil_callbacks_t;

extern void dwc_otg_cil_register_pcd_callbacks(dwc_otg_core_if_t * _core_if,
					       dwc_otg_cil_callbacks_t * _cb,
					       void *_p);
extern void dwc_otg_cil_register_hcd_callbacks(dwc_otg_core_if_t * _core_if,
					       dwc_otg_cil_callbacks_t * _cb,
					       void *_p);

void dwc_otg_initiate_srp(dwc_otg_core_if_t * core_if);

static inline void cil_hcd_start(dwc_otg_core_if_t * core_if)
{
	if (core_if->hcd_cb && core_if->hcd_cb->start) {
		core_if->hcd_cb->start(core_if->hcd_cb->p);
	}
}

static inline void cil_hcd_stop(dwc_otg_core_if_t * core_if)
{
	if (core_if->hcd_cb && core_if->hcd_cb->stop) {
		core_if->hcd_cb->stop(core_if->hcd_cb->p, 0);
	}
}

static inline void cil_hcd_disconnect(dwc_otg_core_if_t * core_if)
{
	if (core_if->hcd_cb && core_if->hcd_cb->disconnect) {
		core_if->hcd_cb->disconnect(core_if->hcd_cb->p);
	}
}

static inline void cil_hcd_session_start(dwc_otg_core_if_t * core_if)
{
	if (core_if->hcd_cb && core_if->hcd_cb->session_start) {
		core_if->hcd_cb->session_start(core_if->hcd_cb->p);
	}
}

#ifdef CONFIG_USB_DWC_OTG_LPM
static inline void cil_hcd_sleep(dwc_otg_core_if_t * core_if)
{
	if (core_if->hcd_cb && core_if->hcd_cb->sleep) {
		core_if->hcd_cb->sleep(core_if->hcd_cb->p);
	}
}
#endif

static inline void cil_hcd_resume(dwc_otg_core_if_t * core_if)
{
	if (core_if->hcd_cb && core_if->hcd_cb->resume_wakeup) {
		core_if->hcd_cb->resume_wakeup(core_if->hcd_cb->p);
	}
}

static inline void cil_pcd_start(dwc_otg_core_if_t * core_if)
{
	if (core_if->pcd_cb && core_if->pcd_cb->start) {
		core_if->pcd_cb->start(core_if->pcd_cb->p);
	}
}

static inline void cil_pcd_stop(dwc_otg_core_if_t * core_if, int mute_disconnect)
{
	if (core_if->pcd_cb && core_if->pcd_cb->stop) {
		core_if->pcd_cb->stop(core_if->pcd_cb->p, mute_disconnect);
	}
}

static inline void cil_pcd_suspend(dwc_otg_core_if_t * core_if)
{
	if (core_if->pcd_cb && core_if->pcd_cb->suspend) {
		core_if->pcd_cb->suspend(core_if->pcd_cb->p);
	}
}

static inline void cil_pcd_resume(dwc_otg_core_if_t * core_if)
{
	if (core_if->pcd_cb && core_if->pcd_cb->resume_wakeup) {
		core_if->pcd_cb->resume_wakeup(core_if->pcd_cb->p);
	}
}


#endif
