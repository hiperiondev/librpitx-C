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

#ifndef MODULATION_PHASEDMASYNC_H_
#define MODULATION_PHASEDMASYNC_H_

#include <stdint.h>

struct phasedmasync {
         uint64_t tunefreq;
              int NumbPhase; // = 2;
         uint32_t SampleRate;
         uint32_t TabPhase[32]; //32 is Max Phase

        clkgpio_t *clkgpio;
        pwmgpio_t *pwmgpio;
        pcmgpio_t *pcmgpio;
    generalgpio_t *gengpio;
};
typedef struct phasedmasync phasedmasync_t;

void phasedmasync_Cphasedmasync(phasedmasync_t **phasedmas, uint64_t TuneFrequency, uint32_t SampleRateIn, int NumberOfPhase, int Channel, uint32_t FifoSize);
void phasedmasync_Dphasedmasync(phasedmasync_t **phasedmas);
void phasedmasync_SetDmaAlgo(phasedmasync_t **phasedmas);
void phasedmasync_SetPhase(phasedmasync_t **phasedmas, uint32_t Index, int Phase);
void phasedmasync_SetPhaseSamples(phasedmasync_t **phasedmas, int *sample, size_t Size);

#endif
