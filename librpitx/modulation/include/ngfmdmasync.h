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

#ifndef DEF_NGFMDMASYNC
#define DEF_NGFMDMASYNC

#include <stdint.h>
#include <stdbool.h>

#include "dma.h"
#include "gpio.h"

struct ngfmdmasync {
     uint64_t tunefreq;
         bool syncwithpwm;
     uint32_t sample_rate;

    clkgpio_t *clkgpio;
    pwmgpio_t *pwmgpio;
    pcmgpio_t *pcmgpio;
};
typedef struct ngfmdmasync ngfmdmasync_t;

void ngfmdmasync_init(ngfmdmasync_t **ngfm,uint64_t tune_frequency, uint32_t sr, int channel, uint32_t fifo_size, bool use_pwm);
void ngfmdmasync_deinit(ngfmdmasync_t **ngfm);
void ngfmdmasync_set_dma_algo(ngfmdmasync_t **ngfm);
void ngfmdmasync_set_phase(ngfmdmasync_t **ngfm, bool inversed);
void ngfmdmasync_set_frequency_sample(ngfmdmasync_t **ngfm,uint32_t index, float frequency);
void ngfmdmasync_set_frequency_samples(ngfmdmasync_t **ngfm,float *sample, size_t size);

#endif
