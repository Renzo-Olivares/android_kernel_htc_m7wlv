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

#ifndef _SHARK_GSP_TYPES_H_
#define _SHARK_GSP_TYPES_H_
#ifdef   __cplusplus
extern   "C"
{
#endif
#include <linux/types.h>

#ifndef LOCAL
#define LOCAL static
#endif

#ifndef PUBLIC
#define PUBLIC
#endif

    typedef enum
    {
        GSP_MODULE_LAYER0,
        GSP_MODULE_LAYER1,
        GSP_MODULE_DST,
        GSP_MODULE_ID_MAX,
    }
    GSP_MODULE_ID_E;

    typedef enum _GSP_IRQ_MODE_
    {
        GSP_IRQ_MODE_PULSE = 0x00,
        GSP_IRQ_MODE_LEVEL,
        GSP_IRQ_MODE_LEVEL_INVALID,
    } GSP_IRQ_MODE_E;

    typedef enum _GSP_IRQ_TYPE_
    {
        GSP_IRQ_TYPE_DISABLE = 0x00,
        GSP_IRQ_TYPE_ENABLE,
        GSP_IRQ_TYPE_INVALID,
    } GSP_IRQ_TYPE_E;

    typedef enum _GSP_RNT_TAG_
    {
        GSP_RTN_SUCESS = 0x00,
        GSP_RTN_POINTER_INVALID,
        GSP_RTN_PARAM_INVALID,
    } GSP_RTN_E;

    typedef enum _GSP_ROT_ANGLE_TAG_
    {
        GSP_ROT_ANGLE_0 = 0x00,
        GSP_ROT_ANGLE_90,
        GSP_ROT_ANGLE_180,
        GSP_ROT_ANGLE_270,
        GSP_ROT_ANGLE_0_M,
        GSP_ROT_ANGLE_90_M,
        GSP_ROT_ANGLE_180_M,
        GSP_ROT_ANGLE_270_M,
        GSP_ROT_ANGLE_MAX_NUM,
    } GSP_ROT_ANGLE_E;


    
    typedef enum _GSP_WORD_ENDIAN_TAG_
    {
        GSP_WORD_ENDN_0 = 0x00,     
        GSP_WORD_ENDN_1,            
        GSP_WORD_ENDN_2,            
        GSP_WORD_ENDN_3,            
        GSP_WORD_ENDN_MAX_NUM,
    } GSP_WORD_ENDN_E;

    
    typedef enum _GSP_LONG_WORD_ENDN_TAG_
    {
        GSP_LNG_WRD_ENDN_0,         
        GSP_LNG_WRD_ENDN_1,
        GSP_LNG_WRD_ENDN_MAX,       
    } GSP_LNG_WRD_ENDN_E;

    typedef enum _GSP_RGB_SWAP_MOD_TAG_
    {
        GSP_RGB_SWP_RGB = 0x00,
        GSP_RGB_SWP_RBG,
        GSP_RGB_SWP_GRB,
        GSP_RGB_SWP_GBR,
        GSP_RGB_SWP_BGR,
        GSP_RGB_SWP_BRG,
        GSP_RGB_SWP_MAX,
    } GSP_RGB_SWAP_MOD_E;

    typedef enum _GSP_A_SWAP_MOD_TAG_
    {
        GSP_A_SWAP_ARGB,
        GSP_A_SWAP_RGBA,
        GSP_A_SWAP_MAX,
    } GSP_A_SWAP_MOD_E;

    typedef enum _GSP_DATA_FMT_TAG_
    {
        GSP_FMT_ARGB888 = 0x00,
        GSP_FMT_RGB888,
        GSP_FMT_CMPESS_RGB888,
        GSP_FMT_ARGB565,
        GSP_FMT_RGB565,
        GSP_FMT_YUV420_2P,
        GSP_FMT_YUV420_3P,
        GSP_FMT_YUV400,
        GSP_FMT_YUV422,
        GSP_FMT_8BPP,
        GSP_FMT_PMARGB,
        GSP_FMT_MAX_NUM,
    } GSP_DATA_FMT_E;


    typedef enum _GSP_ERR_CODE_TAG_
    {
   	    
        GSP_NO_ERR = 0,
        GSP_DES_SIZE_ERR = 1,
        GSP_SCL_OUT_RNG_ERR = 2,
        GSP_SCAL_NO_SAME_XY_COOR_ERR = 3,
        GSP_DES_FRMT_ERR0 = 4,
        GSP_LYER0_FRMT_ERR = 5,
        GSP_LYER1_FRMT_ERR = 6,
        GSP_DES_R5_SWAP_ERR = 7,
        GSP_LYER0_R5_SWAP_ERR = 8,
        GSP_LYER1_R5_SWAP_ERR = 9,
        GSP_LYER0_CLP_SIZE_ZERO_ERR = 10,
        GSP_LYER1_CLP_SIZE_ZERO_ERR = 11,
        GSP_DES_PITCH_ZERO_ERR = 12,
        GSP_LYER0_PITCH_ZERO_ERR = 13,
        GSP_LYER1_PITCH_ZERO_ERR = 14,
        GSP_LYER0_CLP_SITUATION_ERR = 15,
        GSP_LYER1_CLP_SITUATION_ERR = 16,
        GSP_LYER0_OUT_SITUATION_ERR = 17,
        GSP_LYER1_OUT_SITUATION_ERR = 18,
        GSP_CMD_NUM_ERR = 19,
        GSP_ALL_MODULE_DISABLE_ERR = 20,
        

		
        GSP_KERNEL_FULL = 0x81,
        GSP_KERNEL_OPEN_INTR = 0x82,
        GSP_KERNEL_CFG_INTR = 0x83,
        GSP_KERNEL_COPY_ERR = 0x84,
        GSP_KERNEL_CALLER_NOT_OWN_HW = 0x85,
        GSP_KERNEL_WORKAROUND_ALLOC_ERR = 0x86,
        GSP_KERNEL_WORKAROUND_WAITDONE_TIMEOUT = 0x87,
        GSP_KERNEL_WORKAROUND_WAITDONE_INTR = 0x88,
        GSP_KERNEL_GEN_OUT_RANG = 0x89,
        GSP_KERNEL_GEN_ALLOC_ERR = 0x8A,
        GSP_KERNEL_GEN_COMMON_ERR = 0x8B,
        GSP_KERNEL_WAITDONE_TIMEOUT = 0x8C,
        GSP_KERNEL_WAITDONE_INTR = 0x8D,
        GSP_KERNEL_FORCE_EXIT = 0x8E,
        GSP_KERNEL_CTL_CMD_ERR = 0x8F,
        

		

		GSP_HAL_PARAM_ERR = 0xA0,
		GSP_HAL_PARAM_CHECK_ERR = 0xA1,
		GSP_HAL_VITUAL_ADDR_NOT_SUPPORT = 0xA2,
		GSP_HAL_ALLOC_ERR = 0xA3,
        

        GSP_ERR_MAX_NUM,
    } GSP_ERR_CODE_E;


    typedef enum _GSP_LAYER_SRC_DATA_FMT_TAG_
    {
        GSP_SRC_FMT_ARGB888 = 0x00,
        GSP_SRC_FMT_RGB888,
        GSP_SRC_FMT_ARGB565,
        GSP_SRC_FMT_RGB565,
        GSP_SRC_FMT_YUV420_2P,
        GSP_SRC_FMT_YUV420_3P,
        GSP_SRC_FMT_YUV400_1P,
        GSP_SRC_FMT_YUV422_2P,
        GSP_SRC_FMT_8BPP,
        GSP_SRC_FMT_MAX_NUM,
    } GSP_LAYER_SRC_DATA_FMT_E;

    typedef enum _GSP_LAYER_DST_DATA_FMT_TAG_
    {
        GSP_DST_FMT_ARGB888 = 0x00,
        GSP_DST_FMT_RGB888,
        GSP_DST_FMT_ARGB565,
        GSP_DST_FMT_RGB565,
        GSP_DST_FMT_YUV420_2P,
        GSP_DST_FMT_YUV420_3P,
        GSP_DST_FMT_YUV422_2P,
        GSP_DST_FMT_MAX_NUM,
    } GSP_LAYER_DST_DATA_FMT_E;

    typedef struct _GSP_ENDIAN_INFO_TAG_
    {
        GSP_LNG_WRD_ENDN_E          y_lng_wrd_endn;
        GSP_WORD_ENDN_E             y_word_endn;
        GSP_LNG_WRD_ENDN_E          uv_lng_wrd_endn;
        GSP_WORD_ENDN_E             uv_word_endn;
        GSP_LNG_WRD_ENDN_E          va_lng_wrd_endn;
        GSP_WORD_ENDN_E             va_word_endn;
        GSP_RGB_SWAP_MOD_E          rgb_swap_mode;
        GSP_A_SWAP_MOD_E            a_swap_mode;
    }
    GSP_ENDIAN_INFO_PARAM_T;

    typedef struct _GSP_RGB_TAG_
    {
        uint8_t b_val;
        uint8_t g_val;
        uint8_t r_val;
        uint8_t a_val;
    }
    GSP_RGB_T;

    typedef struct _GSP_POS_TAG_
    {
        uint16_t pos_pt_x;
        uint16_t pos_pt_y;
    }
    GSP_POS_PT_T;

    typedef struct _GSP_RECT_TAG
    {
        uint16_t st_x;
        uint16_t st_y;
        uint16_t rect_w;
        uint16_t rect_h;
    }
    GSP_RECT_T;

    typedef struct _GSP_DATA_ADDR_TAG
    {
        uint32_t addr_y;
        uint32_t addr_uv;
        uint32_t addr_v;   
    }
    GSP_DATA_ADDR_T;




    typedef struct _GSP_LAYER0_CONFIG_INFO_TAG_
    {
        GSP_DATA_ADDR_T             src_addr;
        uint32_t                    pitch;
        GSP_RECT_T                      clip_rect;
        GSP_RECT_T                      des_rect;
        GSP_RGB_T                       grey;
        GSP_RGB_T                       colorkey;
        GSP_ENDIAN_INFO_PARAM_T     endian_mode;
        GSP_LAYER_SRC_DATA_FMT_E        img_format;
        GSP_ROT_ANGLE_E             rot_angle;
        uint8_t                         row_tap_mode;
        uint8_t                         col_tap_mode;
        uint8_t                         alpha;
        uint8_t                        colorkey_en;
        uint8_t                        pallet_en;
        uint8_t                        scaling_en;
        uint8_t                        layer_en;
        uint8_t                        pmargb_en;
        uint8_t                        pmargb_mod;
    }
    GSP_LAYER0_CONFIG_INFO_T;

    typedef struct _GSP_LAYER1_CONFIG_INFO_TAG_
    {
        GSP_DATA_ADDR_T             src_addr;
        uint32_t                            pitch;
        GSP_RECT_T                      clip_rect;
        GSP_POS_PT_T                    des_pos;
        GSP_RGB_T                       grey;
        GSP_RGB_T                       colorkey;
        GSP_ENDIAN_INFO_PARAM_T     endian_mode;
        GSP_LAYER_SRC_DATA_FMT_E        img_format;
        GSP_ROT_ANGLE_E             rot_angle;
        uint8_t                         row_tap_mode;
        uint8_t                         col_tap_mode;
        uint8_t                         alpha;
        uint8_t                        colorkey_en;
        uint8_t                        pallet_en;
        uint8_t                        layer_en;
        uint8_t                        pmargb_en;
        uint8_t                        pmargb_mod;
    }
    GSP_LAYER1_CONFIG_INFO_T;

    typedef struct _GSP_LAYER_DST_CONFIG_INFO_TAG_
    {
        GSP_DATA_ADDR_T             src_addr;
        uint32_t                            pitch;
        GSP_ENDIAN_INFO_PARAM_T     endian_mode;
        GSP_LAYER_DST_DATA_FMT_E        img_format;
        uint8_t                        compress_r8_en;
        
    }
    GSP_LAYER_DES_CONFIG_INFO_T;

    typedef struct _GSP_MISC_CONFIG_INFO_TAG_
    {
        uint8_t                        dithering_en;
		uint8_t                        gsp_gap;
		uint8_t                        gsp_clock;
		uint8_t                        ahb_clock;
    }
    GSP_MISC_CONFIG_INFO_T;


    typedef struct _GSP_CONFIG_INFO_TAG_
    {
        GSP_MISC_CONFIG_INFO_T          misc_info;
        GSP_LAYER0_CONFIG_INFO_T        layer0_info;
        GSP_LAYER1_CONFIG_INFO_T        layer1_info;
        GSP_LAYER_DES_CONFIG_INFO_T layer_des_info;
    }
    GSP_CONFIG_INFO_T;

    enum gsp_ioctl_id
    {
        GSP_SET_PARAM = 0,
        GSP_TRIGGER_RUN,
        GSP_WAIT_FINISH,
    };

#define GSP_IO_MAGIC                'G'
#define GSP_IO_SET_PARAM            _IOW(GSP_IO_MAGIC, GSP_SET_PARAM,GSP_CONFIG_INFO_T)
#define GSP_IO_TRIGGER_RUN          _IO(GSP_IO_MAGIC, GSP_TRIGGER_RUN)
#define GSP_IO_WAIT_FINISH          _IO(GSP_IO_MAGIC, GSP_WAIT_FINISH)

#ifndef CEIL
#define CEIL(x,y)   ({uint32_t __x = (x),__y = (y);(__x + __y -1)/__y;})
#endif

    typedef struct _GSP_REG_T_TAG_
    {
        union _GSP_CFG_TAG 
        {
            struct _GSP_CFG_MAP
            {
                volatile uint32_t gsp_run               :1;
                volatile uint32_t gsp_busy          :1;
                volatile uint32_t error_flag            :1;
                volatile uint32_t error_code            :5;
                volatile uint32_t dither_en         :1;
                volatile uint32_t pmargb_mod0           :1;
                volatile uint32_t pmargb_mod1           :1;
                volatile uint32_t pmargb_en         :1;
                volatile uint32_t scale_en          :1;
                volatile uint32_t reserved2         :2;
                volatile uint32_t scale_status_clr    :1;
                volatile uint32_t l0_en             :1;
                volatile uint32_t l1_en             :1;
                volatile uint32_t reserved3         :6;
                volatile uint32_t dist_rb           :8;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_cfg_u;

        union _GSP_INT_CFG_TAG
        {
            struct _GSP_INT_CFG_MAP
            {
                volatile uint32_t int_en            :1;
                volatile uint32_t reserved1     :7;
                volatile uint32_t int_mod           :1;
                volatile uint32_t reserved2     :7;
                volatile uint32_t int_clr           :1;
                volatile uint32_t reserved3     :15;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_int_cfg_u;

        union _GSP_CMD_ADDR_TAG
        {
            struct _GSP_CMD_ADDR_MAP
            {
                volatile uint32_t cmd_base_addr     :32;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_cmd_addr_u;

        union _GSP_CMD_CFG_TAG
        {
            struct _GSP_CMD_CFG_MAP
            {
                volatile uint32_t cmd_num           :16;
                volatile uint32_t cmd_en            :1;
                volatile uint32_t reserved1     :7;
                volatile uint32_t cmd_endian_mod    :3;
                volatile uint32_t reserved2     :5;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_cmd_cfg_u;

        union _GSP_DES_DATA_CFG_TAG
        {
            struct _GSP_DES_DATA_CFG_MAP
            {
                volatile uint32_t des_img_format    :3;
                volatile uint32_t reserved1     :1;
                volatile uint32_t compress_r8       :1;
                volatile uint32_t reserved2     :27;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_des_data_cfg_u;

        union _GSP_DES_Y_ADDR_TAG
        {
            struct _GSP_DES_Y_ADDR_MAP
            {
                volatile uint32_t des_y_base_addr       :32;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_des_y_addr_u;

        union _GSP_DES_UV_ADDR_TAG
        {
            struct _GSP_DES_UV_ADDR_MAP
            {
                volatile uint32_t des_uv_base_addr   :32;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_des_uv_addr_u;

        union _GSP_DES_V_ADDR_TAG
        {
            struct _GSP_DES_V_ADDR_MAP
            {
                volatile uint32_t des_v_base_addr        :32;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_des_v_addr_u;

        union _GSP_DES_PITCH_TAG
        {
            struct _GSP_DES_PITCH_MAP
            {
                volatile uint32_t des_pitch         :12;
                volatile uint32_t reserved          :20;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_des_pitch_u;

        union _GSP_DES_DATA_ENDIAN_TAG
        {
            struct _GSP_DES_DATA_ENDIAN_MAP
            {
                volatile uint32_t y_endian_mod      :3;
                volatile uint32_t uv_endian_mod     :3;
                volatile uint32_t v_endian_mod      :3;
                volatile uint32_t rgb_swap_mod      :3;
                volatile uint32_t a_swap_mod            :1;
                volatile uint32_t reserved_unpred       :19;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_des_data_endian_u;

        union _GSP_DES_SIZE_TAG
        {
            struct _GSP_DES_SIZE_MAP
            {
                volatile uint32_t des_size_x_l0     :12;
                volatile uint32_t reserved0         :4;
                volatile uint32_t des_size_y_l0     :12;
                volatile uint32_t reserved1         :4;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_des_size_u;

        volatile uint32_t resevered0;

        union _GSP_LAYER0_CFG_TAG
        {
            struct _GSP_LAYER0_CFG_MAP
            {
                volatile uint32_t img_format_l0     :4;
                volatile uint32_t reserved0         :12;
                volatile uint32_t rot_mod_l0            :3;
                volatile uint32_t ck_en_l0          :1;
                volatile uint32_t pallet_en_l0      :1;
                volatile uint32_t reserved1         :3;
                volatile uint32_t row_tap_mod       :2;
                volatile uint32_t col_tap_mod       :2;
                volatile uint32_t reserved2         :4;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_cfg_u;

        union _GSP_LAYER0_Y_ADDR_TAG
        {
            struct _GSP_LAYER0_Y_ADDR_MAP
            {
                volatile uint32_t y_base_addr_l0        :32;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_y_addr_u;

        union _GSP_LAYER0_UV_ADDR_TAG
        {
            struct _GSP_LAYER0_UV_ADDR_MAP
            {
                volatile uint32_t uv_base_addr_l0       :32;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_uv_addr_u;

        union _GSP_LAYER0_VA_ADDR_TAG
        {
            struct _GSP_LAYER0_VA_ADDR_MAP
            {
                volatile uint32_t va_base_addr_l0       :32;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_va_addr_u;

        union _GSP_LAYER0_PITCH_TAG
        {
            struct _GSP_LAYER0_PITCH_MAP
            {
                volatile uint32_t pitch0            :12;
                volatile uint32_t reserved      :20;
            }
            mBits;
        } gsp_layer0_pitch_u;

        union _GSP_LAYER0_CLIP_START_TAG
        {
            struct _GSP_LAYER0_CLIP_START_MAP
            {
                volatile uint32_t clip_start_x_l0       :12;
                volatile uint32_t reserved0         :4;
                volatile uint32_t clip_start_y_l0       :12;
                volatile uint32_t reserved1         :4;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_clip_start_u;

        union _GSP_LAYER0_CLIP_SIZE_TAG
        {
            struct _GSP_LAYER0_CLIP_SIZE_MAP
            {
                volatile uint32_t clip_size_x_l0        :12;
                volatile uint32_t reserved0         :4;
                volatile uint32_t clip_size_y_l0        :12;
                volatile uint32_t reserved1         :4;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_clip_size_u;

        union _GSP_LAYER0_DES_START_TAG
        {
            struct _GSP_LAYER0_DES_START_MAP
            {
                volatile uint32_t des_start_x_l0        :12;
                volatile uint32_t reserved0         :4;
                volatile uint32_t des_start_y_l0        :12;
                volatile uint32_t reserved1         :4;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_des_start_u;

        union _GSP_LAYER0_GREY_RGB_TAG
        {
            struct _GSP_LAYER0_GREY_RGB_MAP
            {
                volatile uint32_t layer0_grey_b     :8;
                volatile uint32_t layer0_grey_g     :8;
                volatile uint32_t layer0_grey_r     :8;
                volatile uint32_t reserved          :8;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_grey_rgb_u;

        union _GSP_LAYER0_ENDIAN_TAG
        {
            struct _GSP_LAYER0_ENDIAN_MAP
            {
                volatile uint32_t y_endian_mod_l0       :3;
                volatile uint32_t uv_endian_mod_l0  :3;
                volatile uint32_t va_endian_mod_l0  :3;
                volatile uint32_t rgb_swap_mod_l0       :3;
                volatile uint32_t a_swap_mod_l0     :1;
                volatile uint32_t reserved          :19;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_endian_u;

        union _GSP_LAYER0_ALPHA_TAG
        {
            struct _GSP_LAYER0_ALPHA_MAP
            {
                volatile uint32_t alpha_l0          :8;
                volatile uint32_t reserved          :24;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_alpha_u;

        union _GSP_LAYER0_CK_TAG
        {
            struct _GSP_LAYER0_CK_MAP
            {
                volatile uint32_t ck_b_l0           :8;
                volatile uint32_t ck_g_l0           :8;
                volatile uint32_t ck_r_l0           :8;
                volatile uint32_t reserved      :8;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer0_ck_u;

        union _GSP_LAYER1_CFG_TAG
        {
            struct _GSP_LAYER1_CFG_MAP
            {
                volatile uint32_t img_format_l1     :4;
                volatile uint32_t reserved0         :12;
                volatile uint32_t rot_mod_l1            :3;
                volatile uint32_t ck_en_l1          :1;
                volatile uint32_t pallet_en_l1      :1;
                volatile uint32_t reserved1         :11;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_cfg_u;

        union _GSP_LAYER1_Y_ADDR_TAG
        {
            struct _GSP_LAYER1_Y_ADDR_MAP
            {
                volatile uint32_t y_base_addr_l1        :32;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_y_addr_u;

        union _GSP_LAYER1_UV_ADDR_TAG
        {
            struct _GSP_LAYER1_UV_ADDR_MAP
            {
                volatile uint32_t uv_base_addr_l1          :32;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_uv_addr_u;

        union _GSP_LAYER1_VA_ADDR_TAG
        {
            struct _GSP_LAYER1_VA_ADDR_MAP
            {
                volatile uint32_t va_base_addr_l1          :32;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_va_addr_u;

        union _GSP_LAYER1_PITCH_TAG
        {
            struct _GSP_LAYER1_PITCH_MAP
            {
                volatile uint32_t pitch1                :12;
                volatile uint32_t reserved_unpred       :20;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_pitch_u;

        union _GSP_LAYER1_CLIP_START_TAG
        {
            struct _GSP_LAYER1_CLIP_START_MAP
            {
                volatile uint32_t clip_start_x_l1       :12;
                volatile uint32_t reserved0         :4;
                volatile uint32_t clip_start_y_l1       :12;
                volatile uint32_t reserved1         :4;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_clip_start_u;

        union _GSP_LAYER1_CLIP_SIZE_TAG
        {
            struct _GSP_LAYER1_CLIP_SIZE_MAP
            {
                volatile uint32_t clip_size_x_l1        :12;
                volatile uint32_t reserved0         :4;
                volatile uint32_t clip_size_y_l1        :12;
                volatile uint32_t reserved1         :4;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_clip_size_u;

        union _GSP_LAYER1_DES_START_TAG
        {
            struct _GSP_LAYER1_DES_START_MAP
            {
                volatile uint32_t des_start_x_l1        :12;
                volatile uint32_t reserved0         :4;
                volatile uint32_t des_start_y_l1        :12;
                volatile uint32_t reserved1         :4;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_des_start_u;

        union _GSP_LAYER1_GREY_RGB_TAG
        {
            struct _GSP_LAYER1_GREY_RGB_MAP
            {
                volatile uint32_t grey_b_l1         :8;
                volatile uint32_t grey_g_l1         :8;
                volatile uint32_t grey_r_l1         :8;
                volatile uint32_t reserved          :8;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_grey_rgb_u;

        union _GSP_LAYER1_ENDIAN_TAG
        {
            struct _GSP_LAYER1_ENDIAN_MAP
            {
                volatile uint32_t y_endian_mod_l1       :3;
                volatile uint32_t uv_endian_mod_l1  :3;
                volatile uint32_t va_endian_mod_l1  :3;
                volatile uint32_t rgb_swap_mod_l1       :3;
                volatile uint32_t a_swap_mod_l1     :1;
                volatile uint32_t reserved          :19;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_endian_u;

        union _GSP_LAYER1_ALPHA_TAG
        {
            struct _GSP_LAYER1_ALPHA_MAP
            {
                volatile uint32_t alpha_l1          :8;
                volatile uint32_t reserved          :24;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_alpha_u;

        union _GSP_LAYER1_CK_TAG
        {
            struct _GSP_LAYER1_CK_MAP
            {
                volatile uint32_t ck_b_l1           :8;
                volatile uint32_t ck_g_l1           :8;
                volatile uint32_t ck_r_l1           :8;
                volatile uint32_t reserved      :8;
            }
            mBits;
            volatile uint32_t dwValue;
        } gsp_layer1_ck_u;
    }
    GSP_REG_T;
#ifdef   __cplusplus
}
#endif
#endif
