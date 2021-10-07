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

#ifndef MODULATION_FSKBURST_H_
#define MODULATION_FSKBURST_H_

#include <stdint.h>
#include <stdbool.h>

#include "dma.h"
#include "gpio.h"

struct fskburst {
        float freqdeviation;
     uint32_t Originfsel;
         bool syncwithpwm;
     dma_cb_t *lastcbp;
       size_t SR_upsample;
       size_t Ramp;

    clkgpio_t *clkgpio;
    pwmgpio_t *pwmgpio;
    pcmgpio_t *pcmgpio;
};
typedef struct fskburst fskburst_t;

void fskburst_Cfskburst(fskburst_t **fskbrst, uint64_t TuneFrequency, float SymbolRate, float Deviation, int Channel, uint32_t FifoSize, size_t upsample, float RatioRamp);
void fskburst_Dfskburst(fskburst_t **fskbrst);
void fskburst_SetDmaAlgo(fskburst_t **fskbrst);
void fskburst_SetSymbols(fskburst_t **fskbrst, unsigned char *Symbols, uint32_t Size);


/*
public:
    fskburst(uint64_t TuneFrequency, float SymbolRate, float Deviation, int Channel, uint32_t FifoSize, size_t upsample = 1, float RatioRamp = 0);
    ~fskburst();
    void SetDmaAlgo();
    void SetSymbols(unsigned char *Symbols, uint32_t Size);
};
*/

#endif
