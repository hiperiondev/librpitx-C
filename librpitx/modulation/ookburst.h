#ifndef MODULATION_OOKBURST_H_
#define MODULATION_OOKBURST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "dma.h"
#include "gpio.h"

struct ookburst {
        float timegranularity; //ns
     uint32_t Originfsel;
         bool syncwithpwm;
     dma_cb_t *lastcbp;
       size_t SR_upsample;
       size_t Ramp;

    clkgpio_t *clkgpio;
    pwmgpio_t *pwmgpio;
    pcmgpio_t *pcmgpio;
};
typedef struct ookburst ookburst_t;

struct ookbursttiming {
    unsigned char *ookrenderbuffer;
           size_t m_MaxMessage;
};
typedef struct ookbursttiming ookbursttiming_t;
typedef struct SampleOOKTiming {
    unsigned char value;
           size_t duration;
} SampleOOKTiming;

void ookburst_Cookburst(ookburst_t **ookbrst, uint64_t TuneFrequency, float SymbolRate, int Channel, uint32_t FifoSize, size_t upsample, float RatioRamp);
void ookburst_Dookburst(ookburst_t **ookburst);
void ookburst_SetDmaAlgo(ookburst_t **ookburst);
void ookburst_SetSymbols(ookburst_t **ookbrst, unsigned char *Symbols, uint32_t Size);

void ookbursttiming_Cookbursttiming(ookbursttiming_t **ookbursttm, ookburst_t **ookbrst, uint64_t TuneFrequency, size_t MaxMessageDuration);
void ookbursttiming_Dookbursttiming(ookbursttiming_t **ookbursttm);
void ookbursttiming_SendMessage(ookbursttiming_t **ookbursttm, ookburst_t **ookbrst, SampleOOKTiming *TabSymbols, size_t Size);

#ifdef __cplusplus
}
#endif

#endif
