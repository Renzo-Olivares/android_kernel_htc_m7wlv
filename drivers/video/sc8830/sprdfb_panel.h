/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _SPRDFB_PANEL_H_
#define _SPRDFB_PANEL_H_

#include "sprdfb.h"

#define SPRDFB_MAINLCD_ID 0
#define SPRDFB_SUBLCD_ID 1
#define SPRDFB_UNDEFINELCD_ID -1

#define SPRDFB_I2C_WRITE_DELAY 0xffff


#define LCD_MODE_MCU			0
#define LCD_MODE_RGB			1
#define LCD_MODE_DSI			2

#define LCD_BUS_8080			0
#define LCD_BUS_6800			1
#define LCD_BUS_SPI			2

#define LCD_DIRECT_NORMAL		0
#define LCD_DIRECT_ROT_90		1
#define LCD_DIRECT_ROT_180		2
#define LCD_DIRECT_ROT_270		3
#define LCD_DIRECT_MIR_H		4
#define LCD_DIRECT_MIR_V		5
#define LCD_DIRECT_MIR_HV		6

#define PANEL_CAP_NORMAL               0x0

#define PANEL_CAP_NOT_PARTIAL_UPDATE   0x1

#define PANEL_CAP_UNIQUE_TIMING        0x2

#define PANEL_CAP_NOT_TEAR_SYNC        0x4

#define PANEL_CAP_MANUAL_REFRESH       0x8


#define LCD_DelayMS      msleep


#define MAX_DATA   56

typedef struct LCM_Init_Code_tag {
	unsigned int tag;
	unsigned char data[MAX_DATA];
}LCM_Init_Code;

typedef struct LCM_force_cmd_code_tag{
	unsigned int datatype;
	LCM_Init_Code real_cmd_code;
}LCM_Force_Cmd_Code;


#define LCM_TAG_SHIFT 24
#define LCM_TAG_MASK  ((1 << 24) -1)
#define LCM_SEND(len) ((1 << LCM_TAG_SHIFT)| len)
#define LCM_SLEEP(ms) ((2 << LCM_TAG_SHIFT)| ms)

#define LCM_TAG_SEND  (1<< 0)
#define LCM_TAG_SLEEP (1 << 1)

#define REGU_NAME_LCD_1V8	"vddcamio"
#define REGU_NAME_LCD_3V0	"vddcama"

enum{
	SPRDFB_PANEL_TYPE_MCU = 0,
	SPRDFB_PANEL_TYPE_RGB,
	SPRDFB_PANEL_TYPE_MIPI,
	SPRDFB_PANEL_TYPE_LIMIT
};

enum{
	SPRDFB_POLARITY_POS = 0,
	SPRDFB_POLARITY_NEG,
	SPRDFB_POLARITY_LIMIT
};

enum{
	SPRDFB_RGB_BUS_TYPE_I2C = 0,
	SPRDFB_RGB_BUS_TYPE_SPI,
	SPRDFB_RGB_BUG_TYPE_LIMIT
};

enum{
	SPRDFB_MIPI_MODE_CMD = 0,
	SPRDFB_MIPI_MODE_VIDEO,
	SPRDFB_MIPI_MODE_LIMIT
};

struct panel_spec;

typedef int32_t (*send_cmd_t)(uint32_t data);
typedef int32_t (*send_data_t)(uint32_t data);
typedef int32_t (*send_cmd_data_t)(uint32_t cmd, uint32_t data);
typedef uint32_t (*read_data_t)(void);


typedef int32_t (*mipi_set_cmd_mode_t)(void);
typedef int32_t (*mipi_set_video_mode_t)(void);
typedef int32_t (*mipi_set_lp_mode_t)(void);
typedef int32_t (*mipi_set_hs_mode_t)(void);
typedef int32_t (*mipi_gen_write_t)(uint8_t *param, uint16_t param_length);
typedef int32_t (*mipi_gen_read_t)(uint8_t *param, uint16_t param_length, uint8_t bytes_to_read, uint8_t *read_buffer);
typedef int32_t (*mipi_dcs_write_t)(uint8_t *param, uint16_t param_length);
typedef int32_t (*mipi_dcs_read_t)(uint8_t command, uint8_t bytes_to_read, uint8_t *read_buffer);
typedef	int32_t (*mipi_force_write_t)(uint8_t data_type, uint8_t *param, uint16_t param_length);	
typedef	int32_t (*mipi_force_read_t)(uint8_t command, uint8_t bytes_to_read, uint8_t *read_buffer);
typedef	int32_t (*mipi_eotp_set_t)(uint8_t rx_en, uint8_t tx_en);

typedef int32_t (*i2c_write_8bits_t)(uint8_t reg, uint8_t val);
typedef int32_t (*i2c_read_8bits_t)(uint8_t reg, uint8_t *val);
typedef int32_t (*i2c_write_16bits_t)(uint16_t reg, bool reg_is_8bit, uint16_t val, bool val_is_8bit);
typedef int32_t (*i2c_read_16bits_t)(uint16_t reg, bool reg_is_8bit, uint16_t *val, bool val_is_8bit);
typedef int32_t (*i2c_write_burst_t)(uint8_t* buf, int num);

typedef void (*spi_send_cmd_t)(uint32_t cmd);
typedef void (*spi_send_data_t)(uint32_t data);
typedef void (*spi_read_t)(uint32_t *data);

struct panel_operations {
	int32_t (*panel_init)(struct panel_spec *self);
	int32_t (*panel_close)(struct panel_spec *self);
	int32_t (*panel_reset)(struct panel_spec *self, struct sprdfb_device *dev);
	int32_t (*panel_enter_sleep)(struct panel_spec *self, uint8_t is_sleep);
	int32_t (*panel_after_suspend)(struct panel_spec *self);
	int32_t (*panel_before_resume)(struct panel_spec *self);
	int32_t (*panel_set_contrast)(struct panel_spec *self, uint16_t contrast);
	int32_t (*panel_set_brightness)(struct panel_spec *self,
				uint16_t brightness);
	int32_t (*panel_set_window)(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom);
	int32_t (*panel_invalidate)(struct panel_spec *self);
	int32_t (*panel_invalidate_rect)(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom);
	int32_t (*panel_rotate_invalidate_rect)(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom,
				uint16_t angle);
	int32_t (*panel_set_direction)(struct panel_spec *self, uint16_t direction);
	uint32_t (*panel_readid)(struct panel_spec *self);
	int32_t (*panel_esd_check)(struct panel_spec *self);
	int32_t (*panel_change_fps)(struct panel_spec *self, int fps_level);
#ifdef CONFIG_FB_CABC_LEVEL_CONTROL
	void (*set_cabc) (struct panel_spec *, int);
#endif
};

struct timing_mcu {
	uint16_t rcss; 
	uint16_t rlpw;
	uint16_t rhpw;
	uint16_t wcss;
	uint16_t wlpw;
	uint16_t whpw;
};

struct timing_rgb {
	uint16_t hfp;
	uint16_t hbp;
	uint16_t hsync;
	uint16_t vfp; 
	uint16_t vbp;
	uint16_t vsync;
};

struct ops_mcu {
	int32_t (*send_cmd)(uint32_t cmd);
	int32_t (*send_cmd_data)(uint32_t cmd, uint32_t data);
	int32_t (*send_data)(uint32_t data);
	uint32_t (*read_data)(void);
};

struct ops_i2c {
	int32_t (*i2c_write_8bits)(uint8_t reg, uint8_t val);
	int32_t (*i2c_read_8bits)(uint8_t reg, uint8_t *val);
	int32_t (*i2c_write_16bits)(uint16_t reg, bool reg_is_8bit, uint16_t val, bool val_is_8bit);
	int32_t (*i2c_read_16bits)(uint16_t reg, bool reg_is_8bit, uint16_t *val, bool val_is_8bit);
	int32_t (*i2c_write_burst)(uint8_t* buf, int num);
};

struct ops_spi{
	void (*spi_send_cmd)(uint32_t cmd);
	void (*spi_send_data)(uint32_t data);
	void (*spi_read)(uint32_t *data);
};

struct ops_mipi{
	int32_t (*mipi_set_cmd_mode)(void);
	int32_t (*mipi_set_video_mode)(void);
	int32_t (*mipi_set_lp_mode)(void);
	int32_t (*mipi_set_hs_mode)(void);
	int32_t (*mipi_set_data_lp_mode)(void);
	int32_t (*mipi_set_data_hs_mode)(void);
	int32_t (*mipi_gen_write)(uint8_t *param, uint16_t param_length);
	int32_t (*mipi_gen_read)(uint8_t *param, uint16_t param_length, uint8_t bytes_to_read, uint8_t *read_buffer);
	int32_t (*mipi_dcs_write)(uint8_t *param, uint16_t param_length);
	int32_t (*mipi_dcs_read)(uint8_t command, uint8_t bytes_to_read, uint8_t *read_buffer);
	int32_t (*mipi_force_write)(uint8_t data_type, uint8_t *param, uint16_t param_length);				
	int32_t (*mipi_force_read)(uint8_t command, uint8_t bytes_to_read, uint8_t *read_buffer);
	int32_t (*mipi_eotp_set)(uint8_t rx_en, uint8_t tx_en);
};

struct i2c_info{
	uint32_t i2c_addr;
	struct ops_i2c *ops;
};

struct spi_info{
	struct ops_spi *ops;
};


struct info_mipi {
	uint16_t work_mode; 
	uint16_t video_bus_width;
	uint32_t lan_number;
	uint32_t phy_feq;  
	uint16_t h_sync_pol;
	uint16_t v_sync_pol;
	uint16_t de_pol;
	uint16_t te_pol; 
	uint16_t color_mode_pol;
	uint16_t shut_down_pol;
	struct timing_rgb *timing;
	struct ops_mipi *ops;
};

struct info_rgb {
	uint16_t cmd_bus_mode; 
	uint16_t video_bus_width;
	uint16_t h_sync_pol;
	uint16_t v_sync_pol;
	uint16_t de_pol;
	struct timing_rgb *timing;
	union{
		struct i2c_info *i2c;
		struct spi_info *spi;
	}bus_info;
};

struct info_mcu {
	uint16_t bus_mode; 
	uint16_t bus_width;
	uint16_t bpp;
	uint16_t te_pol;
	uint32_t te_sync_delay;
	struct timing_mcu *timing;
	struct ops_mcu *ops;
};

struct panel_if_ctrl{
	const char *if_name;

	int32_t (*panel_if_check)(struct panel_spec *self);
	void (*panel_if_mount)(struct sprdfb_device *dev);
	bool (*panel_if_init)(struct sprdfb_device *dev);
	void (*panel_if_ready)(struct sprdfb_device *dev);
	void (*panel_if_uninit)(struct sprdfb_device *dev);
	void (*panel_if_before_refresh)(struct sprdfb_device *dev);
	void (*panel_if_after_refresh)(struct sprdfb_device *dev);
	void (*panel_if_before_panel_reset)(struct sprdfb_device *dev);
	void (*panel_if_enter_ulps)(struct sprdfb_device *dev);
	void (*panel_if_suspend)(struct sprdfb_device *dev);
	void (*panel_if_resume)(struct sprdfb_device *dev);
	uint32_t  (*panel_if_get_status)(struct sprdfb_device *dev);

};

struct panel_spec {
	uint32_t cap;
	uint16_t width;
	uint16_t height;
	uint32_t fps;
	uint16_t type; 
	uint16_t direction;
	bool	 is_clean_lcd;
	union {
		struct info_mcu *mcu;
		struct info_rgb *rgb;
		struct info_mipi * mipi;
	} info;
	struct panel_if_ctrl *if_ctrl;
	struct panel_operations *ops;
};

struct panel_cfg {
	struct list_head list;
	uint32_t dev_id; 
	uint32_t lcd_id;
	const char *lcd_name;
	struct panel_spec *panel;
};

int sprdfb_panel_register(struct panel_cfg *cfg);

void sprdfb_panel_change_fps(struct sprdfb_device *dev, int fps_level);

#endif
