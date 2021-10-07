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
#include <unistd.h>
#include <time.h>

#include "util.h"
#include "gpio_enum.h"
#include "gpio.h"
#include "dma.h"
#include "dma_registers.h"
#include "phasedmasync.h"

//Stable tune for this pwm mode is up to 90MHZ
void phasedmasync_init(phasedmasync_t **phasedmas, uint64_t TuneFrequency, uint32_t SampleRateIn, int NumberOfPhase, int Channel, uint32_t FifoSize) {
    *phasedmas = (phasedmasync_t*) malloc(sizeof(struct phasedmasync));
    bufferdma_Cbufferdma(Channel, FifoSize, 2, 1); // Number of phase between 2 and 16
    clkgpio_Cclkgpio(&((*phasedmas)->clkgpio));
    pwmgpio_Cpwmgpio(&((*phasedmas)->pwmgpio));
    pcmgpio_Cpcmgpio(&((*phasedmas)->pcmgpio));
    generalgpio_Cgeneralgpio(&((*phasedmas)->gengpio));

    (*phasedmas)->SampleRate = SampleRateIn;
    pwmgpio_SetMode(&((*phasedmas)->pwmgpio), pwm1pinrepeat);
    pwmgpio_SetPllNumber(&((*phasedmas)->pwmgpio), clk_pllc, 0);

    (*phasedmas)->tunefreq = TuneFrequency * NumberOfPhase;
#define MAX_PWM_RATE 360000000
    if ((*phasedmas)->tunefreq > MAX_PWM_RATE)
        librpitx_dbg_printf(1, "Critical error : Frequency to high > %d\n", MAX_PWM_RATE / NumberOfPhase);
    if ((NumberOfPhase == 2) || (NumberOfPhase == 4) || (NumberOfPhase == 8) || (NumberOfPhase == 16) || (NumberOfPhase == 32))
        (*phasedmas)->NumbPhase = NumberOfPhase;
    else
        librpitx_dbg_printf(1, "PWM critical error: %d is not a legal number of phase\n", NumberOfPhase);
    clkgpio_SetAdvancedPllMode(&((*phasedmas)->clkgpio), true);

    clkgpio_ComputeBestLO(&((*phasedmas)->clkgpio), (*phasedmas)->tunefreq, 0); // compute PWM divider according to MasterPLL clkgpio_PllFixDivider
    double FloatMult = ((double) ((*phasedmas)->tunefreq) * (*phasedmas)->clkgpio->PllFixDivider) / (double) ((*phasedmas)->pwmgpio->h_gpio->XOSC_FREQUENCY);
    uint32_t freqctl = FloatMult * ((double) (1 << 20));
    int IntMultiply = freqctl >> 20; // Need to be calculated to have a center frequency
    freqctl &= 0xFFFFF; // Fractionnal is 20bits
    uint32_t FracMultiply = freqctl & 0xFFFFF;
    clkgpio_SetMasterMultFrac(&((*phasedmas)->clkgpio), IntMultiply, FracMultiply);
    librpitx_dbg_printf(1, "PWM Mult %d Frac %d Div %d\n", IntMultiply, FracMultiply, (*phasedmas)->clkgpio->PllFixDivider);

    (*phasedmas)->pwmgpio->clk->h_gpio->gpioreg[PWMCLK_DIV] = 0x5A000000 | (((*phasedmas)->clkgpio->PllFixDivider) << 12) | (*phasedmas)->pwmgpio->pllnumber; // PWM clock input divider
    usleep(100);

    (*phasedmas)->pwmgpio->clk->h_gpio->gpioreg[PWMCLK_CNTL] = 0x5A000000 | ((*phasedmas)->pwmgpio->Mash << 9) | (((*phasedmas)->clkgpio->PllFixDivider) << 12)
            | (*phasedmas)->pwmgpio->pllnumber | (1 << 4); //4 is START CLK
    usleep(100);
    pwmgpio_SetPrediv(&((*phasedmas)->pwmgpio), NumberOfPhase);	//Originaly 32 but To minimize jitter , we set minimal buffer to repeat

    pwmgpio_enablepwm(&((*phasedmas)->pwmgpio), 12, 0); // By default PWM on GPIO 12/pin 32

    pcmgpio_SetPllNumber(&((*phasedmas)->pcmgpio), clk_plld, 1); // Clk for Samplerate by PCM
    pcmgpio_SetFrequency(&((*phasedmas)->pcmgpio), (*phasedmas)->SampleRate);

    phasedmasync_SetDmaAlgo(phasedmas);

    uint32_t ZeroPhase = 0;
    switch ((*phasedmas)->NumbPhase) {
        case 2:
            ZeroPhase = 0xAAAAAAAA;
            break; //1,0,1,0 1,0,1,0
        case 4:
            ZeroPhase = 0xCCCCCCCC;
            break; //1,1,0,0 //4
        case 8:
            ZeroPhase = 0xF0F0F0F0;
            break; //1,1,1,1,0,0,0,0 //8
        case 16:
            ZeroPhase = 0xFF00FF00;
            break; //1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 //16
        case 32:
            ZeroPhase = 0xFFFF0000;
            break; //1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 //32
        default:
            librpitx_dbg_printf(1, "Zero phase not initialized\n");
            break;
    }

    for (int i = 0; i < (*phasedmas)->NumbPhase; i++) {
        (*phasedmas)->TabPhase[i] = ZeroPhase;
        //dbg_printf(1,"Phase[%d]=%x\n",i,TabPhase[i]);
        ZeroPhase = (ZeroPhase << 1) | (ZeroPhase >> 31);
    }
}

void phasedmasync_deinit(phasedmasync_t **phasedmas) {
    pwmgpio_disablepwm(&((*phasedmas)->pwmgpio), 12);

    clkgpio_Dclkgpio(&((*phasedmas)->clkgpio));
    pwmgpio_Dpwmgpio(&((*phasedmas)->pwmgpio));
    pcmgpio_Dpcmgpio(&((*phasedmas)->pcmgpio));
    bufferdma_Dbufferdma();
    free(*phasedmas);
}

void phasedmasync_SetDmaAlgo(phasedmasync_t **phasedmas) {
    dma_cb_t *cbp = cbarray;
    for (uint32_t samplecnt = 0; samplecnt < buffersize; samplecnt++) {
        cbp->info = BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP;
        cbp->src = dma_mem_virt_to_phys(&usermem[samplecnt * registerbysample]);
        cbp->dst = 0x7E000000 + (PWM_FIFO << 2) + PWM_BASE;
        cbp->length = 4;
        cbp->stride = 0;
        cbp->next = dma_mem_virt_to_phys(cbp + 1);
        //dbg_printf(1,"cbp : sample %x src %x dest %x next %x\n",samplecnt,cbp->src,cbp->dst,cbp->next);
        cbp++;

        cbp->info = BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP | BCM2708_DMA_D_DREQ | BCM2708_DMA_PER_MAP(DREQ_PCM_TX);
        cbp->src = dma_mem_virt_to_phys(&usermem[(samplecnt + 1) * registerbysample]); //mem_virt_to_phys(cbarray); // Data is not important as we use it only to feed the PWM
        cbp->dst = 0x7E000000 + (PCM_FIFO_A << 2) + PCM_BASE;
        cbp->length = 4;
        cbp->stride = 0;
        cbp->next = dma_mem_virt_to_phys(cbp + 1);
        //dbg_printf(1,"cbp : sample %x src %x dest %x next %x\n",samplecnt,cbp->src,cbp->dst,cbp->next);
        cbp++;
    }

    cbp--;
    cbp->next = dma_mem_virt_to_phys(cbarray); // We loop to the first CB
    //dbg_printf(1,"Last cbp :  src %x dest %x next %x\n",cbp->src,cbp->dst,cbp->next);
}

void phasedmasync_SetPhase(phasedmasync_t **phasedmas, uint32_t Index, int Phase) {
    Index = Index % buffersize;
    Phase = (Phase + (*phasedmas)->NumbPhase) % (*phasedmas)->NumbPhase;
    sampletab[Index] = (*phasedmas)->TabPhase[Phase];
    bufferdma_PushSample(Index);

}

void phasedmasync_SetPhaseSamples(phasedmasync_t **phasedmas, int *sample, size_t Size) {
    size_t NbWritten = 0;
    //int OSGranularity = 100;
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
        int TimeToSleep = 1e6 * ((int) buffersize * 3 / 4 - Available) / (float) (*phasedmas)->SampleRate/*-OSGranularity*/; // Sleep for theorically fill 3/4 of Fifo
        if (TimeToSleep > 0) {
            //dbg_printf(1,"buffer size %d Available %d SampleRate %d Sleep %d\n",buffersize,Available,SampleRate,TimeToSleep);
            usleep(TimeToSleep);
        } else {
            //dbg_printf(1,"No Sleep %d\n",TimeToSleep);
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
        for (int i = 0; i < ToWrite; i++) {

            phasedmasync_SetPhase(phasedmas, Index + i, sample[NbWritten++]);
        }
    }
}
