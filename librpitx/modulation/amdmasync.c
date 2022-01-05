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

#include "gpio.h"
#include "amdmasync.h"
#include "util.h"

void amdmasync_init(amdmasync_t **amdma, uint64_t tune_frequency, uint32_t sr, int channel, uint32_t fifo_size) {
    *amdma = (amdmasync_t*) malloc(sizeof(struct amdmasync));
    bufferdma_init(channel, fifo_size, 3, 2);
    clkgpio_init(&((*amdma)->clkgpio));
    pwmgpio_init(&((*amdma)->pwmgpio));
    pcmgpio_init(&((*amdma)->pcmgpio));

    (*amdma)->SampleRate = sr;
    (*amdma)->tunefreq = tune_frequency;
    clkgpio_set_advanced_pll_mode(&((*amdma)->clkgpio), true);
    clkgpio_set_center_frequency(&((*amdma)->clkgpio), tune_frequency, (*amdma)->SampleRate);
    clkgpio_set_frequency(&((*amdma)->clkgpio), 0);
    clkgpio_enableclk(&((*amdma)->clkgpio), 4); // GPIO 4 CLK by default
    (*amdma)->syncwithpwm = false;

    if ((*amdma)->syncwithpwm) {
        pwmgpio_set_pll_number(&((*amdma)->pwmgpio), clk_plld, 1);
        pwmgpio_set_frequency(&((*amdma)->pwmgpio), (*amdma)->SampleRate);
    } else {
        pcmgpio_set_pll_number(&((*amdma)->pcmgpio), clk_plld, 1);
        pcmgpio_set_frequency(&((*amdma)->pcmgpio), (*amdma)->SampleRate);
    }

    padgpio_t *pad;
    padgpio_init(&pad);
    (*amdma)->Originfsel = (*pad).h_gpio->gpioreg[PADS_GPIO_0];
    amdmasync_set_dma_algo(amdma);
    padgpio_deinit(&pad);
}

void amdmasync_deinit(amdmasync_t **amdma) {
    clkgpio_disableclk(&((*amdma)->clkgpio), 4);
    padgpio_t *pad;
    padgpio_init(&pad);
    (*pad).h_gpio->gpioreg[PADS_GPIO_0] = (*amdma)->Originfsel;
    clkgpio_deinit(&((*amdma)->clkgpio));
    pwmgpio_deinit(&((*amdma)->pwmgpio));
    pcmgpio_deinit(&((*amdma)->pcmgpio));
    padgpio_deinit(&pad);

    FREE(*amdma);
}

void amdmasync_set_dma_algo(amdmasync_t **amdma) {
    dma_cb_t *cbp = cbarray;
    for (uint32_t samplecnt = 0; samplecnt < buffersize; samplecnt++) {

        //@0
        //Set Amplitude by writing to PADS
        dma_set_easy_cb(cbp, samplecnt * registerbysample, dma_pad, 1);
        cbp++;

        //@1
        //Set Amplitude  to FSEL for amplitude=0
        dma_set_easy_cb(cbp, samplecnt * registerbysample + 1, dma_fsel, 1);
        cbp++;

        // Delay
        dma_set_easy_cb(cbp, samplecnt * registerbysample, (*amdma)->syncwithpwm ? dma_pwm : dma_pcm, 1);
        cbp++;

    }

    cbp--;
    cbp->next = dma_mem_virt_to_phys(cbarray); // We loop to the first CB
    //dbg_printf(1,"Last cbp :  src %x dest %x next %x\n",cbp->src,cbp->dst,cbp->next);
}

void amdmasync_set_am_sample(amdmasync_t **amdma, uint32_t index, float amplitude) //-1;1
{
    index = index % buffersize;

    int IntAmplitude = round(abs(amplitude) * 8.0) - 1;

    int IntAmplitudePAD = IntAmplitude;
    if (IntAmplitudePAD > 7)
        IntAmplitudePAD = 7;
    if (IntAmplitudePAD < 0)
        IntAmplitudePAD = 0;

    //dbg_printf(1,"Amplitude=%f PAD %d\n",Amplitude,IntAmplitudePAD);
    sampletab[index * registerbysample] = (0x5A << 24) + (IntAmplitudePAD & 0x7) + (1 << 4) + (0 << 3); // Amplitude PAD

    //sampletab[Index*registerbysample+2]=(Originfsel & ~(7 << 12)) | (4 << 12); //Alternate is CLK
    if (IntAmplitude == -1) {
        sampletab[index * registerbysample + 1] = ((*amdma)->Originfsel & ~(7 << 12)) | (0 << 12); //Pin is in -> Amplitude 0
    } else {
        sampletab[index * registerbysample + 1] = ((*amdma)->Originfsel & ~(7 << 12)) | (4 << 12); //Alternate is CLK
    }

    bufferdma_push_sample(index);
}

void amdmasync_set_am_samples(amdmasync_t **amdma, float *sample, size_t size) {
    size_t NbWritten = 0;
    int OSGranularity = 100;
    long int start_time;
    long time_difference = 0;
    struct timespec gettime_now;

    while (NbWritten < size) {
        clock_gettime(CLOCK_REALTIME, &gettime_now);
        start_time = gettime_now.tv_nsec;
        int Available = bufferdma_get_buffer_available();
        int TimeToSleep = 1e6 * ((int) buffersize * 3 / 4 - Available) / (*amdma)->SampleRate - OSGranularity; // Sleep for theorically fill 3/4 of Fifo
        if (TimeToSleep > 0) {
            //dbg_printf(1,"buffer size %d Available %d SampleRate %d Sleep %d\n",buffersize,Available,SampleRate,TimeToSleep);
            usleep(TimeToSleep);
        } else {
            //dbg_printf(1,"No Sleep %d\n",TimeToSleep);
            sched_yield();
        }
        clock_gettime(CLOCK_REALTIME, &gettime_now);
        time_difference = gettime_now.tv_nsec - start_time;
        if (time_difference < 0)
            time_difference += 1E9;
        //dbg_printf(1,"Measure samplerate=%d\n",(int)((GetBufferAvailable()-Available)*1e9/time_difference));
        Available = bufferdma_get_buffer_available();
        int Index = bufferdma_get_user_mem_index();
        int ToWrite = ((int) size - (int) NbWritten) < Available ? size - NbWritten : Available;

        for (int i = 0; i < ToWrite; i++) {
            amdmasync_set_am_sample(amdma, Index + i, sample[NbWritten++]);
        }
    }
}
