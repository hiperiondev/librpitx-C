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
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sched.h>
#include <stdlib.h>

#include "util.h"
#include "gpio.h"
#include "dma.h"
#include "atv.h"

//#define CB_ATV (6 * 4 + 5 * 4 + 5 * 4 + (304 + 305) * (4 + 52 * 2))
#define CB_ATV 70000

void atv_init(atv_t **atvl, uint64_t TuneFrequency, uint32_t SR, int Channel, uint32_t Lines) {

    *atvl = (atv_t*) malloc(sizeof(struct atv));
    dma_init(Channel, CB_ATV, Lines * 52 + 3);
    clkgpio_init(&((*atvl)->clkgpio));
    pwmgpio_init(&((*atvl)->pwmgpio));
    pcmgpio_init(&((*atvl)->pcmgpio));
// Need 2 more bytes for 0 and 1
// Need 6 CB more for sync, if so as 2CBby sample : 3

    (*atvl)->SampleRate = SR;
    (*atvl)->tunefreq = TuneFrequency;
    clkgpio_SetAdvancedPllMode(&((*atvl)->clkgpio), true);
    clkgpio_SetCenterFrequency(&((*atvl)->clkgpio), TuneFrequency, (*atvl)->SampleRate);
    clkgpio_SetFrequency(&((*atvl)->clkgpio), 0);
    clkgpio_enableclk(&((*atvl)->clkgpio), 4); // GPIO 4 CLK by default
    (*atvl)->syncwithpwm = true;

    if ((*atvl)->syncwithpwm) {
        pwmgpio_SetPllNumber(&((*atvl)->pwmgpio), clk_plld, 1);
        pwmgpio_SetFrequency(&((*atvl)->pwmgpio), (*atvl)->SampleRate);
    } else {
        pcmgpio_SetPllNumber(&((*atvl)->pcmgpio), clk_plld, 1);
        pcmgpio_SetFrequency(&((*atvl)->pcmgpio), (*atvl)->SampleRate);
    }

    padgpio_t *pad = (padgpio_t*) malloc(sizeof(struct padgpio));
    (*atvl)->Originfsel = pad->h_gpio->gpioreg[PADS_GPIO_0];

    usermem[(usermemsize - 2)] = (0x5A << 24) + (1 & 0x7) + (1 << 4) + (0 << 3); // Amp 1
    usermem[(usermemsize - 1)] = (0x5A << 24) + (0 & 0x7) + (1 << 4) + (0 << 3); // Amp 0
    usermem[(usermemsize - 3)] = (0x5A << 24) + (4 & 0x7) + (1 << 4) + (0 << 3); // Amp 4

    atv_SetDmaAlgo(atvl);
    padgpio_deinit(&pad);
}

void atv_deinit(atv_t **atvl) {
    clkgpio_disableclk(&((*atvl)->clkgpio), 4);
    padgpio_t *pad = (padgpio_t*) malloc(sizeof(struct padgpio));
    pad->h_gpio->gpioreg[PADS_GPIO_0] = (*atvl)->Originfsel;

    clkgpio_deinit(&((*atvl)->clkgpio));
    pwmgpio_deinit(&((*atvl)->pwmgpio));
    pcmgpio_deinit(&((*atvl)->pcmgpio));
    padgpio_deinit(&pad);
    free(*atvl);
}

void atv_SetDmaAlgo(atv_t **atvl) {
    dma_cb_t *cbp = cbarray;
    //int LineResolution = 625;

    //uint32_t level0 = mem_virt_to_phys(&usermem[(usermemsize - 1)]);
    //uint32_t level1 = mem_virt_to_phys(&usermem[(usermemsize - 2)]);
    //uint32_t level4 = mem_virt_to_phys(&usermem[(usermemsize - 3)]);

    uint32_t index_level0 = usermemsize - 1;
    uint32_t index_level1 = usermemsize - 2;
    //uint32_t index_level4 = usermemsize - 3;

    int shortsync_0 = 2;
    int shortsync_1 = 30;

    int longsync_0 = 30;
    int longsync_1 = 2;

    int normalsync_0 = 4;
    int normalsync_1 = 6;

    int frontsync_1 = 2;

    for (int frame = 0; frame < 2; frame++) {
        //Preegalisation //6*4*2FrameCB
        for (int i = 0; i < 5 + frame; i++) {
            //2us 0,30us 1
            //@0
            //SYNC preegalisation 2us
            dma_SetEasyCB(cbp++, index_level0, dma_pad, 1);
            dma_SetEasyCB(cbp++, 0, (*atvl)->syncwithpwm ? dma_pwm : dma_pcm, shortsync_0);

            //SYNC preegalisation 30us
            dma_SetEasyCB(cbp++, index_level1, dma_pad, 1);
            dma_SetEasyCB(cbp++, 0, (*atvl)->syncwithpwm ? dma_pwm : dma_pcm, shortsync_1);
        }
        //SYNC top trame 5*4*2frameCB
        for (int i = 0; i < 5; i++) {
            dma_SetEasyCB(cbp++, index_level0, dma_pad, 1);
            dma_SetEasyCB(cbp++, 0, (*atvl)->syncwithpwm ? dma_pwm : dma_pcm, longsync_0);

            dma_SetEasyCB(cbp++, index_level1, dma_pad, 1);
            dma_SetEasyCB(cbp++, 0, (*atvl)->syncwithpwm ? dma_pwm : dma_pcm, longsync_1);
        }
        //postegalisation ; copy paste from preegalisation
        //5*4*2CB
        for (int i = 0; i < 5 + frame; i++) {
            //2us 0,30us 1
            //@0
            //SYNC preegalisation 2us
            dma_SetEasyCB(cbp++, index_level0, dma_pad, 1);
            dma_SetEasyCB(cbp++, 0, (*atvl)->syncwithpwm ? dma_pwm : dma_pcm, shortsync_0);

            //SYNC preegalisation 30us
            dma_SetEasyCB(cbp++, index_level1, dma_pad, 1);
            dma_SetEasyCB(cbp++, 0, (*atvl)->syncwithpwm ? dma_pwm : dma_pcm, shortsync_1);
        }

        //(304+305)*(4+52*2+2)CB
        for (int line = 0; line < 305 /* 317 + frame*/; line++) {

            //@0
            //SYNC 0/ 5us
            dma_SetEasyCB(cbp++, index_level0, dma_pad, 1);
            dma_SetEasyCB(cbp++, 0, (*atvl)->syncwithpwm ? dma_pwm : dma_pcm, normalsync_0);

            //SYNC 1/ 5us
            dma_SetEasyCB(cbp++, index_level1, dma_pad, 1);
            dma_SetEasyCB(cbp++, 0, (*atvl)->syncwithpwm ? dma_pwm : dma_pcm, normalsync_1);

            for (uint32_t samplecnt = 0; samplecnt < 52; samplecnt++) //52 us
                    {
                dma_SetEasyCB(cbp++, samplecnt + line * 52 + frame * 312 * 52, dma_pad, 1);
                dma_SetEasyCB(cbp++, 0, (*atvl)->syncwithpwm ? dma_pwm : dma_pcm, 1);
            }

            //FRONT PORSH
            //SYNC 2us
            dma_SetEasyCB(cbp++, index_level1, dma_pad, 1);
            dma_SetEasyCB(cbp++, 0, (*atvl)->syncwithpwm ? dma_pwm : dma_pcm, frontsync_1);
        }
    }
    cbp--;
    cbp->next = dma_mem_virt_to_phys(cbarray); // We loop to the first CB
    librpitx_dbg_printf(1, "Last cbp :  %d \n", ((uintptr_t) (cbp) - (uintptr_t) (cbarray)) / sizeof(dma_cb_t));
}

void atv_SetFrame(atv_t **atvl, unsigned char *Luminance, size_t Lines) {
    for (size_t i = 0; i < Lines; i++) {
        for (size_t x = 0; x < 52; x++) {
            int AmplitudePAD = (Luminance[i * 52 + x] / 255.0) * 6.0 + 1; //1 to 7

            if (i % 2 == 0)                                                                          // First field
                usermem[i * 52 / 2 + x] = (0x5A << 24) + (AmplitudePAD & 0x7) + (1 << 4) + (0 << 3); // Amplitude PAD
            else
                usermem[(i - 1) * 52 / 2 + x + 52 * 312] = (0x5A << 24) + (AmplitudePAD & 0x7) + (1 << 4) + (0 << 3); // Amplitude PAD
        }
    }
}
