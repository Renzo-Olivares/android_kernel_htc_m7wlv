/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_core_if.h $
 * $Revision: #13 $
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
#if !defined(__DWC_CORE_IF_H__)
#define __DWC_CORE_IF_H__

#include "dwc_os.h"


struct dwc_otg_core_if;
typedef struct dwc_otg_core_if dwc_otg_core_if_t;

#define MAX_PERIO_FIFOS 15
#define MAX_TX_FIFOS 15

#define MAX_EPS_CHANNELS 16

extern dwc_otg_core_if_t *dwc_otg_cil_init(const uint32_t * _reg_base_addr);
extern void dwc_otg_core_init(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_cil_remove(dwc_otg_core_if_t * _core_if);

extern void dwc_otg_enable_global_interrupts(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_disable_global_interrupts(dwc_otg_core_if_t * _core_if);

extern uint8_t dwc_otg_is_device_mode(dwc_otg_core_if_t * _core_if);
extern uint8_t dwc_otg_is_host_mode(dwc_otg_core_if_t * _core_if);

extern uint8_t dwc_otg_is_dma_enable(dwc_otg_core_if_t * core_if);

extern int32_t dwc_otg_handle_common_intr(dwc_otg_core_if_t *core_if);

#ifndef DWC_DEVICE_ONLY
extern void dwc_otg_core_fore_host(dwc_otg_core_if_t * core_if);
extern int32_t otg_cable_disconnect(dwc_otg_core_if_t * core_if);
#endif

extern int dwc_otg_set_param_otg_cap(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_otg_cap(dwc_otg_core_if_t * core_if);
#define DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE 0
#define DWC_OTG_CAP_PARAM_SRP_ONLY_CAPABLE 1
#define DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE 2
#define dwc_param_otg_cap_default DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE

extern int dwc_otg_set_param_opt(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_opt(dwc_otg_core_if_t * core_if);
#define dwc_param_opt_default 1

extern int dwc_otg_set_param_dma_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_dma_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_dma_enable_default 1

extern int dwc_otg_set_param_dma_desc_enable(dwc_otg_core_if_t * core_if,
					     int32_t val);
extern int32_t dwc_otg_get_param_dma_desc_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_dma_desc_enable_default 1

extern int dwc_otg_set_param_dma_burst_size(dwc_otg_core_if_t * core_if,
					    int32_t val);
extern int32_t dwc_otg_get_param_dma_burst_size(dwc_otg_core_if_t * core_if);
#define dwc_param_dma_burst_size_default 32

extern int dwc_otg_set_param_speed(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_speed(dwc_otg_core_if_t * core_if);
#define dwc_param_speed_default 0
#define DWC_SPEED_PARAM_HIGH 0
#define DWC_SPEED_PARAM_FULL 1

extern int dwc_otg_set_param_host_support_fs_ls_low_power(dwc_otg_core_if_t *
							  core_if, int32_t val);
extern int32_t dwc_otg_get_param_host_support_fs_ls_low_power(dwc_otg_core_if_t
							      * core_if);
#define dwc_param_host_support_fs_ls_low_power_default 0

extern int dwc_otg_set_param_host_ls_low_power_phy_clk(dwc_otg_core_if_t *
						       core_if, int32_t val);
extern int32_t dwc_otg_get_param_host_ls_low_power_phy_clk(dwc_otg_core_if_t *
							   core_if);
#define dwc_param_host_ls_low_power_phy_clk_default 0
#define DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_48MHZ 0
#define DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_6MHZ 1

extern int dwc_otg_set_param_enable_dynamic_fifo(dwc_otg_core_if_t * core_if,
						 int32_t val);
extern int32_t dwc_otg_get_param_enable_dynamic_fifo(dwc_otg_core_if_t *
						     core_if);
#define dwc_param_enable_dynamic_fifo_default 1

extern int dwc_otg_set_param_data_fifo_size(dwc_otg_core_if_t * core_if,
					    int32_t val);
extern int32_t dwc_otg_get_param_data_fifo_size(dwc_otg_core_if_t * core_if);
#define dwc_param_data_fifo_size_default 8192

extern int dwc_otg_set_param_dev_rx_fifo_size(dwc_otg_core_if_t * core_if,
					      int32_t val);
extern int32_t dwc_otg_get_param_dev_rx_fifo_size(dwc_otg_core_if_t * core_if);
#define dwc_param_dev_rx_fifo_size_default 1064

extern int dwc_otg_set_param_dev_nperio_tx_fifo_size(dwc_otg_core_if_t *
						     core_if, int32_t val);
extern int32_t dwc_otg_get_param_dev_nperio_tx_fifo_size(dwc_otg_core_if_t *
							 core_if);
#define dwc_param_dev_nperio_tx_fifo_size_default 1024

extern int dwc_otg_set_param_dev_perio_tx_fifo_size(dwc_otg_core_if_t * core_if,
						    int32_t val, int fifo_num);
extern int32_t dwc_otg_get_param_dev_perio_tx_fifo_size(dwc_otg_core_if_t *
							core_if, int fifo_num);
#define dwc_param_dev_perio_tx_fifo_size_default 256

extern int dwc_otg_set_param_host_rx_fifo_size(dwc_otg_core_if_t * core_if,
					       int32_t val);
extern int32_t dwc_otg_get_param_host_rx_fifo_size(dwc_otg_core_if_t * core_if);
#define dwc_param_host_rx_fifo_size_default 1024

extern int dwc_otg_set_param_host_nperio_tx_fifo_size(dwc_otg_core_if_t *
						      core_if, int32_t val);
extern int32_t dwc_otg_get_param_host_nperio_tx_fifo_size(dwc_otg_core_if_t *
							  core_if);
#define dwc_param_host_nperio_tx_fifo_size_default 1024

extern int dwc_otg_set_param_host_perio_tx_fifo_size(dwc_otg_core_if_t *
						     core_if, int32_t val);
extern int32_t dwc_otg_get_param_host_perio_tx_fifo_size(dwc_otg_core_if_t *
							 core_if);
#define dwc_param_host_perio_tx_fifo_size_default 1024

extern int dwc_otg_set_param_max_transfer_size(dwc_otg_core_if_t * core_if,
					       int32_t val);
extern int32_t dwc_otg_get_param_max_transfer_size(dwc_otg_core_if_t * core_if);
#define dwc_param_max_transfer_size_default 65535

extern int dwc_otg_set_param_max_packet_count(dwc_otg_core_if_t * core_if,
					      int32_t val);
extern int32_t dwc_otg_get_param_max_packet_count(dwc_otg_core_if_t * core_if);
#define dwc_param_max_packet_count_default 511

extern int dwc_otg_set_param_host_channels(dwc_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t dwc_otg_get_param_host_channels(dwc_otg_core_if_t * core_if);
#define dwc_param_host_channels_default 12

extern int dwc_otg_set_param_dev_endpoints(dwc_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t dwc_otg_get_param_dev_endpoints(dwc_otg_core_if_t * core_if);
#define dwc_param_dev_endpoints_default 6

extern int dwc_otg_set_param_phy_type(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_phy_type(dwc_otg_core_if_t * core_if);
#define DWC_PHY_TYPE_PARAM_FS 0
#define DWC_PHY_TYPE_PARAM_UTMI 1
#define DWC_PHY_TYPE_PARAM_ULPI 2
#define dwc_param_phy_type_default DWC_PHY_TYPE_PARAM_UTMI

extern int dwc_otg_set_param_phy_utmi_width(dwc_otg_core_if_t * core_if,
					    int32_t val);
extern int32_t dwc_otg_get_param_phy_utmi_width(dwc_otg_core_if_t * core_if);
#define dwc_param_phy_utmi_width_default 16

extern int dwc_otg_set_param_phy_ulpi_ddr(dwc_otg_core_if_t * core_if,
					  int32_t val);
extern int32_t dwc_otg_get_param_phy_ulpi_ddr(dwc_otg_core_if_t * core_if);
#define dwc_param_phy_ulpi_ddr_default 0

extern int dwc_otg_set_param_phy_ulpi_ext_vbus(dwc_otg_core_if_t * core_if,
					       int32_t val);
extern int32_t dwc_otg_get_param_phy_ulpi_ext_vbus(dwc_otg_core_if_t * core_if);
#define DWC_PHY_ULPI_INTERNAL_VBUS 0
#define DWC_PHY_ULPI_EXTERNAL_VBUS 1
#define dwc_param_phy_ulpi_ext_vbus_default DWC_PHY_ULPI_INTERNAL_VBUS

extern int dwc_otg_set_param_i2c_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_i2c_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_i2c_enable_default 0

extern int dwc_otg_set_param_ulpi_fs_ls(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_ulpi_fs_ls(dwc_otg_core_if_t * core_if);
#define dwc_param_ulpi_fs_ls_default 0

extern int dwc_otg_set_param_ts_dline(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_ts_dline(dwc_otg_core_if_t * core_if);
#define dwc_param_ts_dline_default 0

extern int dwc_otg_set_param_en_multiple_tx_fifo(dwc_otg_core_if_t * core_if,
						 int32_t val);
extern int32_t dwc_otg_get_param_en_multiple_tx_fifo(dwc_otg_core_if_t *
						     core_if);
#define dwc_param_en_multiple_tx_fifo_default 1

extern int dwc_otg_set_param_dev_tx_fifo_size(dwc_otg_core_if_t * core_if,
					      int fifo_num, int32_t val);
extern int32_t dwc_otg_get_param_dev_tx_fifo_size(dwc_otg_core_if_t * core_if,
						  int fifo_num);
#define dwc_param_dev_tx_fifo_size_default 768

extern int dwc_otg_set_param_thr_ctl(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_thr_ctl(dwc_otg_core_if_t * core_if, int fifo_num);
#define dwc_param_thr_ctl_default 0

extern int dwc_otg_set_param_tx_thr_length(dwc_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t dwc_otg_get_tx_thr_length(dwc_otg_core_if_t * core_if);
#define dwc_param_tx_thr_length_default 64

extern int dwc_otg_set_param_rx_thr_length(dwc_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t dwc_otg_get_rx_thr_length(dwc_otg_core_if_t * core_if);
#define dwc_param_rx_thr_length_default 64

extern int dwc_otg_set_param_lpm_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_lpm_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_lpm_enable_default 1

extern int dwc_otg_set_param_pti_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_pti_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_pti_enable_default 0

extern int dwc_otg_set_param_mpi_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_mpi_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_mpi_enable_default 0

extern int dwc_otg_set_param_adp_enable(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_adp_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_adp_enable_default 0


extern int dwc_otg_set_param_ic_usb_cap(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_ic_usb_cap(dwc_otg_core_if_t * core_if);
#define dwc_param_ic_usb_cap_default 0

extern int dwc_otg_set_param_ahb_thr_ratio(dwc_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t dwc_otg_get_param_ahb_thr_ratio(dwc_otg_core_if_t * core_if);
#define dwc_param_ahb_thr_ratio_default 0

extern int dwc_otg_set_param_power_down(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_power_down(dwc_otg_core_if_t * core_if);
#define dwc_param_power_down_default 0

extern int dwc_otg_set_param_reload_ctl(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_reload_ctl(dwc_otg_core_if_t * core_if);
#define dwc_param_reload_ctl_default 0

extern int dwc_otg_set_param_dev_out_nak(dwc_otg_core_if_t * core_if,
					 int32_t val);
extern int32_t dwc_otg_get_param_dev_out_nak(dwc_otg_core_if_t * core_if);
#define dwc_param_dev_out_nak_default 0

extern int dwc_otg_set_param_cont_on_bna(dwc_otg_core_if_t * core_if,
					 int32_t val);
extern int32_t dwc_otg_get_param_cont_on_bna(dwc_otg_core_if_t * core_if);
#define dwc_param_cont_on_bna_default 0

extern int dwc_otg_set_param_ahb_single(dwc_otg_core_if_t * core_if,
					int32_t val);
extern int32_t dwc_otg_get_param_ahb_single(dwc_otg_core_if_t * core_if);
#define dwc_param_ahb_single_default 0

extern int dwc_otg_set_param_otg_ver(dwc_otg_core_if_t * core_if, int32_t val);
extern int32_t dwc_otg_get_param_otg_ver(dwc_otg_core_if_t * core_if);
#define dwc_param_otg_ver_default 0



extern void dwc_otg_dump_dev_registers(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_dump_spram(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_dump_host_registers(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_dump_global_registers(dwc_otg_core_if_t * _core_if);

extern uint32_t dwc_otg_get_hnpstatus(dwc_otg_core_if_t * core_if);

extern uint32_t dwc_otg_get_srpstatus(dwc_otg_core_if_t * core_if);

extern void dwc_otg_set_hnpreq(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_gsnpsid(dwc_otg_core_if_t * core_if);

extern uint32_t dwc_otg_get_mode(dwc_otg_core_if_t * core_if);

extern uint32_t dwc_otg_get_hnpcapable(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_hnpcapable(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_srpcapable(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_srpcapable(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_devspeed(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_devspeed(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_busconnected(dwc_otg_core_if_t * core_if);

extern uint32_t dwc_otg_get_enumspeed(dwc_otg_core_if_t * core_if);

extern uint32_t dwc_otg_get_prtpower(dwc_otg_core_if_t * core_if);

extern uint32_t dwc_otg_get_core_state(dwc_otg_core_if_t * core_if);

extern void dwc_otg_set_prtpower(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_prtsuspend(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_prtsuspend(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_mode_ch_tim(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_mode_ch_tim(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_fr_interval(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_fr_interval(dwc_otg_core_if_t * core_if, uint32_t val);

extern void dwc_otg_set_prtresume(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_remotewakesig(dwc_otg_core_if_t * core_if);

extern uint32_t dwc_otg_get_lpm_portsleepstatus(dwc_otg_core_if_t * core_if);

extern uint32_t dwc_otg_get_lpm_remotewakeenabled(dwc_otg_core_if_t * core_if);

extern uint32_t dwc_otg_get_lpmresponse(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_lpmresponse(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_hsic_connect(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_hsic_connect(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_inv_sel_hsic(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_inv_sel_hsic(dwc_otg_core_if_t * core_if, uint32_t val);


extern uint32_t dwc_otg_get_gotgctl(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_gotgctl(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_gusbcfg(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_gusbcfg(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_grxfsiz(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_grxfsiz(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_gnptxfsiz(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_gnptxfsiz(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_gpvndctl(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_gpvndctl(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_ggpio(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_ggpio(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_guid(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_guid(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_hprt0(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_hprt0(dwc_otg_core_if_t * core_if, uint32_t val);

extern uint32_t dwc_otg_get_hptxfsiz(dwc_otg_core_if_t * core_if);


#endif 
