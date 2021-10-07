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

#ifndef MODULATION_OOKBURST_H_
#define MODULATION_OOKBURST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "dma.h"
#include "gpio.h"

struct ookburst {
        float timegranularity; //ns
     uint32_t Originfsel;
         bool syncwithpwm;
     dma_cb_t *lastcbp;
       size_t SR_upsample;
       size_t Ramp;

    clkgpio_t *clkgpio;
    pwmgpio_t *pwmgpio;
    pcmgpio_t *pcmgpio;
};
typedef struct ookburst ookburst_t;

struct ookbursttiming {
    unsigned char *ookrenderbuffer;
           size_t m_MaxMessage;
};
typedef struct ookbursttiming ookbursttiming_t;
typedef struct SampleOOKTiming {
    unsigned char value;
           size_t duration;
} SampleOOKTiming;

void ookburst_init(ookburst_t **ookbrst, uint64_t TuneFrequency, float SymbolRate, int Channel, uint32_t FifoSize, size_t upsample, float RatioRamp);
void ookburst_deinit(ookburst_t **ookburst);
void ookburst_SetDmaAlgo(ookburst_t **ookburst);
void ookburst_SetSymbols(ookburst_t **ookbrst, unsigned char *Symbols, uint32_t Size);

void ookbursttiming_Cookbursttiming(ookbursttiming_t **ookbursttm, ookburst_t **ookbrst, uint64_t TuneFrequency, size_t MaxMessageDuration);
void ookbursttiming_Dookbursttiming(ookbursttiming_t **ookbursttm);
void ookbursttiming_SendMessage(ookbursttiming_t **ookbursttm, ookburst_t **ookbrst, SampleOOKTiming *TabSymbols, size_t Size);

#ifdef __cplusplus
}
#endif

#endif
