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

#include <stdlib.h>
#include <math.h>
#include <complex.h>

#include "dsp.h"
#include "util.h"

void dsp_init(dsp_t **dsp, uint32_t srate) {
	*dsp = (dsp_t*) malloc(sizeof(dsp));
	LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);
	(*dsp)->prev_phase = 0;
	(*dsp)->samplerate = srate;
	(*dsp)->frequency = 0;
	(*dsp)->amplitude = 0;

	LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void dsp_deinit(dsp_t **dsp) {
    FREE(*dsp);
}

#define ln(x) (log(x)/log(2.718281828459045235f))

// Again some functions taken gracefully from F4GKR : https://github.com/f4gkr/RadiantBee

// Normalize to [-180,180):
inline double dsp_constrainAngle(double x) {
	LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__,
	__FILE__, __LINE__);

	x = fmod(x + M_PI, 2 * M_PI);
	if (x < 0)
		x += 2 * M_PI;

	LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

	return x - M_PI;

}

// convert to [-360,360]
inline double dsp_angleConv(double angle) {
	LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__,
	__FILE__, __LINE__);
	LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

	return fmod(dsp_constrainAngle(angle), 2 * M_PI);
}

inline double dsp_angleDiff(double a, double b) {
	LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__,
	__FILE__, __LINE__);

	double dif = fmod(b - a + M_PI, 2 * M_PI);
	if (dif < 0)
		dif += 2 * M_PI;

	LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

	return dif - M_PI;
}

inline double dsp_unwrap(double previous_angle, double new_angle) {
	LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__,
	__FILE__, __LINE__);
	LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

	return previous_angle - dsp_angleDiff(new_angle, dsp_angleConv(previous_angle));
}

int dsp_arctan2(int y, int x) { // Should be replaced with fast_atan2 from rtl_fm
	LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__,
	__FILE__, __LINE__);

	int abs_y = abs(y);
	int angle;
	if ((x == 0) && (y == 0)) {
		LIBRPITX_DBG_PRINTF(2, "< func: %s -a|\n", __func__);
		return 0;
	}
	if (x >= 0) {
		angle = 45 - 45 * (x - abs_y) / ((x + abs_y) == 0 ? 1 : (x + abs_y));
	} else {
		angle = 135 - 45 * (x + abs_y) / ((abs_y - x) == 0 ? 1 : (abs_y - x));
	}

	LIBRPITX_DBG_PRINTF(2, "< func: %s -b|\n", __func__);

	return (y < 0) ? -angle : angle; // negate if in quad III or IV
}

//void dsp_pushsample(std::complex<float> sample) {
void dsp_pushsample(dsp_t **dsp, float _Complex sample) {
	LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

	(*dsp)->amplitude = cabs(sample);

	double phase = atan2(cimag(sample), creal(sample));
	// dbg_printf(1,"phase %f\n",phase);
	phase = dsp_unwrap((*dsp)->prev_phase, phase);

	double dp = phase - (*dsp)->prev_phase;

	(*dsp)->frequency = (dp * (double) (*dsp)->samplerate) / (2.0 * M_PI);
	(*dsp)->prev_phase = phase;

	LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

