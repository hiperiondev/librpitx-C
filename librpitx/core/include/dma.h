/*
 * dma.h
 *
 *  Created on: 5 oct. 2021
 *      Author: egonzalez
 */

#ifndef DMA_H_
#define DMA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "gpio_enum.h"
#include "gpio.h"

typedef struct {
    uint32_t info, src, dst, length, stride, next, pad[2];
} dma_cb_t; // 8*4=32 bytes

//class dma
//public:
extern dmagpio_t *dmagpio;
 extern dma_cb_t *cbarray;
 extern uint32_t cbsize;
 extern uint32_t *usermem;
 extern uint32_t usermemsize;
     extern bool Started;

//class bufferdma: public dma
//public:
extern uint32_t buffersize;
extern uint32_t cbbysample;
extern uint32_t registerbysample;
extern uint32_t *sampletab;

void dma_Cdma(int Channel, uint32_t CBSize, uint32_t UserMemSize);
void dma_GetRpiInfo(void);
void dma_Ddma(void);
uint32_t dma_mem_virt_to_phys(volatile void *virt);
uint32_t dma_mem_phys_to_virt(volatile uint32_t phys);
int dma_start(void);
int dma_stop(void);
int dma_getcbposition(void);
bool dma_isrunning(void);
bool dma_isunderflow(void);
bool dma_SetCB(dma_cb_t *cbp, uint32_t dma_flag, uint32_t src, uint32_t dst, uint32_t repeat);
bool dma_SetEasyCB(dma_cb_t *cbp, uint32_t index, dma_common_reg_t dst, uint32_t repeat);

void bufferdma_Cbufferdma(int Channel, uint32_t tbuffersize, uint32_t tcbbysample, uint32_t tregisterbysample);
void bufferdma_SetDmaAlgo(void);
int bufferdma_GetBufferAvailable(void);
int bufferdma_GetUserMemIndex(void);
int bufferdma_PushSample(int Index);

#ifdef __cplusplus
}
#endif

#endif /* DMA_H_ */
