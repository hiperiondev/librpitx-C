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

#include "util.h"
#include "gpio_enum.h"
#include "gpio.h"
#include "ookburst.h"

void ookburst_init(ookburst_t **ookbrst, uint64_t tune_frequency, float symbol_rate, int channel, uint32_t fifo_size, size_t upsample, float ratio_ramp) {
    *ookbrst = (ookburst_t*) malloc(sizeof(struct ookburst));
    bufferdma_init(channel, fifo_size * upsample + 2, 2, 1);
    clkgpio_init(&((*ookbrst)->clkgpio));
    pwmgpio_init(&((*ookbrst)->pwmgpio));
    pcmgpio_init(&((*ookbrst)->pcmgpio));
    (*ookbrst)->sr_upsample = upsample;
    (*ookbrst)->ramp = 0.0;

    clkgpio_set_advanced_pll_mode(&((*ookbrst)->clkgpio), true);
    clkgpio_set_center_frequency(&((*ookbrst)->clkgpio), tune_frequency, 0); // Bandwidth is 0 because frequency always the same
    clkgpio_set_frequency(&((*ookbrst)->clkgpio), 0);

    (*ookbrst)->syncwithpwm = false;
    (*ookbrst)->ramp = (*ookbrst)->sr_upsample * ratio_ramp; //Ramp time

    if ((*ookbrst)->syncwithpwm) {
        pwmgpio_set_pll_number(&((*ookbrst)->pwmgpio), clk_plld, 1);
        pwmgpio_set_frequency(&((*ookbrst)->pwmgpio), symbol_rate * (float) upsample);
    } else {
        pcmgpio_set_pll_number(&((*ookbrst)->pcmgpio), clk_plld, 1);
        pcmgpio_set_frequency(&((*ookbrst)->pcmgpio), symbol_rate * (float) upsample);
    }

    (*ookbrst)->originfsel = (*ookbrst)->clkgpio->gengpio->h_gpio->gpioreg[GPFSEL0];
    ookburst_set_dma_algo(ookbrst);
}

void ookburst_deinit(ookburst_t **ookburst) {
    clkgpio_deinit(&((*ookburst)->clkgpio));
    pwmgpio_deinit(&((*ookburst)->pwmgpio));
    pcmgpio_deinit(&((*ookburst)->pcmgpio));
    free(*ookburst);
}

void ookburst_set_dma_algo(ookburst_t **ookburst) {
    dma_cb_t *cbp = cbarray;
// We must fill the FIFO (PWM or PCM) to be Synchronized from start
// PWM FIFO = 16
// PCM FIFO = 64
    if ((*ookburst)->syncwithpwm) {
        dma_set_easy_cb(cbp++, 0, dma_pwm, 16 + 1);
    } else {
        dma_set_easy_cb(cbp++, 0, dma_pcm, 64 + 1);
    }

    for (uint32_t samplecnt = 0; samplecnt < buffersize - 2; samplecnt++) {

        //Set Amplitude  to FSEL for amplitude=0
        dma_set_easy_cb(cbp++, samplecnt * registerbysample, dma_fsel, 1);
        // Delay
        dma_set_easy_cb(cbp++, samplecnt * registerbysample, (*ookburst)->syncwithpwm ? dma_pwm : dma_pcm, 1);
    }
    (*ookburst)->lastcbp = cbp;

// Last CBP before stopping : disable output
    sampletab[buffersize * registerbysample - 1] = ((*ookburst)->originfsel & ~(7 << 12)) | (0 << 12); //Disable Clk
    dma_set_easy_cb(cbp, buffersize * registerbysample - 1, dma_fsel, 1);
    cbp->next = 0; // Stop DMA
}
void ookburst_set_symbols(ookburst_t **ookbrst, unsigned char *symbols, uint32_t size) {
    if (size > buffersize - 2) {
        librpitx_dbg_printf(1, "Buffer overflow\n");
        return;
    }

    dma_cb_t *cbp = cbarray;
    cbp++; // Skip the first which is the Fiiling of Fifo

    for (unsigned i = 0; i < size; i++) {
        for (size_t j = 0; j < (*ookbrst)->sr_upsample - (*ookbrst)->ramp; j++) {
            sampletab[i * (*ookbrst)->sr_upsample + j] =
                    (symbols[i] == 0) ? (((*ookbrst)->originfsel & ~(7 << 12)) | (0 << 12)) : (((*ookbrst)->originfsel & ~(7 << 12)) | (4 << 12));
            cbp++; //SKIP FSEL CB
            cbp->next = dma_mem_virt_to_phys(cbp + 1);
            cbp++;
        }
        for (size_t j = 0; j < (*ookbrst)->ramp; j++) {
            if (i < size - 1) {
                sampletab[i * (*ookbrst)->sr_upsample + j + (*ookbrst)->sr_upsample - (*ookbrst)->ramp] =
                        (symbols[i] == 0) ? (((*ookbrst)->originfsel & ~(7 << 12)) | (0 << 12)) : (((*ookbrst)->originfsel & ~(7 << 12)) | (4 << 12));

            } else {
                sampletab[i * (*ookbrst)->sr_upsample + j + (*ookbrst)->sr_upsample - (*ookbrst)->ramp] =
                        (symbols[i] == 0) ? (((*ookbrst)->originfsel & ~(7 << 12)) | (0 << 12)) : (((*ookbrst)->originfsel & ~(7 << 12)) | (4 << 12));
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
void ookbursttiming_init(ookbursttiming_t **ookbursttm, ookburst_t **ookbrst, uint64_t tune_frequency, size_t max_message_duration) {
    *ookbursttm = (ookbursttiming_t*) malloc(sizeof(struct ookbursttiming));
    ookburst_init(ookbrst, tune_frequency, 1e5, 14, max_message_duration / 10, 1, 0.0);
    (*ookbursttm)->m_max_message = max_message_duration;
    (*ookbursttm)->ookrenderbuffer = (unsigned char*) malloc(sizeof(unsigned char) * (*ookbursttm)->m_max_message);
}

void ookbursttiming_deinit(ookbursttiming_t **ookbursttm) {
    if ((*ookbursttm)->ookrenderbuffer != NULL)
        free((*ookbursttm)->ookrenderbuffer);
    free(*ookbursttm);
}

void ookbursttiming_send_message(ookbursttiming_t **ookbursttm, ookburst_t **ookbrst, SampleOOKTiming *tab_symbols, size_t size) {
    size_t n = 0;
    for (size_t i = 0; i < size; i++) {
        for (size_t j = 0; j < tab_symbols[i].duration / 10; j++) {
            (*ookbursttm)->ookrenderbuffer[n++] = tab_symbols[i].value;
            if (n >= (*ookbursttm)->m_max_message) {
                librpitx_dbg_printf(1, "OOK Message too long abort time(%d/%d)\n", n, (*ookbursttm)->m_max_message);
                return;
            }
        }
    }
    ookburst_set_symbols(ookbrst, (*ookbursttm)->ookrenderbuffer, n);
}
