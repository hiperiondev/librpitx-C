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

#include <stdio.h>
#include <stdlib.h>

#include "gpio.h"
#include "gpio_enum.h"
#include "dma.h"
#include "serialdmasync.h"

void serialdmasync_Cserialdmasync(serialdmasync_t **serialdmas, uint32_t SampleRate, int Channel, uint32_t FifoSize, bool dualoutput) {
    *serialdmas = (serialdmasync_t*) malloc(sizeof(struct serialdmasync));
       bufferdma_Cbufferdma(Channel, FifoSize, 1, 1);
       clkgpio_Cclkgpio(&((*serialdmas)->clkgpio));
       pwmgpio_Cpwmgpio(&((*serialdmas)->pwmgpio));

    if (dualoutput) //Fixme if 2pin we want maybe 2*SRATE as it is distributed over 2 pin
    {
        pwmgpio_SetMode(&((*serialdmas)->pwmgpio), pwm2pin);
        SampleRate *= 2;
    } else {
        pwmgpio_SetMode(&((*serialdmas)->pwmgpio), pwm1pin);
    }

    if (SampleRate > 250000) {
        pwmgpio_SetPllNumber(&((*serialdmas)->pwmgpio), clk_plld, 1);
        pwmgpio_SetFrequency(&((*serialdmas)->pwmgpio), SampleRate);
    } else {
        pwmgpio_SetPllNumber(&((*serialdmas)->pwmgpio), clk_osc, 1);
        pwmgpio_SetFrequency(&((*serialdmas)->pwmgpio), SampleRate);
    }

    pwmgpio_enablepwm(&((*serialdmas)->pwmgpio), 12, 0); // By default PWM on GPIO 12/pin 32
    pwmgpio_enablepwm(&((*serialdmas)->pwmgpio), 13, 0); // By default PWM on GPIO 13/pin 33

    serialdmasync_SetDmaAlgo(serialdmas);

}

void serialdmasync_Dserialdmasync(serialdmasync_t **serialdmas) {
}

void serialdmasync_SetDmaAlgo(serialdmasync_t **serialdmas) {
    dma_cb_t *cbp = cbarray;
    for (uint32_t samplecnt = 0; samplecnt < buffersize; samplecnt++) {
        dma_SetEasyCB(cbp, samplecnt * registerbysample, dma_pwm, 1);
        cbp++;
    }

    cbp--;
    cbp->next = dma_mem_virt_to_phys(cbarray); // We loop to the first CB
    //dbg_printf(1,"Last cbp :  src %x dest %x next %x\n",cbp->src,cbp->dst,cbp->next);
}

void serialdmasync_SetSample(serialdmasync_t **serialdmas, uint32_t Index, int Sample) {
    Index = Index % buffersize;
    sampletab[Index] = Sample;

    bufferdma_PushSample(Index);
}
