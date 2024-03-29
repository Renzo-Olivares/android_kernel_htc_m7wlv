#ifndef _DWC_OS_DEP_H_
#define _DWC_OS_DEP_H_
 
 #if 0
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/stat.h>
#include <linux/pci.h>
 
#include <linux/version.h>
 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
# include <linux/irq.h>
#endif
 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
# include <linux/usb/ch9.h>
#else
# include <linux/usb_ch9.h>
#endif
 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
# include <linux/usb/gadget.h>
#else
# include <linux/usb_gadget.h>
#endif
#endif
 

#include <linux/platform_device.h>
#include <asm/mach/map.h>

 
#define DWC_OS_PAGE_SIZE PAGE_SIZE
 

typedef struct os_dependent {
void *base;
 
uint32_t reg_offset;


struct platform_device *platformdev;

 
} os_dependent_t;
 
#ifdef __cplusplus
}
#endif
 

typedef struct platform_device dwc_bus_dev_t;

 

#define DWC_OTG_BUSDRVDATA(_dev) platform_get_drvdata(_dev)

 

#define DWC_OTG_GETDRVDEV(_var, _dev) do { \
struct platform_device *platform_dev = \
container_of(_dev, struct platform_device, dev); \
_var = platform_get_drvdata(platform_dev); \
} while (0)

 
 

#define DWC_OTG_OS_GETDEV(_osdep) \
((_osdep).platformdev == NULL? NULL: &(_osdep).platformdev->dev)

 
 
 
 
#endif 
