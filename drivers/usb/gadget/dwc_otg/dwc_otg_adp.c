/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_adp.c $
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

#include "dwc_os.h"
#include "dwc_otg_regs.h"
#include "dwc_otg_cil.h"
#include "dwc_otg_adp.h"


void dwc_otg_adp_write_reg(dwc_otg_core_if_t * core_if, uint32_t value)
{
	adpctl_data_t adpctl;

	adpctl.d32 = value;
	adpctl.b.ar = 0x2;

	DWC_WRITE_REG32(&core_if->core_global_regs->adpctl, adpctl.d32);

	while (adpctl.b.ar) {
		adpctl.d32 = DWC_READ_REG32(&core_if->core_global_regs->adpctl);
	}

}

uint32_t dwc_otg_adp_read_reg(dwc_otg_core_if_t * core_if)
{
	adpctl_data_t adpctl;

	adpctl.d32 = 0;
	adpctl.b.ar = 0x1;

	DWC_WRITE_REG32(&core_if->core_global_regs->adpctl, adpctl.d32);

	while (adpctl.b.ar) {
		adpctl.d32 = DWC_READ_REG32(&core_if->core_global_regs->adpctl);
	}

	return adpctl.d32;
}

uint32_t dwc_otg_adp_read_reg_filter(dwc_otg_core_if_t * core_if)
{
	adpctl_data_t adpctl;

	adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	adpctl.b.adp_tmout_int = 0;
	adpctl.b.adp_prb_int = 0;
	adpctl.b.adp_tmout_int = 0;
		
	return adpctl.d32;
}

void dwc_otg_adp_modify_reg(dwc_otg_core_if_t * core_if, uint32_t clr,
			    uint32_t set)
{
	dwc_otg_adp_write_reg(core_if,
			      (dwc_otg_adp_read_reg(core_if) & (~clr)) | set);
}

static void adp_sense_timeout(void *ptr)
{
	dwc_otg_core_if_t *core_if = (dwc_otg_core_if_t *) ptr;
	core_if->adp.sense_timer_started = 0;
	DWC_PRINTF("ADP SENSE TIMEOUT\n");
	if (core_if->adp_enable) {
		dwc_otg_adp_sense_stop(core_if);
		dwc_otg_adp_probe_start(core_if);
	}
}

static void adp_vbuson_timeout(void *ptr)
{
	gpwrdn_data_t gpwrdn;
	dwc_otg_core_if_t *core_if = (dwc_otg_core_if_t *) ptr;
	hprt0_data_t hprt0 = {.d32 = 0 };
	pcgcctl_data_t pcgcctl = {.d32 = 0 };
	DWC_PRINTF("%s: 1.1 seconds expire after turning on VBUS\n",__FUNCTION__);
	if (core_if) {
		core_if->adp.vbuson_timer_started = 0;
		
		hprt0.b.prtpwr = 1;
		DWC_MODIFY_REG32(core_if->host_if->hprt0, hprt0.d32, 0);
		gpwrdn.d32 = 0;

		
		if (core_if->power_down == 2) {
			
			gpwrdn.b.pmuactv = 0;
			gpwrdn.b.pwrdnrstn = 1;
			gpwrdn.b.pwrdnclmp = 1;
			DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0,
					 gpwrdn.d32);

			
			pcgcctl.b.stoppclk = 1;
			DWC_MODIFY_REG32(core_if->pcgcctl, 0, pcgcctl.d32);

			
			gpwrdn.b.pmuactv = 1;
			gpwrdn.b.pwrdnrstn = 1;
			gpwrdn.b.pwrdnclmp = 1;
			DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0,
					 gpwrdn.d32);
		} else {
			
			gpwrdn.b.pmuintsel = 1;
			gpwrdn.b.pmuactv = 1;
			DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);
		}

		
		if (core_if->power_down == 2) {
			gpwrdn.d32 = 0;
			gpwrdn.b.pwrdnswtch = 1;
			DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn,
					 gpwrdn.d32, 0);
		}

		
		gpwrdn.d32 = 0;
		gpwrdn.b.srp_det_msk = 1;
		DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);

		dwc_otg_adp_probe_start(core_if);
		dwc_otg_dump_global_registers(core_if);
		dwc_otg_dump_host_registers(core_if);
	}

}

void dwc_otg_adp_vbuson_timer_start(dwc_otg_core_if_t * core_if)
{
	core_if->adp.vbuson_timer_started = 1;
	if (core_if->adp.vbuson_timer)
	{
		DWC_PRINTF("SCHEDULING VBUSON TIMER\n");
		
		DWC_TIMER_SCHEDULE(core_if->adp.vbuson_timer, 1160);
	} else {
		DWC_WARN("VBUSON_TIMER = %p\n",core_if->adp.vbuson_timer);
	}
}

#if 0
static void mask_all_interrupts(dwc_otg_core_if_t * core_if)
{
	int i;
	gahbcfg_data_t ahbcfg = {.d32 = 0 };

	

	
	for (i = 0; i < core_if->core_params->host_channels; i++) {
		DWC_WRITE_REG32(&core_if->host_if->hc_regs[i]->hcintmsk, 0);
		DWC_WRITE_REG32(&core_if->host_if->hc_regs[i]->hcint, 0xFFFFFFFF);

	}

	
	DWC_WRITE_REG32(&core_if->host_if->host_global_regs->haintmsk, 0x0000);
	DWC_WRITE_REG32(&core_if->host_if->host_global_regs->haint, 0xFFFFFFFF);

	
	if (!core_if->multiproc_int_enable) {
		
		DWC_WRITE_REG32(&core_if->dev_if->dev_global_regs->diepmsk, 0);
		for (i = 0; i <= core_if->dev_if->num_in_eps; i++) {
			DWC_WRITE_REG32(&core_if->dev_if->in_ep_regs[i]->
					diepint, 0xFFFFFFFF);
		}

		
		DWC_WRITE_REG32(&core_if->dev_if->dev_global_regs->doepmsk, 0);
		for (i = 0; i <= core_if->dev_if->num_out_eps; i++) {
			DWC_WRITE_REG32(&core_if->dev_if->out_ep_regs[i]->
					doepint, 0xFFFFFFFF);
		}

		
		DWC_WRITE_REG32(&core_if->dev_if->dev_global_regs->daint,
				0xFFFFFFFF);
		DWC_WRITE_REG32(&core_if->dev_if->dev_global_regs->daintmsk, 0);
	} else {
		for (i = 0; i < core_if->dev_if->num_in_eps; ++i) {
			DWC_WRITE_REG32(&core_if->dev_if->dev_global_regs->
					diepeachintmsk[i], 0);
			DWC_WRITE_REG32(&core_if->dev_if->in_ep_regs[i]->
					diepint, 0xFFFFFFFF);
		}

		for (i = 0; i < core_if->dev_if->num_out_eps; ++i) {
			DWC_WRITE_REG32(&core_if->dev_if->dev_global_regs->
					doepeachintmsk[i], 0);
			DWC_WRITE_REG32(&core_if->dev_if->out_ep_regs[i]->
					doepint, 0xFFFFFFFF);
		}

		DWC_WRITE_REG32(&core_if->dev_if->dev_global_regs->deachintmsk,
				0);
		DWC_WRITE_REG32(&core_if->dev_if->dev_global_regs->deachint,
				0xFFFFFFFF);

	}

	
	ahbcfg.b.glblintrmsk = 1;
	DWC_MODIFY_REG32(&core_if->core_global_regs->gahbcfg, ahbcfg.d32, 0);

	
	DWC_WRITE_REG32(&core_if->core_global_regs->gintmsk, 0);

	
	DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, 0xFFFFFFFF);

	
	DWC_WRITE_REG32(&core_if->core_global_regs->gotgint, 0xFFFFFFFF);
}

static void unmask_conn_det_intr(dwc_otg_core_if_t * core_if)
{
	gintmsk_data_t gintmsk = {.d32 = 0,.b.portintr = 1 };

	DWC_WRITE_REG32(&core_if->core_global_regs->gintmsk, gintmsk.d32);
}
#endif

uint32_t dwc_otg_adp_probe_start(dwc_otg_core_if_t * core_if)
{

	adpctl_data_t adpctl = {.d32 = 0};
	gpwrdn_data_t gpwrdn;
#if 0
	adpctl_data_t adpctl_int = {.d32 = 0, .b.adp_prb_int = 1,
								.b.adp_sns_int = 1, b.adp_tmout_int};
#endif
	dwc_otg_disable_global_interrupts(core_if);
	DWC_PRINTF("ADP Probe Start\n");
	core_if->adp.probe_enabled = 1;

	adpctl.b.adpres = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	while (adpctl.b.adpres) {
		adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	}

	adpctl.d32 = 0;
	gpwrdn.d32 = DWC_READ_REG32(&core_if->core_global_regs->gpwrdn);

	
	gpwrdn.d32 = 0;
	gpwrdn.b.sts_chngint_msk = 1;
	if (!gpwrdn.b.idsts) {
		gpwrdn.b.srp_det_msk = 1;
	}
	DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);

	adpctl.b.adp_tmout_int_msk = 1;
	adpctl.b.adp_prb_int_msk = 1;
	adpctl.b.prb_dschg = 1;
	adpctl.b.prb_delta = 1;
	adpctl.b.prb_per = 1;
	adpctl.b.adpen = 1;
	adpctl.b.enaprb = 1;

	dwc_otg_adp_write_reg(core_if, adpctl.d32);
	DWC_PRINTF("ADP Probe Finish\n");
	return 0;
}

void dwc_otg_adp_sense_timer_start(dwc_otg_core_if_t * core_if)
{
	core_if->adp.sense_timer_started = 1;
	DWC_TIMER_SCHEDULE(core_if->adp.sense_timer, 3000  );
}

uint32_t dwc_otg_adp_sense_start(dwc_otg_core_if_t * core_if)
{
	adpctl_data_t adpctl;

	DWC_PRINTF("ADP Sense Start\n");

	
	adpctl.d32 = dwc_otg_adp_read_reg_filter(core_if);
	adpctl.b.adp_sns_int_msk = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);
	dwc_otg_disable_global_interrupts(core_if); 

	
	adpctl.d32 = dwc_otg_adp_read_reg_filter(core_if);
	adpctl.b.adpres = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	while (adpctl.b.adpres) {
		adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	}

	adpctl.b.adpres = 0;
	adpctl.b.adpen = 1;
	adpctl.b.enasns = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	dwc_otg_adp_sense_timer_start(core_if);

	return 0;
}

uint32_t dwc_otg_adp_probe_stop(dwc_otg_core_if_t * core_if)
{

	adpctl_data_t adpctl;
	DWC_PRINTF("Stop ADP probe\n");
	core_if->adp.probe_enabled = 0;
	core_if->adp.probe_counter = 0;
	adpctl.d32 = dwc_otg_adp_read_reg(core_if);

	adpctl.b.adpen = 0;
	adpctl.b.adp_prb_int = 1;
	adpctl.b.adp_tmout_int = 1;
	adpctl.b.adp_sns_int = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	return 0;
}

uint32_t dwc_otg_adp_sense_stop(dwc_otg_core_if_t * core_if)
{
	adpctl_data_t adpctl;

	core_if->adp.sense_enabled = 0;

	adpctl.d32 = dwc_otg_adp_read_reg_filter(core_if);
	adpctl.b.enasns = 0;
	adpctl.b.adp_sns_int = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	return 0;
}

void dwc_otg_adp_turnon_vbus(dwc_otg_core_if_t * core_if)
{
	hprt0_data_t hprt0 = {.d32 = 0 };
	hprt0.d32 = dwc_otg_read_hprt0(core_if);
	DWC_PRINTF("Turn on VBUS for 1.1s, port power is %d\n", hprt0.b.prtpwr);

	if (hprt0.b.prtpwr == 0) {
		hprt0.b.prtpwr = 1;
		
	}
	
	dwc_otg_adp_vbuson_timer_start(core_if);
}

void dwc_otg_adp_start(dwc_otg_core_if_t * core_if, uint8_t is_host)
{
	gpwrdn_data_t gpwrdn;

	DWC_PRINTF("ADP Initial Start\n");
	core_if->adp.adp_started = 1;

	DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, 0xFFFFFFFF);
	dwc_otg_disable_global_interrupts(core_if);
	if (is_host) {
		DWC_PRINTF("HOST MODE\n");
		
		gpwrdn.d32 = 0;
		gpwrdn.b.pmuintsel = 1;
		gpwrdn.b.pmuactv = 1;
		DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);
		
		core_if->adp.initial_probe = 1;
		dwc_otg_adp_probe_start(core_if);
	} else {
		gotgctl_data_t gotgctl;
		gotgctl.d32 = DWC_READ_REG32(&core_if->core_global_regs->gotgctl);
		DWC_PRINTF("DEVICE MODE\n");
		if (gotgctl.b.bsesvld == 0) {
			
			gpwrdn.d32 = 0;
			DWC_PRINTF("VBUS is not valid - start ADP probe\n");
			gpwrdn.b.pmuintsel = 1;
			gpwrdn.b.pmuactv = 1;
			DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);
			core_if->adp.initial_probe = 1;
			dwc_otg_adp_probe_start(core_if);
		} else {
			DWC_PRINTF("VBUS is valid - initialize core as a Device\n");
			core_if->op_state = B_PERIPHERAL;
			dwc_otg_core_init(core_if);
			dwc_otg_enable_global_interrupts(core_if);
			cil_pcd_start(core_if);
			dwc_otg_dump_global_registers(core_if);
			dwc_otg_dump_dev_registers(core_if);
		}
	}
}

void dwc_otg_adp_init(dwc_otg_core_if_t * core_if)
{
	core_if->adp.adp_started = 0;
	core_if->adp.initial_probe = 0;
	core_if->adp.probe_timer_values[0] = -1;
	core_if->adp.probe_timer_values[1] = -1;
	core_if->adp.probe_enabled = 0;
	core_if->adp.sense_enabled = 0;
	core_if->adp.sense_timer_started = 0;
	core_if->adp.vbuson_timer_started = 0;
	core_if->adp.probe_counter = 0;
	core_if->adp.gpwrdn = 0;
	core_if->adp.attached = DWC_OTG_ADP_UNKOWN;
	
	core_if->adp.sense_timer =
	    DWC_TIMER_ALLOC("ADP SENSE TIMER", adp_sense_timeout, core_if);
	core_if->adp.vbuson_timer =
	    DWC_TIMER_ALLOC("ADP VBUS ON TIMER", adp_vbuson_timeout, core_if);
	if (!core_if->adp.sense_timer || !core_if->adp.vbuson_timer)
	{
		DWC_ERROR("Could not allocate memory for ADP timers\n");
	}
}

void dwc_otg_adp_remove(dwc_otg_core_if_t * core_if)
{
	gpwrdn_data_t gpwrdn = { .d32 = 0 };
	gpwrdn.b.pmuintsel = 1;
	gpwrdn.b.pmuactv = 1;
	DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);

	if (core_if->adp.probe_enabled)		
		dwc_otg_adp_probe_stop(core_if);
	if (core_if->adp.sense_enabled)		
		dwc_otg_adp_sense_stop(core_if);
	if (core_if->adp.sense_timer_started)		
		DWC_TIMER_CANCEL(core_if->adp.sense_timer);
	if (core_if->adp.vbuson_timer_started)		
		DWC_TIMER_CANCEL(core_if->adp.vbuson_timer);
	DWC_TIMER_FREE(core_if->adp.sense_timer);
	DWC_TIMER_FREE(core_if->adp.vbuson_timer);
}

static uint32_t set_timer_value(dwc_otg_core_if_t * core_if, uint32_t val)
{
	if (core_if->adp.probe_timer_values[0] == -1) {
		core_if->adp.probe_timer_values[0] = val;
		core_if->adp.probe_timer_values[1] = -1;
		return 1;
	} else {
		core_if->adp.probe_timer_values[1] =
		    core_if->adp.probe_timer_values[0];
		core_if->adp.probe_timer_values[0] = val;
		return 0;
	}
}

static uint32_t compare_timer_values(dwc_otg_core_if_t * core_if)
{
	uint32_t diff;
	if (core_if->adp.probe_timer_values[0]>=core_if->adp.probe_timer_values[1])
			diff = core_if->adp.probe_timer_values[0]-core_if->adp.probe_timer_values[1];
	else
			diff = core_if->adp.probe_timer_values[1]-core_if->adp.probe_timer_values[0];   	
	if(diff < 2) {
		return 0;
	} else {
		return 1;
	}
}

static int32_t dwc_otg_adp_handle_prb_intr(dwc_otg_core_if_t * core_if,
						 uint32_t val)
{
	adpctl_data_t adpctl = {.d32 = 0 };
	gpwrdn_data_t gpwrdn, temp;
	adpctl.d32 = val;

	temp.d32 = DWC_READ_REG32(&core_if->core_global_regs->gpwrdn);
	core_if->adp.probe_counter++;
	core_if->adp.gpwrdn = DWC_READ_REG32(&core_if->core_global_regs->gpwrdn);
	if (adpctl.b.rtim == 0 && !temp.b.idsts){
		DWC_PRINTF("RTIM value is 0\n");	
		goto exit;
	}
	if (set_timer_value(core_if, adpctl.b.rtim) &&
	    core_if->adp.initial_probe) {
		core_if->adp.initial_probe = 0;
		dwc_otg_adp_probe_stop(core_if);
		gpwrdn.d32 = 0;
		gpwrdn.b.pmuactv = 1;
		gpwrdn.b.pmuintsel = 1;
		DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);
		DWC_WRITE_REG32(&core_if->core_global_regs->gintsts, 0xFFFFFFFF);

		
		if (!temp.b.idsts) {	
			core_if->op_state = A_HOST;
			dwc_otg_enable_global_interrupts(core_if);
			DWC_SPINUNLOCK(core_if->lock);
			cil_hcd_start(core_if);
			dwc_otg_adp_turnon_vbus(core_if);
			DWC_SPINLOCK(core_if->lock);
		} else {
			dwc_otg_enable_global_interrupts(core_if);
			dwc_otg_initiate_srp(core_if);
		}
	} else if (core_if->adp.probe_counter > 2){
		gpwrdn.d32 = DWC_READ_REG32(&core_if->core_global_regs->gpwrdn);
		if (compare_timer_values(core_if)) {
			DWC_PRINTF("Difference in timer values !!! \n");
			dwc_otg_adp_probe_stop(core_if);

			
			if (core_if->power_down == 2) {
				gpwrdn.b.pwrdnswtch = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);
			}

			
			if (!temp.b.idsts) {	
				
				gpwrdn.d32 = 0;
				gpwrdn.b.pmuintsel = 1;
				gpwrdn.b.pmuactv = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, gpwrdn.d32, 0);

				core_if->op_state = A_HOST;
				dwc_otg_core_init(core_if);
				dwc_otg_enable_global_interrupts(core_if);
				cil_hcd_start(core_if);
			} else {
				gotgctl_data_t gotgctl;
				
				gpwrdn.d32 = 0;
				gpwrdn.b.srp_det_msk = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, gpwrdn.d32, 0);

				
				gpwrdn.d32 = 0;
				gpwrdn.b.pmuintsel = 1;
				gpwrdn.b.pmuactv = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, gpwrdn.d32, 0);

				core_if->op_state = B_PERIPHERAL;
				dwc_otg_core_init(core_if);
				dwc_otg_enable_global_interrupts(core_if);
				cil_pcd_start(core_if);

				gotgctl.d32 = DWC_READ_REG32(&core_if->core_global_regs->gotgctl);
				if (!gotgctl.b.bsesvld) {
					dwc_otg_initiate_srp(core_if);
				}
			}
		}
		if (core_if->power_down == 2) {
			if (gpwrdn.b.bsessvld) {
				
				gpwrdn.d32 = 0;
				gpwrdn.b.srp_det_msk = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);
				
				
				gpwrdn.d32 = 0;
				gpwrdn.b.pmuactv = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);

				core_if->op_state = B_PERIPHERAL;
				dwc_otg_core_init(core_if);
				dwc_otg_enable_global_interrupts(core_if);
				cil_pcd_start(core_if);
			}
		}
	}
exit:
	
	adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	adpctl.b.adp_prb_int = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	return 0;
}

static int32_t dwc_otg_adp_handle_sns_intr(dwc_otg_core_if_t * core_if)
{
	adpctl_data_t adpctl;
	
	DWC_TIMER_CANCEL(core_if->adp.sense_timer);

	
	dwc_otg_adp_sense_timer_start(core_if);
	
	
	adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	adpctl.b.adp_sns_int = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	return 0;
}

static int32_t dwc_otg_adp_handle_prb_tmout_intr(dwc_otg_core_if_t * core_if,
						 uint32_t val)
{
	adpctl_data_t adpctl = {.d32 = 0 };
	adpctl.d32 = val;
	set_timer_value(core_if, adpctl.b.rtim);
	
	
	adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	adpctl.b.adp_tmout_int = 1;
	dwc_otg_adp_write_reg(core_if, adpctl.d32);

	return 0;
}

int32_t dwc_otg_adp_handle_intr(dwc_otg_core_if_t * core_if)
{
	int retval = 0;
	adpctl_data_t adpctl = {.d32 = 0};

	adpctl.d32 = dwc_otg_adp_read_reg(core_if);
	DWC_PRINTF("ADPCTL = %08x\n",adpctl.d32);

	if (adpctl.b.adp_sns_int & adpctl.b.adp_sns_int_msk) {
		DWC_PRINTF("ADP Sense interrupt\n");
		retval |= dwc_otg_adp_handle_sns_intr(core_if);
	}
	if (adpctl.b.adp_tmout_int & adpctl.b.adp_tmout_int_msk) {
		DWC_PRINTF("ADP timeout interrupt\n");
		retval |= dwc_otg_adp_handle_prb_tmout_intr(core_if, adpctl.d32);
	}
	if (adpctl.b.adp_prb_int & adpctl.b.adp_prb_int_msk) {
		DWC_PRINTF("ADP Probe interrupt\n");
		adpctl.b.adp_prb_int = 1;	
		retval |= dwc_otg_adp_handle_prb_intr(core_if, adpctl.d32);
	}

	
	DWC_PRINTF("RETURN FROM ADP ISR\n");

	return retval;
}

int32_t dwc_otg_adp_handle_srp_intr(dwc_otg_core_if_t * core_if)
{

#ifndef DWC_HOST_ONLY
	hprt0_data_t hprt0;
	gpwrdn_data_t gpwrdn;
	DWC_DEBUGPL(DBG_ANY, "++ Power Down Logic Session Request Interrupt++\n");

	gpwrdn.d32 = DWC_READ_REG32(&core_if->core_global_regs->gpwrdn);
	
	if (!gpwrdn.b.idsts) {	
		DWC_PRINTF("SRP: Host mode\n");

		if (core_if->adp_enable) {
			dwc_otg_adp_probe_stop(core_if);

			
			if (core_if->power_down == 2) {
				gpwrdn.b.pwrdnswtch = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);
			}

			core_if->op_state = A_HOST;
			dwc_otg_core_init(core_if);
			dwc_otg_enable_global_interrupts(core_if);
			cil_hcd_start(core_if);
		}

		
		hprt0.d32 = dwc_otg_read_hprt0(core_if);
		hprt0.b.prtpwr = 1;
		DWC_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);

		cil_hcd_session_start(core_if);
	} else {
		DWC_PRINTF("SRP: Device mode %s\n", __FUNCTION__);
		if (core_if->adp_enable) {
			dwc_otg_adp_probe_stop(core_if);

			
			if (core_if->power_down == 2) {
				gpwrdn.b.pwrdnswtch = 1;
				DWC_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);
			}

			gpwrdn.d32 = 0;
			gpwrdn.b.pmuactv = 0;
			DWC_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0,
					 gpwrdn.d32);

			core_if->op_state = B_PERIPHERAL;
			dwc_otg_core_init(core_if);
			dwc_otg_enable_global_interrupts(core_if);
			cil_pcd_start(core_if);
		}
	}
#endif
	return 1;
}
