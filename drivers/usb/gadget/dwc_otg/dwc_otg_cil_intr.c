/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_cil_intr.c $
 * $Revision: #32 $
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

#include "dwc_os.h"
#include "dwc_otg_regs.h"
#include "dwc_otg_cil.h"
#include "dwc_otg_driver.h"
#include "dwc_otg_pcd.h"
#include "dwc_otg_hcd.h"

#ifdef DEBUG
inline const char *op_state_str(dwc_otg_core_if_t * core_if)
{
	return (core_if->op_state == A_HOST ? "a_host" :
		(core_if->op_state == A_SUSPEND ? "a_suspend" :
		 (core_if->op_state == A_PERIPHERAL ? "a_peripheral" :
		  (core_if->op_state == B_PERIPHERAL ? "b_peripheral" :
		   (core_if->op_state == B_HOST ? "b_host" : "unknown")))));
}
#endif

int32_t dwc_otg_handle_mode_mismatch_intr(dwc_otg_core_if_t * core_if)
{
	gintsts_data_t gintsts;
	DWC_WARN("Mode Mismatch Interrupt: currently in %s mode\n",
		 dwc_otg_mode(core_if) ? "Host" : "Device");

	
	gintsts.d32 = 0;
	gintsts.b.modemismatch = 1;
	DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, gintsts.d32);
	return 1;
}

int32_t dwc_otg_handle_otg_intr(dwc_otg_core_if_t * core_if)
{
	dwc_otg_core_global_regs_t *global_regs = core_if->core_global_regs;
	gotgint_data_t gotgint;
	gotgctl_data_t gotgctl;
	gintmsk_data_t gintmsk;
	gpwrdn_data_t gpwrdn;

	gotgint.d32 = DWC_READ_REG32(&global_regs->gotgint);
	gotgctl.d32 = DWC_READ_REG32(&global_regs->gotgctl);
	DWC_DEBUGPL(DBG_CIL, "++OTG Interrupt gotgint=%0x [%s]\n", gotgint.d32,
		    op_state_str(core_if));

	if (gotgint.b.sesenddet) {
		DWC_DEBUGPL(DBG_ANY, " ++OTG Interrupt: "
			    "Session End Detected++ (%s)\n",
			    op_state_str(core_if));
		gotgctl.d32 = DWC_READ_REG32(&global_regs->gotgctl);

		if (core_if->op_state == B_HOST) {
			cil_pcd_start(core_if);
			core_if->op_state = B_PERIPHERAL;
		} else {
			if (gotgctl.b.devhnpen) {
				DWC_DEBUGPL(DBG_ANY, "Session End Detected\n");
				__DWC_ERROR("Device Not Connected/Responding!\n");
			}

			core_if->lx_state = DWC_OTG_L0;
			DWC_SPINUNLOCK(core_if->lock);
			cil_pcd_stop(core_if, 0);
			DWC_SPINLOCK(core_if->lock);

			if (core_if->adp_enable) {
				if (core_if->power_down == 2) {
					gpwrdn.d32 = 0;
					gpwrdn.b.pwrdnswtch = 1;
					DWC_MODIFY_REG32(&core_if->
							 core_global_regs->
							 gpwrdn, gpwrdn.d32, 0);
				}

				gpwrdn.d32 = 0;
				gpwrdn.b.pmuintsel = 1;
				gpwrdn.b.pmuactv = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);

				dwc_otg_adp_sense_start(core_if);
			}
		}

		gotgctl.d32 = 0;
		gotgctl.b.devhnpen = 1;
		DWC_MODIFY_REG32(&global_regs->gotgctl, gotgctl.d32, 0);
	}
	if (gotgint.b.sesreqsucstschng) {
		DWC_DEBUGPL(DBG_ANY, " ++OTG Interrupt: "
			    "Session Reqeust Success Status Change++\n");
		gotgctl.d32 = DWC_READ_REG32(&global_regs->gotgctl);
		if (gotgctl.b.sesreqscs) {

			if ((core_if->core_params->phy_type ==
			     DWC_PHY_TYPE_PARAM_FS) && (core_if->core_params->i2c_enable)) {
				core_if->srp_success = 1;
			} else {
				DWC_SPINUNLOCK(core_if->lock);
				cil_pcd_resume(core_if);
				DWC_SPINLOCK(core_if->lock);
				
				gotgctl.d32 = 0;
				gotgctl.b.sesreq = 1;
				DWC_MODIFY_REG32(&global_regs->gotgctl,
						 gotgctl.d32, 0);
			}
		}
	}
	if (gotgint.b.hstnegsucstschng) {
		gotgctl.d32 = DWC_READ_REG32(&global_regs->gotgctl);
		if (core_if->snpsid >= OTG_CORE_REV_3_00a)
			dwc_udelay(100);
		if (gotgctl.b.hstnegscs) {
			if (dwc_otg_is_host_mode(core_if)) {
				core_if->op_state = B_HOST;
				gintmsk.d32 = 0;
				gintmsk.b.sofintr = 1;
				DWC_MODIFY_REG32(&global_regs->gintmsk,
						 gintmsk.d32, 0);
				
				DWC_SPINUNLOCK(core_if->lock);
				cil_pcd_stop(core_if, 0);
				cil_hcd_start(core_if);
				DWC_SPINLOCK(core_if->lock);
				core_if->op_state = B_HOST;
			}
		} else {
			gotgctl.d32 = 0;
			gotgctl.b.hnpreq = 1;
			gotgctl.b.devhnpen = 1;
			DWC_MODIFY_REG32(&global_regs->gotgctl, gotgctl.d32, 0);
			DWC_DEBUGPL(DBG_ANY, "HNP Failed\n");
			__DWC_ERROR("Device Not Connected/Responding\n");
		}
	}
	if (gotgint.b.hstnegdet) {
		DWC_DEBUGPL(DBG_ANY, " ++OTG Interrupt: "
			    "Host Negotiation Detected++ (%s)\n",
			    (dwc_otg_is_host_mode(core_if) ? "Host" :
			     "Device"));
		if (dwc_otg_is_device_mode(core_if)) {
			DWC_DEBUGPL(DBG_ANY, "a_suspend->a_peripheral (%d)\n",
				    core_if->op_state);
			DWC_SPINUNLOCK(core_if->lock);
			cil_hcd_disconnect(core_if);
			cil_pcd_start(core_if);
			DWC_SPINLOCK(core_if->lock);
			core_if->op_state = A_PERIPHERAL;
		} else {
			gintmsk.d32 = 0;
			gintmsk.b.sofintr = 1;
			DWC_MODIFY_REG32(&global_regs->gintmsk, gintmsk.d32, 0);
			DWC_SPINUNLOCK(core_if->lock);
			cil_pcd_stop(core_if, 0);
			cil_hcd_start(core_if);
			DWC_SPINLOCK(core_if->lock);
			core_if->op_state = A_HOST;
		}
	}
	if (gotgint.b.adevtoutchng) {
		DWC_DEBUGPL(DBG_ANY, " ++OTG Interrupt: "
			    "A-Device Timeout Change++\n");
	}
	if (gotgint.b.debdone) {
		DWC_DEBUGPL(DBG_ANY, " ++OTG Interrupt: " "Debounce Done++\n");
	}

	
	DWC_WRITE_REG32(&core_if->core_global_regs->gotgint, gotgint.d32);

	return 1;
}

void w_conn_id_status_change(void *p)
{
	dwc_otg_core_if_t *core_if = p;
	uint32_t count = 0;
	gotgctl_data_t gotgctl = {.d32 = 0 };

	gotgctl.d32 = DWC_READ_REG32(&core_if->core_global_regs->gotgctl);
	DWC_DEBUGPL(DBG_CIL, "gotgctl=%0x\n", gotgctl.d32);
	DWC_DEBUGPL(DBG_CIL, "gotgctl.b.conidsts=%d\n", gotgctl.b.conidsts);

	
	if (gotgctl.b.conidsts) {
		
		while (!dwc_otg_is_device_mode(core_if)) {
			DWC_PRINTF("Waiting for Peripheral Mode, Mode=%s\n",
				   (dwc_otg_is_host_mode(core_if) ? "Host" :
				    "Peripheral"));
			dwc_mdelay(100);
			if (++count > 10000)
				break;
		}
		DWC_ASSERT(++count < 10000,
			   "Connection id status change timed out");
		core_if->op_state = B_PERIPHERAL;
		dwc_otg_core_init(core_if);
		dwc_otg_enable_global_interrupts(core_if);
		cil_pcd_start(core_if);
	} else {
		
		while (!dwc_otg_is_host_mode(core_if)) {
			DWC_PRINTF("Waiting for Host Mode, Mode=%s\n",
				   (dwc_otg_is_host_mode(core_if) ? "Host" :
				    "Peripheral"));
			dwc_mdelay(100);
			if (++count > 10000)
				break;
		}
		DWC_ASSERT(++count < 10000,
			   "Connection id status change timed out");
		core_if->op_state = A_HOST;
		dwc_otg_core_init(core_if);
		dwc_otg_enable_global_interrupts(core_if);
		cil_hcd_start(core_if);
	}
}

int32_t dwc_otg_handle_conn_id_status_change_intr(dwc_otg_core_if_t * core_if)
{

	gintmsk_data_t gintmsk = {.d32 = 0 };
	gintsts_data_t gintsts = {.d32 = 0 };

	gintmsk.b.sofintr = 1;
	DWC_MODIFY_REG32(&core_if->core_global_regs->gintmsk, gintmsk.d32, 0);

	DWC_DEBUGPL(DBG_CIL,
		    " ++Connector ID Status Change Interrupt++  (%s)\n",
		    (dwc_otg_is_host_mode(core_if) ? "Host" : "Device"));

	DWC_SPINUNLOCK(core_if->lock);


	DWC_WORKQ_SCHEDULE(core_if->wq_otg, w_conn_id_status_change,
			   core_if, "connection id status change");
	DWC_SPINLOCK(core_if->lock);

	
	gintsts.b.conidstschng = 1;
	DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, gintsts.d32);

	return 1;
}

int32_t dwc_otg_handle_session_req_intr(dwc_otg_core_if_t * core_if)
{
	gintsts_data_t gintsts;

#ifndef DWC_HOST_ONLY
	DWC_DEBUGPL(DBG_ANY, "++Session Request Interrupt++\n");

	if (dwc_otg_is_device_mode(core_if)) {
		DWC_PRINTF("SRP: Device mode\n");
	} else {
		hprt0_data_t hprt0;
		DWC_PRINTF("SRP: Host mode\n");

		
		hprt0.d32 = dwc_otg_read_hprt0(core_if);
		hprt0.b.prtpwr = 1;
		DWC_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);

		cil_hcd_session_start(core_if);
	}
#endif

	
	gintsts.d32 = 0;
	gintsts.b.sessreqintr = 1;
	DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, gintsts.d32);

	return 1;
}

void w_wakeup_detected(void *p)
{
	dwc_otg_core_if_t *core_if = (dwc_otg_core_if_t *) p;
	hprt0_data_t hprt0 = {.d32 = 0 };
#if 0
	pcgcctl_data_t pcgcctl = {.d32 = 0 };
	
	pcgcctl.b.stoppclk = 1;
	DWC_MODIFY_REG32(core_if->pcgcctl, pcgcctl.d32, 0);
	dwc_udelay(10);
#endif 
	hprt0.d32 = dwc_otg_read_hprt0(core_if);
	DWC_DEBUGPL(DBG_ANY, "Resume: HPRT0=%0x\n", hprt0.d32);
	hprt0.b.prtres = 0;	
	DWC_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
	DWC_DEBUGPL(DBG_ANY, "Clear Resume: HPRT0=%0x\n",
		    DWC_READ_REG32(core_if->host_if->hprt0));

	cil_hcd_resume(core_if);

	
	core_if->lx_state = DWC_OTG_L0;
}

int32_t dwc_otg_handle_wakeup_detected_intr(dwc_otg_core_if_t * core_if)
{
	gintsts_data_t gintsts;

	DWC_DEBUGPL(DBG_ANY,
		    "++Resume and Remote Wakeup Detected Interrupt++\n");

	DWC_PRINTF("%s lxstate = %d\n", __func__, core_if->lx_state);

	if (dwc_otg_is_device_mode(core_if)) {
		dctl_data_t dctl = {.d32 = 0 };
		DWC_DEBUGPL(DBG_PCD, "DSTS=0x%0x\n",
			    DWC_READ_REG32(&core_if->dev_if->dev_global_regs->
					   dsts));
		if (core_if->lx_state == DWC_OTG_L2) {
#ifdef PARTIAL_POWER_DOWN
			if (core_if->hwcfg4.b.power_optimiz) {
				pcgcctl_data_t power = {.d32 = 0 };

				power.d32 = DWC_READ_REG32(core_if->pcgcctl);
				DWC_DEBUGPL(DBG_CIL, "PCGCCTL=%0x\n",
					    power.d32);

				power.b.stoppclk = 0;
				DWC_WRITE_REG32(core_if->pcgcctl, power.d32);

				power.b.pwrclmp = 0;
				DWC_WRITE_REG32(core_if->pcgcctl, power.d32);

				power.b.rstpdwnmodule = 0;
				DWC_WRITE_REG32(core_if->pcgcctl, power.d32);
			}
#endif
			
			dctl.b.rmtwkupsig = 1;
			DWC_MODIFY_REG32(&core_if->dev_if->dev_global_regs->
					 dctl, dctl.d32, 0);

			DWC_SPINUNLOCK(core_if->lock);
			if (core_if->pcd_cb && core_if->pcd_cb->resume_wakeup) {
				core_if->pcd_cb->resume_wakeup(core_if->pcd_cb->p);
			}
			DWC_SPINLOCK(core_if->lock);
		} else {
			glpmcfg_data_t lpmcfg;
			lpmcfg.d32 =
			    DWC_READ_REG32(&core_if->core_global_regs->glpmcfg);
			lpmcfg.b.hird_thres &= (~(1 << 4));
			DWC_WRITE_REG32(&core_if->core_global_regs->glpmcfg,
					lpmcfg.d32);
		}
		
		core_if->lx_state = DWC_OTG_L0;
	} else {
		if (core_if->lx_state != DWC_OTG_L1) {
			pcgcctl_data_t pcgcctl = {.d32 = 0 };

			
			pcgcctl.b.stoppclk = 1;
			DWC_MODIFY_REG32(core_if->pcgcctl, pcgcctl.d32, 0);
			DWC_TIMER_SCHEDULE(core_if->wkp_timer, 71);
		} else {
			
			core_if->lx_state = DWC_OTG_L0;
		}
	}

	
	gintsts.d32 = 0;
	gintsts.b.wkupintr = 1;
	DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, gintsts.d32);

	return 1;
}

int32_t dwc_otg_handle_restore_done_intr(dwc_otg_core_if_t * core_if)
{
	pcgcctl_data_t pcgcctl;
	DWC_DEBUGPL(DBG_ANY, "++Restore Done Interrupt++\n");

	
	pcgcctl.d32 = DWC_READ_REG32(core_if->pcgcctl);
	if (pcgcctl.b.restoremode == 1) {
		gintmsk_data_t gintmsk = {.d32 = 0 };
		gintmsk.b.wkupintr = 1;
		DWC_MODIFY_REG32(&core_if->core_global_regs->gintmsk,
				 0, gintmsk.d32);
	}

	return 1;
}

#ifndef DWC_DEVICE_ONLY
int32_t otg_cable_disconnect(dwc_otg_core_if_t * core_if)
{
	cil_hcd_disconnect(core_if);
	cil_hcd_stop(core_if);
	return 1;
}
#endif

int32_t dwc_otg_handle_disconnect_intr(dwc_otg_core_if_t * core_if)
{
	gintsts_data_t gintsts;

	DWC_DEBUGPL(DBG_ANY, "++Disconnect Detected Interrupt++ (%s) %s\n",
		    (dwc_otg_is_host_mode(core_if) ? "Host" : "Device"),
		    op_state_str(core_if));

#ifndef DWC_HOST_ONLY
	if (core_if->op_state == B_HOST) {
		DWC_SPINUNLOCK(core_if->lock);
		cil_hcd_disconnect(core_if);
		cil_pcd_start(core_if);
		DWC_SPINLOCK(core_if->lock);
		core_if->op_state = B_PERIPHERAL;
	} else if (dwc_otg_is_device_mode(core_if)) {
		gotgctl_data_t gotgctl = {.d32 = 0 };
		gotgctl.d32 =
		    DWC_READ_REG32(&core_if->core_global_regs->gotgctl);
		if (gotgctl.b.hstsethnpen == 1) {
		} else if (gotgctl.b.devhnpen == 0) {
			DWC_SPINUNLOCK(core_if->lock);
			cil_hcd_disconnect(core_if);
			cil_pcd_start(core_if);
			DWC_SPINLOCK(core_if->lock);
			core_if->op_state = B_PERIPHERAL;
		} else {
			DWC_DEBUGPL(DBG_ANY, "!a_peripheral && !devhnpen\n");
		}
	} else {
		if (core_if->op_state == A_HOST) {
			
			cil_hcd_disconnect(core_if);
			if (core_if->adp_enable) {
				gpwrdn_data_t gpwrdn = {.d32 = 0 };
				cil_hcd_stop(core_if);
				
				gpwrdn.b.pmuintsel = 1;
				gpwrdn.b.pmuactv = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);
				dwc_otg_adp_probe_start(core_if);

				
				if (core_if->power_down == 2) {
					gpwrdn.d32 = 0;
					gpwrdn.b.pwrdnswtch = 1;
					DWC_MODIFY_REG32
					    (&core_if->core_global_regs->gpwrdn,
					     gpwrdn.d32, 0);
				}
			}
		}
	}
#endif
	
	core_if->lx_state = DWC_OTG_L3;

	gintsts.d32 = 0;
	gintsts.b.disconnect = 1;
	DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, gintsts.d32);
	return 1;
}

int32_t dwc_otg_handle_usb_suspend_intr(dwc_otg_core_if_t * core_if)
{
	dsts_data_t dsts;
	gintsts_data_t gintsts;

	DWC_DEBUGPL(DBG_ANY, "USB SUSPEND\n");

	if (dwc_otg_is_device_mode(core_if)) {
		dsts.d32 =
		    DWC_READ_REG32(&core_if->dev_if->dev_global_regs->dsts);
		DWC_DEBUGPL(DBG_PCD, "DSTS=0x%0x\n", dsts.d32);
		DWC_DEBUGPL(DBG_PCD, "DSTS.Suspend Status=%d "
			    "HWCFG4.power Optimize=%d\n",
			    dsts.b.suspsts, core_if->hwcfg4.b.power_optimiz);

#ifdef PARTIAL_POWER_DOWN

		if (dsts.b.suspsts && core_if->hwcfg4.b.power_optimiz) {
			pcgcctl_data_t power = {.d32 = 0 };
			DWC_DEBUGPL(DBG_CIL, "suspend\n");

			power.b.pwrclmp = 1;
			DWC_WRITE_REG32(core_if->pcgcctl, power.d32);

			power.b.rstpdwnmodule = 1;
			DWC_MODIFY_REG32(core_if->pcgcctl, 0, power.d32);

			power.b.stoppclk = 1;
			DWC_MODIFY_REG32(core_if->pcgcctl, 0, power.d32);

		} else {
			DWC_DEBUGPL(DBG_ANY, "disconnect?\n");
		}
#endif
		
		cil_pcd_suspend(core_if);
	} else {
		if (core_if->op_state == A_PERIPHERAL) {
			DWC_DEBUGPL(DBG_ANY, "a_peripheral->a_host\n");
			
			DWC_SPINUNLOCK(core_if->lock);
			cil_pcd_stop(core_if, 0);
			cil_hcd_start(core_if);
			DWC_SPINLOCK(core_if->lock);
			core_if->op_state = A_HOST;
		}
	}

	
	core_if->lx_state = DWC_OTG_L2;

	
	gintsts.d32 = 0;
	gintsts.b.usbsuspend = 1;
	DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, gintsts.d32);

	return 1;
}

#ifdef CONFIG_USB_DWC_OTG_LPM
static int32_t dwc_otg_handle_lpm_intr(dwc_otg_core_if_t * core_if)
{
	glpmcfg_data_t lpmcfg;
	gintsts_data_t gintsts;

	if (!core_if->core_params->lpm_enable) {
		DWC_PRINTF("Unexpected LPM interrupt\n");
	}

	lpmcfg.d32 = DWC_READ_REG32(&core_if->core_global_regs->glpmcfg);
	DWC_PRINTF("LPM config register = 0x%08x\n", lpmcfg.d32);

	if (dwc_otg_is_host_mode(core_if)) {
		cil_hcd_sleep(core_if);
	} else {
		lpmcfg.b.hird_thres |= (1 << 4);
		DWC_WRITE_REG32(&core_if->core_global_regs->glpmcfg,
				lpmcfg.d32);
	}

	
	dwc_udelay(10);
	lpmcfg.d32 = DWC_READ_REG32(&core_if->core_global_regs->glpmcfg);
	if (lpmcfg.b.prt_sleep_sts) {
		
		core_if->lx_state = DWC_OTG_L1;
	}

	
	gintsts.d32 = 0;
	gintsts.b.lpmtranrcvd = 1;
	DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, gintsts.d32);
	return 1;
}
#endif 

static inline uint32_t dwc_otg_read_common_intr(dwc_otg_core_if_t * core_if)
{
	gahbcfg_data_t gahbcfg = {.d32 = 0 };
	gintsts_data_t gintsts;
	gintmsk_data_t gintmsk;
	gintmsk_data_t gintmsk_common = {.d32 = 0 };
	gintmsk_common.b.wkupintr = 1;
	gintmsk_common.b.sessreqintr = 1;
	gintmsk_common.b.conidstschng = 1;
	gintmsk_common.b.otgintr = 1;
	gintmsk_common.b.modemismatch = 1;
	gintmsk_common.b.disconnect = 1;
	gintmsk_common.b.usbsuspend = 1;
#ifdef CONFIG_USB_DWC_OTG_LPM
	gintmsk_common.b.lpmtranrcvd = 1;
#endif
	gintmsk_common.b.restoredone = 1;
	gintmsk_common.b.portintr = 1;

	gintsts.d32 = DWC_READ_REG32(&core_if->core_global_regs->gintsts);
	gintmsk.d32 = DWC_READ_REG32(&core_if->core_global_regs->gintmsk);
	gahbcfg.d32 = DWC_READ_REG32(&core_if->core_global_regs->gahbcfg);

#ifdef DEBUG
	
	if (gintsts.d32 & gintmsk_common.d32) {
		DWC_DEBUGPL(DBG_ANY, "gintsts=%08x  gintmsk=%08x\n",
			    gintsts.d32, gintmsk.d32);
	}
#endif
	if (gahbcfg.b.glblintrmsk)
		return ((gintsts.d32 & gintmsk.d32) & gintmsk_common.d32);
	else
		return 0;

}

#define CLEAR_GPWRDN_INTR(__core_if,__intr) \
do { \
		gpwrdn_data_t gpwrdn = {.d32=0}; \
		gpwrdn.b.__intr = 1; \
		DWC_MODIFY_REG32(&__core_if->core_global_regs->gpwrdn, \
		0, gpwrdn.d32); \
} while (0)

int32_t dwc_otg_handle_common_intr(dwc_otg_core_if_t *core_if)
{
	int retval = 0;
	gintsts_data_t gintsts;

	if (core_if->lock)
		DWC_SPINLOCK(core_if->lock);

	gintsts.d32 = dwc_otg_read_common_intr(core_if);
	if (gintsts.b.modemismatch) {
		retval |= dwc_otg_handle_mode_mismatch_intr(core_if);
	}
	if (gintsts.b.otgintr) {
		retval |= dwc_otg_handle_otg_intr(core_if);
	}
	if (gintsts.b.conidstschng) {
		retval |=
		    dwc_otg_handle_conn_id_status_change_intr(core_if);
	}
	if (gintsts.b.disconnect) {
		retval |= dwc_otg_handle_disconnect_intr(core_if);
	}
	if (gintsts.b.sessreqintr) {
		retval |= dwc_otg_handle_session_req_intr(core_if);
	}
	if (gintsts.b.wkupintr) {
		retval |= dwc_otg_handle_wakeup_detected_intr(core_if);
	}
	if (gintsts.b.usbsuspend) {
		retval |= dwc_otg_handle_usb_suspend_intr(core_if);
	}
#ifdef CONFIG_USB_DWC_OTG_LPM
	if (gintsts.b.lpmtranrcvd) {
		retval |= dwc_otg_handle_lpm_intr(core_if);
	}
#endif
	if (gintsts.b.portintr && dwc_otg_is_device_mode(core_if)) {
		gintsts.d32 = 0;
		gintsts.b.portintr = 1;
		dwc_write_reg32(&core_if->core_global_regs->gintsts,
				gintsts.d32);
		retval |= 1;
	}
	if (core_if->lock)
		DWC_SPINUNLOCK(core_if->lock);

	return retval;
}
