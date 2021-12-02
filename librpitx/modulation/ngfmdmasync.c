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
#include <unistd.h>
#include <sched.h>
#include <time.h>

#include "util.h"
#include "ngfmdmasync.h"

void ngfmdmasync_init(ngfmdmasync_t **ngfm, uint64_t TuneFrequency, uint32_t SR, int Channel, uint32_t FifoSize, bool UsePwm) {
    *ngfm = (ngfmdmasync_t*) malloc(sizeof(struct ngfmdmasync));
    bufferdma_init(Channel, FifoSize, 2, 1);
    clkgpio_init(&((*ngfm)->clkgpio));
    pwmgpio_init(&((*ngfm)->pwmgpio));
    pcmgpio_init(&((*ngfm)->pcmgpio));

    bufferdma_init(Channel, FifoSize, 2, 1);

    (*ngfm)->SampleRate = SR;
    (*ngfm)->tunefreq = TuneFrequency;
    clkgpio_SetAdvancedPllMode(&((*ngfm)->clkgpio), true);
    clkgpio_SetCenterFrequency(&((*ngfm)->clkgpio), TuneFrequency, (*ngfm)->SampleRate); // Write Mult Int and Frac : FixMe carrier is already there
    clkgpio_SetFrequency(&((*ngfm)->clkgpio), 0);
    clkgpio_enableclk(&((*ngfm)->clkgpio), 4); // GPIO 4 CLK by default
    (*ngfm)->syncwithpwm = UsePwm;

    if ((*ngfm)->syncwithpwm) {
        pwmgpio_SetPllNumber(&((*ngfm)->pwmgpio), clk_plld, 1);
        pwmgpio_SetFrequency(&((*ngfm)->pwmgpio), (*ngfm)->SampleRate);
    } else {
        pcmgpio_SetPllNumber(&((*ngfm)->pcmgpio), clk_plld, 1);
        pcmgpio_SetFrequency(&((*ngfm)->pcmgpio), (*ngfm)->SampleRate);
    }

    bufferdma_SetDmaAlgo();

    // Note : Spurious are at +/-(19.2MHZ/2^20)*Div*N : (N=1,2,3...) So we need to have a big div to spurious away BUT
    // Spurious are ALSO at +/-(19.2MHZ/2^20)*(2^20-Div)*N
    // Max spurious avoid is to be in the center ! Theory shoud be that spurious are set away at 19.2/2= 9.6Mhz ! But need to get account of div of PLLClock
}

void ngfmdmasync_deinit(ngfmdmasync_t **ngfm) {
    clkgpio_disableclk(&((*ngfm)->clkgpio), 4);

    clkgpio_deinit(&((*ngfm)->clkgpio));
    pwmgpio_deinit(&((*ngfm)->pwmgpio));
    pcmgpio_deinit(&((*ngfm)->pcmgpio));
    free(*ngfm);
}

void ngfmdmasync_SetPhase(ngfmdmasync_t **ngfm, bool inversed) {
    clkgpio_SetPhase(&((*ngfm)->clkgpio), inversed);
}

void ngfmdmasync_SetDmaAlgo(ngfmdmasync_t **ngfm) {
    dma_cb_t *cbp = cbarray;
    for (uint32_t samplecnt = 0; samplecnt < buffersize; samplecnt++) {

        // Write a frequency sample
        dma_SetEasyCB(cbp, samplecnt * registerbysample, dma_pllc_frac, 1);
        cbp++;

        // Delay
        dma_SetEasyCB(cbp, samplecnt * registerbysample, (*ngfm)->syncwithpwm ? dma_pwm : dma_pcm, 1);
        cbp++;
    }

    cbp--;
    cbp->next = dma_mem_virt_to_phys(cbarray); // We loop to the first CB
    //dbg_printf(1,"Last cbp :  src %x dest %x next %x\n",cbp->src,cbp->dst,cbp->next);
}

void ngfmdmasync_SetFrequencySample(ngfmdmasync_t **ngfm, uint32_t Index, float Frequency) {
    Index = Index % buffersize;
    sampletab[Index] = (0x5A << 24) | clkgpio_GetMasterFrac(&((*ngfm)->clkgpio), Frequency);
    //dbg_printf(1,"Frac=%d\n",GetMasterFrac(Frequency));
    bufferdma_PushSample(Index);
}

void ngfmdmasync_SetFrequencySamples(ngfmdmasync_t **ngfm, float *sample, size_t Size) {
    size_t NbWritten = 0;
    int OSGranularity = 200;
    long int start_time;
    long time_difference = 0;
    struct timespec gettime_now;

    while (NbWritten < Size) {
        clock_gettime(CLOCK_REALTIME, &gettime_now);
        start_time = gettime_now.tv_nsec;
        int Available = bufferdma_GetBufferAvailable();
        int TimeToSleep = 1e6 * ((int) buffersize * 3 / 4 - Available) / (*ngfm)->SampleRate - OSGranularity; // Sleep for theorically fill 3/4 of Fifo
        if (TimeToSleep > 0) {
            librpitx_dbg_printf(1, "buffer size %d Available %d SampleRate %d Sleep %d\n", buffersize, Available, (*ngfm)->SampleRate, TimeToSleep);
            usleep(TimeToSleep);
        } else {
            librpitx_dbg_printf(1, "No Sleep %d\n", TimeToSleep);
            sched_yield();
        }
        clock_gettime(CLOCK_REALTIME, &gettime_now);
        time_difference = gettime_now.tv_nsec - start_time;
        if (time_difference < 0)
            time_difference += 1E9;
        int NewAvailable = bufferdma_GetBufferAvailable();
        librpitx_dbg_printf(1, "New available %d Measure samplerate=%d\n", NewAvailable,
                (int) ((bufferdma_GetBufferAvailable() - Available) * 1e9 / time_difference));
        Available = NewAvailable;
        int Index = bufferdma_GetUserMemIndex();
        int ToWrite = ((int) Size - (int) NbWritten) < Available ? Size - NbWritten : Available;

        for (int i = 0; i < ToWrite; i++) {
            ngfmdmasync_SetFrequencySample(ngfm, Index + i, sample[NbWritten++]);
        }
    }
}
