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

#ifndef DEF_AMDMASYNC
#define DEF_AMDMASYNC

#include <stdint.h>
#include <stdbool.h>
#include "dma.h"
#include "gpio.h"

struct amdmasync {
     uint64_t tunefreq;
         bool syncwithpwm;
     uint32_t Originfsel;
     uint32_t SampleRate;

    clkgpio_t *clkgpio;
    pwmgpio_t *pwmgpio;
    pcmgpio_t *pcmgpio;
};
typedef struct amdmasync amdmasync_t;

void amdmasync_init(amdmasync_t **amdma, uint64_t TuneFrequency, uint32_t SR, int Channel, uint32_t FifoSize);
void amdmasync_deinit(amdmasync_t **amdma);
void amdmasync_SetDmaAlgo(amdmasync_t **amdma);

void amdmasync_SetAmSample(amdmasync_t **amdma, uint32_t Index, float Amplitude);
void amdmasync_SetAmSamples(amdmasync_t **amdma, float *sample, size_t Size);

#endif
