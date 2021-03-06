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

void ngfmdmasync_init(ngfmdmasync_t **ngfm, uint64_t tune_frequency, uint32_t sr, int channel, uint32_t fifo_size, bool use_pwm) {
    *ngfm = (ngfmdmasync_t*) malloc(sizeof(struct ngfmdmasync));
    bufferdma_init(channel, fifo_size, 2, 1);
    clkgpio_init(&((*ngfm)->clkgpio));
    pwmgpio_init(&((*ngfm)->pwmgpio));
    pcmgpio_init(&((*ngfm)->pcmgpio));

    bufferdma_init(channel, fifo_size, 2, 1);

    (*ngfm)->sample_rate = sr;
    (*ngfm)->tunefreq = tune_frequency;
    clkgpio_set_advanced_pll_mode(&((*ngfm)->clkgpio), true);
    clkgpio_set_center_frequency(&((*ngfm)->clkgpio), tune_frequency, (*ngfm)->sample_rate); // Write Mult Int and Frac : FixMe carrier is already there
    clkgpio_set_frequency(&((*ngfm)->clkgpio), 0);
    clkgpio_enableclk(&((*ngfm)->clkgpio), 4); // GPIO 4 CLK by default
    (*ngfm)->syncwithpwm = use_pwm;

    if ((*ngfm)->syncwithpwm) {
        pwmgpio_set_pll_number(&((*ngfm)->pwmgpio), clk_plld, 1);
        pwmgpio_set_frequency(&((*ngfm)->pwmgpio), (*ngfm)->sample_rate);
    } else {
        pcmgpio_set_pll_number(&((*ngfm)->pcmgpio), clk_plld, 1);
        pcmgpio_set_frequency(&((*ngfm)->pcmgpio), (*ngfm)->sample_rate);
    }

    bufferdma_set_dma_algo();

    // Note : Spurious are at +/-(19.2MHZ/2^20)*Div*N : (N=1,2,3...) So we need to have a big div to spurious away BUT
    // Spurious are ALSO at +/-(19.2MHZ/2^20)*(2^20-Div)*N
    // Max spurious avoid is to be in the center ! Theory shoud be that spurious are set away at 19.2/2= 9.6Mhz ! But need to get account of div of PLLClock
}

void ngfmdmasync_deinit(ngfmdmasync_t **ngfm) {
    clkgpio_disableclk(&((*ngfm)->clkgpio), 4);

    clkgpio_deinit(&((*ngfm)->clkgpio));
    pwmgpio_deinit(&((*ngfm)->pwmgpio));
    pcmgpio_deinit(&((*ngfm)->pcmgpio));

    FREE(*ngfm);
}

void ngfmdmasync_set_phase(ngfmdmasync_t **ngfm, bool inversed) {
    clkgpio_set_phase(&((*ngfm)->clkgpio), inversed);
}

void ngfmdmasync_set_dma_algo(ngfmdmasync_t **ngfm) {
    dma_cb_t *cbp = cbarray;
    for (uint32_t samplecnt = 0; samplecnt < buffersize; samplecnt++) {

        // Write a frequency sample
        dma_set_easy_cb(cbp, samplecnt * registerbysample, dma_pllc_frac, 1);
        cbp++;

        // Delay
        dma_set_easy_cb(cbp, samplecnt * registerbysample, (*ngfm)->syncwithpwm ? dma_pwm : dma_pcm, 1);
        cbp++;
    }

    cbp--;
    cbp->next = dma_mem_virt_to_phys(cbarray); // We loop to the first CB
    //dbg_printf(1,"Last cbp :  src %x dest %x next %x\n",cbp->src,cbp->dst,cbp->next);
}

void ngfmdmasync_set_frequency_sample(ngfmdmasync_t **ngfm, uint32_t index, float frequency) {
    index = index % buffersize;
    sampletab[index] = (0x5A << 24) | clkgpio_get_master_frac(&((*ngfm)->clkgpio), frequency);
    //dbg_printf(1,"Frac=%d\n",GetMasterFrac(Frequency));
    bufferdma_push_sample(index);
}

void ngfmdmasync_set_frequency_samples(ngfmdmasync_t **ngfm, float *sample, size_t size) {
    size_t NbWritten = 0;
    int OSGranularity = 200;
    long int start_time;
    long time_difference = 0;
    struct timespec gettime_now;

    while (NbWritten < size) {
        clock_gettime(CLOCK_REALTIME, &gettime_now);
        start_time = gettime_now.tv_nsec;
        int Available = bufferdma_get_buffer_available();
        int TimeToSleep = 1e6 * ((int) buffersize * 3 / 4 - Available) / (*ngfm)->sample_rate - OSGranularity; // Sleep for theorically fill 3/4 of Fifo
        if (TimeToSleep > 0) {
            LIBRPITX_DBG_PRINTF(1, "buffer size %d Available %d SampleRate %d Sleep %d\n", buffersize, Available, (*ngfm)->sample_rate, TimeToSleep);
            usleep(TimeToSleep);
        } else {
            LIBRPITX_DBG_PRINTF(1, "No Sleep %d\n", TimeToSleep);
            sched_yield();
        }
        clock_gettime(CLOCK_REALTIME, &gettime_now);
        time_difference = gettime_now.tv_nsec - start_time;
        if (time_difference < 0)
            time_difference += 1E9;
        int NewAvailable = bufferdma_get_buffer_available();
        LIBRPITX_DBG_PRINTF(1, "New available %d Measure samplerate=%d\n", NewAvailable,
                (int) ((bufferdma_get_buffer_available() - Available) * 1e9 / time_difference));
        Available = NewAvailable;
        int Index = bufferdma_get_user_mem_index();
        int ToWrite = ((int) size - (int) NbWritten) < Available ? size - NbWritten : Available;

        for (int i = 0; i < ToWrite; i++) {
            ngfmdmasync_set_frequency_sample(ngfm, Index + i, sample[NbWritten++]);
        }
    }
}
