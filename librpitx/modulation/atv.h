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

#ifndef MODULATION_ATV_H_
#define MODULATION_ATV_H_

#include <stdbool.h>

struct atv {
     uint64_t tunefreq;
         bool syncwithpwm;
     uint32_t Originfsel;
     uint32_t SampleRate;

    clkgpio_t *clkgpio;
    pwmgpio_t *pwmgpio;
    pcmgpio_t *pcmgpio;
};
typedef struct atv atv_t;

void atv_Catv(atv_t **atvl, uint64_t TuneFrequency, uint32_t SR, int Channel, uint32_t Lines);
void atv_Datv(atv_t **atvl);
void atv_SetDmaAlgo(atv_t **atvl);
void atv_SetFrame(atv_t **atvl,unsigned char *Luminance, size_t Lines);

#endif
