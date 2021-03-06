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

#include <stdio.h>

#include "util.h"
#include "dma.h"
#include "dma_registers.h"
#include "gpio.h"
#include "fmdmasync.h"

void fmdmasync_init(int channel, uint32_t fifo_size) {
    dma_init(channel, fifo_size * 2, fifo_size);
    fmdmasync_set_dma_algo();
    fmdmasync_fill_memory(12, 1472);
}

void fmdmasync_deinit(void) {
    dma_deinit();
}

void fmdmasync_set_dma_algo(void) {
    dma_cb_t *cbp = cbarray;
    for (uint32_t samplecnt = 0; samplecnt < cbsize / 2; samplecnt++) { //cbsize/2 because we have 2 CB by sample

        // Write a frequency sample

        cbp->info = BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP;
        cbp->src = dma_mem_virt_to_phys(&usermem[samplecnt]);
        cbp->dst = 0x7E000000 + (GPCLK_DIV << 2) + CLK_BASE;
        cbp->length = 4;
        cbp->stride = 0;
        cbp->next = dma_mem_virt_to_phys(cbp + 1);
        //dbg_printf(1,"cbp : sample %x src %x dest %x next %x\n",samplecnt,cbp->src,cbp->dst,cbp->next);
        cbp++;

        // Delay

        cbp->info = /*BCM2708_DMA_SRC_IGNOR  | */BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP | BCM2708_DMA_D_DREQ | BCM2708_DMA_PER_MAP(DREQ_PWM);
        cbp->src = dma_mem_virt_to_phys(cbarray); // Data is not important as we use it only to feed the PWM
        cbp->dst = 0x7E000000 + (PWM_FIFO << 2) + PWM_BASE;
        cbp->length = 4;
        cbp->stride = 0;
        cbp->next = dma_mem_virt_to_phys(cbp + 1);
        //dbg_printf(1,"cbp : sample %x src %x dest %x next %x\n",samplecnt,cbp->src,cbp->dst,cbp->next);
        cbp++;
    }

    cbp--;
    cbp->next = dma_mem_virt_to_phys(cbarray); // We loop to the first CB
    //dbg_printf(1,"Last cbp :  src %x dest %x next %x\n",cbp->src,cbp->dst,cbp->next);
}

void fmdmasync_fill_memory(uint32_t freq_divider, uint32_t freq_fractionnal) {

    for (uint32_t samplecnt = 0; samplecnt < usermemsize; samplecnt++) {
        usermem[samplecnt] = 0x5A000000 | ((freq_divider) << 12) | freq_fractionnal;
        freq_fractionnal = (freq_fractionnal + 1) % 4096;
        if (freq_fractionnal == 0)
            freq_divider++;
    }
}
