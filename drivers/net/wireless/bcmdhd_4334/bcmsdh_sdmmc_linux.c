/*
 * BCMSDH Function Driver for the native SDIO/MMC driver in the Linux Kernel
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
 * $Id: bcmsdh_sdmmc_linux.c 312783 2012-02-03 22:53:56Z $
 */

#include <typedefs.h>
#include <bcmutils.h>
#include <sdio.h>	
#include <bcmsdbus.h>	
#include <sdiovar.h>	

#include <linux/sched.h>	

#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#ifdef CONFIG_SDIO_CARD
#include <linux/pm_runtime.h>
#endif

#if !defined(SDIO_VENDOR_ID_BROADCOM)
#define SDIO_VENDOR_ID_BROADCOM		0x02d0
#endif 

#define SDIO_DEVICE_ID_BROADCOM_DEFAULT	0x0000

#if !defined(SDIO_DEVICE_ID_BROADCOM_4325_SDGWB)
#define SDIO_DEVICE_ID_BROADCOM_4325_SDGWB	0x0492	
#endif 
#if !defined(SDIO_DEVICE_ID_BROADCOM_4325)
#define SDIO_DEVICE_ID_BROADCOM_4325	0x0493
#endif 
#if !defined(SDIO_DEVICE_ID_BROADCOM_4329)
#define SDIO_DEVICE_ID_BROADCOM_4329	0x4329
#endif 
#if !defined(SDIO_DEVICE_ID_BROADCOM_4319)
#define SDIO_DEVICE_ID_BROADCOM_4319	0x4319
#endif 
#if !defined(SDIO_DEVICE_ID_BROADCOM_4330)
#define SDIO_DEVICE_ID_BROADCOM_4330	0x4330
#endif 
#if !defined(SDIO_DEVICE_ID_BROADCOM_4334)
#define SDIO_DEVICE_ID_BROADCOM_4334    0x4334
#endif 
#if !defined(SDIO_DEVICE_ID_BROADCOM_4324)
#define SDIO_DEVICE_ID_BROADCOM_4324    0x4324
#endif 
#if !defined(SDIO_DEVICE_ID_BROADCOM_43239)
#define SDIO_DEVICE_ID_BROADCOM_43239    43239
#endif 


#include <bcmsdh_sdmmc.h>

#include <dhd_dbg.h>

#ifdef WL_CFG80211
extern void wl_cfg80211_set_parent_dev(void *dev);
#endif

extern void sdioh_sdmmc_devintr_off(sdioh_info_t *sd);
extern void sdioh_sdmmc_devintr_on(sdioh_info_t *sd);
extern int dhd_os_check_wakelock(void *dhdp);
extern int dhd_os_check_if_up(void *dhdp);
extern void *bcmsdh_get_drvdata(void);

int sdio_function_init(void);
void sdio_function_cleanup(void);

#define DESCRIPTION "bcmsdh_sdmmc Driver"
#define AUTHOR "Broadcom Corporation"

static int clockoverride = 0;

module_param(clockoverride, int, 0644);
MODULE_PARM_DESC(clockoverride, "SDIO card clock override");

PBCMSDH_SDMMC_INSTANCE gInstance;

#define BCMSDH_SDMMC_MAX_DEVICES 1

extern int bcmsdh_probe(struct device *dev);
extern int bcmsdh_remove(struct device *dev);
extern volatile bool dhd_mmc_suspend;

#ifdef CONFIG_SDIO_CARD
	static void bcmsdh_start_runtime(struct sdio_func *func) {
	pm_runtime_no_callbacks(&func->dev);
	pm_suspend_ignore_children(&func->dev, true);
	pm_runtime_set_autosuspend_delay(&func->dev, 50);
	pm_runtime_use_autosuspend(&func->dev);
	pm_runtime_put_autosuspend(&func->dev);
	}
#endif

static int bcmsdh_sdmmc_probe(struct sdio_func *func,
                              const struct sdio_device_id *id)
{
	int ret = 0;
	static struct sdio_func sdio_func_0;
	if (func) {
		sd_trace(("bcmsdh_sdmmc: %s Enter\n", __FUNCTION__));
		sd_trace(("sdio_bcmsdh: func->class=%x\n", func->class));
		sd_trace(("sdio_vendor: 0x%04x\n", func->vendor));
		sd_trace(("sdio_device: 0x%04x\n", func->device));
		sd_trace(("Function#: 0x%04x\n", func->num));

		if (func->num == 1) {
			sdio_func_0.num = 0;
			sdio_func_0.card = func->card;
			gInstance->func[0] = &sdio_func_0;
			if(func->device == 0x4) { 
				gInstance->func[2] = NULL;
				sd_trace(("NIC found, calling bcmsdh_probe...\n"));
				ret = bcmsdh_probe(&func->dev);
			}
		}

		gInstance->func[func->num] = func;

		if (func->num == 2) {
	#ifdef WL_CFG80211
			wl_cfg80211_set_parent_dev(&func->dev);
	#endif
			sd_trace(("F2 found, calling bcmsdh_probe...\n"));
			ret = bcmsdh_probe(&func->dev);
		}
	} else {
		ret = -ENODEV;
	}
#ifdef CONFIG_SDIO_CARD
	if(!ret) {
	bcmsdh_start_runtime(func);
	}
#endif

	return ret;
}

static void bcmsdh_sdmmc_remove(struct sdio_func *func)
{
	if (func) {
		sd_trace(("bcmsdh_sdmmc: %s Enter\n", __FUNCTION__));
		sd_info(("sdio_bcmsdh: func->class=%x\n", func->class));
		sd_info(("sdio_vendor: 0x%04x\n", func->vendor));
		sd_info(("sdio_device: 0x%04x\n", func->device));
		sd_info(("Function#: 0x%04x\n", func->num));

		if (func->num == 2) {
			sd_trace(("F2 found, calling bcmsdh_remove...\n"));
			bcmsdh_remove(&func->dev);
		} else if (func->num == 1) {
			sdio_claim_host(func);
			sdio_disable_func(func);
			sdio_release_host(func);
			gInstance->func[1] = NULL;
		}
	}
}

static const struct sdio_device_id bcmsdh_sdmmc_ids[] = {
	{ SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_DEFAULT) },
	{ SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4325_SDGWB) },
	{ SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4325) },
	{ SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4329) },
	{ SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4319) },
	{ SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4330) },
	{ SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4334) },
	{ SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4324) },
	{ SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_43239) },
	{ SDIO_DEVICE_CLASS(SDIO_CLASS_NONE)		},
	{ 				},
};

MODULE_DEVICE_TABLE(sdio, bcmsdh_sdmmc_ids);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 39)) && defined(CONFIG_PM)
static int bcmsdh_sdmmc_suspend(struct device *pdev)
{
	struct sdio_func *func = dev_to_sdio_func(pdev);
	mmc_pm_flag_t sdio_flags;
	int ret;

	if (func->num != 2)
		return 0;

	sd_trace(("%s Enter\n", __FUNCTION__));

	if (dhd_os_check_wakelock(bcmsdh_get_drvdata()))
		return -EBUSY;
	sdio_flags = sdio_get_host_pm_caps(func);

	if (!(sdio_flags & MMC_PM_KEEP_POWER)) {
		sd_err(("%s: can't keep power while host is suspended\n", __FUNCTION__));
		return  -EINVAL;
	}

	
	ret = sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
	if (ret) {
		sd_err(("%s: error while trying to keep power\n", __FUNCTION__));
		return ret;
	}
#if !defined(CUSTOMER_HW4)
#if defined(OOB_INTR_ONLY)
	bcmsdh_oob_intr_set(0);
#endif	
#endif  
	dhd_mmc_suspend = TRUE;
#if defined(CUSTOMER_HW4) && defined(CONFIG_ARCH_TEGRA)
	irq_set_irq_wake(390, 1);
#endif
	smp_mb();

	return 0;
}

static int bcmsdh_sdmmc_resume(struct device *pdev)
{
#if !defined(CUSTOMER_HW4)
#if defined(OOB_INTR_ONLY)
	struct sdio_func *func = dev_to_sdio_func(pdev);
#endif 
#endif 
	sd_trace(("%s Enter\n", __FUNCTION__));
	dhd_mmc_suspend = FALSE;
#if !defined(CUSTOMER_HW4)
#if defined(OOB_INTR_ONLY)
	if ((func->num == 2) && dhd_os_check_if_up(bcmsdh_get_drvdata()))
		bcmsdh_oob_intr_set(1);
#endif 

#endif 
#if defined(CUSTOMER_HW4) && defined(CONFIG_ARCH_TEGRA)
	if (func->num == 2)
		irq_set_irq_wake(390, 0);
#endif
	smp_mb();
	return 0;
}

static const struct dev_pm_ops bcmsdh_sdmmc_pm_ops = {
	.suspend	= bcmsdh_sdmmc_suspend,
	.resume		= bcmsdh_sdmmc_resume,
};
#endif  

#if defined(BCMLXSDMMC)
static struct semaphore *notify_semaphore = NULL;

static int dummy_probe(struct sdio_func *func,
                              const struct sdio_device_id *id)
{
	if (notify_semaphore)
		up(notify_semaphore);
	return 0;
}

static void dummy_remove(struct sdio_func *func)
{
}

static struct sdio_driver dummy_sdmmc_driver = {
	.probe		= dummy_probe,
	.remove		= dummy_remove,
	.name		= "dummy_sdmmc",
	.id_table	= bcmsdh_sdmmc_ids,
	};

int sdio_func_reg_notify(void* semaphore)
{
	notify_semaphore = semaphore;
	return sdio_register_driver(&dummy_sdmmc_driver);
}

void sdio_func_unreg_notify(void)
{
	sdio_unregister_driver(&dummy_sdmmc_driver);
}

#endif 

static struct sdio_driver bcmsdh_sdmmc_driver = {
	.probe		= bcmsdh_sdmmc_probe,
	.remove		= bcmsdh_sdmmc_remove,
	.name		= "bcmsdh_sdmmc",
	.id_table	= bcmsdh_sdmmc_ids,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 39)) && defined(CONFIG_PM)
	.drv = {
	.pm	= &bcmsdh_sdmmc_pm_ops,
	},
#endif 
	};

struct sdos_info {
	sdioh_info_t *sd;
	spinlock_t lock;
};


int
sdioh_sdmmc_osinit(sdioh_info_t *sd)
{
	struct sdos_info *sdos;

	if (!sd)
		return BCME_BADARG;

	sdos = (struct sdos_info*)MALLOC(sd->osh, sizeof(struct sdos_info));
	sd->sdos_info = (void*)sdos;
	if (sdos == NULL)
		return BCME_NOMEM;

	sdos->sd = sd;
	spin_lock_init(&sdos->lock);
	return BCME_OK;
}

void
sdioh_sdmmc_osfree(sdioh_info_t *sd)
{
	struct sdos_info *sdos;
	ASSERT(sd && sd->sdos_info);

	sdos = (struct sdos_info *)sd->sdos_info;
	MFREE(sd->osh, sdos, sizeof(struct sdos_info));
}

SDIOH_API_RC
sdioh_interrupt_set(sdioh_info_t *sd, bool enable)
{
	ulong flags;
	struct sdos_info *sdos;

	if (!sd)
		return BCME_BADARG;

	sd_trace(("%s: %s\n", __FUNCTION__, enable ? "Enabling" : "Disabling"));

	sdos = (struct sdos_info *)sd->sdos_info;
	ASSERT(sdos);

#if !defined(OOB_INTR_ONLY)
	if (enable && !(sd->intr_handler && sd->intr_handler_arg)) {
		sd_err(("%s: no handler registered, will not enable\n", __FUNCTION__));
		return SDIOH_API_RC_FAIL;
	}
#endif 

	
	spin_lock_irqsave(&sdos->lock, flags);

	sd->client_intr_enabled = enable;
	if (enable) {
		sdioh_sdmmc_devintr_on(sd);
	} else {
		sdioh_sdmmc_devintr_off(sd);
	}

	spin_unlock_irqrestore(&sdos->lock, flags);

	return SDIOH_API_RC_SUCCESS;
}


#ifdef BCMSDH_MODULE
static int __init
bcmsdh_module_init(void)
{
	int error = 0;
	sdio_function_init();
	return error;
}

static void __exit
bcmsdh_module_cleanup(void)
{
	sdio_function_cleanup();
}

module_init(bcmsdh_module_init);
module_exit(bcmsdh_module_cleanup);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_AUTHOR(AUTHOR);

#endif 
int sdio_function_init(void)
{
	int error = 0;
	sd_trace(("bcmsdh_sdmmc: %s Enter\n", __FUNCTION__));

	gInstance = kzalloc(sizeof(BCMSDH_SDMMC_INSTANCE), GFP_KERNEL);
	if (!gInstance)
		return -ENOMEM;

	error = sdio_register_driver(&bcmsdh_sdmmc_driver);

	return error;
}

extern int bcmsdh_remove(struct device *dev);
void sdio_function_cleanup(void)
{
	sd_trace(("%s Enter\n", __FUNCTION__));


	sdio_unregister_driver(&bcmsdh_sdmmc_driver);

	if (gInstance)
		kfree(gInstance);
}
