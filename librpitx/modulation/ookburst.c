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

#include "util.h"
#include "gpio_enum.h"
#include "gpio.h"
#include "ookburst.h"

void ookburst_Cookburst(ookburst_t **ookbrst, uint64_t TuneFrequency, float SymbolRate, int Channel, uint32_t FifoSize, size_t upsample, float RatioRamp) {
    *ookbrst = (ookburst_t*) malloc(sizeof(struct ookburst));
    bufferdma_Cbufferdma(Channel, FifoSize * upsample + 2, 2, 1);
    clkgpio_Cclkgpio(&((*ookbrst)->clkgpio));
    pwmgpio_Cpwmgpio(&((*ookbrst)->pwmgpio));
    pcmgpio_Cpcmgpio(&((*ookbrst)->pcmgpio));
    (*ookbrst)->SR_upsample = upsample;
    (*ookbrst)->Ramp = 0.0;

    clkgpio_SetAdvancedPllMode(&((*ookbrst)->clkgpio), true);
    clkgpio_SetCenterFrequency(&((*ookbrst)->clkgpio), TuneFrequency, 0); // Bandwidth is 0 because frequency always the same
    clkgpio_SetFrequency(&((*ookbrst)->clkgpio), 0);

    (*ookbrst)->syncwithpwm = false;
    (*ookbrst)->Ramp = (*ookbrst)->SR_upsample * RatioRamp; //Ramp time

    if ((*ookbrst)->syncwithpwm) {
        pwmgpio_SetPllNumber(&((*ookbrst)->pwmgpio), clk_plld, 1);
        pwmgpio_SetFrequency(&((*ookbrst)->pwmgpio), SymbolRate * (float) upsample);
    } else {
        pcmgpio_SetPllNumber(&((*ookbrst)->pcmgpio), clk_plld, 1);
        pcmgpio_SetFrequency(&((*ookbrst)->pcmgpio), SymbolRate * (float) upsample);
    }

    (*ookbrst)->Originfsel = (*ookbrst)->clkgpio->gengpio->h_gpio->gpioreg[GPFSEL0];
    ookburst_SetDmaAlgo(ookbrst);
}

void ookburst_Dookburst(ookburst_t **ookburst) {
}

void ookburst_SetDmaAlgo(ookburst_t **ookburst) {
    dma_cb_t *cbp = cbarray;
// We must fill the FIFO (PWM or PCM) to be Synchronized from start
// PWM FIFO = 16
// PCM FIFO = 64
    if ((*ookburst)->syncwithpwm) {
        dma_SetEasyCB(cbp++, 0, dma_pwm, 16 + 1);
    } else {
        dma_SetEasyCB(cbp++, 0, dma_pcm, 64 + 1);
    }

    for (uint32_t samplecnt = 0; samplecnt < buffersize - 2; samplecnt++) {

        //Set Amplitude  to FSEL for amplitude=0
        dma_SetEasyCB(cbp++, samplecnt * registerbysample, dma_fsel, 1);
        // Delay
        dma_SetEasyCB(cbp++, samplecnt * registerbysample, (*ookburst)->syncwithpwm ? dma_pwm : dma_pcm, 1);
    }
    (*ookburst)->lastcbp = cbp;

// Last CBP before stopping : disable output
    sampletab[buffersize * registerbysample - 1] = ((*ookburst)->Originfsel & ~(7 << 12)) | (0 << 12); //Disable Clk
    dma_SetEasyCB(cbp, buffersize * registerbysample - 1, dma_fsel, 1);
    cbp->next = 0; // Stop DMA
}
void ookburst_SetSymbols(ookburst_t **ookbrst, unsigned char *Symbols, uint32_t Size) {
    if (Size > buffersize - 2) {
        librpitx_dbg_printf(1, "Buffer overflow\n");
        return;
    }

    dma_cb_t *cbp = cbarray;
    cbp++; // Skip the first which is the Fiiling of Fifo

    for (unsigned i = 0; i < Size; i++) {
        for (size_t j = 0; j < (*ookbrst)->SR_upsample - (*ookbrst)->Ramp; j++) {
            sampletab[i * (*ookbrst)->SR_upsample + j] =
                    (Symbols[i] == 0) ? (((*ookbrst)->Originfsel & ~(7 << 12)) | (0 << 12)) : (((*ookbrst)->Originfsel & ~(7 << 12)) | (4 << 12));
            cbp++; //SKIP FSEL CB
            cbp->next = dma_mem_virt_to_phys(cbp + 1);
            cbp++;
        }
        for (size_t j = 0; j < (*ookbrst)->Ramp; j++) {
            if (i < Size - 1) {
                sampletab[i * (*ookbrst)->SR_upsample + j + (*ookbrst)->SR_upsample - (*ookbrst)->Ramp] =
                        (Symbols[i] == 0) ? (((*ookbrst)->Originfsel & ~(7 << 12)) | (0 << 12)) : (((*ookbrst)->Originfsel & ~(7 << 12)) | (4 << 12));

            } else {
                sampletab[i * (*ookbrst)->SR_upsample + j + (*ookbrst)->SR_upsample - (*ookbrst)->Ramp] =
                        (Symbols[i] == 0) ? (((*ookbrst)->Originfsel & ~(7 << 12)) | (0 << 12)) : (((*ookbrst)->Originfsel & ~(7 << 12)) | (4 << 12));
            }

            cbp++; //SKIP FREQ CB
            cbp->next = dma_mem_virt_to_phys(cbp + 1);
            cbp++;
        }
    }

    cbp--;
    cbp->next = dma_mem_virt_to_phys((*ookbrst)->lastcbp);

    dma_start();

    while (dma_isrunning()) //Block function : return until sent completely signal
    {
        usleep(100);
    }
    usleep(100); //To be sure last symbol Tx ?
}

//****************************** OOK BURST TIMING *****************************************
// SampleRate is set to 0.1MHZ,means 10us granularity, MaxMessageDuration in us
void ookbursttiming_Cookbursttiming(ookbursttiming_t **ookbursttm, ookburst_t **ookbrst, uint64_t TuneFrequency, size_t MaxMessageDuration) {
    ookburst_Cookburst(ookbrst, TuneFrequency, 1e5, 14, MaxMessageDuration / 10, 1, 0.0);
    (*ookbursttm)->m_MaxMessage = MaxMessageDuration;
    (*ookbursttm)->ookrenderbuffer = (unsigned char*) malloc(sizeof(unsigned char) * (*ookbursttm)->m_MaxMessage);
}

void ookbursttiming_Dookbursttiming(ookbursttiming_t **ookbursttm) {
    if ((*ookbursttm)->ookrenderbuffer != NULL)
        free((*ookbursttm)->ookrenderbuffer);
}

void ookbursttiming_SendMessage(ookbursttiming_t **ookbursttm, ookburst_t **ookbrst, SampleOOKTiming *TabSymbols, size_t Size) {
    size_t n = 0;
    for (size_t i = 0; i < Size; i++) {
        for (size_t j = 0; j < TabSymbols[i].duration / 10; j++) {
            (*ookbursttm)->ookrenderbuffer[n++] = TabSymbols[i].value;
            if (n >= (*ookbursttm)->m_MaxMessage) {
                librpitx_dbg_printf(1, "OOK Message too long abort time(%d/%d)\n", n, (*ookbursttm)->m_MaxMessage);
                return;
            }
        }
    }
    ookburst_SetSymbols(ookbrst, (*ookbursttm)->ookrenderbuffer, n);
}
