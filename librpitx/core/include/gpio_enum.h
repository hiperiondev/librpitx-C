/*
 * Copyright 2021 Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site:  *
 *
 * This is based on other projects:
 *    librpitx (https://github.com/F5OEO/librpitx)
 *        Copyright (C) 2018  Evariste COURJAUD F5OEO
 *
 *    please contact their authors for more information.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef GPIO_ENUM_H_
#define GPIO_ENUM_H_

#include "gpio_registers.h"

enum {
    fsel_input,
    fsel_output,
    fsel_alt5,
    fsel_alt4,
    fsel_alt0,
    fsel_alt1,
    fsel_alt2,
    fsel_alt3
};

//Parent PLL
enum {
    clk_gnd,
    clk_osc,
    clk_debug0,
    clk_debug1,
    clk_plla,
    clk_pllc,
    clk_plld,
    clk_hdmi
};

enum pwmmode {
    pwm1pin,
    pwm2pin,
    pwm1pinrepeat
};

enum dma_common_reg {
    dma_pllc_frac = 0x7E000000 + (PLLC_FRAC << 2)   + CLK_BASE,
    dma_pwm       = 0x7E000000 + (PWM_FIFO << 2)    + PWM_BASE,
    dma_pcm       = 0x7E000000 + (PCM_FIFO_A << 2)  + PCM_BASE,
    dma_fsel      = 0x7E000000 + (GPFSEL0 << 2)     + GENERAL_BASE,
    dma_pad       = 0x7E000000 + (PADS_GPIO_0 << 2) + PADS_GPIO
};
typedef enum dma_common_reg dma_common_reg_t;


#endif /* GPIO_ENUM_H_ */
