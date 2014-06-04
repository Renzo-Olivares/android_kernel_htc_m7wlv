
#include <linux/if_ether.h>

struct android_usb_platform_data {
	
	__u16 vendor_id;
	
	__u16 product_id;
	int	usb_id_pin_gpio;
	char	*serial_number;

	char	*manufacturer_name;
	char	*product_name;

	
	char	*rndisVendorDescr;
	u32	rndisVendorID;
	u8	rndisEthaddr[ETH_ALEN];

	
	char	*ecmVendorDescr;
	u32	ecmVendorID;
	u8	ecmEthaddr[ETH_ALEN];
	
	char *fserial_init_string;
	
	unsigned char diag_init:1;
	unsigned char modem_init:1;
	unsigned char rmnet_init:1;
	unsigned char reserved:5;

	
	int nluns;
	int cdrom_lun;
	int (*match)(int product_id, int intrsharing);
	u8			usb_core_id;
	int adb_perf_lock_on;
	int mtp_perf_lock_on;
};
int htc_usb_enable_function(char *name, int ebl);
