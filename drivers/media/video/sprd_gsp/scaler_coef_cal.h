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

#ifndef SCALER_COEF_CAL_
#define SCALER_COEF_CAL_
#include <linux/types.h>

#ifndef COUNT
#define COUNT 256
#endif

#define GSP_COEFF_BUF_SIZE                              (8 << 10)
#define GSP_COEFF_COEF_SIZE                             (1 << 10)
#define GSP_COEFF_POOL_SIZE                             (6 << 10)



uint8_t GSP_Gen_Block_Ccaler_Coef(uint32_t i_w,
                                  uint32_t i_h,
                                  uint32_t o_w,
                                  uint32_t o_h,                         
                                  uint32_t hor_tap,
                                  uint32_t ver_tap,
                                  uint32_t *coeff_h_ptr,
                                  uint32_t *coeff_v_ptr,
                                  void *temp_buf_ptr,
                                  uint32_t temp_buf_size);

void GSP_Scale_Coef_Tab_Config(uint32_t *p_h_coeff,uint32_t *p_v_coeff);
#endif



