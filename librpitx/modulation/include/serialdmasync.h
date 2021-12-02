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

#ifndef MODULATION_SERIALDMASYNC_H_
#define MODULATION_SERIALDMASYNC_H_

#include <stdint.h>
#include <stdbool.h>

#include "gpio.h"

struct serialdmasync {
     uint64_t tunefreq;
         bool syncwithpwm;

    clkgpio_t *clkgpio;
    pwmgpio_t *pwmgpio;
};
typedef struct serialdmasync serialdmasync_t;

void serialdmasync_init(serialdmasync_t **serialdmas, uint32_t SampleRate, int Channel, uint32_t FifoSize, bool dualoutput);
void serialdmasync_deinit(serialdmasync_t **serialdmas);
void serialdmasync_set_dma_algo(serialdmasync_t **serialdmas);
void serialdmasync_set_sample(serialdmasync_t **serialdmas, uint32_t Index, int Sample);

#endif
