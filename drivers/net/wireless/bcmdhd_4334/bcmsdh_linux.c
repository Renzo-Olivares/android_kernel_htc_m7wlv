/*
 * SDIO access interface for drivers - linux specific (pci only)
 *
 * Copyright (C) 1999-2012, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: bcmsdh_linux.c 312788 2012-02-03 23:06:32Z $
 */


#define __UNDEF_NO_VERSION__

#include <typedefs.h>
#include <linuxver.h>

#include <linux/pci.h>
#include <linux/completion.h>

#include <osl.h>
#include <pcicfg.h>
#include <bcmdefs.h>
#include <bcmdevs.h>

#if defined(OOB_INTR_ONLY) || defined(BCMSPI_ANDROID)
#include <linux/irq.h>
extern void dhdsdio_isr(void * args);
#include <bcmutils.h>
#include <dngl_stats.h>
#include <dhd.h>
#endif 

#if defined(CUSTOMER_HW4) && defined(CONFIG_PM_SLEEP) && defined(PLATFORM_SLP)
struct device *pm_dev;
#endif 

typedef struct bcmsdh_hc bcmsdh_hc_t;

struct bcmsdh_hc {
	bcmsdh_hc_t *next;
#ifdef BCMPLATFORM_BUS
	struct device *dev;			
#else
	struct pci_dev *dev;		
#endif 
	osl_t *osh;
	void *regs;			
	bcmsdh_info_t *sdh;		
	void *ch;
	unsigned int oob_irq;
	unsigned long oob_flags; 
	bool oob_irq_registered;
	bool oob_irq_enable_flag;
#if defined(OOB_INTR_ONLY) || defined(BCMSPI_ANDROID)
	spinlock_t irq_lock;
#endif 
};
static bcmsdh_hc_t *sdhcinfo = NULL;

static bcmsdh_driver_t drvinfo = {NULL, NULL};

#define SDLX_MSG(x)

bool
bcmsdh_chipmatch(uint16 vendor, uint16 device)
{
	

#ifdef BCMSDIOH_STD
	
	if (vendor == VENDOR_SI_IMAGE) {
		return (TRUE);
	}
	
	if (device == BCM27XX_SDIOH_ID && vendor == VENDOR_BROADCOM) {
		return (TRUE);
	}
	
	if (device == SDIOH_FPGA_ID && vendor == VENDOR_BROADCOM) {
		return (TRUE);
	}
	
	if (device == PCIXX21_SDIOH_ID && vendor == VENDOR_TI) {
		return (TRUE);
	}
	if (device == PCIXX21_SDIOH0_ID && vendor == VENDOR_TI) {
		return (TRUE);
	}
	
	if (device == R5C822_SDIOH_ID && vendor == VENDOR_RICOH) {
		return (TRUE);
	}
	
	if (device == JMICRON_SDIOH_ID && vendor == VENDOR_JMICRON) {
		return (TRUE);
	}

#endif 
#ifdef BCMSDIOH_SPI
	
	if (device == SPIH_FPGA_ID && vendor == VENDOR_BROADCOM) {
		printf("Found PCI SPI Host Controller\n");
		return (TRUE);
	}

#endif 

	return (FALSE);
}

#if defined(BCMPLATFORM_BUS)
#if defined(BCMLXSDMMC) || defined(BCMSPI_ANDROID)
int bcmsdh_probe(struct device *dev);
int bcmsdh_remove(struct device *dev);

EXPORT_SYMBOL(bcmsdh_probe);
EXPORT_SYMBOL(bcmsdh_remove);

#else
static int __devinit bcmsdh_probe(struct device *dev);
static int __devexit bcmsdh_remove(struct device *dev);
#endif 

#if !defined(BCMLXSDMMC) && !defined(BCMSPI_ANDROID)
static
#endif 
int bcmsdh_probe(struct device *dev)
{
	osl_t *osh = NULL;
	bcmsdh_hc_t *sdhc = NULL;
	ulong regs = 0;
	bcmsdh_info_t *sdh = NULL;
#if !defined(BCMLXSDMMC) && defined(BCMPLATFORM_BUS) && !defined(BCMSPI_ANDROID)
	struct platform_device *pdev;
	struct resource *r;
#endif 
	int irq = 0;
	uint32 vendevid;
	unsigned long irq_flags = 0;
#if defined(CUSTOMER_HW4) && defined(CONFIG_PM_SLEEP) && defined(PLATFORM_SLP)
	int ret = 0;
#endif 

#if !defined(BCMLXSDMMC) && defined(BCMPLATFORM_BUS) && !defined(BCMSPI_ANDROID)
	pdev = to_platform_device(dev);
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);
	if (!r || irq == NO_IRQ)
		return -ENXIO;
#endif 

#if defined(OOB_INTR_ONLY) || defined(BCMSPI_ANDROID)
#ifdef HW_OOB
	irq_flags =
		IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE;
#else
	 irq_flags = IRQF_TRIGGER_FALLING;
#endif 

	
	irq = dhd_customer_oob_irq_map(&irq_flags);
	if  (irq < 0) {
		SDLX_MSG(("%s: Host irq is not defined\n", __FUNCTION__));
		return 1;
	}
#endif 
	
	if (!(osh = osl_attach(dev, PCI_BUS, FALSE))) {
		SDLX_MSG(("%s: osl_attach failed\n", __FUNCTION__));
		goto err;
	}
	if (!(sdhc = MALLOC(osh, sizeof(bcmsdh_hc_t)))) {
		SDLX_MSG(("%s: out of memory, allocated %d bytes\n",
			__FUNCTION__,
			MALLOCED(osh)));
		goto err;
	}
	bzero(sdhc, sizeof(bcmsdh_hc_t));
	sdhc->osh = osh;

	sdhc->dev = (void *)dev;

#if defined(BCMLXSDMMC) || defined(BCMSPI_ANDROID)
	if (!(sdh = bcmsdh_attach(osh, (void *)0,
	                          (void **)&regs, irq))) {
		SDLX_MSG(("%s: bcmsdh_attach failed\n", __FUNCTION__));
		goto err;
	}
#else
	if (!(sdh = bcmsdh_attach(osh, (void *)r->start,
	                          (void **)&regs, irq))) {
		SDLX_MSG(("%s: bcmsdh_attach failed\n", __FUNCTION__));
		goto err;
	}
#endif 
	sdhc->sdh = sdh;
	sdhc->oob_irq = irq;
	sdhc->oob_flags = irq_flags;
	sdhc->oob_irq_registered = FALSE;	
	sdhc->oob_irq_enable_flag = FALSE;
#if defined(OOB_INTR_ONLY) || defined(BCMSPI_ANDROID)
	spin_lock_init(&sdhc->irq_lock);
#endif 

	
	sdhc->next = sdhcinfo;
	sdhcinfo = sdhc;
#if defined(CUSTOMER_HW4) && defined(CONFIG_PM_SLEEP) && defined(PLATFORM_SLP)
	
	pm_dev = sdhc->dev;
	ret = device_init_wakeup(pm_dev, 1);
	printf("%s : device_init_wakeup(pm_dev) enable, ret = %d\n", __func__, ret);
#endif 

	
	vendevid = bcmsdh_query_device(sdh);
	
	if (!(sdhc->ch = drvinfo.attach((vendevid >> 16),
	                                 (vendevid & 0xFFFF), 0, 0, 0, 0,
	                                (void *)regs, NULL, sdh))) {
		SDLX_MSG(("%s: device attach failed\n", __FUNCTION__));
		goto err;
	}

	return 0;

	
err:
	if (sdhc) {
		if (sdhc->sdh)
			bcmsdh_detach(sdhc->osh, sdhc->sdh);
		MFREE(osh, sdhc, sizeof(bcmsdh_hc_t));
	}
	if (osh)
		osl_detach(osh);
	return -ENODEV;
}

#if !defined(BCMLXSDMMC) && !defined(BCMSPI_ANDROID)
static
#endif 
int bcmsdh_remove(struct device *dev)
{
	bcmsdh_hc_t *sdhc, *prev;
	osl_t *osh;

	sdhc = sdhcinfo;
#if defined(CUSTOMER_HW4) && defined(CONFIG_PM_SLEEP) && defined(PLATFORM_SLP)
	
	device_init_wakeup(pm_dev, 0);
	printf("%s : device_init_wakeup(pm_dev) disable\n", __func__);
#endif 
	drvinfo.detach(sdhc->ch);
	bcmsdh_detach(sdhc->osh, sdhc->sdh);

	
	for (sdhc = sdhcinfo, prev = NULL; sdhc; sdhc = sdhc->next) {
		if (sdhc->dev == (void *)dev) {
			if (prev)
				prev->next = sdhc->next;
			else
				sdhcinfo = NULL;
			break;
		}
		prev = sdhc;
	}
	if (!sdhc) {
		SDLX_MSG(("%s: failed\n", __FUNCTION__));
		return 0;
	}

	
	osh = sdhc->osh;
	MFREE(osh, sdhc, sizeof(bcmsdh_hc_t));
	osl_detach(osh);

#if !defined(BCMLXSDMMC) || defined(OOB_INTR_ONLY) || defined(BCMSPI_ANDROID)
	dev_set_drvdata(dev, NULL);
#endif 

	return 0;
}

#else 

#if !defined(BCMLXSDMMC)
static int __devinit bcmsdh_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
static void __devexit bcmsdh_pci_remove(struct pci_dev *pdev);

static struct pci_device_id bcmsdh_pci_devid[] __devinitdata = {
	{ vendor: PCI_ANY_ID,
	device: PCI_ANY_ID,
	subvendor: PCI_ANY_ID,
	subdevice: PCI_ANY_ID,
	class: 0,
	class_mask: 0,
	driver_data: 0,
	},
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, bcmsdh_pci_devid);

static struct pci_driver bcmsdh_pci_driver = {
	node:		{},
	name:		"bcmsdh",
	id_table:	bcmsdh_pci_devid,
	probe:		bcmsdh_pci_probe,
	remove:		bcmsdh_pci_remove,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
	save_state:	NULL,
#endif
	suspend:	NULL,
	resume:		NULL,
	};


extern uint sd_pci_slot;	
							
							
							
							
							
							
							
							
module_param(sd_pci_slot, uint, 0);


static int __devinit
bcmsdh_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	osl_t *osh = NULL;
	bcmsdh_hc_t *sdhc = NULL;
	ulong regs;
	bcmsdh_info_t *sdh = NULL;
	int rc;

	if (sd_pci_slot != 0xFFFFffff) {
		if (pdev->bus->number != (sd_pci_slot>>16) ||
			PCI_SLOT(pdev->devfn) != (sd_pci_slot&0xffff)) {
			SDLX_MSG(("%s: %s: bus %X, slot %X, vend %X, dev %X\n",
				__FUNCTION__,
				bcmsdh_chipmatch(pdev->vendor, pdev->device)
				?"Found compatible SDIOHC"
				:"Probing unknown device",
				pdev->bus->number, PCI_SLOT(pdev->devfn), pdev->vendor,
				pdev->device));
			return -ENODEV;
		}
		SDLX_MSG(("%s: %s: bus %X, slot %X, vendor %X, device %X (good PCI location)\n",
			__FUNCTION__,
			bcmsdh_chipmatch(pdev->vendor, pdev->device)
			?"Using compatible SDIOHC"
			:"WARNING, forced use of unkown device",
			pdev->bus->number, PCI_SLOT(pdev->devfn), pdev->vendor, pdev->device));
	}

	if ((pdev->vendor == VENDOR_TI) && ((pdev->device == PCIXX21_FLASHMEDIA_ID) ||
	    (pdev->device == PCIXX21_FLASHMEDIA0_ID))) {
		uint32 config_reg;

		SDLX_MSG(("%s: Disabling TI FlashMedia Controller.\n", __FUNCTION__));
		if (!(osh = osl_attach(pdev, PCI_BUS, FALSE))) {
			SDLX_MSG(("%s: osl_attach failed\n", __FUNCTION__));
			goto err;
		}

		config_reg = OSL_PCI_READ_CONFIG(osh, 0x4c, 4);

		config_reg |= 0x02;
		OSL_PCI_WRITE_CONFIG(osh, 0x4c, 4, config_reg);
		osl_detach(osh);
	}
	
	
	if (!bcmsdh_chipmatch(pdev->vendor, pdev->device)) {
		return -ENODEV;
	}

	
	SDLX_MSG(("%s: Found possible SDIO Host Controller: bus %d slot %d func %d irq %d\n",
		__FUNCTION__,
		pdev->bus->number, PCI_SLOT(pdev->devfn),
		PCI_FUNC(pdev->devfn), pdev->irq));


	
	if (!(osh = osl_attach(pdev, PCI_BUS, FALSE))) {
		SDLX_MSG(("%s: osl_attach failed\n", __FUNCTION__));
		goto err;
	}
	if (!(sdhc = MALLOC(osh, sizeof(bcmsdh_hc_t)))) {
		SDLX_MSG(("%s: out of memory, allocated %d bytes\n",
			__FUNCTION__,
			MALLOCED(osh)));
		goto err;
	}
	bzero(sdhc, sizeof(bcmsdh_hc_t));
	sdhc->osh = osh;

	sdhc->dev = pdev;

	
	pci_set_master(pdev);
	rc = pci_enable_device(pdev);
	if (rc) {
		SDLX_MSG(("%s: Cannot enable PCI device\n", __FUNCTION__));
		goto err;
	}
	if (!(sdh = bcmsdh_attach(osh, (void *)(uintptr)pci_resource_start(pdev, 0),
	                          (void **)&regs, pdev->irq))) {
		SDLX_MSG(("%s: bcmsdh_attach failed\n", __FUNCTION__));
		goto err;
	}

	sdhc->sdh = sdh;

	
	if (!(sdhc->ch = drvinfo.attach(VENDOR_BROADCOM, 
	                                bcmsdh_query_device(sdh) & 0xFFFF, 0, 0, 0, 0,
	                                (void *)regs, NULL, sdh))) {
		SDLX_MSG(("%s: device attach failed\n", __FUNCTION__));
		goto err;
	}

	
	sdhc->next = sdhcinfo;
	sdhcinfo = sdhc;

	return 0;

	
err:
	if (sdhc) {
		if (sdhc->sdh)
			bcmsdh_detach(sdhc->osh, sdhc->sdh);
		MFREE(osh, sdhc, sizeof(bcmsdh_hc_t));
	}
	if (osh)
		osl_detach(osh);
	return -ENODEV;
}


static void __devexit
bcmsdh_pci_remove(struct pci_dev *pdev)
{
	bcmsdh_hc_t *sdhc, *prev;
	osl_t *osh;

	
	for (sdhc = sdhcinfo, prev = NULL; sdhc; sdhc = sdhc->next) {
		if (sdhc->dev == pdev) {
			if (prev)
				prev->next = sdhc->next;
			else
				sdhcinfo = NULL;
			break;
		}
		prev = sdhc;
	}
	if (!sdhc)
		return;

	drvinfo.detach(sdhc->ch);

	bcmsdh_detach(sdhc->osh, sdhc->sdh);

	
	osh = sdhc->osh;
	MFREE(osh, sdhc, sizeof(bcmsdh_hc_t));
	osl_detach(osh);
}
#endif 
#endif 

#ifdef BCMSPI_ANDROID
extern int spi_function_init(void);
#else
extern int sdio_function_init(void);
#endif 

extern int sdio_func_reg_notify(void* semaphore);
extern void sdio_func_unreg_notify(void);

#if defined(BCMLXSDMMC)
int bcmsdh_reg_sdio_notify(void* semaphore)
{
	return sdio_func_reg_notify(semaphore);
}

void bcmsdh_unreg_sdio_notify(void)
{
	sdio_func_unreg_notify();
}
#endif 

int
bcmsdh_register(bcmsdh_driver_t *driver)
{
	int error = 0;

	drvinfo = *driver;

#if defined(BCMPLATFORM_BUS)
#ifdef BCMSPI_ANDROID
	SDLX_MSG(("Linux Kernel SPI Driver\n"));
	error = spi_function_init();
#else 
	SDLX_MSG(("Linux Kernel SDIO/MMC Driver\n"));
	error = sdio_function_init();
#endif 
	return error;
#endif 

#if !defined(BCMPLATFORM_BUS) && !defined(BCMLXSDMMC)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
	if (!(error = pci_module_init(&bcmsdh_pci_driver)))
		return 0;
#else
	if (!(error = pci_register_driver(&bcmsdh_pci_driver)))
		return 0;
#endif

	SDLX_MSG(("%s: pci_module_init failed 0x%x\n", __FUNCTION__, error));
#endif 

	return error;
}

#ifdef BCMSPI_ANDROID
extern void spi_function_cleanup(void);
#else
extern void sdio_function_cleanup(void);
#endif 

void
bcmsdh_unregister(void)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
	if (bcmsdh_pci_driver.node.next)
#endif

#ifdef BCMSPI_ANDROID
	spi_function_cleanup();
#endif 
#if defined(BCMLXSDMMC)
	sdio_function_cleanup();
#endif 

#if !defined(BCMPLATFORM_BUS) && !defined(BCMLXSDMMC)
	pci_unregister_driver(&bcmsdh_pci_driver);
#endif 
}

#if defined(OOB_INTR_ONLY) || defined(BCMSPI_ANDROID)
static atomic_t bcmsdh_irq_status = ATOMIC_INIT(1);
void bcmsdh_oob_intr_set(bool enable)
{
	static bool curstate = 1;
	unsigned long flags;

	spin_lock_irqsave(&sdhcinfo->irq_lock, flags);
	if (curstate != enable) {
		if (enable) {
			curstate = 1;
			atomic_set(&bcmsdh_irq_status, 1);
			enable_irq(sdhcinfo->oob_irq);
		} else {
			disable_irq_nosync(sdhcinfo->oob_irq);
			atomic_set(&bcmsdh_irq_status, 0);
			curstate = 0;
		}
	}
	spin_unlock_irqrestore(&sdhcinfo->irq_lock, flags);
}

static irqreturn_t wlan_oob_irq(int irq, void *dev_id)
{
	dhd_pub_t *dhdp;

	dhdp = (dhd_pub_t *)dev_get_drvdata(sdhcinfo->dev);

#if defined(OOB_INTR_ONLY)
	if (atomic_read(&bcmsdh_irq_status) == 0) {
		printf("GPIO IRQ disabled, skip it!\n");
		return IRQ_HANDLED;
	}
#endif
#ifdef HW_OOB
	bcmsdh_oob_intr_set(0);
#endif

	if (dhdp == NULL) {
#ifndef HW_OOB
		bcmsdh_oob_intr_set(0);
#endif
		SDLX_MSG(("Out of band GPIO interrupt fired way too early\n"));
		return IRQ_HANDLED;
	}

	dhdsdio_isr((void *)dhdp->bus);

	return IRQ_HANDLED;
}

int bcmsdh_register_oob_intr(void * dhdp)
{
	int error = 0;

	SDLX_MSG(("%s Enter \n", __FUNCTION__));

	

	dev_set_drvdata(sdhcinfo->dev, dhdp);

	if (!sdhcinfo->oob_irq_registered) {
		SDLX_MSG(("%s IRQ=%d Type=%X \n", __FUNCTION__,
			(int)sdhcinfo->oob_irq, (int)sdhcinfo->oob_flags));
		
		error = request_irq(sdhcinfo->oob_irq, wlan_oob_irq, sdhcinfo->oob_flags|IRQF_NO_SUSPEND,
			"bcmsdh_sdmmc", NULL);
		if (error)
			return -ENODEV;

		error = enable_irq_wake(sdhcinfo->oob_irq);
		if (error)
			SDLX_MSG(("%s enable_irq_wake error=%d \n", __FUNCTION__, error));
		sdhcinfo->oob_irq_registered = TRUE;
		sdhcinfo->oob_irq_enable_flag = TRUE;
	}

	return 0;
}

void bcmsdh_set_irq(int flag)
{
	if (sdhcinfo->oob_irq_registered && sdhcinfo->oob_irq_enable_flag != flag) {
		SDLX_MSG(("%s Flag = %d", __FUNCTION__, flag));
		sdhcinfo->oob_irq_enable_flag = flag;
		if (flag) {
			enable_irq(sdhcinfo->oob_irq);
			enable_irq_wake(sdhcinfo->oob_irq);
		} else {
#if !(defined(BCMSPI_ANDROID) && defined(CUSTOMER_HW4) && defined(CONFIG_NKERNEL))
			disable_irq_wake(sdhcinfo->oob_irq);
#endif 
			disable_irq(sdhcinfo->oob_irq);
		}
	}
}

void bcmsdh_unregister_oob_intr(void)
{
	SDLX_MSG(("%s: Enter\n", __FUNCTION__));

	if (sdhcinfo->oob_irq_registered == TRUE) {
		bcmsdh_set_irq(FALSE);
		free_irq(sdhcinfo->oob_irq, NULL);
		sdhcinfo->oob_irq_registered = FALSE;
	}
}
#endif 

#if defined(BCMLXSDMMC)
void *bcmsdh_get_drvdata(void)
{
	if (!sdhcinfo)
		return NULL;
	return dev_get_drvdata(sdhcinfo->dev);
}
#endif


extern uint sd_msglevel;	
module_param(sd_msglevel, uint, 0);

extern uint sd_power;	
module_param(sd_power, uint, 0);

extern uint sd_clock;	
module_param(sd_clock, uint, 0);

extern uint sd_divisor;	
module_param(sd_divisor, uint, 0);

extern uint sd_sdmode;	
module_param(sd_sdmode, uint, 0);

extern uint sd_hiok;	
module_param(sd_hiok, uint, 0);

extern uint sd_f2_blocksize;
module_param(sd_f2_blocksize, int, 0);

#ifdef BCMSDIOH_STD
extern int sd_uhsimode;
module_param(sd_uhsimode, int, 0);
#endif

#ifdef BCMSDIOH_TXGLOM
extern uint sd_txglom;
module_param(sd_txglom, uint, 0);
#endif

#ifdef BCMSDH_MODULE
EXPORT_SYMBOL(bcmsdh_attach);
EXPORT_SYMBOL(bcmsdh_detach);
EXPORT_SYMBOL(bcmsdh_intr_query);
EXPORT_SYMBOL(bcmsdh_intr_enable);
EXPORT_SYMBOL(bcmsdh_intr_disable);
EXPORT_SYMBOL(bcmsdh_intr_reg);
EXPORT_SYMBOL(bcmsdh_intr_dereg);

#if defined(DHD_DEBUG)
EXPORT_SYMBOL(bcmsdh_intr_pending);
#endif

EXPORT_SYMBOL(bcmsdh_devremove_reg);
EXPORT_SYMBOL(bcmsdh_cfg_read);
EXPORT_SYMBOL(bcmsdh_cfg_write);
EXPORT_SYMBOL(bcmsdh_cis_read);
EXPORT_SYMBOL(bcmsdh_reg_read);
EXPORT_SYMBOL(bcmsdh_reg_write);
EXPORT_SYMBOL(bcmsdh_regfail);
EXPORT_SYMBOL(bcmsdh_send_buf);
EXPORT_SYMBOL(bcmsdh_recv_buf);

EXPORT_SYMBOL(bcmsdh_rwdata);
EXPORT_SYMBOL(bcmsdh_abort);
EXPORT_SYMBOL(bcmsdh_query_device);
EXPORT_SYMBOL(bcmsdh_query_iofnum);
EXPORT_SYMBOL(bcmsdh_iovar_op);
EXPORT_SYMBOL(bcmsdh_register);
EXPORT_SYMBOL(bcmsdh_unregister);
EXPORT_SYMBOL(bcmsdh_chipmatch);
EXPORT_SYMBOL(bcmsdh_reset);
EXPORT_SYMBOL(bcmsdh_waitlockfree);

EXPORT_SYMBOL(bcmsdh_get_dstatus);
EXPORT_SYMBOL(bcmsdh_cfg_read_word);
EXPORT_SYMBOL(bcmsdh_cfg_write_word);
EXPORT_SYMBOL(bcmsdh_cur_sbwad);
EXPORT_SYMBOL(bcmsdh_chipinfo);

#endif 
