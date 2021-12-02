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
#include <complex.h>
#include <time.h>
#include <math.h>

#include "util.h"
#include "gpio_enum.h"
#include "gpio.h"
#include "dma.h"
#include "dsp.h"
#include "iqdmasync.h"

void iqdmasync_init(iqdmasync_t **iqdmas, uint64_t TuneFrequency, uint32_t SR, int Channel, uint32_t FifoSize, int Mode) {
    librpitx_dbg_printf(2, "> func: (iqdmasync_iqdmasync) %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    *iqdmas = (iqdmasync_t*) malloc(sizeof(struct iqdmasync));
    bufferdma_init(Channel, FifoSize, 4, 3);
    clkgpio_init(&((*iqdmas)->clkgpio));
    pwmgpio_init(&((*iqdmas)->pwmgpio));
    pcmgpio_init(&((*iqdmas)->pcmgpio));
// Usermem :
// FRAC frequency
// PAD Amplitude
// FSEL for amplitude 0
    (*iqdmas)->ModeIQ = Mode;
    (*iqdmas)->SampleRate = SR;
    (*iqdmas)->tunefreq = TuneFrequency;
    clkgpio_set_advanced_pll_mode(&((*iqdmas)->clkgpio), true);
    clkgpio_set_center_frequency(&((*iqdmas)->clkgpio), TuneFrequency, (*iqdmas)->SampleRate); // Write Mult Int and Frac : FixMe carrier is already there
    clkgpio_set_frequency(&((*iqdmas)->clkgpio), 0);
    clkgpio_enableclk(&((*iqdmas)->clkgpio), 4);
    (*iqdmas)->syncwithpwm = false;

    if ((*iqdmas)->syncwithpwm) {
        pwmgpio_set_pll_number(&((*iqdmas)->pwmgpio), clk_plld, 1);
        pwmgpio_set_frequency(&((*iqdmas)->pwmgpio), (*iqdmas)->SampleRate);
    } else {
        pcmgpio_set_pll_number(&((*iqdmas)->pcmgpio), clk_plld, 1);
        pcmgpio_set_frequency(&((*iqdmas)->pcmgpio), (*iqdmas)->SampleRate);
    }

    dsp_init(&((*iqdmas)->dsp), (*iqdmas)->SampleRate);

    (*iqdmas)->Originfsel = (*iqdmas)->clkgpio->h_gpio->gpioreg[GPFSEL0];

    iqdmasync_set_dma_algo(iqdmas);

    // Note : Spurious are at +/-(19.2MHZ/2^20)*Div*N : (N=1,2,3...) So we need to have a big div to spurious away BUT
    // Spurious are ALSO at +/-(19.2MHZ/2^20)*(2^20-Div)*N
    // Max spurious avoid is to be in the center ! Theory should be that spurious are set away at 19.2/2= 9.6Mhz ! But need to get account of div of PLLClock
    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

void iqdmasync_deinit(iqdmasync_t **iqdmas) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    (*iqdmas)->clkgpio->h_gpio->gpioreg[GPFSEL0] = (*iqdmas)->Originfsel;
    clkgpio_disableclk(&((*iqdmas)->clkgpio), 4);

    clkgpio_deinit(&((*iqdmas)->clkgpio));
    pwmgpio_deinit(&((*iqdmas)->pwmgpio));
    pcmgpio_deinit(&((*iqdmas)->pcmgpio));
    free(*iqdmas);

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

void iqdmasync_set_phase(iqdmasync_t **iqdmas, bool inversed) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    clkgpio_set_phase(&((*iqdmas)->clkgpio), inversed);
    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

void iqdmasync_set_dma_algo(iqdmasync_t **iqdmas) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    dma_cb_t *cbp = cbarray;
    for (uint32_t samplecnt = 0; samplecnt < buffersize; samplecnt++) {

        dma_set_easy_cb(cbp, samplecnt * registerbysample + 1, dma_pad, 1);
        cbp++;

        // @2 Write a frequency sample : Order of DMA CS influence maximum rate : here 0,2,1 is the best : why !!!!!!
        dma_set_easy_cb(cbp, samplecnt * registerbysample, dma_pllc_frac, 1);
        cbp++;

        // @1
        // Set Amplitude  to FSEL for amplitude=0
        dma_set_easy_cb(cbp, samplecnt * registerbysample + 2, dma_fsel, 1);
        cbp++;

        // @3 Delay
        dma_set_easy_cb(cbp, samplecnt * registerbysample, (*iqdmas)->syncwithpwm ? dma_pwm : dma_pcm, 1);
        // dbg_printf(1,"cbp : sample %x src %x dest %x next %x\n",samplecnt,cbp->src,cbp->dst,cbp->next);
        cbp++;

    }

    cbp--;
    cbp->next = dma_mem_virt_to_phys(cbarray); // We loop to the first CB
    //dbg_printf(1,"Last cbp :  src %x dest %x next %x\n",cbp->src,cbp->dst,cbp->next);
    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

void iqdmasync_set_iq_sample(iqdmasync_t **iqdmas, uint32_t Index, float _Complex sample, int Harmonic) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    Index = Index % buffersize;
    dsp_pushsample(&((*iqdmas)->dsp), sample);
    // if(mydsp.frequency>2250) mydsp.frequency=2250;
    // if(mydsp.frequency<1000) mydsp.frequency=1000;
    sampletab[Index * registerbysample] = (0x5A << 24) | clkgpio_get_master_frac(&((*iqdmas)->clkgpio), (*iqdmas)->dsp->frequency / Harmonic);  //Frequency
    int IntAmplitude = (int) ((*iqdmas)->dsp->amplitude * 8.0) - 1; //Fixme 1e4 seems to work with SSB but should be an issue with classical IQ file

    int IntAmplitudePAD = IntAmplitude;
    if (IntAmplitude > 7)
        IntAmplitudePAD = 7;
    if (IntAmplitude < 0) {
        IntAmplitudePAD = 0;
        IntAmplitude = -1;
    }
    sampletab[Index * registerbysample + 1] = (0x5A << 24) + (IntAmplitudePAD & 0x7) + (1 << 4) + (0 << 3); // Amplitude PAD

    //sampletab[Index*registerbysample+2]=(Originfsel & ~(7 << 12)) | (4 << 12); //Alternate is CLK
    if (IntAmplitude == -1) {
        sampletab[Index * registerbysample + 2] = ((*iqdmas)->Originfsel & ~(7 << 12)) | (0 << 12); //Pin is in -> Amplitude 0
    } else {
        sampletab[Index * registerbysample + 2] = ((*iqdmas)->Originfsel & ~(7 << 12)) | (4 << 12); //Alternate is CLK : Fixme : do not work with clk2
    }

    //dbg_printf(1,"amp%f %d\n",mydsp.amplitude,IntAmplitudePAD);
    bufferdma_PushSample(Index);
    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

void iqdmasync_set_freq_ampl_sample(iqdmasync_t **iqdmas, uint32_t Index, float _Complex sample, int Harmonic) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    Index = Index % buffersize;

    //sampletab[Index * registerbysample] = (0x5A << 24) | clkgpio_GetMasterFrac((*iqdmas)->clkgpio, sample.real() / Harmonic); //Frequency
    //int IntAmplitude = (int) roundf(sample.imag()) - 1; //0->8 become -1->7

    sampletab[Index * registerbysample] = (0x5A << 24) | clkgpio_get_master_frac(&((*iqdmas)->clkgpio), creal(sample) / Harmonic); //Frequency
    int IntAmplitude = (int) roundf(cimag(sample)) - 1; //0->8 become -1->7

    int IntAmplitudePAD = IntAmplitude;
    if (IntAmplitude > 7)
        IntAmplitudePAD = 7;
    if (IntAmplitude < 0) {
        IntAmplitudePAD = 0;
        IntAmplitude = -1;
    }
    sampletab[Index * registerbysample + 1] = (0x5A << 24) + (IntAmplitudePAD & 0x7) + (1 << 4) + (0 << 3); // Amplitude PAD

    //dbg_printf(1,"amp%d PAD %d\n",IntAmplitude,IntAmplitudePAD);

    //sampletab[Index*registerbysample+2]=(Originfsel & ~(7 << 12)) | (4 << 12); //Alternate is CLK
    if (IntAmplitude == -1) {
        sampletab[Index * registerbysample + 2] = ((*iqdmas)->Originfsel & ~(7 << 12)) | (0 << 12); //Pin is in -> Amplitude 0
    } else {
        sampletab[Index * registerbysample + 2] = ((*iqdmas)->Originfsel & ~(7 << 12)) | (4 << 12); //Alternate is CLK : Fixme : do not work with clk2
    }

    //dbg_printf(1,"amp%f %d\n",mydsp.amplitude,IntAmplitudePAD);
    bufferdma_PushSample(Index);
    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

void iqdmasync_set_iq_samples(iqdmasync_t **iqdmas, float _Complex *sample, size_t Size, int Harmonic) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    size_t NbWritten = 0;
    long int start_time;
    long time_difference = 0;
    struct timespec gettime_now;

    int debug = 1;

    while (NbWritten < Size) {
        if (debug > 0) {
            clock_gettime(CLOCK_REALTIME, &gettime_now);
            start_time = gettime_now.tv_nsec;
        }
        int Available = bufferdma_GetBufferAvailable();
        //printf("Available before=%d\n",Available);
        int TimeToSleep = 1e6 * ((int) buffersize * 3 / 4 - Available) / (float) (*iqdmas)->SampleRate/*-OSGranularity*/; // Sleep for theorically fill 3/4 of Fifo
        if (TimeToSleep > 0) {
            //dbg_printf(1,"buffer size %d Available %d SampleRate %d Sleep %d\n",buffersize,Available,SampleRate,TimeToSleep);
            usleep(TimeToSleep);
        } else {
            librpitx_dbg_printf(1, "No Sleep %d\n", TimeToSleep);
            //sched_yield();
        }

        if (debug > 0) {
            clock_gettime(CLOCK_REALTIME, &gettime_now);
            time_difference = gettime_now.tv_nsec - start_time;
            if (time_difference < 0)
                time_difference += 1E9;
            //dbg_printf(1,"Available %d Measure samplerate=%d\n",GetBufferAvailable(),(int)((GetBufferAvailable()-Available)*1e9/time_difference));
            debug--;
        }
        Available = bufferdma_GetBufferAvailable();

        int Index = bufferdma_GetUserMemIndex();
        int ToWrite = ((int) Size - (int) NbWritten) < Available ? Size - NbWritten : Available;
        //printf("Available after=%d Timetosleep %d To Write %d\n",Available,TimeToSleep,ToWrite);
        if ((*iqdmas)->ModeIQ == MODE_IQ) {
            for (int i = 0; i < ToWrite; i++) {
                iqdmasync_set_iq_sample(iqdmas, Index + i, sample[NbWritten++], Harmonic);
            }
        }
        if ((*iqdmas)->ModeIQ == MODE_FREQ_A) {
            for (int i = 0; i < ToWrite; i++) {
                iqdmasync_set_freq_ampl_sample(iqdmas, Index + i, sample[NbWritten++], Harmonic);
            }
        }
    }
    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

void iqdmasync_set_ppm(iqdmasync_t **iqdmas, double ppm) {
    clkgpio_set_ppm(&((*iqdmas)->clkgpio), ppm);
}

