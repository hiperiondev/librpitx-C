/*
 * dsp.h
 *
 *  Created on: 5 oct. 2021
 *      Author: egonzalez
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

void dsp_Cdsp(dsp_t **dsp, uint32_t samplerate);
void dsp_Ddsp(dsp_t **dsp);
void dsp_pushsample(dsp_t **dsp, float _Complex sample);

#ifdef __cplusplus
}
#endif

#endif /* DSP_H_ */
