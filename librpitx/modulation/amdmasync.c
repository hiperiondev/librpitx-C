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
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sched.h>
#include <stdlib.h>

#include "gpio.h"
#include "amdmasync.h"

void amdmasync_init(amdmasync_t **amdma, uint64_t TuneFrequency, uint32_t SR, int Channel, uint32_t FifoSize) {
    *amdma = (amdmasync_t*) malloc(sizeof(struct amdmasync));
    bufferdma_Cbufferdma(Channel, FifoSize, 3, 2);
    clkgpio_Cclkgpio(&((*amdma)->clkgpio));
    pwmgpio_Cpwmgpio(&((*amdma)->pwmgpio));
    pcmgpio_Cpcmgpio(&((*amdma)->pcmgpio));

    (*amdma)->SampleRate = SR;
    (*amdma)->tunefreq = TuneFrequency;
    clkgpio_SetAdvancedPllMode(&((*amdma)->clkgpio), true);
    clkgpio_SetCenterFrequency(&((*amdma)->clkgpio), TuneFrequency, (*amdma)->SampleRate);
    clkgpio_SetFrequency(&((*amdma)->clkgpio), 0);
    clkgpio_enableclk(&((*amdma)->clkgpio), 4); // GPIO 4 CLK by default
    (*amdma)->syncwithpwm = false;

    if ((*amdma)->syncwithpwm) {
        pwmgpio_SetPllNumber(&((*amdma)->pwmgpio), clk_plld, 1);
        pwmgpio_SetFrequency(&((*amdma)->pwmgpio), (*amdma)->SampleRate);
    } else {
        pcmgpio_SetPllNumber(&((*amdma)->pcmgpio), clk_plld, 1);
        pcmgpio_SetFrequency(&((*amdma)->pcmgpio), (*amdma)->SampleRate);
    }

    padgpio_t *pad;
    padgpio_Cpadgpio(&pad);
    (*amdma)->Originfsel = (*pad).h_gpio->gpioreg[PADS_GPIO_0];
    amdmasync_SetDmaAlgo(amdma);
    padgpio_Dpadgpio(&pad);
}

void amdmasync_deinit(amdmasync_t **amdma) {
    clkgpio_disableclk(&((*amdma)->clkgpio), 4);
    padgpio_t *pad;
    padgpio_Cpadgpio(&pad);
    (*pad).h_gpio->gpioreg[PADS_GPIO_0] = (*amdma)->Originfsel;
    clkgpio_Dclkgpio(&((*amdma)->clkgpio));
    pwmgpio_Dpwmgpio(&((*amdma)->pwmgpio));
    pcmgpio_Dpcmgpio(&((*amdma)->pcmgpio));
    padgpio_Dpadgpio(&pad);
    free(*amdma);
}

void amdmasync_SetDmaAlgo(amdmasync_t **amdma) {
    dma_cb_t *cbp = cbarray;
    for (uint32_t samplecnt = 0; samplecnt < buffersize; samplecnt++) {

        //@0
        //Set Amplitude by writing to PADS
        dma_SetEasyCB(cbp, samplecnt * registerbysample, dma_pad, 1);
        cbp++;

        //@1
        //Set Amplitude  to FSEL for amplitude=0
        dma_SetEasyCB(cbp, samplecnt * registerbysample + 1, dma_fsel, 1);
        cbp++;

        // Delay
        dma_SetEasyCB(cbp, samplecnt * registerbysample, (*amdma)->syncwithpwm ? dma_pwm : dma_pcm, 1);
        cbp++;

    }

    cbp--;
    cbp->next = dma_mem_virt_to_phys(cbarray); // We loop to the first CB
    //dbg_printf(1,"Last cbp :  src %x dest %x next %x\n",cbp->src,cbp->dst,cbp->next);
}

void amdmasync_SetAmSample(amdmasync_t **amdma, uint32_t Index, float Amplitude) //-1;1
{
    Index = Index % buffersize;

    int IntAmplitude = round(abs(Amplitude) * 8.0) - 1;

    int IntAmplitudePAD = IntAmplitude;
    if (IntAmplitudePAD > 7)
        IntAmplitudePAD = 7;
    if (IntAmplitudePAD < 0)
        IntAmplitudePAD = 0;

    //dbg_printf(1,"Amplitude=%f PAD %d\n",Amplitude,IntAmplitudePAD);
    sampletab[Index * registerbysample] = (0x5A << 24) + (IntAmplitudePAD & 0x7) + (1 << 4) + (0 << 3); // Amplitude PAD

    //sampletab[Index*registerbysample+2]=(Originfsel & ~(7 << 12)) | (4 << 12); //Alternate is CLK
    if (IntAmplitude == -1) {
        sampletab[Index * registerbysample + 1] = ((*amdma)->Originfsel & ~(7 << 12)) | (0 << 12); //Pin is in -> Amplitude 0
    } else {
        sampletab[Index * registerbysample + 1] = ((*amdma)->Originfsel & ~(7 << 12)) | (4 << 12); //Alternate is CLK
    }

    bufferdma_PushSample(Index);
}

void amdmasync_SetAmSamples(amdmasync_t **amdma, float *sample, size_t Size) {
    size_t NbWritten = 0;
    int OSGranularity = 100;
    long int start_time;
    long time_difference = 0;
    struct timespec gettime_now;

    while (NbWritten < Size) {
        clock_gettime(CLOCK_REALTIME, &gettime_now);
        start_time = gettime_now.tv_nsec;
        int Available = bufferdma_GetBufferAvailable();
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
        Available = bufferdma_GetBufferAvailable();
        int Index = bufferdma_GetUserMemIndex();
        int ToWrite = ((int) Size - (int) NbWritten) < Available ? Size - NbWritten : Available;

        for (int i = 0; i < ToWrite; i++) {
            amdmasync_SetAmSample(amdma, Index + i, sample[NbWritten++]);
        }
    }
}
