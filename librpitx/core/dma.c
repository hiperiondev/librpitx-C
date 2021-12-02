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
#include <unistd.h>
#include <stdbool.h>

#include "gpio_enum.h"
#include "gpio_registers.h"
#include "dma_registers.h"
#include "gpio.h"
#include "dma.h"
#include "rpi.h"
#include "util.h"
#include "mailbox.h"
#include "raspberry_pi_revision.h"

#define BUS_TO_PHYS(x) ((x)&~0xC0000000)

struct {
         int handle;      // From mbox_open()
    unsigned mem_ref;     // From mem_alloc()
    unsigned bus_addr;    // From mem_lock()
     uint8_t *virt_addr;  // From mapmem()
} mbox;

typedef struct {
     uint8_t *virtaddr;
    uint32_t physaddr;
} page_map_t;

page_map_t *page_map;

uint8_t *virtbase;
    int NumPages = 0;
    int channel;          // DMA Channel

uint32_t mem_flag;        // Cache or not depending on Rpi1 or 2/3
uint32_t dram_phys_base;

uint32_t current_sample;
uint32_t last_sample;
uint32_t sample_available;

dmagpio_t *dmagpio;
 dma_cb_t *cbarray;
 uint32_t cbsize;
 uint32_t *usermem;
 uint32_t usermemsize;
     bool Started = false;

uint32_t buffersize;
uint32_t cbbysample;
uint32_t registerbysample;
uint32_t *sampletab;

void dma_init(int Channel, uint32_t CBSize, uint32_t UserMemSize) { // Fixme! Need to check to be 256 Aligned for UserMem
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    dmagpio_init(&dmagpio);
    if (dmagpio == NULL)
        librpitx_dbg_printf(2, "1-dmagpio == NULL\n");
    //Channel DMA is now hardcoded according to Raspi Model (DMA 7 for Pi4, DMA 14 for others)
    uint32_t BCM2708_PERI_BASE = bcm_host_get_peripheral_address();

    channel = Channel;
    if (BCM2708_PERI_BASE == 0xFE000000) {
        channel = 7; // Pi4
        librpitx_dbg_printf(1, "dma PI4 using channel %d\n", channel);
    } else {
        channel = 14; // Other Pi
        librpitx_dbg_printf(1, "dma (NOT a PI4)  using channel %d\n", channel);
    }

    librpitx_dbg_printf(1, "channel %d CBSize %u UsermemSize %u\n", channel, CBSize, UserMemSize);

    mbox.handle = mbox_open();
    if (mbox.handle < 0) {
        librpitx_dbg_printf(1, "Failed to open mailbox\n");

    }
    cbsize = CBSize;
    usermemsize = UserMemSize;

    dma_get_rpi_info(); // Fill mem_flag and dram_phys_base

    uint32_t MemoryRequired = CBSize * sizeof(dma_cb_t) + UserMemSize * sizeof(uint32_t);
    int NumPages = (MemoryRequired / PAGE_SIZE) + 1;
    librpitx_dbg_printf(2, "%d Size NUM PAGES %d PAGE_SIZE %d\n", MemoryRequired, NumPages, PAGE_SIZE);
    mbox.mem_ref = mem_alloc(mbox.handle, NumPages * PAGE_SIZE, PAGE_SIZE, mem_flag);
    // TODO: How do we know that succeeded?
    librpitx_dbg_printf(2,"mem_ref %x\n", mbox.mem_ref);
    mbox.bus_addr = mem_lock(mbox.handle, mbox.mem_ref);
    librpitx_dbg_printf(1,"bus_addr = %x\n", mbox.bus_addr);
    mbox.virt_addr = (uint8_t*) mapmem(BUS_TO_PHYS(mbox.bus_addr), NumPages * PAGE_SIZE);
    librpitx_dbg_printf(2,"virt_addr %p\n", mbox.virt_addr);
    virtbase = (uint8_t*) ((uint32_t*) mbox.virt_addr);
    librpitx_dbg_printf(2,"virtbase %p\n", virtbase);
    cbarray = (dma_cb_t*) virtbase; // We place DMA Control Blocks (CB) at beginning of virtual memory
    librpitx_dbg_printf(2,"cbarray %p\n", cbarray);
    usermem = (unsigned int*) (virtbase + CBSize * sizeof(dma_cb_t)); // user memory is placed after
    librpitx_dbg_printf(2,"usermem %p\n", usermem);

    if(dmagpio == NULL)
        librpitx_dbg_printf(2,"dmagpio == NULL\n");
    if(dmagpio->h_gpio == NULL)
        librpitx_dbg_printf(2,"dmagpio->h_gpio == NULL\n");
    if(dmagpio->h_gpio->gpioreg == NULL)
        librpitx_dbg_printf(2,"dmagpio->h_gpio->gpioreg == NULL\n");


    dmagpio->h_gpio->gpioreg[DMA_CS + channel * 0x40] = BCM2708_DMA_RESET | DMA_CS_INT; // Remove int flag
    usleep(100);
    librpitx_dbg_printf(1,"DMA_CONBLK_AD\n");
    dmagpio->h_gpio->gpioreg[DMA_CONBLK_AD + channel * 0x40] = dma_mem_virt_to_phys((void*) cbarray); // reset to beginning

    //get_clocks(mbox.handle);
    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

void dma_deinit(void) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    dma_stop();
    mem_unlock(mbox.handle, mbox.mem_ref);
    mem_free(mbox.handle, mbox.mem_ref);

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

void dma_get_rpi_info(void) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    dram_phys_base = bcm_host_get_sdram_address();

    mem_flag = MEM_FLAG_HINT_PERMALOCK | MEM_FLAG_NO_INIT; //0x0c;
    switch (dram_phys_base) {
    case 0x40000000:
        mem_flag |= MEM_FLAG_L1_NONALLOCATING;
        break;
    case 0xC0000000:
        mem_flag |= MEM_FLAG_DIRECT;
        break;
    default:
        librpitx_dbg_printf(0, "Unknown Raspberry architecture\n");
    }

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

uint32_t dma_mem_virt_to_phys(volatile void *virt) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    //MBOX METHOD
    uint32_t offset = (uint8_t*) virt - mbox.virt_addr;

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);

    return mbox.bus_addr + offset;
}

uint32_t dma_mem_phys_to_virt(volatile uint32_t phys) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    //MBOX METHOD
    uint32_t offset = phys - mbox.bus_addr;
    uint32_t result = (size_t) ((uint8_t*) mbox.virt_addr + offset);
    //librpitx_dbg_printf(2, "MemtoVirt:Offset=%lx phys=%lx -> %lx\n",offset,phys,result);

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);

    return result;
}

int dma_start(void) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    dmagpio->h_gpio->gpioreg[DMA_CS + channel * 0x40] = BCM2708_DMA_RESET;
    usleep(100);
    dmagpio->h_gpio->gpioreg[DMA_CONBLK_AD + channel * 0x40] = dma_mem_virt_to_phys((void*) cbarray); // reset to beginning
    dmagpio->h_gpio->gpioreg[DMA_DEBUG + channel * 0x40] = 7; // clear debug error flags
    usleep(100);
    dmagpio->h_gpio->gpioreg[DMA_CS + channel * 0x40] = DMA_CS_PRIORITY(7) | DMA_CS_PANIC_PRIORITY(7) | DMA_CS_DISDEBUG | DMA_CS_ACTIVE;
    Started = true;

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);

    return 0;
}

int dma_stop(void) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    dmagpio->h_gpio->gpioreg[DMA_CS + channel * 0x40] = BCM2708_DMA_RESET;
    usleep(1000);
    dmagpio->h_gpio->gpioreg[DMA_CS + channel * 0x40] =
    BCM2708_DMA_INT | BCM2708_DMA_END;
    usleep(100);
    dmagpio->h_gpio->gpioreg[DMA_CONBLK_AD + channel * 0x40] = dma_mem_virt_to_phys((void*) cbarray);
    usleep(100);
    dmagpio->h_gpio->gpioreg[DMA_DEBUG + channel * 0x40] = 7; // clear debug error flags
    usleep(100);
    Started = false;

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);

    return 0;
}

int dma_getcbposition(void) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    volatile uint32_t dmacb = (uint32_t) (dmagpio->h_gpio->gpioreg[DMA_CONBLK_AD + channel * 0x40]);
    //dbg_printf(1,"cb=%x\n",dmacb);
    if (dmacb > 0) {
        librpitx_dbg_printf(2, "< func: %s -a|\n", __func__);
        return dma_mem_phys_to_virt(dmacb) - (size_t) virtbase;
    } else {
        librpitx_dbg_printf(2, "< func: %s -b|\n", __func__);
        return -1;
    }
    // dma_reg.gpioreg[DMA_CONBLK_AD+channel*0x40]-mem_virt_to_phys((void *)cbarray );
}

bool dma_isrunning(void) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);
    librpitx_dbg_printf(2, "< func: %s -b|\n", __func__);
    return ((dmagpio->h_gpio->gpioreg[DMA_CS + channel * 0x40] & DMA_CS_ACTIVE) > 0);
}

bool dma_isunderflow(void) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    //if((dma_reg.gpioreg[DMA_CS+channel*0x40]&DMA_CS_INT)>0)  dbg_printf(1,"Status:%x\n",dma_reg.gpioreg[DMA_CS+channel*0x40]);
    librpitx_dbg_printf(2, "< func: %s |\n", __func__);

    return ((dmagpio->h_gpio->gpioreg[DMA_CS + channel * 0x40] & DMA_CS_INT) > 0);
}

bool dma_set_cb(dma_cb_t *cbp, uint32_t dma_flag, uint32_t src, uint32_t dst, uint32_t repeat) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    cbp->info = dma_flag;
    cbp->src = src;
    cbp->dst = dst;
    cbp->length = 4 * repeat;
    cbp->stride = 0;
    cbp->next = dma_mem_virt_to_phys(cbp + 1);

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);

    return true;
}


bool dma_set_easy_cb(dma_cb_t *cbp, uint32_t index, dma_common_reg_t dst, uint32_t repeat) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    uint32_t flag = BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP;
    uint32_t src = dma_mem_virt_to_phys(&usermem[index]);
    switch (dst) {
    case dma_pllc_frac:
        break;
    case dma_fsel:
        break;
    case dma_pad:
        break;
    case dma_pwm:
        flag |= BCM2708_DMA_D_DREQ | BCM2708_DMA_PER_MAP(DREQ_PWM);
        break;
    case dma_pcm:
        flag |= BCM2708_DMA_D_DREQ | BCM2708_DMA_PER_MAP(DREQ_PCM_TX);
        break;
    }
    dma_set_cb(cbp, flag, src, dst, repeat);

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);

    return true;
}

///*************************************** BUFFER DMA ********************************************************
void bufferdma_init(int Channel, uint32_t tbuffersize, uint32_t tcbbysample, uint32_t tregisterbysample) {
    dma_init(Channel, tbuffersize * tcbbysample, tbuffersize * tregisterbysample);

    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    buffersize = tbuffersize;
    cbbysample = tcbbysample;
    registerbysample = tregisterbysample;
    librpitx_dbg_printf(1, "BufferSize %d, cb %d user %d\n", buffersize, buffersize * cbbysample, buffersize * registerbysample);

    current_sample = 0;
    last_sample = 0;
    sample_available = buffersize;

    sampletab = usermem;

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

void bufferdma_deinit(void) {
    dma_deinit();
}

void bufferdma_set_dma_algo(void) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);
    librpitx_dbg_printf(2, "< func: %s |\n", __func__);
}

int bufferdma_GetBufferAvailable(void) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    int diffsample = 0;
    if (Started) {
        int CurrenCbPos = dma_getcbposition();
        if (CurrenCbPos != -1) {
            current_sample = CurrenCbPos / (sizeof(dma_cb_t) * cbbysample);
        } else {
            librpitx_dbg_printf(1, "DMA WEIRD STATE\n");
            current_sample = 0;
        }
        //librpitx_dbg_printf(1,"CurrentCB=%d\n",current_sample);
        diffsample = current_sample - last_sample;
        if (diffsample < 0)
            diffsample += buffersize;

        //librpitx_dbg_printf(1,"cur %d last %d diff%d\n",current_sample,last_sample,diffsample);
    } else {
        //last_sample=buffersize-1;
        diffsample = buffersize;
        current_sample = 0;
        //librpitx_dbg_printf(1,"Warning DMA stopped \n");
        //librpitx_dbg_printf(1,"S:cur %d last %d diff%d\n",current_sample,last_sample,diffsample);
    }

    //
    if (dma_isunderflow()) {
        librpitx_dbg_printf(1, "cur %d last %d \n", current_sample, last_sample);
        librpitx_dbg_printf(1, "Underflow\n");
    }

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);

    return diffsample;

}

int bufferdma_GetUserMemIndex(void) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    int IndexAvailable = -1;
    //librpitx_dbg_printf(1,"Avail=%d\n",GetBufferAvailable());
    if (bufferdma_GetBufferAvailable() > 0) {
        IndexAvailable = last_sample + 1;
        if (IndexAvailable >= (int) buffersize)
            IndexAvailable = 0;
    }

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);

    return IndexAvailable;
}

int bufferdma_PushSample(int Index) {
    librpitx_dbg_printf(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    if (Index < 0)
        return -1; // No buffer available

    //
    dma_cb_t *cbp;
    cbp = &cbarray[last_sample * cbbysample + cbbysample - 1];
    cbp->info = cbp->info & (~BCM2708_DMA_SET_INT);

    last_sample = Index;
    //
    cbp = &cbarray[Index * cbbysample + cbbysample - 1];
    cbp->info = cbp->info | (BCM2708_DMA_SET_INT);

    if (Started == false) {
        if (last_sample > buffersize / 4)
            dma_start(); // 1/4 Fill buffer before starting DMA
    }

    librpitx_dbg_printf(2, "< func: %s |\n", __func__);

    return 0;
}
