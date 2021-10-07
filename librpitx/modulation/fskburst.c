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
#include "fskburst.h"

void fskburst_Cfskburst(fskburst_t **fskbrst, uint64_t TuneFrequency, float SymbolRate, float Deviation, int Channel, uint32_t FifoSize, size_t upsample, float RatioRamp) {

    *fskbrst = (fskburst_t*) malloc(sizeof(struct fskburst));
        bufferdma_Cbufferdma(Channel, FifoSize * upsample + 2, 2, 1);
        clkgpio_Cclkgpio(&((*fskbrst)->clkgpio));
        pwmgpio_Cpwmgpio(&((*fskbrst)->pwmgpio));
        pcmgpio_Cpcmgpio(&((*fskbrst)->pcmgpio));
        (*fskbrst)->SR_upsample = upsample;
        (*fskbrst)->Ramp = 0.0;
        (*fskbrst)->freqdeviation = Deviation;




    //bufferdma(Channel, FifoSize * upsample + 3, 2, 1), freqdeviation(Deviation), SR_upsample(upsample)

    clkgpio_SetAdvancedPllMode(&((*fskbrst)->clkgpio), true);
    clkgpio_SetCenterFrequency(&((*fskbrst)->clkgpio), TuneFrequency, Deviation * 10); // Write Mult Int and Frac : FixMe carrier is already there
    clkgpio_SetFrequency(&((*fskbrst)->clkgpio), 0);
    clkgpio_disableclk(&((*fskbrst)->clkgpio), 4);
    (*fskbrst)->syncwithpwm = false;
    (*fskbrst)->Ramp = (*fskbrst)->SR_upsample * RatioRamp; //Ramp time = 10%

    if ((*fskbrst)->syncwithpwm) {
        pwmgpio_SetPllNumber(&((*fskbrst)->pwmgpio), clk_plld, 1);
        pwmgpio_SetFrequency(&((*fskbrst)->pwmgpio), SymbolRate * (float) (*fskbrst)->SR_upsample);
    } else {
        pcmgpio_SetPllNumber(&((*fskbrst)->pcmgpio), clk_plld, 1);
        pcmgpio_SetFrequency(&((*fskbrst)->pcmgpio), SymbolRate * (float) (*fskbrst)->SR_upsample);
    }

    //Should be obligatory place before setdmaalgo
    (*fskbrst)->Originfsel = (*fskbrst)->clkgpio->gengpio->h_gpio->gpioreg[GPFSEL0];
    librpitx_dbg_printf(1, "FSK Origin fsel %x\n", (*fskbrst)->Originfsel);

    fskburst_SetDmaAlgo(fskbrst);
}

void fskburst_Dfskburst(fskburst_t **fskbrst) {

}

void fskburst_SetDmaAlgo(fskburst_t **fskbrst) {

    sampletab[buffersize * registerbysample - 2] = ((*fskbrst)->Originfsel & ~(7 << 12)) | (4 << 12); //Gpio  Clk
    sampletab[buffersize * registerbysample - 1] = ((*fskbrst)->Originfsel & ~(7 << 12)) | (0 << 12); //Gpio  In

    dma_cb_t *cbp = cbarray;
    // We must fill the FIFO (PWM or PCM) to be Synchronized from start
    // PWM FIFO = 16
    // PCM FIFO = 64
    if ((*fskbrst)->syncwithpwm) {
        dma_SetEasyCB(cbp++, 0, dma_pwm, 16 + 1);
    } else {
        dma_SetEasyCB(cbp++, 0, dma_pcm, 64 + 1);
    }

    dma_SetEasyCB(cbp++, buffersize * registerbysample - 2, dma_fsel, 1); //Enable clk

    for (uint32_t samplecnt = 0; samplecnt < buffersize - 2; samplecnt++) {

        // Write a frequency sample
        dma_SetEasyCB(cbp++, samplecnt * registerbysample, dma_pllc_frac, 1); //FReq

        // Delay
        dma_SetEasyCB(cbp++, samplecnt * registerbysample, (*fskbrst)->syncwithpwm ? dma_pwm : dma_pcm, 1);
    }
    (*fskbrst)->lastcbp = cbp;

    dma_SetEasyCB(cbp, buffersize * registerbysample - 1, dma_fsel, 1); //Disable clk

    cbp->next = 0; // Stop DMA

    librpitx_dbg_printf(2, "Last cbp :  src %x dest %x next %x\n", cbp->src, cbp->dst, cbp->next);
}
void fskburst_SetSymbols(fskburst_t **fskbrst, unsigned char *Symbols, uint32_t Size) {
    if (Size > buffersize - 3) {
        librpitx_dbg_printf(1, "Buffer overflow\n");
        return;
    }

    dma_cb_t *cbp = cbarray;
    cbp += 2; // Skip the first 2 CB (initialisation)

    for (unsigned int i = 0; i < Size; i++) {
        for (size_t j = 0; j < (*fskbrst)->SR_upsample - (*fskbrst)->Ramp; j++) {
            sampletab[i * (*fskbrst)->SR_upsample + j] = (0x5A << 24) | clkgpio_GetMasterFrac(&((*fskbrst)->clkgpio), (*fskbrst)->freqdeviation * Symbols[i]);
            cbp++; //SKIP FREQ CB
            cbp->next = dma_mem_virt_to_phys(cbp + 1);
            cbp++;
        }
        for (size_t j = 0; j < (*fskbrst)->Ramp; j++) {
            if (i < Size - 1) {
                sampletab[i * (*fskbrst)->SR_upsample + j + (*fskbrst)->SR_upsample - (*fskbrst)->Ramp] = (0x5A << 24)
                        | clkgpio_GetMasterFrac(&((*fskbrst)->clkgpio), (*fskbrst)->freqdeviation * Symbols[i] + j * ((*fskbrst)->freqdeviation * Symbols[i + 1] - (*fskbrst)->freqdeviation * Symbols[i]) / (float) (*fskbrst)->Ramp);
                librpitx_dbg_printf(2, "Ramp %f ->%f : %d %f\n", (*fskbrst)->freqdeviation * Symbols[i], (*fskbrst)->freqdeviation * Symbols[i + 1], j,
                        (*fskbrst)->freqdeviation * Symbols[i] + j * ((*fskbrst)->freqdeviation * Symbols[i + 1] - (*fskbrst)->freqdeviation * Symbols[i]) / (float) (*fskbrst)->Ramp);
            } else {
                sampletab[i * (*fskbrst)->SR_upsample + j + (*fskbrst)->SR_upsample - (*fskbrst)->Ramp] = (0x5A << 24) | clkgpio_GetMasterFrac(&((*fskbrst)->clkgpio), (*fskbrst)->freqdeviation * Symbols[i]);
            }

            cbp++; //SKIP FREQ CB
            cbp->next = dma_mem_virt_to_phys(cbp + 1);
            cbp++;
        }
    }
    cbp--;
    cbp->next = dma_mem_virt_to_phys((*fskbrst)->lastcbp);

    dma_start();
    while (dma_isrunning()) //Block function : return until sent completely signal
    {
        //dbg_printf(1,"GPIO %x\n",clkgpio_gengpio.gpioreg[GPFSEL0]);
        usleep(100);
    }
    librpitx_dbg_printf(1, "FSK burst end Tx\n", cbp->src, cbp->dst, cbp->next);
    usleep(100); //To be sure last symbol Tx ?
}
