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

#ifndef IQDMASYNC_H_
#define IQDMASYNC_H_

#include <stdbool.h>
#include <stddef.h>
#include "gpio.h"
#include "dma.h"
#include "dsp.h"

#define MODE_IQ     0
#define MODE_FREQ_A 1

struct iqdmasync {
     uint64_t tunefreq;
         bool syncwithpwm;
     uint32_t originfsel; //Save the original FSEL GPIO
     uint32_t sample_rate;
          int mode_iq; // = MODE_IQ;

    clkgpio_t *clkgpio;
    pwmgpio_t *pwmgpio;
    pcmgpio_t *pcmgpio;
        dsp_t *dsp;
};
typedef struct iqdmasync iqdmasync_t;

void iqdmasync_init(iqdmasync_t **iqdmas, uint64_t tune_frequency, uint32_t sr, int channel, uint32_t fifo_size, int mode);
void iqdmasync_deinit(iqdmasync_t **iqdmas);
void iqdmasync_set_phase(iqdmasync_t **iqdmas, bool inversed) ;
void iqdmasync_set_dma_algo(iqdmasync_t **iqdmas);
void iqdmasync_set_iq_sample(iqdmasync_t **iqdmas, uint32_t index, float _Complex sample, int harmonic);
void iqdmasync_set_freq_ampl_sample(iqdmasync_t **iqdmas, uint32_t index, float _Complex sample, int harmonic);
void iqdmasync_set_iq_samples(iqdmasync_t **iqdmas, float _Complex *sample, size_t size, int harmonic);
void iqdmasync_set_ppm(iqdmasync_t **iqdmas, double ppm);

#endif /* IQDMASYNC_H_ */
