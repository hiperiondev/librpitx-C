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

    void dma_init(int Channel, uint32_t CBSize, uint32_t UserMemSize);
    void dma_deinit(void);
    void dma_GetRpiInfo(void);
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
    void bufferdma_Dbufferdma(void);
    void bufferdma_SetDmaAlgo(void);
     int bufferdma_GetBufferAvailable(void);
     int bufferdma_GetUserMemIndex(void);
     int bufferdma_PushSample(int Index);

#ifdef __cplusplus
}
#endif

#endif /* DMA_H_ */
