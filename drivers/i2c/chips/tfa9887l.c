/* driver/i2c/chip/tfa9887.c
 *
 * NXP tfa9887 Speaker Amp
 *
 * Copyright (C) 2012 HTC Corporation
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/i2c/tfa9887.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/mfd/pm8xxx/pm8921.h>

#undef LOG_TAG
#define LOG_TAG "AUD"

#undef AUDIO_DEBUG
#define AUDIO_DEBUG 0

#define AUD_ERR(fmt, ...) pr_tag_err(LOG_TAG, fmt, ##__VA_ARGS__)
#define AUD_INFO(fmt, ...) pr_tag_info(LOG_TAG, fmt, ##__VA_ARGS__)

#if AUDIO_DEBUG
#define AUD_DBG(fmt, ...) pr_tag_info(LOG_TAG, fmt, ##__VA_ARGS__)
#else
#define AUD_DBG(fmt, ...) do { } while (0)
#endif

#define TPA9887_IOCTL_MAGIC 'a'
#define TPA9887_WRITE_CONFIG	_IOW(TPA9887_IOCTL_MAGIC, 0x01, unsigned int)
#define TPA9887_READ_CONFIG	_IOW(TPA9887_IOCTL_MAGIC, 0x02, unsigned int)
#define TPA9887_ENABLE_DSP	_IOW(TPA9887_IOCTL_MAGIC, 0x03, unsigned int)
#define TPA9887_KERNEL_LOCK    _IOW(TPA9887_IOCTL_MAGIC, 0x06, unsigned int)
#define DEBUG (0)

static struct i2c_client *this_client;
static struct tfa9887_platform_data *pdata;
struct mutex spk_ampl_lock;
static int tfa9887l_opened;
static int last_spkampl_state;
static int dspl_enabled;
static int tfa9887_i2c_write(char *txData, int length);
static int tfa9887_i2c_read(char *rxData, int length);
#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_tpa_dent;
static struct dentry *debugfs_peek;
static struct dentry *debugfs_poke;
static unsigned char read_data;

static int get_parameters(char *buf, long int *param1, int num_of_par)
{
	char *token;
	int base, cnt;

	token = strsep(&buf, " ");

	for (cnt = 0; cnt < num_of_par; cnt++) {
		if (token != NULL) {
			if ((token[1] == 'x') || (token[1] == 'X'))
				base = 16;
			else
				base = 10;

			if (strict_strtoul(token, base, &param1[cnt]) != 0)
				return -EINVAL;

			token = strsep(&buf, " ");
			}
		else
			return -EINVAL;
	}
	return 0;
}

static int codec_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t codec_debug_read(struct file *file, char __user *ubuf,
				size_t count, loff_t *ppos)
{
	char lbuf[8];

	snprintf(lbuf, sizeof(lbuf), "0x%x\n", read_data);
	return simple_read_from_buffer(ubuf, count, ppos, lbuf, strlen(lbuf));
}

static ssize_t codec_debug_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char *access_str = filp->private_data;
	char lbuf[32];
	unsigned char reg_idx[2] = {0x00, 0x00};
	int rc;
	long int param[5];

	if (cnt > sizeof(lbuf) - 1)
		return -EINVAL;

	rc = copy_from_user(lbuf, ubuf, cnt);
	if (rc)
		return -EFAULT;

	lbuf[cnt] = '\0';

	if (!strcmp(access_str, "poke")) {
		
		rc = get_parameters(lbuf, param, 2);
		if ((param[0] <= 0xFF) && (param[1] <= 0xFF) &&
			(rc == 0)) {
			reg_idx[0] = param[0];
			reg_idx[1] = param[1];
			tfa9887_i2c_write(reg_idx, 2);
		} else
			rc = -EINVAL;
	} else if (!strcmp(access_str, "peek")) {
		
		rc = get_parameters(lbuf, param, 1);
		if ((param[0] <= 0xFF) && (rc == 0)) {
			reg_idx[0] = param[0];
			tfa9887_i2c_read(&read_data, 1);
		} else
			rc = -EINVAL;
	}

	if (rc == 0)
		rc = cnt;
	else
		AUD_ERR("%s: rc = %d\n", __func__, rc);

	return rc;
}

static const struct file_operations codec_debug_ops = {
	.open = codec_debug_open,
	.write = codec_debug_write,
	.read = codec_debug_read
};
#endif

unsigned char cf_dspl_bypass[4][3] = {
	
	
	{0x04, 0x88, 0xB},
	
        {0x06, 0x00, 0x07},
	{0x09, 0x06, 0x19},
	{0x09, 0x06, 0x18}
};

unsigned char ampl_off[1][3] = {
	{0x09, 0x06, 0x19}
};

static int tfa9887_i2c_write(char *txData, int length)
{
	int rc;
	struct i2c_msg msg[] = {
		{
			.addr = this_client->addr,
			.flags = 0,
			.len = length,
			.buf = txData,
		},
	};

	rc = i2c_transfer(this_client->adapter, msg, 1);
	if (rc < 0) {
		AUD_ERR("%s: transfer error %d\n", __func__, rc);
		return rc;
	}

#if AUDIO_DEBUG
	{
		int i = 0;
		for (i = 0; i < length; i++)
			AUD_INFO("%s: tx[%d] = %2x\n", \
				__func__, i, txData[i]);
	}
#endif

	return 0;
}

static int tfa9887_i2c_read(char *rxData, int length)
{
	int rc;
	struct i2c_msg msgs[] = {
		{
		 .addr = this_client->addr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxData,
		},
	};

	rc = i2c_transfer(this_client->adapter, msgs, 1);
	if (rc < 0) {
		AUD_ERR("%s: transfer error %d\n", __func__, rc);
		return rc;
	}

#if AUDIO_DEBUG
	{
		int i = 0;
		for (i = 0; i < length; i++)
			AUD_INFO("i2c_read %s: rx[%d] = %2x\n", __func__, i, \
				rxData[i]);
	}
#endif

	return 0;
}

static int tfa9887l_open(struct inode *inode, struct file *file)
{
	int rc = 0;

	if (tfa9887l_opened) {
		AUD_INFO("%s: busy\n", __func__);
	}
	tfa9887l_opened = 1;

	return rc;
}

static int tfa9887l_release(struct inode *inode, struct file *file)
{
	tfa9887l_opened = 0;

	return 0;
}


int tfa9887_l_write(char *txData, int length) {
	return tfa9887_i2c_write(txData, length);
}

int tfa9887_l_read(char *rxData, int length) {
    return tfa9887_i2c_read(rxData, length);
}

void set_tfa9887l_spkamp(int en, int dsp_mode)
{
	int i =0;
        
        
        unsigned char mute_reg[1] = {0x06};
	unsigned char mute_data[3] = {0, 0, 0};
        unsigned char power_reg[1] = {0x09};
	unsigned char power_data[3] = {0, 0, 0};
	unsigned char SPK_CR[3] = {0x8, 0x8, 0};

	AUD_INFO("%s: en = %d dsp_enabled = %d\n", __func__, en, dspl_enabled);
	mutex_lock(&spk_ampl_lock);
	if (en && !last_spkampl_state) {
		last_spkampl_state = 1;
		
		if (dspl_enabled == 0) {
			for (i=0; i <4 ; i++)
				tfa9887_i2c_write(cf_dspl_bypass[i], 3);
                
				tfa9887_i2c_write(SPK_CR,1);
				tfa9887_i2c_read(SPK_CR+1,2);
				SPK_CR[1] |= 0x4; 
				tfa9887_i2c_write(SPK_CR,3);
		} else {
			
			
			tfa9887_i2c_write(power_reg, 1);
			tfa9887_i2c_read(power_data + 1, 2);
			tfa9887_i2c_write(mute_reg, 1);
			tfa9887_i2c_read(mute_data + 1, 2);
			mute_data[0] = 0x6;
			mute_data[2] &= 0xef;  
			power_data[0] = 0x9;
			power_data[2] &= 0xfe; 
			tfa9887_i2c_write(power_data, 3);
			tfa9887_i2c_write(mute_data, 3);
		}
	} else if (!en && last_spkampl_state) {
		last_spkampl_state = 0;
		if (dspl_enabled == 0) {
			tfa9887_i2c_write(ampl_off[0], 3);
		} else {
			
			
			tfa9887_i2c_write(power_reg, 1);
			tfa9887_i2c_read(power_data + 1, 2);
			tfa9887_i2c_write(mute_reg, 1);
			tfa9887_i2c_read(mute_data + 1, 2);
			mute_data[0] = 0x6;
			mute_data[2] |= 0x10; 
			power_data[0] = 0x9;
			power_data[2] |= 0x1;  
			tfa9887_i2c_write(mute_data, 3);
			tfa9887_i2c_write(power_data, 3);
		}
	}
	mutex_unlock(&spk_ampl_lock);
}

static long tfa9887l_ioctl(struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	int rc = 0;
	unsigned int reg_value[2];
	unsigned int len = 0;
	char *addr;
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case TPA9887_WRITE_CONFIG:
		AUD_INFO("%s: TPA9887_WRITE_CONFIG\n", __func__);
		rc = copy_from_user(reg_value, argp, sizeof(reg_value));
		if (rc) {
			AUD_ERR("%s: copy from user failed.\n", __func__);
			goto err;
		}

		len = reg_value[0];
		addr = (char *)reg_value[1];

		tfa9887_i2c_write(addr+1, len -1);

		break;
	case TPA9887_READ_CONFIG:
		AUD_INFO("%s: TPA9887_READ_CONFIG\n", __func__);
		rc = copy_from_user(reg_value, argp, sizeof(reg_value));;
		if (rc) {
			AUD_ERR("%s: copy from user failed.\n", __func__);
			goto err;
		}

		len = reg_value[0];
		addr = (char *)reg_value[1];
		tfa9887_i2c_read(addr, len);

		rc = copy_to_user(argp, reg_value, sizeof(reg_value));
		if (rc) {
			pr_err("%s: copy to user failed.\n", __func__);
			goto err;
		}
		break;
	case TPA9887_ENABLE_DSP:
		AUD_INFO("%s: TPA9887_ENABLE_DSP\n", __func__);
		rc = copy_from_user(reg_value, argp, sizeof(reg_value));;
		if (rc) {
			AUD_ERR("%s: copy from user failed.\n", __func__);
			goto err;
		}

		len = reg_value[0];
		dspl_enabled = reg_value[1];
		break;
	case TPA9887_KERNEL_LOCK:
		rc = copy_from_user(reg_value, argp, sizeof(reg_value));;
		if (rc) {
		   pr_err("%s: copy from user failed.\n", __func__);
		   goto err;
		}

		len = reg_value[0];
		
		AUD_INFO("TPA9887_KLOCK2 %d\n", reg_value[1]);
		if (reg_value[1])
		   mutex_lock(&spk_ampl_lock);
		else
		   mutex_unlock(&spk_ampl_lock);
		break;
	}
err:
	return rc;
}

static struct file_operations tfa9887l_fops = {
	.owner = THIS_MODULE,
	.open = tfa9887l_open,
	.release = tfa9887l_release,
	.unlocked_ioctl = tfa9887l_ioctl,
};

static struct miscdevice tfa9887l_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "tfa9887l",
	.fops = &tfa9887l_fops,
};

int tfa9887l_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;

	pdata = client->dev.platform_data;

	if (pdata == NULL) {
		pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
		if (pdata == NULL) {
			ret = -ENOMEM;
			AUD_ERR("%s: platform data is NULL\n", __func__);
			goto err_alloc_data_failed;
		}
	}

	this_client = client;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		AUD_ERR("%s: i2c check functionality error\n", __func__);
		ret = -ENODEV;
		goto err_free_gpio_all;
	}

	ret = misc_register(&tfa9887l_device);
	if (ret) {
		AUD_ERR("%s: tfa9887l_device register failed\n", __func__);
		goto err_free_gpio_all;
	}
#ifdef CONFIG_DEBUG_FS
	debugfs_tpa_dent = debugfs_create_dir("tfa9887", 0);
	if (!IS_ERR(debugfs_tpa_dent)) {
		debugfs_peek = debugfs_create_file("peek",
		S_IFREG | S_IRUGO, debugfs_tpa_dent,
		(void *) "peek", &codec_debug_ops);

		debugfs_poke = debugfs_create_file("poke",
		S_IFREG | S_IRUGO, debugfs_tpa_dent,
		(void *) "poke", &codec_debug_ops);

	}
#endif
	return 0;

err_free_gpio_all:
	return ret;
err_alloc_data_failed:
	return ret;
}

static int tfa9887l_remove(struct i2c_client *client)
{
	struct tfa9887_platform_data *p9887data = i2c_get_clientdata(client);
	kfree(p9887data);

	return 0;
}

static int tfa9887l_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int tfa9887l_resume(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id tfa9887l_id[] = {
	{ "tfa9887l", 0 },
	{ }
};

static struct i2c_driver tfa9887l_driver = {
	.probe = tfa9887l_probe,
	.remove = tfa9887l_remove,
	.suspend = tfa9887l_suspend,
	.resume = tfa9887l_resume,
	.id_table = tfa9887l_id,
	.driver = {
		.name = "tfa9887l",
	},
};

static int __init tfa9887l_init(void)
{
	AUD_INFO("%s\n", __func__);
	mutex_init(&spk_ampl_lock);
        dspl_enabled = 0;
	return i2c_add_driver(&tfa9887l_driver);
}

static void __exit tfa9887l_exit(void)
{
#ifdef CONFIG_DEBUG_FS
	debugfs_remove(debugfs_peek);
	debugfs_remove(debugfs_poke);
	debugfs_remove(debugfs_tpa_dent);
#endif
	i2c_del_driver(&tfa9887l_driver);
}

module_init(tfa9887l_init);
module_exit(tfa9887l_exit);

MODULE_DESCRIPTION("tfa9887 L Speaker Amp driver");
MODULE_LICENSE("GPL");
