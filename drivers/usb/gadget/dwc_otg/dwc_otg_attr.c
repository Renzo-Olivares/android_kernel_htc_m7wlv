/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/dwc_otg_attr.c $
 * $Revision: #34 $
 * $Date: 2008/12/24 $
 * $Change: 1158473 $
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


/** @page "Linux Module Attributes"
 *
 * The Linux module attributes feature is used to provide the Linux
 * Diagnostic Interface.  These attributes are accessed through sysfs.
 * The diagnostic interface will provide access to the controller for
 * bringing up the hardware and testing.

 The following table shows the attributes.
 <table>
 <tr>
 <td><b> Name</b></td>
 <td><b> Description</b></td>
 <td><b> Access</b></td>
 </tr>

 <tr>
 <td> mode </td>
 <td> Returns the current mode: 0 for device mode, 1 for host mode</td>
 <td> Read</td>
 </tr>

 <tr>
 <td> hnpcapable </td>
 <td> Gets or sets the "HNP-capable" bit in the Core USB Configuraton Register.
 Read returns the current value.</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> srpcapable </td>
 <td> Gets or sets the "SRP-capable" bit in the Core USB Configuraton Register.
 Read returns the current value.</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> hsic_connect </td>
 <td> Gets or sets the "HSIC-Connect" bit in the GLPMCFG Register.
 Read returns the current value.</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> inv_sel_hsic </td>
 <td> Gets or sets the "Invert Select HSIC" bit in the GLPMFG Register.
 Read returns the current value.</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> hnp </td>
 <td> Initiates the Host Negotiation Protocol.  Read returns the status.</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> srp </td>
 <td> Initiates the Session Request Protocol.  Read returns the status.</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> buspower </td>
 <td> Gets or sets the Power State of the bus (0 - Off or 1 - On)</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> bussuspend </td>
 <td> Suspends the USB bus.</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> busconnected </td>
 <td> Gets the connection status of the bus</td>
 <td> Read</td>
 </tr>

 <tr>
 <td> gotgctl </td>
 <td> Gets or sets the Core Control Status Register.</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> gusbcfg </td>
 <td> Gets or sets the Core USB Configuration Register</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> grxfsiz </td>
 <td> Gets or sets the Receive FIFO Size Register</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> gnptxfsiz </td>
 <td> Gets or sets the non-periodic Transmit Size Register</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> gpvndctl </td>
 <td> Gets or sets the PHY Vendor Control Register</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> ggpio </td>
 <td> Gets the value in the lower 16-bits of the General Purpose IO Register
 or sets the upper 16 bits.</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> guid </td>
 <td> Gets or sets the value of the User ID Register</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> gsnpsid </td>
 <td> Gets the value of the Synopsys ID Regester</td>
 <td> Read</td>
 </tr>

 <tr>
 <td> devspeed </td>
 <td> Gets or sets the device speed setting in the DCFG register</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> enumspeed </td>
 <td> Gets the device enumeration Speed.</td>
 <td> Read</td>
 </tr>

 <tr>
 <td> hptxfsiz </td>
 <td> Gets the value of the Host Periodic Transmit FIFO</td>
 <td> Read</td>
 </tr>

 <tr>
 <td> hprt0 </td>
 <td> Gets or sets the value in the Host Port Control and Status Register</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> regoffset </td>
 <td> Sets the register offset for the next Register Access</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> regvalue </td>
 <td> Gets or sets the value of the register at the offset in the regoffset attribute.</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> remote_wakeup </td>
 <td> On read, shows the status of Remote Wakeup. On write, initiates a remote
 wakeup of the host. When bit 0 is 1 and Remote Wakeup is enabled, the Remote
 Wakeup signalling bit in the Device Control Register is set for 1
 milli-second.</td>
 <td> Read/Write</td>
 </tr>

 <tr>
 <td> regdump </td>
 <td> Dumps the contents of core registers.</td>
 <td> Read</td>
 </tr>

 <tr>
 <td> spramdump </td>
 <td> Dumps the contents of core registers.</td>
 <td> Read</td>
 </tr>

 <tr>
 <td> hcddump </td>
 <td> Dumps the current HCD state.</td>
 <td> Read</td>
 </tr>

 <tr>
 <td> hcd_frrem </td>
 <td> Shows the average value of the Frame Remaining
 field in the Host Frame Number/Frame Remaining register when an SOF interrupt
 occurs. This can be used to determine the average interrupt latency. Also
 shows the average Frame Remaining value for start_transfer and the "a" and
 "b" sample points. The "a" and "b" sample points may be used during debugging
 bto determine how long it takes to execute a section of the HCD code.</td>
 <td> Read</td>
 </tr>

 <tr>
 <td> rd_reg_test </td>
 <td> Displays the time required to read the GNPTXFSIZ register many times
 (the output shows the number of times the register is read).
 <td> Read</td>
 </tr>

 <tr>
 <td> wr_reg_test </td>
 <td> Displays the time required to write the GNPTXFSIZ register many times
 (the output shows the number of times the register is written).
 <td> Read</td>
 </tr>

 <tr>
 <td> lpm_response </td>
 <td> Gets or sets lpm_response mode. Applicable only in device mode.
 <td> Write</td>
 </tr>

 <tr>
 <td> sleep_local_dev </td>
 <td> Generetates sleep signaling. Applicable only in host mode.
 <td> Write</td>
 </tr>

 <tr>
 <td> sleep_status </td>
 <td> Shows sleep status of device.
 <td> Read</td>
 </tr>

 </table>

 Example usage:
 To get the current mode:
 cat /sys/devices/lm0/mode

 To power down the USB:
 echo 0 > /sys/devices/lm0/buspower
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/stat.h>		
#include <linux/version.h>
#include <linux/param.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

#include "dwc_otg_os_dep.h"
#include "dwc_os.h"
#include "dwc_otg_attr.h"
#include "dwc_otg_driver.h"
#include "dwc_otg_core_if.h"
#include "dwc_otg_pcd_if.h"
#include "dwc_otg_hcd_if.h"

#define DWC_OTG_DEVICE_ATTR_BITFIELD_SHOW(_otg_attr_name_,_string_) \
static ssize_t _otg_attr_name_##_show (struct device *_dev, struct device_attribute *attr, char *buf) \
{ \
	struct platform_device *lm_dev = to_platform_device(_dev);\
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);\
	uint32_t val; \
	val = dwc_otg_get_##_otg_attr_name_ (otg_dev->core_if); \
	return sprintf (buf, "%s = 0x%x\n", _string_, val); \
}
#define DWC_OTG_DEVICE_ATTR_BITFIELD_STORE(_otg_attr_name_,_string_) \
static ssize_t _otg_attr_name_##_store (struct device *_dev, struct device_attribute *attr, \
					const char *buf, size_t count) \
{ \
	struct platform_device *lm_dev = to_platform_device(_dev);\
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);\
	uint32_t set = simple_strtoul(buf, NULL, 16); \
	dwc_otg_set_##_otg_attr_name_(otg_dev->core_if, set);\
	return count; \
}


#define DWC_OTG_DEVICE_ATTR_REG_SHOW(_otg_attr_name_,_string_) \
static ssize_t _otg_attr_name_##_show (struct device *_dev, struct device_attribute *attr, char *buf) \
{ \
	struct platform_device *lm_dev = to_platform_device(_dev);\
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);\
	uint32_t val; \
	val = dwc_otg_get_##_otg_attr_name_ (otg_dev->core_if); \
	return sprintf (buf, "%s = 0x%08x\n", _string_, val); \
}
#define DWC_OTG_DEVICE_ATTR_REG_STORE(_otg_attr_name_,_string_) \
static ssize_t _otg_attr_name_##_store (struct device *_dev, struct device_attribute *attr, \
					const char *buf, size_t count) \
{ \
	struct platform_device *lm_dev = to_platform_device(_dev);\
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);\
	uint32_t val = simple_strtoul(buf, NULL, 16); \
	dwc_otg_set_##_otg_attr_name_ (otg_dev->core_if, val); \
	return count; \
}

#define DWC_OTG_DEVICE_ATTR_BITFIELD_RW(_otg_attr_name_,_string_) \
DWC_OTG_DEVICE_ATTR_BITFIELD_SHOW(_otg_attr_name_,_string_) \
DWC_OTG_DEVICE_ATTR_BITFIELD_STORE(_otg_attr_name_,_string_) \
DEVICE_ATTR(_otg_attr_name_,0644,_otg_attr_name_##_show,_otg_attr_name_##_store);

#define DWC_OTG_DEVICE_ATTR_BITFIELD_RO(_otg_attr_name_,_string_) \
DWC_OTG_DEVICE_ATTR_BITFIELD_SHOW(_otg_attr_name_,_string_) \
DEVICE_ATTR(_otg_attr_name_,0444,_otg_attr_name_##_show,NULL);

#define DWC_OTG_DEVICE_ATTR_REG32_RW(_otg_attr_name_,_addr_,_string_) \
DWC_OTG_DEVICE_ATTR_REG_SHOW(_otg_attr_name_,_string_) \
DWC_OTG_DEVICE_ATTR_REG_STORE(_otg_attr_name_,_string_) \
DEVICE_ATTR(_otg_attr_name_,0644,_otg_attr_name_##_show,_otg_attr_name_##_store);

#define DWC_OTG_DEVICE_ATTR_REG32_RO(_otg_attr_name_,_addr_,_string_) \
DWC_OTG_DEVICE_ATTR_REG_SHOW(_otg_attr_name_,_string_) \
DEVICE_ATTR(_otg_attr_name_,0444,_otg_attr_name_##_show,NULL);


static ssize_t regoffset_show(struct device *_dev,
			      struct device_attribute *attr, char *buf)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	return snprintf(buf, sizeof("0xFFFFFFFF\n") + 1, "0x%08x\n",
			otg_dev->os_dep.reg_offset);
}

static ssize_t regoffset_store(struct device *_dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	uint32_t offset = simple_strtoul(buf, NULL, 16);
        if (offset < SZ_256K) {
		otg_dev->os_dep.reg_offset = offset;
	} else {
		dev_err(_dev, "invalid offset\n");
	}

	return count;
}

DEVICE_ATTR(regoffset, S_IRUGO | S_IWUSR, regoffset_show, regoffset_store);

static ssize_t regvalue_show(struct device *_dev,
			     struct device_attribute *attr, char *buf)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	uint32_t val;
	volatile uint32_t *addr;

	if (otg_dev->os_dep.reg_offset != 0xFFFFFFFF && 0 != otg_dev->os_dep.base) {
		
		addr = (uint32_t *) (otg_dev->os_dep.reg_offset +
				     (uint8_t *) otg_dev->os_dep.base);
		val = DWC_READ_REG32(addr);
		return snprintf(buf,
				sizeof("Reg@0xFFFFFFFF = 0xFFFFFFFF\n") + 1,
				"Reg@0x%06x = 0x%08x\n", otg_dev->os_dep.reg_offset,
				val);
	} else {
		dev_err(_dev, "Invalid offset (0x%0x)\n", otg_dev->os_dep.reg_offset);
		return sprintf(buf, "invalid offset\n");
	}
}

static ssize_t regvalue_store(struct device *_dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	volatile uint32_t *addr;
	uint32_t val = simple_strtoul(buf, NULL, 16);
	
	if (otg_dev->os_dep.reg_offset != 0xFFFFFFFF && 0 != otg_dev->os_dep.base) {
		
		addr = (uint32_t *) (otg_dev->os_dep.reg_offset +
				     (uint8_t *) otg_dev->os_dep.base);
		DWC_WRITE_REG32(addr, val);
	} else {
		dev_err(_dev, "Invalid Register Offset (0x%08x)\n",
			otg_dev->os_dep.reg_offset);
	}
	return count;
}

DEVICE_ATTR(regvalue, S_IRUGO | S_IWUSR, regvalue_show, regvalue_store);

DWC_OTG_DEVICE_ATTR_BITFIELD_RO(mode, "Mode");
DWC_OTG_DEVICE_ATTR_BITFIELD_RW(hnpcapable, "HNPCapable");
DWC_OTG_DEVICE_ATTR_BITFIELD_RW(srpcapable, "Mode");
DWC_OTG_DEVICE_ATTR_BITFIELD_RW(hsic_connect, "HSIC Connect");
DWC_OTG_DEVICE_ATTR_BITFIELD_RW(inv_sel_hsic, "Invert Select HSIC");

DWC_OTG_DEVICE_ATTR_BITFIELD_RO(busconnected, "Bus Connected");

DWC_OTG_DEVICE_ATTR_REG32_RW(gotgctl, 0, "GOTGCTL");
DWC_OTG_DEVICE_ATTR_REG32_RW(gusbcfg,
			     &(otg_dev->core_if->core_global_regs->gusbcfg),
			     "GUSBCFG");
DWC_OTG_DEVICE_ATTR_REG32_RW(grxfsiz,
			     &(otg_dev->core_if->core_global_regs->grxfsiz),
			     "GRXFSIZ");
DWC_OTG_DEVICE_ATTR_REG32_RW(gnptxfsiz,
			     &(otg_dev->core_if->core_global_regs->gnptxfsiz),
			     "GNPTXFSIZ");
DWC_OTG_DEVICE_ATTR_REG32_RW(gpvndctl,
			     &(otg_dev->core_if->core_global_regs->gpvndctl),
			     "GPVNDCTL");
DWC_OTG_DEVICE_ATTR_REG32_RW(ggpio,
			     &(otg_dev->core_if->core_global_regs->ggpio),
			     "GGPIO");
DWC_OTG_DEVICE_ATTR_REG32_RW(guid, &(otg_dev->core_if->core_global_regs->guid),
			     "GUID");
DWC_OTG_DEVICE_ATTR_REG32_RO(gsnpsid,
			     &(otg_dev->core_if->core_global_regs->gsnpsid),
			     "GSNPSID");
DWC_OTG_DEVICE_ATTR_BITFIELD_RW(devspeed, "Device Speed");
DWC_OTG_DEVICE_ATTR_BITFIELD_RO(enumspeed, "Device Enumeration Speed");

DWC_OTG_DEVICE_ATTR_REG32_RO(hptxfsiz,
			     &(otg_dev->core_if->core_global_regs->hptxfsiz),
			     "HPTXFSIZ");
DWC_OTG_DEVICE_ATTR_REG32_RW(hprt0, otg_dev->core_if->host_if->hprt0, "HPRT0");

static ssize_t hnp_show(struct device *_dev,
			struct device_attribute *attr, char *buf)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);
	return sprintf(buf, "HstNegScs = 0x%x\n",
		       dwc_otg_get_hnpstatus(otg_dev->core_if));
}

static ssize_t hnp_store(struct device *_dev,
			 struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);
	
	uint32_t in = simple_strtoul(buf, NULL, 16);
	dwc_otg_set_hnpreq(otg_dev->core_if, in);
	return count;
}

DEVICE_ATTR(hnp, 0644, hnp_show, hnp_store);

static ssize_t srp_show(struct device *_dev,
			struct device_attribute *attr, char *buf)
{
#ifndef DWC_HOST_ONLY
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);
	return sprintf(buf, "SesReqScs = 0x%x\n",
		       dwc_otg_get_srpstatus(otg_dev->core_if));
#else
	return sprintf(buf, "Host Only Mode!\n");
#endif
}

static ssize_t srp_store(struct device *_dev,
			 struct device_attribute *attr,
			 const char *buf, size_t count)
{
#ifndef DWC_HOST_ONLY
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);
	dwc_otg_pcd_initiate_srp(otg_dev->pcd);
#endif
	return count;
}

DEVICE_ATTR(srp, 0644, srp_show, srp_store);

static ssize_t buspower_show(struct device *_dev,
			     struct device_attribute *attr, char *buf)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);
	return sprintf(buf, "Bus Power = 0x%x\n",
		       dwc_otg_get_prtpower(otg_dev->core_if));
}

static ssize_t buspower_store(struct device *_dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);
	uint32_t on = simple_strtoul(buf, NULL, 16);
	dwc_otg_set_prtpower(otg_dev->core_if, on);
	return count;
}

DEVICE_ATTR(buspower, 0644, buspower_show, buspower_store);

static ssize_t bussuspend_show(struct device *_dev,
			       struct device_attribute *attr, char *buf)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	return sprintf(buf, "Bus Suspend = 0x%x\n",
		       dwc_otg_get_prtsuspend(otg_dev->core_if));
}

static ssize_t bussuspend_store(struct device *_dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	uint32_t in = simple_strtoul(buf, NULL, 16);
	dwc_otg_set_prtsuspend(otg_dev->core_if, in);
	return count;
}

DEVICE_ATTR(bussuspend, 0644, bussuspend_show, bussuspend_store);

static ssize_t remote_wakeup_show(struct device *_dev,
				  struct device_attribute *attr, char *buf)
{
#ifndef DWC_HOST_ONLY
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);
	return sprintf(buf,
		       "Remote Wakeup Sig = %d Enabled = %d LPM Remote Wakeup = %d\n",
		       dwc_otg_get_remotewakesig(otg_dev->core_if),
		       dwc_otg_pcd_get_rmwkup_enable(otg_dev->pcd),
		       dwc_otg_get_lpm_remotewakeenabled(otg_dev->core_if));
#else
	return sprintf(buf, "Host Only Mode!\n");
#endif 
}

/**
 * Initiate a remote wakeup of the host.  The Device control register
 * Remote Wakeup Signal bit is written if the PCD Remote wakeup enable
 * flag is set.
 *
 */
static ssize_t remote_wakeup_store(struct device *_dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
#ifndef DWC_HOST_ONLY
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	uint32_t val = simple_strtoul(buf, NULL, 16);

	if (val & 1) {
		dwc_otg_pcd_remote_wakeup(otg_dev->pcd, 1);
	} else {
		dwc_otg_pcd_remote_wakeup(otg_dev->pcd, 0);
	}
#endif 
	return count;
}

DEVICE_ATTR(remote_wakeup, S_IRUGO | S_IWUSR, remote_wakeup_show,
	    remote_wakeup_store);

static ssize_t regdump_show(struct device *_dev,
			    struct device_attribute *attr, char *buf)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);
	dwc_otg_dump_global_registers(otg_dev->core_if);
	if (dwc_otg_is_host_mode(otg_dev->core_if)) {
		dwc_otg_dump_host_registers(otg_dev->core_if);
	} else {
		dwc_otg_dump_dev_registers(otg_dev->core_if);

	}
	return sprintf(buf, "Register Dump\n");
}

DEVICE_ATTR(regdump, S_IRUGO | S_IWUSR, regdump_show, 0);

static ssize_t spramdump_show(struct device *_dev,
			      struct device_attribute *attr, char *buf)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	dwc_otg_dump_spram(otg_dev->core_if);

	return sprintf(buf, "SPRAM Dump\n");
}

DEVICE_ATTR(spramdump, S_IRUGO | S_IWUSR, spramdump_show, 0);

static ssize_t hcddump_show(struct device *_dev,
			    struct device_attribute *attr, char *buf)
{
#ifndef DWC_DEVICE_ONLY
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	dwc_otg_hcd_dump_state(otg_dev->hcd);
#endif 
	return sprintf(buf, "HCD Dump\n");
}

DEVICE_ATTR(hcddump, S_IRUGO | S_IWUSR, hcddump_show, 0);

static ssize_t hcd_frrem_show(struct device *_dev,
			      struct device_attribute *attr, char *buf)
{
#ifndef DWC_DEVICE_ONLY
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);


	dwc_otg_hcd_dump_frrem(otg_dev->hcd);
#endif 
	return sprintf(buf, "HCD Dump Frame Remaining\n");
}

DEVICE_ATTR(hcd_frrem, S_IRUGO | S_IWUSR, hcd_frrem_show, 0);

#define RW_REG_COUNT 10000000
#define MSEC_PER_JIFFIE 1000/HZ
static ssize_t rd_reg_test_show(struct device *_dev,
				struct device_attribute *attr, char *buf)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);


	int i;
	int time;
	int start_jiffies;

	printk("HZ %d, MSEC_PER_JIFFIE %d, loops_per_jiffy %lu\n",
	       HZ, MSEC_PER_JIFFIE, loops_per_jiffy);
	start_jiffies = jiffies;
	for (i = 0; i < RW_REG_COUNT; i++) {
		dwc_otg_get_gnptxfsiz(otg_dev->core_if);
	}
	time = jiffies - start_jiffies;
	return sprintf(buf,
		       "Time to read GNPTXFSIZ reg %d times: %d msecs (%d jiffies)\n",
		       RW_REG_COUNT, time * MSEC_PER_JIFFIE, time);
}

DEVICE_ATTR(rd_reg_test, S_IRUGO | S_IWUSR, rd_reg_test_show, 0);

/**
 * Displays the time required to write the GNPTXFSIZ register many times (the
 * output shows the number of times the register is written).
 */
static ssize_t wr_reg_test_show(struct device *_dev,
				struct device_attribute *attr, char *buf)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	uint32_t reg_val;
	int i;
	int time;
	int start_jiffies;

	printk("HZ %d, MSEC_PER_JIFFIE %d, loops_per_jiffy %lu\n",
	       HZ, MSEC_PER_JIFFIE, loops_per_jiffy);
	reg_val = dwc_otg_get_gnptxfsiz(otg_dev->core_if);
	start_jiffies = jiffies;
	for (i = 0; i < RW_REG_COUNT; i++) {
		dwc_otg_set_gnptxfsiz(otg_dev->core_if, reg_val);
	}
	time = jiffies - start_jiffies;
	return sprintf(buf,
		       "Time to write GNPTXFSIZ reg %d times: %d msecs (%d jiffies)\n",
		       RW_REG_COUNT, time * MSEC_PER_JIFFIE, time);
}

DEVICE_ATTR(wr_reg_test, S_IRUGO | S_IWUSR, wr_reg_test_show, 0);

#ifdef CONFIG_USB_DWC_OTG_LPM

static ssize_t lpmresp_show(struct device *_dev,
			    struct device_attribute *attr, char *buf)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	if (!dwc_otg_get_param_lpm_enable(otg_dev->core_if))
		return sprintf(buf, "** LPM is DISABLED **\n");

	if (!dwc_otg_is_device_mode(otg_dev->core_if)) {
		return sprintf(buf, "** Current mode is not device mode\n");
	}
	return sprintf(buf, "lpm_response = %d\n",
		       dwc_otg_get_lpmresponse(otg_dev->core_if));
}

static ssize_t lpmresp_store(struct device *_dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);


	uint32_t val = simple_strtoul(buf, NULL, 16);

	if (!dwc_otg_get_param_lpm_enable(otg_dev->core_if)) {
		return 0;
	}

	if (!dwc_otg_is_device_mode(otg_dev->core_if)) {
		return 0;
	}

	dwc_otg_set_lpmresponse(otg_dev->core_if, val);
	return count;
}

DEVICE_ATTR(lpm_response, S_IRUGO | S_IWUSR, lpmresp_show, lpmresp_store);

static ssize_t sleepstatus_show(struct device *_dev,
				struct device_attribute *attr, char *buf)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);



	return sprintf(buf, "Sleep Status = %d\n",
		       dwc_otg_get_lpm_portsleepstatus(otg_dev->core_if));
}

static ssize_t sleepstatus_store(struct device *_dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct platform_device *lm_dev = to_platform_device(_dev);
	dwc_otg_device_t *otg_dev = platform_get_drvdata(lm_dev);

	dwc_otg_core_if_t *core_if = otg_dev->core_if;

	if (dwc_otg_get_lpm_portsleepstatus(otg_dev->core_if)) {
		if (dwc_otg_is_host_mode(core_if)) {

			DWC_PRINTF("Host initiated resume\n");
			dwc_otg_set_prtresume(otg_dev->core_if, 1);
		}
	}

	return count;
}

DEVICE_ATTR(sleep_status, S_IRUGO | S_IWUSR, sleepstatus_show,
	    sleepstatus_store);

#endif				


void dwc_otg_attr_create(
	struct platform_device *dev
	)

{
	int error;

	error = device_create_file(&dev->dev, &dev_attr_regoffset);
	error = device_create_file(&dev->dev, &dev_attr_regvalue);
	error = device_create_file(&dev->dev, &dev_attr_mode);
	error = device_create_file(&dev->dev, &dev_attr_hnpcapable);
	error = device_create_file(&dev->dev, &dev_attr_srpcapable);
	error = device_create_file(&dev->dev, &dev_attr_hsic_connect);
	error = device_create_file(&dev->dev, &dev_attr_inv_sel_hsic);
	error = device_create_file(&dev->dev, &dev_attr_hnp);
	error = device_create_file(&dev->dev, &dev_attr_srp);
	error = device_create_file(&dev->dev, &dev_attr_buspower);
	error = device_create_file(&dev->dev, &dev_attr_bussuspend);
	error = device_create_file(&dev->dev, &dev_attr_busconnected);
	error = device_create_file(&dev->dev, &dev_attr_gotgctl);
	error = device_create_file(&dev->dev, &dev_attr_gusbcfg);
	error = device_create_file(&dev->dev, &dev_attr_grxfsiz);
	error = device_create_file(&dev->dev, &dev_attr_gnptxfsiz);
	error = device_create_file(&dev->dev, &dev_attr_gpvndctl);
	error = device_create_file(&dev->dev, &dev_attr_ggpio);
	error = device_create_file(&dev->dev, &dev_attr_guid);
	error = device_create_file(&dev->dev, &dev_attr_gsnpsid);
	error = device_create_file(&dev->dev, &dev_attr_devspeed);
	error = device_create_file(&dev->dev, &dev_attr_enumspeed);
	error = device_create_file(&dev->dev, &dev_attr_hptxfsiz);
	error = device_create_file(&dev->dev, &dev_attr_hprt0);
	error = device_create_file(&dev->dev, &dev_attr_remote_wakeup);
	error = device_create_file(&dev->dev, &dev_attr_regdump);
	error = device_create_file(&dev->dev, &dev_attr_spramdump);
	error = device_create_file(&dev->dev, &dev_attr_hcddump);
	error = device_create_file(&dev->dev, &dev_attr_hcd_frrem);
	error = device_create_file(&dev->dev, &dev_attr_rd_reg_test);
	error = device_create_file(&dev->dev, &dev_attr_wr_reg_test);
#ifdef CONFIG_USB_DWC_OTG_LPM
	error = device_create_file(&dev->dev, &dev_attr_lpm_response);
	error = device_create_file(&dev->dev, &dev_attr_sleep_status);
#endif
}

void dwc_otg_attr_remove(
	struct platform_device *dev
       )

{
	device_remove_file(&dev->dev, &dev_attr_regoffset);
	device_remove_file(&dev->dev, &dev_attr_regvalue);
	device_remove_file(&dev->dev, &dev_attr_mode);
	device_remove_file(&dev->dev, &dev_attr_hnpcapable);
	device_remove_file(&dev->dev, &dev_attr_srpcapable);
	device_remove_file(&dev->dev, &dev_attr_hsic_connect);
	device_remove_file(&dev->dev, &dev_attr_inv_sel_hsic);
	device_remove_file(&dev->dev, &dev_attr_hnp);
	device_remove_file(&dev->dev, &dev_attr_srp);
	device_remove_file(&dev->dev, &dev_attr_buspower);
	device_remove_file(&dev->dev, &dev_attr_bussuspend);
	device_remove_file(&dev->dev, &dev_attr_busconnected);
	device_remove_file(&dev->dev, &dev_attr_gotgctl);
	device_remove_file(&dev->dev, &dev_attr_gusbcfg);
	device_remove_file(&dev->dev, &dev_attr_grxfsiz);
	device_remove_file(&dev->dev, &dev_attr_gnptxfsiz);
	device_remove_file(&dev->dev, &dev_attr_gpvndctl);
	device_remove_file(&dev->dev, &dev_attr_ggpio);
	device_remove_file(&dev->dev, &dev_attr_guid);
	device_remove_file(&dev->dev, &dev_attr_gsnpsid);
	device_remove_file(&dev->dev, &dev_attr_devspeed);
	device_remove_file(&dev->dev, &dev_attr_enumspeed);
	device_remove_file(&dev->dev, &dev_attr_hptxfsiz);
	device_remove_file(&dev->dev, &dev_attr_hprt0);
	device_remove_file(&dev->dev, &dev_attr_remote_wakeup);
	device_remove_file(&dev->dev, &dev_attr_regdump);
	device_remove_file(&dev->dev, &dev_attr_spramdump);
	device_remove_file(&dev->dev, &dev_attr_hcddump);
	device_remove_file(&dev->dev, &dev_attr_hcd_frrem);
	device_remove_file(&dev->dev, &dev_attr_rd_reg_test);
	device_remove_file(&dev->dev, &dev_attr_wr_reg_test);
#ifdef CONFIG_USB_DWC_OTG_LPM
	device_remove_file(&dev->dev, &dev_attr_lpm_response);
	device_remove_file(&dev->dev, &dev_attr_sleep_status);
#endif
}
