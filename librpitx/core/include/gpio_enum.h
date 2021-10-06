/*
 * gpio_enum.h
 *
 *  Created on: 4 oct. 2021
 *      Author: egonzalez
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
