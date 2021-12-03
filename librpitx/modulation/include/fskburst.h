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

#ifndef MODULATION_FSKBURST_H_
#define MODULATION_FSKBURST_H_

#include <stdint.h>
#include <stdbool.h>

#include "dma.h"
#include "gpio.h"

struct fskburst {
        float freqdeviation;
     uint32_t originfsel;
         bool syncwithpwm;
     dma_cb_t *lastcbp;
       size_t sr_upsample;
       size_t ramp;

    clkgpio_t *clkgpio;
    pwmgpio_t *pwmgpio;
    pcmgpio_t *pcmgpio;
};
typedef struct fskburst fskburst_t;

void fskburst_init(fskburst_t **fskbrst, uint64_t tune_frequency, float symbol_rate, float deviation, int channel, uint32_t fifo_size, size_t upsample, float ratio_ramp);
void fskburst_deinit(fskburst_t **fskbrst);
void fskburst_set_dma_algo(fskburst_t **fskbrst);
void fskburst_set_symbols(fskburst_t **fskbrst, unsigned char *symbols, uint32_t size);

#endif
