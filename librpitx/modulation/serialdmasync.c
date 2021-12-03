/*
 * Copyright 2021 Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/librpitx-C *
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

#include <stdio.h>
#include <stdlib.h>

#include "gpio.h"
#include "gpio_enum.h"
#include "dma.h"
#include "serialdmasync.h"

void serialdmasync_init(serialdmasync_t **serialdmas, uint32_t sample_rate, int channel, uint32_t fifo_size, bool dualoutput) {
    *serialdmas = (serialdmasync_t*) malloc(sizeof(struct serialdmasync));
    bufferdma_init(channel, fifo_size, 1, 1);
    clkgpio_init(&((*serialdmas)->clkgpio));
    pwmgpio_init(&((*serialdmas)->pwmgpio));

    if (dualoutput) //Fixme if 2pin we want maybe 2*SRATE as it is distributed over 2 pin
    {
        pwmgpio_set_mode(&((*serialdmas)->pwmgpio), pwm2pin);
        sample_rate *= 2;
    } else {
        pwmgpio_set_mode(&((*serialdmas)->pwmgpio), pwm1pin);
    }

    if (sample_rate > 250000) {
        pwmgpio_set_pll_number(&((*serialdmas)->pwmgpio), clk_plld, 1);
        pwmgpio_set_frequency(&((*serialdmas)->pwmgpio), sample_rate);
    } else {
        pwmgpio_set_pll_number(&((*serialdmas)->pwmgpio), clk_osc, 1);
        pwmgpio_set_frequency(&((*serialdmas)->pwmgpio), sample_rate);
    }

    pwmgpio_enablepwm(&((*serialdmas)->pwmgpio), 12, 0); // By default PWM on GPIO 12/pin 32
    pwmgpio_enablepwm(&((*serialdmas)->pwmgpio), 13, 0); // By default PWM on GPIO 13/pin 33

    serialdmasync_set_dma_algo(serialdmas);

}

void serialdmasync_deinit(serialdmasync_t **serialdmas) {
    clkgpio_deinit(&((*serialdmas)->clkgpio));
    pwmgpio_deinit(&((*serialdmas)->pwmgpio));
    bufferdma_deinit();
    free(*serialdmas);
}

void serialdmasync_set_dma_algo(serialdmasync_t **serialdmas) {
    dma_cb_t *cbp = cbarray;
    for (uint32_t samplecnt = 0; samplecnt < buffersize; samplecnt++) {
        dma_set_easy_cb(cbp, samplecnt * registerbysample, dma_pwm, 1);
        cbp++;
    }

    cbp--;
    cbp->next = dma_mem_virt_to_phys(cbarray); // We loop to the first CB
    //dbg_printf(1,"Last cbp :  src %x dest %x next %x\n",cbp->src,cbp->dst,cbp->next);
}

void serialdmasync_set_sample(serialdmasync_t **serialdmas, uint32_t index, int sample) {
    index = index % buffersize;
    sampletab[index] = sample;

    bufferdma_push_sample(index);
}
