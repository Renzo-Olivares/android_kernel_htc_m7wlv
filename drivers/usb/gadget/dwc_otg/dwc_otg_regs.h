/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_regs.h $
 * $Revision: #98 $
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

#ifndef __DWC_OTG_REGS_H__
#define __DWC_OTG_REGS_H__

#include "dwc_otg_core_if.h"


typedef struct dwc_otg_core_global_regs {
	
	volatile uint32_t gotgctl;
	
	volatile uint32_t gotgint;
	
	volatile uint32_t gahbcfg;

#define DWC_GLBINTRMASK		0x0001
#define DWC_DMAENABLE		0x0020
#define DWC_NPTXEMPTYLVL_EMPTY	0x0080
#define DWC_NPTXEMPTYLVL_HALFEMPTY	0x0000
#define DWC_PTXEMPTYLVL_EMPTY	0x0100
#define DWC_PTXEMPTYLVL_HALFEMPTY	0x0000

	
	volatile uint32_t gusbcfg;
	
	volatile uint32_t grstctl;
	
	volatile uint32_t gintsts;
	
	volatile uint32_t gintmsk;
	
	volatile uint32_t grxstsr;
	
	volatile uint32_t grxstsp;
	
	volatile uint32_t grxfsiz;
	
	volatile uint32_t gnptxfsiz;
	volatile uint32_t gnptxsts;
	
	volatile uint32_t gi2cctl;
	
	volatile uint32_t gpvndctl;
	
	volatile uint32_t ggpio;
	
	volatile uint32_t guid;
	
	volatile uint32_t gsnpsid;
	
	volatile uint32_t ghwcfg1;
	
	volatile uint32_t ghwcfg2;
#define DWC_SLAVE_ONLY_ARCH 0
#define DWC_EXT_DMA_ARCH 1
#define DWC_INT_DMA_ARCH 2

#define DWC_MODE_HNP_SRP_CAPABLE	0
#define DWC_MODE_SRP_ONLY_CAPABLE	1
#define DWC_MODE_NO_HNP_SRP_CAPABLE		2
#define DWC_MODE_SRP_CAPABLE_DEVICE		3
#define DWC_MODE_NO_SRP_CAPABLE_DEVICE	4
#define DWC_MODE_SRP_CAPABLE_HOST	5
#define DWC_MODE_NO_SRP_CAPABLE_HOST	6

	
	volatile uint32_t ghwcfg3;
	
	volatile uint32_t ghwcfg4;
	
	volatile uint32_t glpmcfg;
	
	volatile uint32_t gpwrdn;
	
	volatile uint32_t gdfifocfg;
	
	volatile uint32_t adpctl;
	
	volatile uint32_t reserved39[39];
	
	volatile uint32_t hptxfsiz;
	volatile uint32_t dtxfsiz[15];
} dwc_otg_core_global_regs_t;

typedef union gotgctl_data {
	
	uint32_t d32;
	
	struct {
		unsigned sesreqscs:1;
		unsigned sesreq:1;
		unsigned vbvalidoven:1;
		unsigned vbvalidovval:1;
		unsigned avalidoven:1;
		unsigned avalidovval:1;
		unsigned bvalidoven:1;
		unsigned bvalidovval:1;
		unsigned hstnegscs:1;
		unsigned hnpreq:1;
		unsigned hstsethnpen:1;
		unsigned devhnpen:1;
		unsigned reserved12_15:4;
		unsigned conidsts:1;
		unsigned dbnctime:1;
		unsigned asesvld:1;
		unsigned bsesvld:1;
		unsigned otgver:1;
		unsigned reserved1:1;
		unsigned multvalidbc:5;
		unsigned chirpen:1;
		unsigned reserved28_31:4;
	} b;
} gotgctl_data_t;

typedef union gotgint_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned reserved0_1:2;

		
		unsigned sesenddet:1;

		unsigned reserved3_7:5;

		
		unsigned sesreqsucstschng:1;
		
		unsigned hstnegsucstschng:1;

		unsigned reserved10_16:7;

		
		unsigned hstnegdet:1;
		
		unsigned adevtoutchng:1;
		
		unsigned debdone:1;
		
		unsigned mvic:1;

		unsigned reserved31_21:11;

	} b;
} gotgint_data_t;

typedef union gahbcfg_data {
	
	uint32_t d32;
	
	struct {
		unsigned glblintrmsk:1;
#define DWC_GAHBCFG_GLBINT_ENABLE		1

		unsigned hburstlen:4;
#define DWC_GAHBCFG_INT_DMA_BURST_SINGLE	0
#define DWC_GAHBCFG_INT_DMA_BURST_INCR		1
#define DWC_GAHBCFG_INT_DMA_BURST_INCR4		3
#define DWC_GAHBCFG_INT_DMA_BURST_INCR8		5
#define DWC_GAHBCFG_INT_DMA_BURST_INCR16	7

		unsigned dmaenable:1;
#define DWC_GAHBCFG_DMAENABLE			1
		unsigned reserved:1;
		unsigned nptxfemplvl_txfemplvl:1;
		unsigned ptxfemplvl:1;
#define DWC_GAHBCFG_TXFEMPTYLVL_EMPTY		1
#define DWC_GAHBCFG_TXFEMPTYLVL_HALFEMPTY	0
		unsigned reserved9_20:12;
		unsigned remmemsupp:1;
		unsigned notialldmawrit:1;
		unsigned ahbsingle:1;
		unsigned reserved24_31:8;
	} b;
} gahbcfg_data_t;

typedef union gusbcfg_data {
	
	uint32_t d32;
	
	struct {
		unsigned toutcal:3;
		unsigned phyif:1;
		unsigned ulpi_utmi_sel:1;
		unsigned fsintf:1;
		unsigned physel:1;
		unsigned ddrsel:1;
		unsigned srpcap:1;
		unsigned hnpcap:1;
		unsigned usbtrdtim:4;
		unsigned reserved1:1;
		unsigned phylpwrclksel:1;
		unsigned otgutmifssel:1;
		unsigned ulpi_fsls:1;
		unsigned ulpi_auto_res:1;
		unsigned ulpi_clk_sus_m:1;
		unsigned ulpi_ext_vbus_drv:1;
		unsigned ulpi_int_vbus_indicator:1;
		unsigned term_sel_dl_pulse:1;
		unsigned indicator_complement:1;
		unsigned indicator_pass_through:1;
		unsigned ulpi_int_prot_dis:1;
		unsigned ic_usb_cap:1;
		unsigned ic_traffic_pull_remove:1;
		unsigned tx_end_delay:1;
		unsigned force_host_mode:1;
		unsigned force_dev_mode:1;
		unsigned reserved31:1;
	} b;
} gusbcfg_data_t;

typedef union grstctl_data {
	
	uint32_t d32;
	
	struct {
		unsigned csftrst:1;
		unsigned hsftrst:1;
		unsigned hstfrm:1;
		unsigned intknqflsh:1;
		unsigned rxfflsh:1;
		unsigned txfflsh:1;

		unsigned txfnum:5;
		
		unsigned reserved11_29:19;
		unsigned dmareq:1;
		unsigned ahbidle:1;
	} b;
} grstctl_t;

typedef union gintmsk_data {
	
	uint32_t d32;
	
	struct {
		unsigned reserved0:1;
		unsigned modemismatch:1;
		unsigned otgintr:1;
		unsigned sofintr:1;
		unsigned rxstsqlvl:1;
		unsigned nptxfempty:1;
		unsigned ginnakeff:1;
		unsigned goutnakeff:1;
		unsigned ulpickint:1;
		unsigned i2cintr:1;
		unsigned erlysuspend:1;
		unsigned usbsuspend:1;
		unsigned usbreset:1;
		unsigned enumdone:1;
		unsigned isooutdrop:1;
		unsigned eopframe:1;
		unsigned restoredone:1;
		unsigned epmismatch:1;
		unsigned inepintr:1;
		unsigned outepintr:1;
		unsigned incomplisoin:1;
		unsigned incomplisoout:1;
		unsigned fetsusp:1;
		unsigned resetdet:1;
		unsigned portintr:1;
		unsigned hcintr:1;
		unsigned ptxfempty:1;
		unsigned lpmtranrcvd:1;
		unsigned conidstschng:1;
		unsigned disconnect:1;
		unsigned sessreqintr:1;
		unsigned wkupintr:1;
	} b;
} gintmsk_data_t;
typedef union gintsts_data {
	
	uint32_t d32;
#define DWC_SOF_INTR_MASK 0x0008
	
	struct {
#define DWC_HOST_MODE 1
		unsigned curmode:1;
		unsigned modemismatch:1;
		unsigned otgintr:1;
		unsigned sofintr:1;
		unsigned rxstsqlvl:1;
		unsigned nptxfempty:1;
		unsigned ginnakeff:1;
		unsigned goutnakeff:1;
		unsigned ulpickint:1;
		unsigned i2cintr:1;
		unsigned erlysuspend:1;
		unsigned usbsuspend:1;
		unsigned usbreset:1;
		unsigned enumdone:1;
		unsigned isooutdrop:1;
		unsigned eopframe:1;
		unsigned restoredone:1;
		unsigned epmismatch:1;
		unsigned inepint:1;
		unsigned outepintr:1;
		unsigned incomplisoin:1;
		unsigned incomplisoout:1;
		unsigned fetsusp:1;
		unsigned resetdet:1;
		unsigned portintr:1;
		unsigned hcintr:1;
		unsigned ptxfempty:1;
		unsigned lpmtranrcvd:1;
		unsigned conidstschng:1;
		unsigned disconnect:1;
		unsigned sessreqintr:1;
		unsigned wkupintr:1;
	} b;
} gintsts_data_t;

typedef union device_grxsts_data {
	
	uint32_t d32;
	
	struct {
		unsigned epnum:4;
		unsigned bcnt:11;
		unsigned dpid:2;

#define DWC_STS_DATA_UPDT		0x2	
#define DWC_STS_XFER_COMP		0x3	

#define DWC_DSTS_GOUT_NAK		0x1	
#define DWC_DSTS_SETUP_COMP		0x4	
#define DWC_DSTS_SETUP_UPDT 0x6	
		unsigned pktsts:4;
		unsigned fn:4;
		unsigned reserved25_31:7;
	} b;
} device_grxsts_data_t;

typedef union host_grxsts_data {
	
	uint32_t d32;
	
	struct {
		unsigned chnum:4;
		unsigned bcnt:11;
		unsigned dpid:2;

		unsigned pktsts:4;
#define DWC_GRXSTS_PKTSTS_IN			  0x2
#define DWC_GRXSTS_PKTSTS_IN_XFER_COMP	  0x3
#define DWC_GRXSTS_PKTSTS_DATA_TOGGLE_ERR 0x5
#define DWC_GRXSTS_PKTSTS_CH_HALTED		  0x7

		unsigned reserved21_31:11;
	} b;
} host_grxsts_data_t;

typedef union fifosize_data {
	
	uint32_t d32;
	
	struct {
		unsigned startaddr:16;
		unsigned depth:16;
	} b;
} fifosize_data_t;

typedef union gnptxsts_data {
	
	uint32_t d32;
	
	struct {
		unsigned nptxfspcavail:16;
		unsigned nptxqspcavail:8;
		unsigned nptxqtop_terminate:1;
		unsigned nptxqtop_token:2;
		unsigned nptxqtop_chnep:4;
		unsigned reserved:1;
	} b;
} gnptxsts_data_t;

typedef union dtxfsts_data {
	
	uint32_t d32;
	
	struct {
		unsigned txfspcavail:16;
		unsigned reserved:16;
	} b;
} dtxfsts_data_t;

typedef union gi2cctl_data {
	
	uint32_t d32;
	
	struct {
		unsigned rwdata:8;
		unsigned regaddr:8;
		unsigned addr:7;
		unsigned i2cen:1;
		unsigned ack:1;
		unsigned i2csuspctl:1;
		unsigned i2cdevaddr:2;
		unsigned i2cdatse0:1;
		unsigned reserved:1;
		unsigned rw:1;
		unsigned bsydne:1;
	} b;
} gi2cctl_data_t;

typedef union gpvndctl_data {
	
	uint32_t d32;
	
	struct {
		unsigned regdata:8;
		unsigned vctrl:8;
		unsigned regaddr16_21:6;
		unsigned regwr:1;
		unsigned reserved23_24:2;
		unsigned newregreq:1;
		unsigned vstsbsy:1;
		unsigned vstsdone:1;
		unsigned reserved28_30:3;
		unsigned disulpidrvr:1;
	} b;
} gpvndctl_data_t;

typedef union ggpio_data {
	
	uint32_t d32;
	
	struct {
		unsigned gpi:16;
		unsigned gpo:16;
	} b;
} ggpio_data_t;

typedef union guid_data {
	
	uint32_t d32;
	
	struct {
		unsigned rwdata:32;
	} b;
} guid_data_t;

typedef union gsnpsid_data {
	
	uint32_t d32;
	
	struct {
		unsigned rwdata:32;
	} b;
} gsnpsid_data_t;

typedef union hwcfg1_data {
	
	uint32_t d32;
	
	struct {
		unsigned ep_dir0:2;
		unsigned ep_dir1:2;
		unsigned ep_dir2:2;
		unsigned ep_dir3:2;
		unsigned ep_dir4:2;
		unsigned ep_dir5:2;
		unsigned ep_dir6:2;
		unsigned ep_dir7:2;
		unsigned ep_dir8:2;
		unsigned ep_dir9:2;
		unsigned ep_dir10:2;
		unsigned ep_dir11:2;
		unsigned ep_dir12:2;
		unsigned ep_dir13:2;
		unsigned ep_dir14:2;
		unsigned ep_dir15:2;
	} b;
} hwcfg1_data_t;

typedef union hwcfg2_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned op_mode:3;
#define DWC_HWCFG2_OP_MODE_HNP_SRP_CAPABLE_OTG 0
#define DWC_HWCFG2_OP_MODE_SRP_ONLY_CAPABLE_OTG 1
#define DWC_HWCFG2_OP_MODE_NO_HNP_SRP_CAPABLE_OTG 2
#define DWC_HWCFG2_OP_MODE_SRP_CAPABLE_DEVICE 3
#define DWC_HWCFG2_OP_MODE_NO_SRP_CAPABLE_DEVICE 4
#define DWC_HWCFG2_OP_MODE_SRP_CAPABLE_HOST 5
#define DWC_HWCFG2_OP_MODE_NO_SRP_CAPABLE_HOST 6

		unsigned architecture:2;
		unsigned point2point:1;
		unsigned hs_phy_type:2;
#define DWC_HWCFG2_HS_PHY_TYPE_NOT_SUPPORTED 0
#define DWC_HWCFG2_HS_PHY_TYPE_UTMI 1
#define DWC_HWCFG2_HS_PHY_TYPE_ULPI 2
#define DWC_HWCFG2_HS_PHY_TYPE_UTMI_ULPI 3

		unsigned fs_phy_type:2;
		unsigned num_dev_ep:4;
		unsigned num_host_chan:4;
		unsigned perio_ep_supported:1;
		unsigned dynamic_fifo:1;
		unsigned multi_proc_int:1;
		unsigned reserved21:1;
		unsigned nonperio_tx_q_depth:2;
		unsigned host_perio_tx_q_depth:2;
		unsigned dev_token_q_depth:5;
		unsigned otg_enable_ic_usb:1;
	} b;
} hwcfg2_data_t;

typedef union hwcfg3_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned xfer_size_cntr_width:4;
		unsigned packet_size_cntr_width:3;
		unsigned otg_func:1;
		unsigned i2c:1;
		unsigned vendor_ctrl_if:1;
		unsigned optional_features:1;
		unsigned synch_reset_type:1;
		unsigned adp_supp:1;
		unsigned otg_enable_hsic:1;
		unsigned bc_support:1;
		unsigned otg_lpm_en:1;
		unsigned dfifo_depth:16;
	} b;
} hwcfg3_data_t;

typedef union hwcfg4_data {
	
	uint32_t d32;
	
	struct {
		unsigned num_dev_perio_in_ep:4;
		unsigned power_optimiz:1;
		unsigned min_ahb_freq:1;
		unsigned hiber:1;
		unsigned xhiber:1;
		unsigned reserved:6;
		unsigned utmi_phy_data_width:2;
		unsigned num_dev_mode_ctrl_ep:4;
		unsigned iddig_filt_en:1;
		unsigned vbus_valid_filt_en:1;
		unsigned a_valid_filt_en:1;
		unsigned b_valid_filt_en:1;
		unsigned session_end_filt_en:1;
		unsigned ded_fifo_en:1;
		unsigned num_in_eps:4;
		unsigned desc_dma:1;
		unsigned desc_dma_dyn:1;
	} b;
} hwcfg4_data_t;

typedef union glpmctl_data {
	
	uint32_t d32;
	
	struct {
		unsigned lpm_cap_en:1;
		unsigned appl_resp:1;
		unsigned hird:4;
		unsigned rem_wkup_en:1;
		unsigned en_utmi_sleep:1;
		unsigned hird_thres:5;
		unsigned lpm_resp:2;
		unsigned prt_sleep_sts:1;
		unsigned sleep_state_resumeok:1;
		unsigned lpm_chan_index:4;
		unsigned retry_count:3;
		unsigned send_lpm:1;
		unsigned retry_count_sts:3;
		unsigned reserved28_29:2;
		unsigned hsic_connect:1;
		unsigned inv_sel_hsic:1;
	} b;
} glpmcfg_data_t;

typedef union adpctl_data {
	
	uint32_t d32;
	
	struct {
		unsigned prb_dschg:2;
		unsigned prb_delta:2;
		unsigned prb_per:2;
		unsigned rtim:11;
		unsigned enaprb:1;
		unsigned enasns:1;
		unsigned adpres:1;
		unsigned adpen:1;
		unsigned adp_prb_int:1;
		unsigned adp_sns_int:1;
		unsigned adp_tmout_int:1;
		unsigned adp_prb_int_msk:1;
		unsigned adp_sns_int_msk:1;
		unsigned adp_tmout_int_msk:1;
		unsigned ar:2;
		 
		unsigned reserved29_31:3;
	} b;
} adpctl_data_t;

typedef struct dwc_otg_dev_global_regs {
	
	volatile uint32_t dcfg;
	
	volatile uint32_t dctl;
	
	volatile uint32_t dsts;
	
	uint32_t unused;
	volatile uint32_t diepmsk;
	volatile uint32_t doepmsk;
	
	volatile uint32_t daint;
	volatile uint32_t daintmsk;
	volatile uint32_t dtknqr1;
	volatile uint32_t dtknqr2;
	
	volatile uint32_t dvbusdis;
	
	volatile uint32_t dvbuspulse;
	volatile uint32_t dtknqr3_dthrctl;
	volatile uint32_t dtknqr4_fifoemptymsk;
	volatile uint32_t deachint;
	volatile uint32_t deachintmsk;
	volatile uint32_t diepeachintmsk[MAX_EPS_CHANNELS];
	volatile uint32_t doepeachintmsk[MAX_EPS_CHANNELS];
} dwc_otg_device_global_regs_t;

typedef union dcfg_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned devspd:2;
		
		unsigned nzstsouthshk:1;
#define DWC_DCFG_SEND_STALL 1

		unsigned ena32khzs:1;
		
		unsigned devaddr:7;
		
		unsigned perfrint:2;
#define DWC_DCFG_FRAME_INTERVAL_80 0
#define DWC_DCFG_FRAME_INTERVAL_85 1
#define DWC_DCFG_FRAME_INTERVAL_90 2
#define DWC_DCFG_FRAME_INTERVAL_95 3

		
		unsigned endevoutnak:1;

		unsigned reserved14_17:4;
		
		unsigned epmscnt:5;
		
		unsigned descdma:1;
		unsigned perschintvl:2;
		unsigned resvalid:6;
	} b;
} dcfg_data_t;

typedef union dctl_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned rmtwkupsig:1;
		
		unsigned sftdiscon:1;
		
		unsigned gnpinnaksts:1;
		
		unsigned goutnaksts:1;
		
		unsigned tstctl:3;
		
		unsigned sgnpinnak:1;
		
		unsigned cgnpinnak:1;
		
		unsigned sgoutnak:1;
		
		unsigned cgoutnak:1;
		
		unsigned pwronprgdone:1;
		
		unsigned reserved:1;
		
		unsigned gmc:2;
		
		unsigned ifrmnum:1;
		
		unsigned nakonbble:1;
		
		unsigned encontonbna:1;

		unsigned reserved18_31:14;
	} b;
} dctl_data_t;

typedef union dsts_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned suspsts:1;
		
		unsigned enumspd:2;
#define DWC_DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ 0
#define DWC_DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ 1
#define DWC_DSTS_ENUMSPD_LS_PHY_6MHZ		   2
#define DWC_DSTS_ENUMSPD_FS_PHY_48MHZ		   3
		
		unsigned errticerr:1;
		unsigned reserved4_7:4;
		
		unsigned soffn:14;
		unsigned reserved22_31:10;
	} b;
} dsts_data_t;

typedef union diepint_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned xfercompl:1;
		
		unsigned epdisabled:1;
		
		unsigned ahberr:1;
		
		unsigned timeout:1;
		
		unsigned intktxfemp:1;
		
		unsigned intknepmis:1;
		
		unsigned inepnakeff:1;
		
		unsigned emptyintr:1;

		unsigned txfifoundrn:1;

		
		unsigned bna:1;

		unsigned reserved10_12:3;
		
		unsigned nak:1;

		unsigned reserved14_31:18;
	} b;
} diepint_data_t;

typedef union diepint_data diepmsk_data_t;

typedef union doepint_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned xfercompl:1;
		
		unsigned epdisabled:1;
		
		unsigned ahberr:1;
		
		unsigned setup:1;
		
		unsigned outtknepdis:1;

		unsigned stsphsercvd:1;
		
		unsigned back2backsetup:1;

		unsigned reserved7:1;
		
		unsigned outpkterr:1;
		
		unsigned bna:1;

		unsigned reserved10:1;
		
		unsigned pktdrpsts:1;
		
		unsigned babble:1;
		
		unsigned nak:1;
		
		unsigned nyet:1;
		
		unsigned sr:1;

		unsigned reserved16_31:16;
	} b;
} doepint_data_t;

typedef union doepint_data doepmsk_data_t;

typedef union daint_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned in:16;
		
		unsigned out:16;
	} ep;
	struct {
		
		unsigned inep0:1;
		unsigned inep1:1;
		unsigned inep2:1;
		unsigned inep3:1;
		unsigned inep4:1;
		unsigned inep5:1;
		unsigned inep6:1;
		unsigned inep7:1;
		unsigned inep8:1;
		unsigned inep9:1;
		unsigned inep10:1;
		unsigned inep11:1;
		unsigned inep12:1;
		unsigned inep13:1;
		unsigned inep14:1;
		unsigned inep15:1;
		
		unsigned outep0:1;
		unsigned outep1:1;
		unsigned outep2:1;
		unsigned outep3:1;
		unsigned outep4:1;
		unsigned outep5:1;
		unsigned outep6:1;
		unsigned outep7:1;
		unsigned outep8:1;
		unsigned outep9:1;
		unsigned outep10:1;
		unsigned outep11:1;
		unsigned outep12:1;
		unsigned outep13:1;
		unsigned outep14:1;
		unsigned outep15:1;
	} b;
} daint_data_t;

typedef union dtknq1_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned intknwptr:5;
		
		unsigned reserved05_06:2;
		
		unsigned wrap_bit:1;
		
		unsigned epnums0_5:24;
	} b;
} dtknq1_data_t;

typedef union dthrctl_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned non_iso_thr_en:1;
		
		unsigned iso_thr_en:1;
		
		unsigned tx_thr_len:9;
		
		unsigned ahb_thr_ratio:2;
		
		unsigned reserved13_15:3;
		
		unsigned rx_thr_en:1;
		
		unsigned rx_thr_len:9;
		unsigned reserved26:1;
		
		unsigned arbprken:1;
		
		unsigned reserved28_31:4;
	} b;
} dthrctl_data_t;

typedef struct dwc_otg_dev_in_ep_regs {
	volatile uint32_t diepctl;
	
	uint32_t reserved04;
	volatile uint32_t diepint;
	
	uint32_t reserved0C;
	volatile uint32_t dieptsiz;
	volatile uint32_t diepdma;
	volatile uint32_t dtxfsts;
	volatile uint32_t diepdmab;
} dwc_otg_dev_in_ep_regs_t;

typedef struct dwc_otg_dev_out_ep_regs {
	volatile uint32_t doepctl;
	
	uint32_t reserved04;
	volatile uint32_t doepint;
	
	uint32_t reserved0C;
	volatile uint32_t doeptsiz;
	volatile uint32_t doepdma;
	
	uint32_t unused;
	uint32_t doepdmab;
} dwc_otg_dev_out_ep_regs_t;

typedef union depctl_data {
	
	uint32_t d32;
	
	struct {
		unsigned mps:11;
#define DWC_DEP0CTL_MPS_64	 0
#define DWC_DEP0CTL_MPS_32	 1
#define DWC_DEP0CTL_MPS_16	 2
#define DWC_DEP0CTL_MPS_8	 3

		unsigned nextep:4;

		
		unsigned usbactep:1;

		unsigned dpid:1;

		
		unsigned naksts:1;

		unsigned eptype:2;

		unsigned snp:1;

		
		unsigned stall:1;

		unsigned txfnum:4;

		
		unsigned cnak:1;
		
		unsigned snak:1;
		unsigned setd0pid:1;
		unsigned setd1pid:1;

		
		unsigned epdis:1;
		
		unsigned epena:1;
	} b;
} depctl_data_t;

typedef union deptsiz_data {
		
	uint32_t d32;
		
	struct {
		
		unsigned xfersize:19;
#define MAX_PKT_CNT 1023
		
		unsigned pktcnt:10;
		
		unsigned mc:2;
		unsigned reserved:1;
	} b;
} deptsiz_data_t;

typedef union deptsiz0_data {
		
	uint32_t d32;
		
	struct {
		
		unsigned xfersize:7;
				
		unsigned reserved7_18:12;
		
		unsigned pktcnt:2;
				
		unsigned reserved21_28:8;
				
		unsigned supcnt:2;
		unsigned reserved31;
	} b;
} deptsiz0_data_t;



#define BS_HOST_READY	0x0
#define BS_DMA_BUSY		0x1
#define BS_DMA_DONE		0x2
#define BS_HOST_BUSY	0x3


#define RTS_SUCCESS		0x0
#define RTS_BUFFLUSH	0x1
#define RTS_RESERVED	0x2
#define RTS_BUFERR		0x3

typedef union dev_dma_desc_sts {
		
	uint32_t d32;
		
	struct {
		
		unsigned bytes:16;
		
		unsigned nak:1;
		unsigned reserved17_22:6;
		
		unsigned mtrf:1;
		
		unsigned sr:1;
		
		unsigned ioc:1;
		
		unsigned sp:1;
		
		unsigned l:1;
		
		unsigned sts:2;
		
		unsigned bs:2;
	} b;

		
	struct {
		
		unsigned rxbytes:11;

		unsigned reserved11:1;
		
		unsigned framenum:11;
		
		unsigned pid:2;
		
		unsigned ioc:1;
		
		unsigned sp:1;
		
		unsigned l:1;
		
		unsigned rxsts:2;
		
		unsigned bs:2;
	} b_iso_out;

		
	struct {
		
		unsigned txbytes:12;
		
		unsigned framenum:11;
		
		unsigned pid:2;
		
		unsigned ioc:1;
		
		unsigned sp:1;
		
		unsigned l:1;
		
		unsigned txsts:2;
		
		unsigned bs:2;
	} b_iso_in;
} dev_dma_desc_sts_t;

typedef struct dwc_otg_dev_dma_desc {
	
	dev_dma_desc_sts_t status;
	
	uint32_t buf;
} dwc_otg_dev_dma_desc_t;

typedef struct dwc_otg_dev_if {
	dwc_otg_device_global_regs_t *dev_global_regs;
#define DWC_DEV_GLOBAL_REG_OFFSET 0x800

	dwc_otg_dev_in_ep_regs_t *in_ep_regs[MAX_EPS_CHANNELS];
#define DWC_DEV_IN_EP_REG_OFFSET 0x900
#define DWC_EP_REG_OFFSET 0x20

	
	dwc_otg_dev_out_ep_regs_t *out_ep_regs[MAX_EPS_CHANNELS];
#define DWC_DEV_OUT_EP_REG_OFFSET 0xB00

	
	uint8_t speed;				 
	uint8_t num_in_eps;		 
	uint8_t num_out_eps;		 

	
	uint16_t perio_tx_fifo_size[MAX_PERIO_FIFOS];

	
	uint16_t tx_fifo_size[MAX_TX_FIFOS];

	
	uint16_t rx_thr_en;
	uint16_t iso_tx_thr_en;
	uint16_t non_iso_tx_thr_en;

	uint16_t rx_thr_length;
	uint16_t tx_thr_length;


	
	dwc_dma_t dma_setup_desc_addr[2];
	dwc_otg_dev_dma_desc_t *setup_desc_addr[2];

	
	dwc_otg_dev_dma_desc_t *psetup;

	
	uint32_t setup_desc_index;

	
	dwc_dma_t dma_in_desc_addr;
	dwc_otg_dev_dma_desc_t *in_desc_addr;

	
	dwc_dma_t dma_out_desc_addr;
	dwc_otg_dev_dma_desc_t *out_desc_addr;

	
	uint32_t spd;
	
	void *isoc_ep;

} dwc_otg_dev_if_t;

typedef struct dwc_otg_host_global_regs {
	
	volatile uint32_t hcfg;
	
	volatile uint32_t hfir;
	
	volatile uint32_t hfnum;
	
	uint32_t reserved40C;
	
	volatile uint32_t hptxsts;
	
	volatile uint32_t haint;
	
	volatile uint32_t haintmsk;
	
	volatile uint32_t hflbaddr;
} dwc_otg_host_global_regs_t;

typedef union hcfg_data {
	
	uint32_t d32;

	
	struct {
		
		unsigned fslspclksel:2;
#define DWC_HCFG_30_60_MHZ 0
#define DWC_HCFG_48_MHZ	   1
#define DWC_HCFG_6_MHZ	   2

		
		unsigned fslssupp:1;
		unsigned reserved3_6:4;
		
		unsigned ena32khzs:1;
		
		unsigned resvalid:8;
		unsigned reserved16_22:7;
		
		unsigned descdma:1;
		
		unsigned frlisten:2;
		
		unsigned perschedena:1;
		unsigned reserved27_30:4;
		unsigned modechtimen:1;
	} b;
} hcfg_data_t;

typedef union hfir_data {
	
	uint32_t d32;

	
	struct {
		unsigned frint:16;
		unsigned hfirrldctrl:1;
		unsigned reserved:15;
	} b;
} hfir_data_t;

typedef union hfnum_data {
	
	uint32_t d32;

	
	struct {
		unsigned frnum:16;
#define DWC_HFNUM_MAX_FRNUM 0x3FFF
		unsigned frrem:16;
	} b;
} hfnum_data_t;

typedef union hptxsts_data {
	
	uint32_t d32;

	
	struct {
		unsigned ptxfspcavail:16;
		unsigned ptxqspcavail:8;
		unsigned ptxqtop_terminate:1;
		unsigned ptxqtop_token:2;
		unsigned ptxqtop_chnum:4;
		unsigned ptxqtop_odd:1;
	} b;
} hptxsts_data_t;

typedef union hprt0_data {
	
	uint32_t d32;
	
	struct {
		unsigned prtconnsts:1;
		unsigned prtconndet:1;
		unsigned prtena:1;
		unsigned prtenchng:1;
		unsigned prtovrcurract:1;
		unsigned prtovrcurrchng:1;
		unsigned prtres:1;
		unsigned prtsusp:1;
		unsigned prtrst:1;
		unsigned reserved9:1;
		unsigned prtlnsts:2;
		unsigned prtpwr:1;
		unsigned prttstctl:4;
		unsigned prtspd:2;
#define DWC_HPRT0_PRTSPD_HIGH_SPEED 0
#define DWC_HPRT0_PRTSPD_FULL_SPEED 1
#define DWC_HPRT0_PRTSPD_LOW_SPEED	2
		unsigned reserved19_31:13;
	} b;
} hprt0_data_t;

typedef union haint_data {
	
	uint32_t d32;
	
	struct {
		unsigned ch0:1;
		unsigned ch1:1;
		unsigned ch2:1;
		unsigned ch3:1;
		unsigned ch4:1;
		unsigned ch5:1;
		unsigned ch6:1;
		unsigned ch7:1;
		unsigned ch8:1;
		unsigned ch9:1;
		unsigned ch10:1;
		unsigned ch11:1;
		unsigned ch12:1;
		unsigned ch13:1;
		unsigned ch14:1;
		unsigned ch15:1;
		unsigned reserved:16;
	} b;

	struct {
		unsigned chint:16;
		unsigned reserved:16;
	} b2;
} haint_data_t;

typedef union haintmsk_data {
	
	uint32_t d32;
	
	struct {
		unsigned ch0:1;
		unsigned ch1:1;
		unsigned ch2:1;
		unsigned ch3:1;
		unsigned ch4:1;
		unsigned ch5:1;
		unsigned ch6:1;
		unsigned ch7:1;
		unsigned ch8:1;
		unsigned ch9:1;
		unsigned ch10:1;
		unsigned ch11:1;
		unsigned ch12:1;
		unsigned ch13:1;
		unsigned ch14:1;
		unsigned ch15:1;
		unsigned reserved:16;
	} b;

	struct {
		unsigned chint:16;
		unsigned reserved:16;
	} b2;
} haintmsk_data_t;

typedef struct dwc_otg_hc_regs {
	
	volatile uint32_t hcchar;
	
	volatile uint32_t hcsplt;
	
	volatile uint32_t hcint;
	
	volatile uint32_t hcintmsk;
	
	volatile uint32_t hctsiz;
	
	volatile uint32_t hcdma;
	volatile uint32_t reserved;
	
	volatile uint32_t hcdmab;
} dwc_otg_hc_regs_t;

typedef union hcchar_data {
	
	uint32_t d32;

	
	struct {
		
		unsigned mps:11;

		
		unsigned epnum:4;

		
		unsigned epdir:1;

		unsigned reserved:1;

		
		unsigned lspddev:1;

		
		unsigned eptype:2;

		
		unsigned multicnt:2;

		
		unsigned devaddr:7;

		unsigned oddfrm:1;

		
		unsigned chdis:1;

		
		unsigned chen:1;
	} b;
} hcchar_data_t;

typedef union hcsplt_data {
	
	uint32_t d32;

	
	struct {
		
		unsigned prtaddr:7;

		
		unsigned hubaddr:7;

		
		unsigned xactpos:2;
#define DWC_HCSPLIT_XACTPOS_MID 0
#define DWC_HCSPLIT_XACTPOS_END 1
#define DWC_HCSPLIT_XACTPOS_BEGIN 2
#define DWC_HCSPLIT_XACTPOS_ALL 3

		
		unsigned compsplt:1;

		
		unsigned reserved:14;

		
		unsigned spltena:1;
	} b;
} hcsplt_data_t;

typedef union hcint_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned xfercomp:1;
		
		unsigned chhltd:1;
		
		unsigned ahberr:1;
		
		unsigned stall:1;
		
		unsigned nak:1;
		
		unsigned ack:1;
		
		unsigned nyet:1;
		
		unsigned xacterr:1;
		
		unsigned bblerr:1;
		
		unsigned frmovrun:1;
		
		unsigned datatglerr:1;
		
		unsigned bna:1;
		
		unsigned xcs_xact:1;
		
		unsigned frm_list_roll:1;
		
		unsigned reserved14_31:18;
	} b;
} hcint_data_t;

typedef union hcintmsk_data {
	
	uint32_t d32;

	
	struct {
		unsigned xfercompl:1;
		unsigned chhltd:1;
		unsigned ahberr:1;
		unsigned stall:1;
		unsigned nak:1;
		unsigned ack:1;
		unsigned nyet:1;
		unsigned xacterr:1;
		unsigned bblerr:1;
		unsigned frmovrun:1;
		unsigned datatglerr:1;
		unsigned bna:1;
		unsigned xcs_xact:1;
		unsigned frm_list_roll:1;
		unsigned reserved14_31:18;
	} b;
} hcintmsk_data_t;


typedef union hctsiz_data {
	
	uint32_t d32;

	
	struct {
		
		unsigned xfersize:19;

		
		unsigned pktcnt:10;

		unsigned pid:2;
#define DWC_HCTSIZ_DATA0 0
#define DWC_HCTSIZ_DATA1 2
#define DWC_HCTSIZ_DATA2 1
#define DWC_HCTSIZ_MDATA 3
#define DWC_HCTSIZ_SETUP 3

		
		unsigned dopng:1;
	} b;

	
	struct {
		
		unsigned schinfo:8;

		unsigned ntd:8;

		
		unsigned reserved16_28:13;

		unsigned pid:2;

		
		unsigned dopng:1;
	} b_ddma;
} hctsiz_data_t;

typedef union hcdma_data {
	
	uint32_t d32;
	
	struct {
		unsigned reserved0_2:3;
		
		unsigned ctd:8;
		
		unsigned dma_addr:21;
	} b;
} hcdma_data_t;

typedef union host_dma_desc_sts {
	
	uint32_t d32;
	

	
	struct {
		
		unsigned n_bytes:17;
		
		unsigned qtd_offset:6;
		unsigned a_qtd:1;
		unsigned sup:1;
		
		unsigned ioc:1;
		
		unsigned eol:1;
		unsigned reserved27:1;
		
		unsigned sts:2;
#define DMA_DESC_STS_PKTERR	1
		unsigned reserved30:1;
		
		unsigned a:1;
	} b;
	
	struct {
		
		unsigned n_bytes:12;
		unsigned reserved12_24:13;
		
		unsigned ioc:1;
		unsigned reserved26_27:2;
		
		unsigned sts:2;
		unsigned reserved30:1;
		
		unsigned a:1;
	} b_isoc;
} host_dma_desc_sts_t;

#define	MAX_DMA_DESC_SIZE		131071
#define MAX_DMA_DESC_NUM_GENERIC	64
#define MAX_DMA_DESC_NUM_HS_ISOC	256
#define MAX_FRLIST_EN_NUM		64
typedef struct dwc_otg_host_dma_desc {
	
	host_dma_desc_sts_t status;
	
	uint32_t buf;
} dwc_otg_host_dma_desc_t;

typedef struct dwc_otg_host_if {
	
	dwc_otg_host_global_regs_t *host_global_regs;
#define DWC_OTG_HOST_GLOBAL_REG_OFFSET 0x400

	
	volatile uint32_t *hprt0;
#define DWC_OTG_HOST_PORT_REGS_OFFSET 0x440

	
	dwc_otg_hc_regs_t *hc_regs[MAX_EPS_CHANNELS];
#define DWC_OTG_HOST_CHAN_REGS_OFFSET 0x500
#define DWC_OTG_CHAN_REGS_OFFSET 0x20

	
	
	uint8_t num_host_channels;
	
	uint8_t perio_eps_supported;
	
	uint16_t perio_tx_fifo_size;

} dwc_otg_host_if_t;

typedef union pcgcctl_data {
	
	uint32_t d32;

	
	struct {
		
		unsigned stoppclk:1;
		
		unsigned gatehclk:1;
		
		unsigned pwrclmp:1;
		
		unsigned rstpdwnmodule:1;
		
		unsigned reserved:1;
		
		unsigned enbl_sleep_gating:1;
		
		unsigned phy_in_sleep:1;
		
		unsigned deep_sleep:1;
		unsigned resetaftsusp:1;
		unsigned restoremode:1;
		unsigned enbl_extnd_hiber:1;
		unsigned extnd_hiber_pwrclmp:1;
		unsigned extnd_hiber_switch:1;
		unsigned ess_reg_restored:1;
		unsigned prt_clk_sel:2;
		unsigned port_power:1;
		unsigned max_xcvrselect:2;
		unsigned max_termsel:1;
		unsigned mac_dev_addr:7;
		unsigned p2hd_dev_enum_spd:2;
		unsigned p2hd_prt_spd:2;
		unsigned if_dev_mode:1;
	} b;
} pcgcctl_data_t;

typedef union gdfifocfg_data {
	
	uint32_t d32;
	
	struct {
		
		unsigned gdfifocfg:16;
		
		unsigned epinfobase:16;
	} b;
} gdfifocfg_data_t;

typedef union gpwrdn_data {
	
	uint32_t d32;

	
	struct {
		
		unsigned pmuintsel:1;
		
		unsigned pmuactv:1;
		
		unsigned restore:1;
		
		unsigned pwrdnclmp:1;
		
		unsigned pwrdnrstn:1;
		
		unsigned pwrdnswtch:1;
		
		unsigned dis_vbus:1;
		
		unsigned lnstschng:1;
		
		unsigned lnstchng_msk:1;
		
		unsigned rst_det:1;
		
		unsigned rst_det_msk:1;
		
		unsigned disconn_det:1;
		
		unsigned disconn_det_msk:1;
		
		unsigned connect_det:1;
		
		unsigned connect_det_msk:1;
		
		unsigned srp_det:1;
		
		unsigned srp_det_msk:1;
		
		unsigned sts_chngint:1;
		
		unsigned sts_chngint_msk:1;
		
		unsigned linestate:2;
		
		unsigned idsts:1;
		
		unsigned bsessvld:1;
		
		unsigned adp_int:1;
		
		unsigned mult_val_id_bc:5;
		
		unsigned reserved29_31:3;
	} b;
} gpwrdn_data_t;

#endif
