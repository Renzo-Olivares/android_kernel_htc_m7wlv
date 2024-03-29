#ifndef AKM8975_H
#define AKM8975_H

#include <linux/ioctl.h>

#define AKM8975_I2C_NAME "akm8975"

#define AKM8975_I2C_ADDR 0x0C

#define SENSOR_DATA_SIZE	8
#define YPR_DATA_SIZE		12
#define RWBUF_SIZE			16

#define ACC_DATA_FLAG		0
#define MAG_DATA_FLAG		1
#define ORI_DATA_FLAG		2
#define AKM_NUM_SENSORS		3

#define ACC_DATA_READY		(1<<(ACC_DATA_FLAG))
#define MAG_DATA_READY		(1<<(MAG_DATA_FLAG))
#define ORI_DATA_READY		(1<<(ORI_DATA_FLAG))

#define AK8975_MEASUREMENT_TIME_US	10000

#define AK8975_MODE_SNG_MEASURE	0x01
#define	AK8975_MODE_SELF_TEST	0x08
#define	AK8975_MODE_FUSE_ACCESS	0x0F
#define	AK8975_MODE_POWERDOWN	0x00

#define AK8975_REG_WIA		0x00
#define AK8975_REG_INFO		0x01
#define AK8975_REG_ST1		0x02
#define AK8975_REG_HXL		0x03
#define AK8975_REG_HXH		0x04
#define AK8975_REG_HYL		0x05
#define AK8975_REG_HYH		0x06
#define AK8975_REG_HZL		0x07
#define AK8975_REG_HZH		0x08
#define AK8975_REG_ST2		0x09
#define AK8975_REG_CNTL		0x0A
#define AK8975_REG_RSV		0x0B
#define AK8975_REG_ASTC		0x0C
#define AK8975_REG_TS1		0x0D
#define AK8975_REG_TS2		0x0E
#define AK8975_REG_I2CDIS	0x0F

#define AK8975_FUSE_ASAX	0x10
#define AK8975_FUSE_ASAY	0x11
#define AK8975_FUSE_ASAZ	0x12

#define AKMIO                   0xA1

#define ECS_IOCTL_READ              _IOWR(AKMIO, 0x01, char*)
#define ECS_IOCTL_WRITE             _IOW(AKMIO, 0x02, char*)
#define ECS_IOCTL_SET_MODE          _IOW(AKMIO, 0x03, short)
#define ECS_IOCTL_GETDATA           _IOR(AKMIO, 0x04, char[SENSOR_DATA_SIZE])
#define ECS_IOCTL_SET_YPR           _IOW(AKMIO, 0x0C, short[YPR_DATA_SIZE])
#define ECS_IOCTL_GET_OPEN_STATUS   _IOR(AKMIO, 0x06, int)
#define ECS_IOCTL_GET_CLOSE_STATUS  _IOR(AKMIO, 0x07, int)
#define ECS_IOCTL_GET_DELAY         _IOR(AKMIO, 0x08, long long int[AKM_NUM_SENSORS])
#define ECS_IOCTL_GET_LAYOUT        _IOR(AKMIO, 0x09, char)
#define ECS_IOCTL_GET_ACCEL			_IOR(AKMIO, 0x30, short[3])

#define ECS_IOCTL_APP_SET_MODE         _IOW(AKMIO, 0x10, short)
#define ECS_IOCTL_APP_SET_MFLAG        _IOW(AKMIO, 0x11, short)
#define ECS_IOCTL_APP_GET_MFLAG        _IOW(AKMIO, 0x12, short)
#define ECS_IOCTL_APP_SET_AFLAG        _IOW(AKMIO, 0x13, short)
#define ECS_IOCTL_APP_GET_AFLAG        _IOR(AKMIO, 0x14, short)
#define ECS_IOCTL_APP_SET_TFLAG        _IOR(AKMIO, 0x15, short)
#define ECS_IOCTL_APP_GET_TFLAG        _IOR(AKMIO, 0x16, short)
#define ECS_IOCTL_APP_RESET_PEDOMETER  _IO(AKMIO, 0x17)
#define ECS_IOCTL_APP_SET_DELAY        _IOW(AKMIO, 0x18, short)
#define ECS_IOCTL_APP_GET_DELAY	       ECS_IOCTL_GET_DELAY

#define ECS_IOCTL_APP_SET_MVFLAG       _IOW(AKMIO, 0x19, short)

#define ECS_IOCTL_APP_GET_MVFLAG       _IOR(AKMIO, 0x1A, short)


struct akm8975_platform_data {
	char layout;
	int gpio_DRDY;
	int mag_low_x;
	int mag_high_x;
	int mag_low_y;
	int mag_high_y;
	int mag_low_z;
	int mag_high_z;
	int (*power_on)(void);
	int (*power_off)(void);
};

#endif

