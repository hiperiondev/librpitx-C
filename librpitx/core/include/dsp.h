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

#ifndef DSP_H_
#define DSP_H_

#include <stdint.h>
#include <complex.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dsp {
      double prev_phase;
    uint32_t samplerate;
      double amplitude;
      double frequency;
};
typedef struct dsp dsp_t;

void dsp_init(dsp_t **dsp, uint32_t samplerate);
void dsp_deinit(dsp_t **dsp);
void dsp_pushsample(dsp_t **dsp, float _Complex sample);

#ifdef __cplusplus
}
#endif

#endif /* DSP_H_ */
