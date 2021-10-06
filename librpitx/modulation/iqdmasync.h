/*
 * iqdmasync.h
 *
 *  Created on: 6 oct. 2021
 *      Author: egonzalez
 */

#ifndef IQDMASYNC_H_
#define IQDMASYNC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "gpio.h"
#include "dma.h"
#include "dsp.h"

#define MODE_IQ     0
#define MODE_FREQ_A 1

struct iqdma {
     uint64_t tunefreq;
         bool syncwithpwm;
     uint32_t Originfsel; //Save the original FSEL GPIO
     uint32_t SampleRate;
          int ModeIQ; // = MODE_IQ;

    clkgpio_t *clkgpio;
    pwmgpio_t *pwmgpio;
    pcmgpio_t *pcmgpio;
    dsp_t *dsp;
} iqdma;
typedef struct iqdma iqdma_t;

void iqdmasync_Ciqdmasync(iqdma_t **iqdmas, uint64_t TuneFrequency, uint32_t SR, int Channel, uint32_t FifoSize, int Mode);
void iqdmasync_Diqdmasync(iqdma_t **iqdmas);
void iqdmasync_SetPhase(iqdma_t **iqdmas, bool inversed) ;
void iqdmasync_SetDmaAlgo(iqdma_t **iqdmas);
void iqdmasync_SetIQSample(iqdma_t **iqdmas, uint32_t Index, float _Complex sample, int Harmonic);
void iqdmasync_SetFreqAmplitudeSample(iqdma_t **iqdmas, uint32_t Index, float _Complex sample, int Harmonic);
void iqdmasync_SetIQSamples(iqdma_t **iqdmas, float _Complex *sample, size_t Size, int Harmonic);
void iqdmasync_Setppm(iqdma_t **iqdmas, double ppm);

#ifdef __cplusplus
}
#endif


#endif /* IQDMASYNC_H_ */
