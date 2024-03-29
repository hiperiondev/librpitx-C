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

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/timex.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#include "util.h"
#include "rpi.h"
#include "mailbox.h"
#include "gpio_registers.h"
#include "gpio_enum.h"
#include "gpio.h"

void gpio_init(gpio_t **gpio, uint32_t base, uint32_t len) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s base:%d len:%d (file %s | line %d)\n", __func__, base, len, __FILE__, __LINE__);

    *gpio = (gpio_t*) malloc(sizeof(struct gpio));
    (*gpio)->pi_is_2711 = false;
    (*gpio)->xosc_frequency = 19200000;
    (*gpio)->gpioreg = NULL;
    (*gpio)->gpioreg = (uint32_t*) mapmem(gpio_get_peripheral_base(gpio) + base, len);
    (*gpio)->gpiolen = len;

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void gpio_deinit(gpio_t **gpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    if ((*gpio)->gpioreg != NULL)
        unmapmem((void*) (*gpio)->gpioreg, (*gpio)->gpiolen);

    FREE(*gpio);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

uint32_t gpio_get_hwbase(void) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);
    const char *ranges_file = "/proc/device-tree/soc/ranges";

    uint8_t ranges[12];
    FILE *fd;
    uint32_t ret = 0;

    memset(ranges, 0, sizeof(ranges));

    if ((fd = fopen(ranges_file, "rb")) == NULL) {
        printf("Can't open '%s'\n", ranges_file);
    } else if (fread(ranges, 1, sizeof(ranges), fd) >= 8) {
        ret = (ranges[4] << 24) | (ranges[5] << 16) | (ranges[6] << 8) | (ranges[7] << 0);
        if (!ret)
            ret = (ranges[8] << 24) | (ranges[9] << 16) | (ranges[10] << 8) | (ranges[11] << 0);
        if ((ranges[0] != 0x7e) || (ranges[1] != 0x00) || (ranges[2] != 0x00) || (ranges[3] != 0x00)
                || ((ret != 0x20000000) && (ret != 0x3f000000) && (ret != 0xfe000000))) {
            printf("Unexpected ranges data (%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x)\n", ranges[0], ranges[1], ranges[2], ranges[3], ranges[4],
                    ranges[5], ranges[6], ranges[7], ranges[8], ranges[9], ranges[10], ranges[11]);
            ret = 0;
        }
    } else {
        printf("Ranges data too short\n");
    }

    fclose(fd);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return ret;
}

uint32_t gpio_get_peripheral_base(gpio_t **gpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);
    // RASPBERRY_PI_INFO_T info;

    uint32_t BCM2708_PERI_BASE = bcm_host_get_peripheral_address();
    LIBRPITX_DBG_PRINTF(0, "Peri Base = %x SDRAM %x\n", // get_hwbase()
            bcm_host_get_peripheral_address(), bcm_host_get_sdram_address());
    if (BCM2708_PERI_BASE == 0xFE000000) { // Fixme, could be inspect without this hardcoded value
        LIBRPITX_DBG_PRINTF(0, "RPi4 GPIO detected\n");
        (*gpio)->pi_is_2711 = true;  //Rpi4
        (*gpio)->xosc_frequency = 54000000;
    }
    if (BCM2708_PERI_BASE == 0) {
        LIBRPITX_DBG_PRINTF(0, "Unknown peripheral base, switch to PI4 \n");
        BCM2708_PERI_BASE = 0xfe000000;
        (*gpio)->xosc_frequency = 54000000;
        (*gpio)->pi_is_2711 = true;
    }
    if ((*gpio)->pi_is_2711)
        LIBRPITX_DBG_PRINTF(1, "Running on Pi4\n");

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return BCM2708_PERI_BASE;
}

// DMA Registers

void dmagpio_init(dmagpio_t **dmagio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    *dmagio = (dmagpio_t*) malloc(sizeof(struct dmagpio));
    gpio_init(&((*dmagio)->h_gpio), DMA_BASE, DMA_LEN);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void dmagpio_deinit(dmagpio_t **dmagio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    gpio_deinit(&(*dmagio)->h_gpio);
    FREE(*dmagio);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

// CLK Registers
void clkgpio_init(clkgpio_t **clkgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    *clkgpio = (clkgpio_t*) malloc(sizeof(struct clkgpio));
    gpio_init(&((*clkgpio)->h_gpio), CLK_BASE, CLK_LEN);
    (*clkgpio)->gengpio = (generalgpio_t*) malloc(sizeof(struct generalgpio));
    generalgpio_init(&(*clkgpio)->gengpio);
    clkgpio_set_ppm_from_ntp(clkgpio);
    padgpio_t *level;
    padgpio_init(&level);
    padgpio_setlevel(&(level->h_gpio), 7); //MAX Power

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void clkgpio_deinit(clkgpio_t **clkgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    (*clkgpio)->h_gpio->gpioreg[GPCLK_CNTL] = 0x5A000000 | ((*clkgpio)->mash << 9) | (*clkgpio)->pllnumber | (0 << 4); //4 is START CLK
    //gpioreg[GPCLK_CNTL_2] = 0x5A000000 | (Mash << 9) | pllnumber | (0 << 4); //4 is START CLK
    usleep(100);
    generalgpio_deinit(&(*clkgpio)->gengpio);
    gpio_deinit(&(*clkgpio)->h_gpio);
    FREE(*clkgpio);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

int clkgpio_set_pll_number(clkgpio_t **clkgpio, int pll_no, int mash_type) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    //print_clock_tree();
    if (pll_no < 8)
        (*clkgpio)->pllnumber = pll_no;
    else
        (*clkgpio)->pllnumber = clk_pllc;

    if (mash_type < 4)
        (*clkgpio)->mash = mash_type;
    else
        (*clkgpio)->mash = 0;
    (*clkgpio)->h_gpio->gpioreg[GPCLK_CNTL] = 0x5A000000 | ((*clkgpio)->mash << 9) | (*clkgpio)->pllnumber /*|(1 << 5)*/; // 5 is Reset CLK
    usleep(100);
    //gpioreg[GPCLK_CNTL_2] = 0x5A000000 | (Mash << 9) | pllnumber /*|(1 << 5)*/; // 5 is Reset CLK
    //usleep(100);
    (*clkgpio)->pll_frequency = clkgpio_get_pll_frequency(clkgpio, (*clkgpio)->pllnumber);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

uint64_t clkgpio_get_pll_frequency(clkgpio_t **clkgpio, int pll_no) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    uint64_t Freq = 0;
    clkgpio_set_ppm_from_ntp(clkgpio);
    switch (pll_no) {
        case clk_osc:
            Freq = (*clkgpio)->h_gpio->xosc_frequency;
            break;
        case clk_plla:
            Freq = ((*clkgpio)->h_gpio->xosc_frequency * ((uint64_t) (*clkgpio)->h_gpio->gpioreg[PLLA_CTRL] & 0x3ff)
                    + (*clkgpio)->h_gpio->xosc_frequency * (uint64_t) (*clkgpio)->h_gpio->gpioreg[PLLA_FRAC] / (1 << 20))
                    / (2 * ((*clkgpio)->h_gpio->gpioreg[PLLA_CTRL] >> 12) & 0x7);
            break;
            //case clk_pllb:Freq=XOSC_FREQUENCY*((uint64_t)gpioreg[PLLB_CTRL]&0x3ff) +XOSC_FREQUENCY*(uint64_t)gpioreg[PLLB_FRAC]/(1<<20);break;
        case clk_pllc:
            if (((*clkgpio)->h_gpio->gpioreg[PLLC_PER] != 0) && ((*clkgpio)->h_gpio->gpioreg[PLLC_CTRL] >> 12 != 0))
                Freq = (((*clkgpio)->h_gpio->xosc_frequency * ((uint64_t) (*clkgpio)->h_gpio->gpioreg[PLLC_CTRL] & 0x3ff)
                        + ((*clkgpio)->h_gpio->xosc_frequency * (uint64_t) (*clkgpio)->h_gpio->gpioreg[PLLC_FRAC]) / (1 << 20))
                        / (2 * (*clkgpio)->h_gpio->gpioreg[PLLC_PER] >> 1)) / (((*clkgpio)->h_gpio->gpioreg[PLLC_CTRL] >> 12) & 0x7);
            break;
        case clk_plld:
            Freq = (((*clkgpio)->h_gpio->xosc_frequency * ((uint64_t) (*clkgpio)->h_gpio->gpioreg[PLLD_CTRL] & 0x3ff)
                    + ((*clkgpio)->h_gpio->xosc_frequency * (uint64_t) (*clkgpio)->h_gpio->gpioreg[PLLD_FRAC]) / (1 << 20))
                    / (2 * (*clkgpio)->h_gpio->gpioreg[PLLD_PER] >> 1)) / (((*clkgpio)->h_gpio->gpioreg[PLLD_CTRL] >> 12) & 0x7);
            break;
        case clk_hdmi:
            Freq = ((*clkgpio)->h_gpio->xosc_frequency * ((uint64_t) (*clkgpio)->h_gpio->gpioreg[PLLH_CTRL] & 0x3ff)
                    + (*clkgpio)->h_gpio->xosc_frequency * (uint64_t) (*clkgpio)->h_gpio->gpioreg[PLLH_FRAC] / (1 << 20))
                    / (2 * ((*clkgpio)->h_gpio->gpioreg[PLLH_CTRL] >> 12) & 0x7);
            break;
    }
    if (!(*clkgpio)->h_gpio->pi_is_2711) // FixMe : Surely a register which say it is a 2x
        Freq *= 2LL;
    Freq = Freq * (1.0 - (*clkgpio)->clk_ppm * 1e-6);
    LIBRPITX_DBG_PRINTF(1, "Pi4=%d Xosc = %llu Freq PLL no %d= %llu\n", (*clkgpio)->h_gpio->pi_is_2711, (*clkgpio)->h_gpio->xosc_frequency, pll_no, Freq);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return Freq;
}

int clkgpio_set_clk_div_frac(clkgpio_t **clkgpio, uint32_t div, uint32_t frac) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    (*clkgpio)->h_gpio->gpioreg[GPCLK_DIV] = 0x5A000000 | ((div) << 12) | frac;
    usleep(100);
    //dbg_printf(1, "Clk Number %d div %d frac %d\n", pllnumber, div, frac);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

int clkgpio_set_master_mult_frac(clkgpio_t **clkgpio, uint32_t mult, uint32_t frac) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    //dbg_printf(1,"Master Mult %d Frac %d\n",mult,frac);
    (*clkgpio)->h_gpio->gpioreg[PLLC_CTRL] = (0x5a << 24) | (0x21 << 12) | mult; //PDIV=1
    usleep(100);
    (*clkgpio)->h_gpio->gpioreg[PLLC_FRAC] = 0x5A000000 | frac;

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

int clkgpio_set_frequency(clkgpio_t **clkgpio, double frequency) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    if ((*clkgpio)->modulate_from_master_pll) {
        double FloatMult = 0;
        if ((*clkgpio)->pll_fix_divider == 1) { //Using PDIV thus frequency/2
            if ((*clkgpio)->h_gpio->pi_is_2711)
                FloatMult = ((double) ((*clkgpio)->central_frequency + frequency) * 2)
                        / ((double) ((*clkgpio)->h_gpio->xosc_frequency) * (1 - (*clkgpio)->clk_ppm * 1e-6));
            else {
                FloatMult = ((double) ((*clkgpio)->central_frequency + frequency))
                        / ((double) ((*clkgpio)->h_gpio->xosc_frequency * 2) * (1 - (*clkgpio)->clk_ppm * 1e-6));
            }

        } else {
            if ((*clkgpio)->h_gpio->pi_is_2711)
                FloatMult = ((double) ((*clkgpio)->central_frequency + frequency) * (*clkgpio)->pll_fix_divider * 2)
                        / ((double) ((*clkgpio)->h_gpio->xosc_frequency) * (1 - (*clkgpio)->clk_ppm * 1e-6));
            else {
                FloatMult = ((double) ((*clkgpio)->central_frequency + frequency) * (*clkgpio)->pll_fix_divider)
                        / ((double) ((*clkgpio)->h_gpio->xosc_frequency) * (1 - (*clkgpio)->clk_ppm * 1e-6));
            }
        }

        uint32_t freqctl = FloatMult * ((double) (1 << 20));
        int IntMultiply = freqctl >> 20; // Need to be calculated to have a center frequency
        freqctl &= 0xFFFFF;              // Fractional is 20bits
        uint32_t FracMultiply = freqctl & 0xFFFFF;

        clkgpio_set_master_mult_frac(clkgpio, IntMultiply, FracMultiply);
    } else {
        double Freqresult = (double) (*clkgpio)->pll_frequency / (double) ((*clkgpio)->central_frequency + frequency);
        uint32_t FreqDivider = (uint32_t) Freqresult;
        uint32_t FreqFractionnal = (uint32_t) (4096 * (Freqresult - (double) FreqDivider));
        if ((FreqDivider > 4096) || (FreqDivider < 2))
            LIBRPITX_DBG_PRINTF(0, "Frequency out of range\n");
        LIBRPITX_DBG_PRINTF(1, "DIV/FRAC %u/%u \n", FreqDivider, FreqFractionnal);

        clkgpio_set_clk_div_frac(clkgpio, FreqDivider, FreqFractionnal);
    }

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

uint32_t clkgpio_get_master_frac(clkgpio_t **clkgpio, double frequency) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    if ((*clkgpio)->modulate_from_master_pll) {
        double FloatMult = 0;
        if (((*clkgpio)->pll_fix_divider == 1)) // There is no Prediv on Pi4 //Using PDIV thus frequency/2
        {
            if ((*clkgpio)->h_gpio->pi_is_2711) // No PDIV on pi4
                FloatMult = ((double) ((*clkgpio)->central_frequency + frequency) * 2)
                        / ((double) ((*clkgpio)->h_gpio->xosc_frequency) * (1 - (*clkgpio)->clk_ppm * 1e-6));
            else {
                FloatMult = ((double) ((*clkgpio)->central_frequency + frequency))
                        / ((double) ((*clkgpio)->h_gpio->xosc_frequency * 2) * (1 - (*clkgpio)->clk_ppm * 1e-6));
            }

        } else {
            if ((*clkgpio)->h_gpio->pi_is_2711)
                FloatMult = ((double) ((*clkgpio)->central_frequency + frequency) * (*clkgpio)->pll_fix_divider * 2)
                        / ((double) ((*clkgpio)->h_gpio->xosc_frequency) * (1 - (*clkgpio)->clk_ppm * 1e-6));
            else {
                FloatMult = ((double) ((*clkgpio)->central_frequency + frequency) * (*clkgpio)->pll_fix_divider)
                        / ((double) ((*clkgpio)->h_gpio->xosc_frequency) * (1 - (*clkgpio)->clk_ppm * 1e-6));
            }
        }
        uint32_t freqctl = FloatMult * ((double) (1 << 20));
        //int IntMultiply = freqctl >> 20; // Need to be calculated to have a center frequency
        freqctl &= 0xFFFFF;                // Fractional is 20bits
        uint32_t FracMultiply = freqctl & 0xFFFFF;
        LIBRPITX_DBG_PRINTF(2, "< func: %s -a|\n", __func__);
        return FracMultiply;
    } else {
        LIBRPITX_DBG_PRINTF(2, "< func: %s -b|\n", __func__);
        return 0; // Not in Master Clk mode
    }
}

int clkgpio_compute_best_lo(clkgpio_t **clkgpio, uint64_t frequency, int bandwidth) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    // Algorithm adapted from https://github.com/SaucySoliton/PiFmRds/blob/master/src/pi_fm_rds.c
    // Choose an integer divider for GPCLK0
    // There may be improvements possible to this algorithm.
    // Constants taken https://github.com/raspberrypi/linux/blob/ffd7bf4085b09447e5db96edd74e524f118ca3fe/drivers/clk/bcm/clk-bcm2835.c#L1763
    // MIN RATE is NORMALLY 600MHZ
#define MIN_PLL_RATE 200e6
#define MIN_PLL_RATE_USE_PDIV 1500e6 //1700 works but some ticky breaks in clock..PLL should be at limit
#define MAX_PLL_RATE 4e9
#define XTAL_RATE (*clkgpio)->h_gpio->xosc_frequency
    // Pi4 seems 54Mhz
    double xtal_freq_recip = 1.0 / XTAL_RATE; // todo PPM correction
    int best_divider = 0;
    //int solution_count = 0;
    //printf("carrier:%3.2f ",carrier_freq/1e6);
    int divider = 0, min_int_multiplier, max_int_multiplier; // fom,, best_fom = 0, int_multiplier
    //double Multiplier=0.0;
    best_divider = 0;
    bool cross_boundary = false;

    if (frequency < MIN_PLL_RATE / 4095) {
        LIBRPITX_DBG_PRINTF(1, "Frequency too low !!!!!!\n");
        return -1;
    }
    if (frequency * 2 > MAX_PLL_RATE) {
        LIBRPITX_DBG_PRINTF(1, "Frequency too high !!!!!!\n");
        return -1;
    }
    if (frequency * 2 > MIN_PLL_RATE_USE_PDIV) {
        best_divider = 1; // We will use PREDIV 2 for PLL
    } else {
        for (divider = 4095; divider > 1; divider--) { // 1 is allowed only for MASH=0
            if (frequency * divider < MIN_PLL_RATE)
                continue; // widest accepted frequency range
            if (frequency * divider > MIN_PLL_RATE_USE_PDIV) // By Experiment on Rpi3B
            {
                continue;
            }
            max_int_multiplier = ((int) ((double) (frequency + bandwidth) * divider * xtal_freq_recip));
            min_int_multiplier = ((int) ((double) (frequency - bandwidth) * divider * xtal_freq_recip));
            if (min_int_multiplier != max_int_multiplier) {
                //dbg_printf(1,"Warning : cross boundary frequency\n");
                best_divider = divider;
                cross_boundary = true;
                continue; // don't cross integer boundary
            } else {
                cross_boundary = false;
                best_divider = divider;
                break;
            }
        }
    }
    if (best_divider != 0) {
        (*clkgpio)->pll_fix_divider = best_divider;

        if (cross_boundary)
            LIBRPITX_DBG_PRINTF(1, "Warning : cross boundary frequency\n");
        LIBRPITX_DBG_PRINTF(1, "Found PLL solution for frequency %4.1fMHz : divider:%d VCO: %4.1fMHz\n", (frequency / 1e6), (*clkgpio)->pll_fix_divider,
                (frequency / 1e6) * (((*clkgpio)->pll_fix_divider == 1) ? 2.0 : (double) (*clkgpio)->pll_fix_divider));
        LIBRPITX_DBG_PRINTF(2, "< func: %s -a|\n", __func__);
        return 0;
    } else {
        LIBRPITX_DBG_PRINTF(1, "Central frequency not available !!!!!!\n");
        LIBRPITX_DBG_PRINTF(2, "< func: %s -b|\n", __func__);
        return -1;
    }
}

double clkgpio_GetFrequencyResolution(clkgpio_t **clkgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    double res = 0;
    if ((*clkgpio)->modulate_from_master_pll) {
        res = (*clkgpio)->h_gpio->xosc_frequency / (double) (1 << 20) / (*clkgpio)->pll_fix_divider;
    } else {
        double Freqresult = (double) (*clkgpio)->pll_frequency / (double) ((*clkgpio)->central_frequency);
        uint32_t FreqDivider = (uint32_t) Freqresult;
        res = ((*clkgpio)->pll_frequency / (double) (FreqDivider + 1) - (*clkgpio)->pll_frequency / (double) (FreqDivider)) / 4096.0;
    }

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return res;
}

double clkgpio_get_real_frequency(clkgpio_t **clkgpio, double frequency) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    double FloatMult = ((double) ((*clkgpio)->central_frequency + frequency) * (*clkgpio)->pll_fix_divider) / (double) ((*clkgpio)->h_gpio->xosc_frequency);
    uint32_t freqctl = FloatMult * ((double) (1 << 20));
    int IntMultiply = freqctl >> 20; // Need to be calculated to have a center frequency
    freqctl &= 0xFFFFF;              // Fractionnal is 20bits
    uint32_t FracMultiply = freqctl & 0xFFFFF;
    double RealFrequency = ((double) IntMultiply + (FracMultiply / (double) (1 << 20))) * (double) ((*clkgpio)->h_gpio->xosc_frequency)
            / (*clkgpio)->pll_fix_divider - ((*clkgpio)->central_frequency + frequency);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return RealFrequency;
}

int clkgpio_set_center_frequency(clkgpio_t **clkgpio, uint64_t frequency, int bandwidth) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    (*clkgpio)->central_frequency = frequency;
    if ((*clkgpio)->modulate_from_master_pll) {
        // Choose best PLLDiv and Div

        if ((*clkgpio)->h_gpio->pi_is_2711) {
            clkgpio_compute_best_lo(clkgpio, frequency * 2, bandwidth); //FixeDivider update
            // PllFixDivider=PllFixDivider/2;
        } else {
            clkgpio_compute_best_lo(clkgpio, frequency, bandwidth); //FixeDivider update
        }

        if ((*clkgpio)->pll_fix_divider == 1) {
            // We will use PDIV by 2, means like we have a 2 times more
            clkgpio_set_clk_div_frac(clkgpio, 2, 0x0); // NO MASH !!!!
            LIBRPITX_DBG_PRINTF(1, "Pll Fix Divider\n");

        } else {
            clkgpio_set_clk_div_frac(clkgpio, (*clkgpio)->pll_fix_divider, 0x0); // NO MASH !!!!

        }

        // Apply PREDIV for PLL or not
        uint32_t ana[4];
        for (int i = 3; i >= 0; i--) {
            ana[i] = (*clkgpio)->h_gpio->gpioreg[(A2W_PLLC_ANA0) + i];
            usleep(100);
            //dbg_printf(1,"PLLC %d =%x\n",i,ana[i]);
            ana[i] &= ~(1 << 14);
        }

        if ((*clkgpio)->pll_fix_divider == 1) {
            LIBRPITX_DBG_PRINTF(1, "Use PLL Prediv\n");
            ana[1] |= (1 << 14); // use prediv means Frequency*2
        } else {
            ana[1] |= (0 << 14); // No use prediv means Frequenc
        }

        // ANA register setup is done as a series of writes to
        // ANA3-ANA0, in that order.  This lets us write all 4
        // registers as a single cycle of the serdes interface (taking
        // 100 xosc clocks), whereas if we were to update ana0, 1, and
        // 3 individually through their partial-write registers, each
        // would be their own serdes cycle.

        for (int i = 3; i >= 0; i--) {
            ana[i] |= (0x5A << 24);
            (*clkgpio)->h_gpio->gpioreg[(A2W_PLLC_ANA0) + i] = ana[i];
            //dbg_printf(1,"Write %d = %x\n",i,ana[i]);
            usleep(100);
        }
        /*
         for (int i = 3; i >= 0; i--)
         {
         dbg_printf(1,"PLLC after %d =%x\n",i,gpioreg[(A2W_PLLC_ANA0 ) + i]);
         }
         */
        clkgpio_set_frequency(clkgpio, 0);
        usleep(100);
        if (((*clkgpio)->h_gpio->gpioreg[CM_LOCK] & CM_LOCK_FLOCKC) > 0)
            LIBRPITX_DBG_PRINTF(1, "Master PLLC Locked\n");
        else
            LIBRPITX_DBG_PRINTF(1, "Warning ! Master PLLC NOT Locked !!!!\n");

        usleep(100);
        (*clkgpio)->h_gpio->gpioreg[GPCLK_CNTL] = 0x5A000000 | ((*clkgpio)->mash << 9) | (*clkgpio)->pllnumber | (1 << 4); //4 is START CLK
        usleep(100);

        (*clkgpio)->h_gpio->gpioreg[GPCLK_CNTL] = 0x5A000000 | ((*clkgpio)->mash << 9) | (*clkgpio)->pllnumber | (1 << 4); //4 is START CLK
        usleep(100);

    } else {
        clkgpio_get_pll_frequency(clkgpio, (*clkgpio)->pllnumber);   // Be sure to get the master PLL frequency
        (*clkgpio)->h_gpio->gpioreg[GPCLK_CNTL] = 0x5A000000 | ((*clkgpio)->mash << 9) | (*clkgpio)->pllnumber | (1 << 4); //4 is START CLK
    }

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

void clkgpio_set_phase(clkgpio_t **clkgpio, bool inversed) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    uint32_t StateBefore = (*clkgpio)->h_gpio->gpioreg[GPCLK_CNTL];
    (*clkgpio)->h_gpio->gpioreg[GPCLK_CNTL] = (0x5A << 24) | StateBefore | ((inversed ? 1 : 0) << 8) | 1 << 5;
    //clkgpio_gpioreg[GPCLK_CNTL_2] = (0x5A << 24) | StateBefore | ((inversed ? 1 : 0) << 8) | 1 << 5;
    (*clkgpio)->h_gpio->gpioreg[GPCLK_CNTL] = (0x5A << 24) | StateBefore | ((inversed ? 1 : 0) << 8) | 0 << 5;
    //clkgpio_gpioreg[GPCLK_CNTL_2] = (0x5A << 24) | StateBefore | ((inversed ? 1 : 0) << 8) | 0 << 5;

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

//https://elinux.org/The_Undocumented_Pi
//Should inspect https://github.com/raspberrypi/linux/blob/ffd7bf4085b09447e5db96edd74e524f118ca3fe/drivers/clk/bcm/clk-bcm2835.c#L695
void clkgpio_set_advanced_pll_mode(clkgpio_t **clkgpio, bool advanced) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    (*clkgpio)->modulate_from_master_pll = advanced;
    if ((*clkgpio)->modulate_from_master_pll) {
        //We must change Clk dependant from PLLC as we will modulate it
        // switch the core over to PLLA
        (*clkgpio)->h_gpio->gpioreg[CORECLK_DIV] = (0x5a << 24) | (4 << 12); // core div 4
        usleep(100);
        (*clkgpio)->h_gpio->gpioreg[CORECLK_CNTL] = (0x5a << 24) | (1 << 4) | (4); // run, src=PLLA

        // switch the EMMC over to PLLD

        int clktmp;
        clktmp = (*clkgpio)->h_gpio->gpioreg[EMMCCLK_CNTL];
        (*clkgpio)->h_gpio->gpioreg[EMMCCLK_CNTL] = (0xF0F & clktmp) | (0x5a << 24); // clear run
        usleep(100);
        (*clkgpio)->h_gpio->gpioreg[EMMCCLK_CNTL] = (0xF00 & clktmp) | (0x5a << 24) | (6); // src=PLLD
        usleep(100);
        (*clkgpio)->h_gpio->gpioreg[EMMCCLK_CNTL] = (0xF00 & clktmp) | (0x5a << 24) | (1 << 4) | (6); // run , src=PLLD

        clkgpio_set_pll_number(clkgpio, clk_pllc, 0); // Use PLL_C , Do not USE MASH which generates spurious

        (*clkgpio)->h_gpio->gpioreg[CM_PLLC] = 0x5A00022A; // Enable PllC_PER
        usleep(100);

        (*clkgpio)->h_gpio->gpioreg[PLLC_CORE0] = 0x5A000000 | (1 << 8); //Disable
        if ((*clkgpio)->h_gpio->pi_is_2711)
            (*clkgpio)->h_gpio->gpioreg[PLLC_PER] = 0x5A000002; // Divisor by 2 (1 seems not working on pi4)
        else {
            (*clkgpio)->h_gpio->gpioreg[PLLC_PER] = 0x5A000001; // Divisor 1 for max frequence
        }

        usleep(100);
    }

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void clkgpio_set_pll_master_loop(clkgpio_t **clkgpio, int ki, int kp, int ka) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    uint32_t ana[4];
    for (int i = 3; i >= 0; i--)
        ana[i] = (*clkgpio)->h_gpio->gpioreg[(A2W_PLLC_ANA0) + i];

    // Fix me : Should make a OR with old value
    ana[1] &= (uint32_t) ~((0x7 << A2W_PLL_KI_SHIFT) | (0xF << A2W_PLL_KP_SHIFT) | (0x7 << A2W_PLL_KA_SHIFT));
    ana[1] |= (ki << A2W_PLL_KI_SHIFT) | (kp << A2W_PLL_KP_SHIFT) | (ka << A2W_PLL_KA_SHIFT);
    LIBRPITX_DBG_PRINTF(1, "Loop parameter =%x\n", ana[1]);
    for (int i = 3; i >= 0; i--) {
        (*clkgpio)->h_gpio->gpioreg[(A2W_PLLC_ANA0) + i] = (0x5A << 24) | ana[i];
        usleep(100);
    }
    usleep(100);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void clkgpio_print_clock_tree(clkgpio_t **clkgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);
    printf("PLLC_DIG0=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x1020 / 4)]);
    printf("PLLC_DIG1=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x1024 / 4)]);
    printf("PLLC_DIG2=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x1028 / 4)]);
    printf("PLLC_DIG3=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x102c / 4)]);
    printf("PLLC_ANA0=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x1030 / 4)]);
    printf("PLLC_ANA1=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x1034 / 4)]);
    printf("PLLC_ANA2=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x1038 / 4)]);
    printf("PLLC_ANA3=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x103c / 4)]);
    printf("PLLC_DIG0R=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x1820 / 4)]);
    printf("PLLC_DIG1R=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x1824 / 4)]);
    printf("PLLC_DIG2R=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x1828 / 4)]);
    printf("PLLC_DIG3R=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x182c / 4)]);

    printf("PLLA_ANA0=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x1010 / 4)]);
    printf("PLLA_ANA1=%08x prediv=%d\n", (*clkgpio)->h_gpio->gpioreg[(0x1014 / 4)], ((*clkgpio)->h_gpio->gpioreg[(0x1014 / 4)] >> 14) & 1);
    printf("PLLA_ANA2=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x1018 / 4)]);
    printf("PLLA_ANA3=%08x\n", (*clkgpio)->h_gpio->gpioreg[(0x101c / 4)]);

    printf("GNRIC CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[0], (*clkgpio)->h_gpio->gpioreg[1]);
    printf("VPU   CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[2], (*clkgpio)->h_gpio->gpioreg[3]);
    printf("SYS   CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[4], (*clkgpio)->h_gpio->gpioreg[5]);
    printf("PERIA CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[6], (*clkgpio)->h_gpio->gpioreg[7]);
    printf("PERII CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[8], (*clkgpio)->h_gpio->gpioreg[9]);
    printf("H264  CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[10], (*clkgpio)->h_gpio->gpioreg[11]);
    printf("ISP   CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[12], (*clkgpio)->h_gpio->gpioreg[13]);
    printf("V3D   CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[14], (*clkgpio)->h_gpio->gpioreg[15]);

    printf("CAM0  CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[16], (*clkgpio)->h_gpio->gpioreg[17]);
    printf("CAM1  CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[18], (*clkgpio)->h_gpio->gpioreg[19]);
    printf("CCP2  CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[20], (*clkgpio)->h_gpio->gpioreg[21]);
    printf("DSI0E CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[22], (*clkgpio)->h_gpio->gpioreg[23]);
    printf("DSI0P CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[24], (*clkgpio)->h_gpio->gpioreg[25]);
    printf("DPI   CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[26], (*clkgpio)->h_gpio->gpioreg[27]);
    printf("GP0   CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[0x70 / 4], (*clkgpio)->h_gpio->gpioreg[0x74 / 4]);
    printf("GP1   CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[30], (*clkgpio)->h_gpio->gpioreg[31]);

    printf("GP2   CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[32], (*clkgpio)->h_gpio->gpioreg[33]);
    printf("HSM   CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[34], (*clkgpio)->h_gpio->gpioreg[35]);
    printf("OTP   CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[36], (*clkgpio)->h_gpio->gpioreg[37]);
    printf("PCM   CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[38], (*clkgpio)->h_gpio->gpioreg[39]);
    printf("PWM   CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[40], (*clkgpio)->h_gpio->gpioreg[41]);
    printf("SLIM  CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[42], (*clkgpio)->h_gpio->gpioreg[43]);
    printf("SMI   CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[44], (*clkgpio)->h_gpio->gpioreg[45]);
    printf("SMPS  CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[46], (*clkgpio)->h_gpio->gpioreg[47]);

    printf("TCNT  CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[48], (*clkgpio)->h_gpio->gpioreg[49]);
    printf("TEC   CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[50], (*clkgpio)->h_gpio->gpioreg[51]);
    printf("TD0   CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[52], (*clkgpio)->h_gpio->gpioreg[53]);
    printf("TD1   CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[54], (*clkgpio)->h_gpio->gpioreg[55]);

    printf("TSENS CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[56], (*clkgpio)->h_gpio->gpioreg[57]);
    printf("TIMER CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[58], (*clkgpio)->h_gpio->gpioreg[59]);
    printf("UART  CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[60], (*clkgpio)->h_gpio->gpioreg[61]);
    printf("VEC   CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[62], (*clkgpio)->h_gpio->gpioreg[63]);

    printf("PULSE CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[100], (*clkgpio)->h_gpio->gpioreg[101]);
    printf("PLLT  CTL=%08x DIV=????????\n", (*clkgpio)->h_gpio->gpioreg[76]);

    printf("DSI1E CTL=%08x DIV=%8x  ", (*clkgpio)->h_gpio->gpioreg[86], (*clkgpio)->h_gpio->gpioreg[87]);
    printf("DSI1P CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[88], (*clkgpio)->h_gpio->gpioreg[89]);
    printf("AVE0  CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[90], (*clkgpio)->h_gpio->gpioreg[91]);

    printf("CMPLLA=%08x  ", (*clkgpio)->h_gpio->gpioreg[0x104 / 4]);
    printf("CMPLLC=%08x \n", (*clkgpio)->h_gpio->gpioreg[0x108 / 4]);
    printf("CMPLLD=%08x   ", (*clkgpio)->h_gpio->gpioreg[0x10C / 4]);
    printf("CMPLLH=%08x \n", (*clkgpio)->h_gpio->gpioreg[0x110 / 4]);

    printf("EMMC  CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[112], (*clkgpio)->h_gpio->gpioreg[113]);
    printf("EMMC  CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[112], (*clkgpio)->h_gpio->gpioreg[113]);
    printf("EMMC  CTL=%08x DIV=%8x\n", (*clkgpio)->h_gpio->gpioreg[112], (*clkgpio)->h_gpio->gpioreg[113]);

    // Sometimes calculated frequencies are off by a factor of 2
    // ANA1 bit 14 may indicate that a /2 prescaler is active
    double xoscmhz = (*clkgpio)->h_gpio->xosc_frequency / 1e6;
    printf("PLLA PDIV=%d NDIV=%d FRAC=%d  ", ((*clkgpio)->h_gpio->gpioreg[PLLA_CTRL] >> 12) & 0x7, (*clkgpio)->h_gpio->gpioreg[PLLA_CTRL] & 0x3ff,
            (*clkgpio)->h_gpio->gpioreg[PLLA_FRAC]);
    printf(" %f MHz\n",
            xoscmhz * ((float) ((*clkgpio)->h_gpio->gpioreg[PLLA_CTRL] & 0x3ff) + ((float) (*clkgpio)->h_gpio->gpioreg[PLLA_FRAC]) / ((float) (1 << 20))));
    printf("DSI0=%d CORE=%d PER=%d CCP2=%d\n\n", (*clkgpio)->h_gpio->gpioreg[PLLA_DSI0], (*clkgpio)->h_gpio->gpioreg[PLLA_CORE],
            (*clkgpio)->h_gpio->gpioreg[PLLA_PER], (*clkgpio)->h_gpio->gpioreg[PLLA_CCP2]);

    printf("PLLB PDIV=%d NDIV=%d FRAC=%d  ", ((*clkgpio)->h_gpio->gpioreg[PLLB_CTRL] >> 12) & 0x7, (*clkgpio)->h_gpio->gpioreg[PLLB_CTRL] & 0x3ff,
            (*clkgpio)->h_gpio->gpioreg[PLLB_FRAC]);
    printf(" %f MHz\n",
            xoscmhz * ((float) ((*clkgpio)->h_gpio->gpioreg[PLLB_CTRL] & 0x3ff) + ((float) (*clkgpio)->h_gpio->gpioreg[PLLB_FRAC]) / ((float) (1 << 20))));
    printf("ARM=%d SP0=%d SP1=%d SP2=%d\n\n", (*clkgpio)->h_gpio->gpioreg[PLLB_ARM], (*clkgpio)->h_gpio->gpioreg[PLLB_SP0],
            (*clkgpio)->h_gpio->gpioreg[PLLB_SP1], (*clkgpio)->h_gpio->gpioreg[PLLB_SP2]);

    printf("PLLC PDIV=%d NDIV=%d FRAC=%d  ", ((*clkgpio)->h_gpio->gpioreg[PLLC_CTRL] >> 12) & 0x7, (*clkgpio)->h_gpio->gpioreg[PLLC_CTRL] & 0x3ff,
            (*clkgpio)->h_gpio->gpioreg[PLLC_FRAC]);
    printf(" %f MHz\n",
            xoscmhz * ((float) ((*clkgpio)->h_gpio->gpioreg[PLLC_CTRL] & 0x3ff) + ((float) (*clkgpio)->h_gpio->gpioreg[PLLC_FRAC]) / ((float) (1 << 20))));
    printf("CORE2=%d CORE1=%d PER=%d CORE0=%d\n\n", (*clkgpio)->h_gpio->gpioreg[PLLC_CORE2], (*clkgpio)->h_gpio->gpioreg[PLLC_CORE1],
            (*clkgpio)->h_gpio->gpioreg[PLLC_PER], (*clkgpio)->h_gpio->gpioreg[PLLC_CORE0]);

    printf("PLLD %x PDIV=%d NDIV=%d FRAC=%d  ", (*clkgpio)->h_gpio->gpioreg[PLLD_CTRL], ((*clkgpio)->h_gpio->gpioreg[PLLD_CTRL] >> 12) & 0x7,
            (*clkgpio)->h_gpio->gpioreg[PLLD_CTRL] & 0x3ff, (*clkgpio)->h_gpio->gpioreg[PLLD_FRAC]);
    printf(" %f MHz\n",
            xoscmhz * ((float) ((*clkgpio)->h_gpio->gpioreg[PLLD_CTRL] & 0x3ff) + ((float) (*clkgpio)->h_gpio->gpioreg[PLLD_FRAC]) / ((float) (1 << 20))));
    printf("DSI0=%d CORE=%d PER=%d DSI1=%d\n\n", (*clkgpio)->h_gpio->gpioreg[PLLD_DSI0], (*clkgpio)->h_gpio->gpioreg[PLLD_CORE],
            (*clkgpio)->h_gpio->gpioreg[PLLD_PER], (*clkgpio)->h_gpio->gpioreg[PLLD_DSI1]);

    printf("PLLH PDIV=%d NDIV=%d FRAC=%d  ", ((*clkgpio)->h_gpio->gpioreg[PLLH_CTRL] >> 12) & 0x7, (*clkgpio)->h_gpio->gpioreg[PLLH_CTRL] & 0x3ff,
            (*clkgpio)->h_gpio->gpioreg[PLLH_FRAC]);
    printf(" %f MHz\n",
            xoscmhz * ((float) ((*clkgpio)->h_gpio->gpioreg[PLLH_CTRL] & 0x3ff) + ((float) (*clkgpio)->h_gpio->gpioreg[PLLH_FRAC]) / ((float) (1 << 20))));
    printf("AUX=%d RCAL=%d PIX=%d STS=%d\n\n", (*clkgpio)->h_gpio->gpioreg[PLLH_AUX], (*clkgpio)->h_gpio->gpioreg[PLLH_RCAL],
            (*clkgpio)->h_gpio->gpioreg[PLLH_PIX], (*clkgpio)->h_gpio->gpioreg[PLLH_STS]);
    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void clkgpio_enableclk(clkgpio_t **clkgpio, int gpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    switch (gpio) {
        case 4:
            generalgpio_setmode(&((*clkgpio)->gengpio), gpio, fsel_alt0);
            break;
        case 20:
            generalgpio_setmode(&((*clkgpio)->gengpio), gpio, fsel_alt5);
            break;
        case 32:
            generalgpio_setmode(&((*clkgpio)->gengpio), gpio, fsel_alt0);
            break;
        case 34:
            generalgpio_setmode(&((*clkgpio)->gengpio), gpio, fsel_alt0);
            break;
            //CLK2
        case 6:
            generalgpio_setmode(&((*clkgpio)->gengpio), gpio, fsel_alt0);
            break;
        default:
            LIBRPITX_DBG_PRINTF(1, "gpio %d has no clk - available(4,20,32,34)\n", gpio);
            break;
    }
    usleep(100);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void clkgpio_disableclk(clkgpio_t **clkgpio, int gpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    generalgpio_setmode(&((*clkgpio)->gengpio), gpio, fsel_input);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void clkgpio_set_ppm(clkgpio_t **clkgpio, double ppm) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    (*clkgpio)->clk_ppm = ppm; // -2 is empiric : FixMe

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void clkgpio_set_ppm_from_ntp(clkgpio_t **clkgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    struct timex ntx;
    int status;
    // Calibrate Clock system (surely depends also on PLL PPM
    // ======================================================

    ntx.modes = 0; /* only read */
    status = ntp_adjtime(&ntx);
    double ntp_ppm;

    if (status != TIME_OK) {
        LIBRPITX_DBG_PRINTF(1, "Warning: NTP calibrate failed\n");
    } else {

        ntp_ppm = (double) ntx.freq / (double) (1 << 16);
        LIBRPITX_DBG_PRINTF(1, "Info:NTP find offset %ld freq %ld pps=%ld ppm=%f\n", ntx.offset, ntx.freq, ntx.ppsfreq, ntp_ppm);

        if (fabs(ntp_ppm) < 200)
            clkgpio_set_ppm(clkgpio, ntp_ppm/*+0.70*/); // 0.7 is empiric
    }

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

// GENERAL GPIO

void generalgpio_init(generalgpio_t **generalgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    *generalgpio = (generalgpio_t*) malloc(sizeof(struct generalgpio));
    gpio_init(&((*generalgpio)->h_gpio), /*gpio_GetPeripheralBase() + */GENERAL_BASE, GENERAL_LEN);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void generalgpio_deinit(generalgpio_t **generalgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    gpio_deinit(&(*generalgpio)->h_gpio);
    FREE(*generalgpio);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

int generalgpio_setmode(generalgpio_t **generalgpio, uint32_t gpio, uint32_t mode) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    int reg, shift;

    reg = gpio / 10;
    shift = (gpio % 10) * 3;

    (*generalgpio)->h_gpio->gpioreg[reg] = ((*generalgpio)->h_gpio->gpioreg[reg] & ~(7 << shift)) | (mode << shift);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

int generalgpio_setpulloff(generalgpio_t **generalgpio, uint32_t gpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    if (!(*generalgpio)->h_gpio->pi_is_2711) {
        (*generalgpio)->h_gpio->gpioreg[GPPUD] = 0;
        usleep(150);
        (*generalgpio)->h_gpio->gpioreg[GPPUDCLK0] = 1 << gpio;
        usleep(150);
        (*generalgpio)->h_gpio->gpioreg[GPPUDCLK0] = 0;
    } else {
        uint32_t bits;
        uint32_t pull = 0; // 0 OFF, 1 = UP, 2= DOWN
        int shift = (gpio & 0xf) << 1;
        bits = (*generalgpio)->h_gpio->gpioreg[GPPUPPDN0 + (gpio >> 4)];
        bits &= ~(3 << shift);
        bits |= (pull << shift);
        (*generalgpio)->h_gpio->gpioreg[GPPUPPDN0 + (gpio >> 4)] = bits;
    }

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

// PWM GPIO

void pwmgpio_init(pwmgpio_t **pwmgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    *pwmgpio = (pwmgpio_t*) malloc(sizeof(struct pwmgpio));
    gpio_init(&((*pwmgpio)->h_gpio), /*gpio_GetPeripheralBase() + */PWM_BASE, PWM_LEN);
    generalgpio_init(&((*pwmgpio)->gengpio));
    clkgpio_init(&(*pwmgpio)->clk);
    (*pwmgpio)->pllnumber = 0;
    (*pwmgpio)->mash = 0;
    (*pwmgpio)->prediv = 0;
    (*pwmgpio)->pll_frequency = 0;
    (*pwmgpio)->h_gpio->gpioreg[PWM_CTL] = 0;

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void pwmgpio_deinit(pwmgpio_t **pwmgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    (*pwmgpio)->h_gpio->gpioreg[PWM_CTL] = 0;
    (*pwmgpio)->h_gpio->gpioreg[PWM_DMAC] = 0;

    generalgpio_deinit(&(*pwmgpio)->gengpio);
    clkgpio_deinit(&(*pwmgpio)->clk);
    gpio_deinit(&(*pwmgpio)->h_gpio);

    FREE(*pwmgpio);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void pwmgpio_enablepwm(pwmgpio_t **pwmgpio, int gpio, int pwm_number) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    if (pwm_number == 0) {
        switch (gpio) {
            case 12:
                generalgpio_setmode(&((*pwmgpio)->gengpio), gpio, fsel_alt0);
                break;
            case 18:
                generalgpio_setmode(&((*pwmgpio)->gengpio), gpio, fsel_alt5);
                break;
            case 40:
                generalgpio_setmode(&((*pwmgpio)->gengpio), gpio, fsel_alt0);
                break;

            default:
                LIBRPITX_DBG_PRINTF(1, "gpio %d has no pwm - available(12,18,40)\n", gpio);
                break;
        }
    }
    if (pwm_number == 1) {
        switch (gpio) {
            case 13:
                generalgpio_setmode(&((*pwmgpio)->gengpio), gpio, fsel_alt0);
                break;
            case 19:
                generalgpio_setmode(&((*pwmgpio)->gengpio), gpio, fsel_alt5);
                break;
            case 41:
                generalgpio_setmode(&((*pwmgpio)->gengpio), gpio, fsel_alt0);
                break;
            case 45:
                generalgpio_setmode(&((*pwmgpio)->gengpio), gpio, fsel_alt0);
                break;
            default:
                LIBRPITX_DBG_PRINTF(1, "gpio %d has no pwm - available(13,19,41,45)\n", gpio);
                break;
        }
    }
    usleep(100);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void pwmgpio_disablepwm(pwmgpio_t **pwmgpio, int gpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    generalgpio_setmode(&((*pwmgpio)->gengpio), gpio, fsel_input);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

int pwmgpio_set_pll_number(pwmgpio_t **pwmgpio, int pll_no, int mash_type) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    if (pll_no < 8)
        (*pwmgpio)->pllnumber = pll_no;
    else
        (*pwmgpio)->pllnumber = clk_pllc;
    if (mash_type < 4)
        (*pwmgpio)->mash = mash_type;
    else
        (*pwmgpio)->mash = 0;
    (*pwmgpio)->clk->h_gpio->gpioreg[PWMCLK_CNTL] = 0x5A000000 | ((*pwmgpio)->mash << 9) | (*pwmgpio)->pllnumber | (0 << 4); //4 is STOP CLK
    usleep(100);
    (*pwmgpio)->pll_frequency = pwmgpio_get_pll_frequency(pwmgpio, (*pwmgpio)->pllnumber);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

uint64_t pwmgpio_get_pll_frequency(pwmgpio_t **pwmgpio, int pll_no) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);
    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return clkgpio_get_pll_frequency(&((*pwmgpio)->clk), pll_no);
}

int pwmgpio_set_frequency(pwmgpio_t **pwmgpio, uint64_t frequency) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    (*pwmgpio)->prediv = 32; // Fixe for now , need investigation if not 32 !!!! FixMe !
    double Freqresult = (double) (*pwmgpio)->pll_frequency / (double) (frequency * (*pwmgpio)->prediv);
    uint32_t FreqDivider = (uint32_t) Freqresult;
    uint32_t FreqFractionnal = (uint32_t) (4096 * (Freqresult - (double) FreqDivider));
    if ((FreqDivider > 4096) || (FreqDivider < 2))
        LIBRPITX_DBG_PRINTF(1, "Frequency out of range\n");
    LIBRPITX_DBG_PRINTF(1, "PWM clk=%d / %d\n", FreqDivider, FreqFractionnal);
    (*pwmgpio)->clk->h_gpio->gpioreg[PWMCLK_DIV] = 0x5A000000 | ((FreqDivider) << 12) | FreqFractionnal;

    usleep(100);
    (*pwmgpio)->clk->h_gpio->gpioreg[PWMCLK_CNTL] = 0x5A000000 | ((*pwmgpio)->mash << 9) | (*pwmgpio)->pllnumber | (1 << 4); //4 is STAR CLK
    usleep(100);

    pwmgpio_set_prediv(pwmgpio, (*pwmgpio)->prediv); // SetMode should be called before

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

void pwmgpio_set_mode(pwmgpio_t **pwmgpio, int mode) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    if ((mode >= pwm1pin) && (mode <= pwm1pinrepeat))
        (*pwmgpio)->mode_pwm = mode;

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

int pwmgpio_set_prediv(pwmgpio_t **pwmgpio, int predivisor) { // Mode should be only for SYNC or a Data serializer : Todo
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    (*pwmgpio)->prediv = predivisor;
    if ((*pwmgpio)->prediv > 32) {
        LIBRPITX_DBG_PRINTF(1, "PWM Prediv is max 32\n");
        (*pwmgpio)->prediv = 2;
    }
    LIBRPITX_DBG_PRINTF(1, "PWM Prediv %d\n", (*pwmgpio)->prediv);
    (*pwmgpio)->h_gpio->gpioreg[PWM_RNG1] = (*pwmgpio)->prediv; // 250 -> 8KHZ
    usleep(100);
    (*pwmgpio)->h_gpio->gpioreg[PWM_RNG2] = (*pwmgpio)->prediv; // 32 Mandatory for Serial Mode without gap

    //gpioreg[PWM_FIFO]=0xAAAAAAAA;

    (*pwmgpio)->h_gpio->gpioreg[PWM_DMAC] = PWMDMAC_ENAB | PWMDMAC_THRSHLD;
    usleep(100);
    (*pwmgpio)->h_gpio->gpioreg[PWM_CTL] = PWMCTL_CLRF;
    usleep(100);

    //gpioreg[PWM_CTL] = PWMCTL_USEF1| PWMCTL_MODE1| PWMCTL_PWEN1|PWMCTL_MSEN1;
    switch ((*pwmgpio)->mode_pwm) {
        case pwm1pin:
            (*pwmgpio)->h_gpio->gpioreg[PWM_CTL] = PWMCTL_USEF1 | PWMCTL_MODE1 | PWMCTL_PWEN1 | PWMCTL_MSEN1;
            break; // All serial go to 1 pin
        case pwm2pin:
            (*pwmgpio)->h_gpio->gpioreg[PWM_CTL] = PWMCTL_USEF2 | PWMCTL_PWEN2 | PWMCTL_MODE2 | PWMCTL_USEF1 | PWMCTL_MODE1 | PWMCTL_PWEN1;
            break; // Alternate bit to pin 1 and 2
        case pwm1pinrepeat:
            (*pwmgpio)->h_gpio->gpioreg[PWM_CTL] = PWMCTL_USEF1 | PWMCTL_MODE1 | PWMCTL_PWEN1 | PWMCTL_RPTL1;
            break; // All serial go to 1 pin, repeat if empty : RF mode with PWM
    }
    usleep(100);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

// PCM GPIO (I2S)
void pcmgpio_init(pcmgpio_t **pcmgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    *pcmgpio = (pcmgpio_t*) malloc(sizeof(struct pcmgpio));
    gpio_init(&((*pcmgpio)->h_gpio), PCM_BASE, PCM_LEN);
    clkgpio_init(&(*pcmgpio)->clk);
    (*pcmgpio)->pllnumber = 0;
    (*pcmgpio)->mash = 0;
    (*pcmgpio)->prediv = 0;
    (*pcmgpio)->pll_frequency = 0;
    (*pcmgpio)->h_gpio->gpioreg[PCM_CS_A] = 1; // Disable Rx+Tx, Enable PCM block

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void pcmgpio_deinit(pcmgpio_t **pcmgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    gpio_deinit(&(*pcmgpio)->h_gpio);
    clkgpio_deinit(&(*pcmgpio)->clk);
    FREE(*pcmgpio);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

int pcmgpio_set_pll_number(pcmgpio_t **pcmgpio, int pll_no, int mash_type) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    if (pll_no < 8)
        (*pcmgpio)->pllnumber = pll_no;
    else
        (*pcmgpio)->pllnumber = clk_pllc;
    if (mash_type < 4)
        (*pcmgpio)->mash = mash_type;
    else
        (*pcmgpio)->mash = 0;
    (*pcmgpio)->clk->h_gpio->gpioreg[PCMCLK_CNTL] = 0x5A000000 | ((*pcmgpio)->mash << 9) | (*pcmgpio)->pllnumber | (1 << 4); //4 is START CLK
    (*pcmgpio)->pll_frequency = pcmgpio_get_pll_frequency(pcmgpio, (*pcmgpio)->pllnumber);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
    return 0;
}

uint64_t pcmgpio_get_pll_frequency(pcmgpio_t **pcmgpio, int pll_no) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);
    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return clkgpio_get_pll_frequency(&((*pcmgpio)->clk), pll_no);
}

int pcmgpio_compute_prediv(pcmgpio_t **pcmgpio, uint64_t frequency) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    int prediv = 5;
    for (prediv = 10; prediv < 1000; prediv++) {
        double Freqresult = (double) (*pcmgpio)->pll_frequency / (double) (frequency * prediv);
        if ((Freqresult < 4096.0) && (Freqresult > 2.0)) {
            LIBRPITX_DBG_PRINTF(1, "PCM prediv = %d\n", prediv);
            break;
        }
    }

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return prediv;
}

int pcmgpio_set_frequency(pcmgpio_t **pcmgpio, uint64_t frequency) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    (*pcmgpio)->prediv = pcmgpio_compute_prediv(pcmgpio, frequency);
    double Freqresult = (double) (*pcmgpio)->pll_frequency / (double) (frequency * (*pcmgpio)->prediv);
    uint32_t FreqDivider = (uint32_t) Freqresult;
    uint32_t FreqFractionnal = (uint32_t) (4096 * (Freqresult - (double) FreqDivider));
    LIBRPITX_DBG_PRINTF(1, "PCM clk=%d / %d\n", FreqDivider, FreqFractionnal);
    if ((FreqDivider > 4096) || (FreqDivider < 2))
        LIBRPITX_DBG_PRINTF(1, "PCM frequency out of range\n");
    (*pcmgpio)->clk->h_gpio->gpioreg[PCMCLK_DIV] = 0x5A000000 | ((FreqDivider) << 12) | FreqFractionnal;
    pcmgpio_set_prediv(pcmgpio, (*pcmgpio)->prediv);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

int pcmgpio_set_prediv(pcmgpio_t **pcmgpio, int predivisor) { // Carefull we use a 10 fixe divisor for now : frequency is thus f/10 as a samplerate
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    if (predivisor > 1000) {
        LIBRPITX_DBG_PRINTF(1, "PCM prediv should be <1000");
        predivisor = 1000;
    }

    (*pcmgpio)->h_gpio->gpioreg[PCM_TXC_A] = 0 << 31 | 1 << 30 | 0 << 20 | 0 << 16; // 1 channel, 8 bits
    usleep(100);

    //printf("Nb PCM STEP (<1000):%d\n",NbStepPCM);
    (*pcmgpio)->h_gpio->gpioreg[PCM_MODE_A] = (predivisor - 1) << 10; // SHOULD NOT EXCEED 1000 !!!
    usleep(100);
    (*pcmgpio)->h_gpio->gpioreg[PCM_CS_A] |= 1 << 4 | 1 << 3; // Clear FIFOs
    usleep(100);
    (*pcmgpio)->h_gpio->gpioreg[PCM_DREQ_A] = 64 << 24 | 64 << 8; //TX Fifo PCM=64 DMA Req when one slot is free? : Fixme
    usleep(100);
    (*pcmgpio)->h_gpio->gpioreg[PCM_CS_A] |= 1 << 9; // Enable DMA
    usleep(100);
    (*pcmgpio)->h_gpio->gpioreg[PCM_CS_A] |= 1 << 2; //START TX PCM

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}

// PADGPIO (Amplitude)

void padgpio_init(padgpio_t **padgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    *padgpio = (padgpio_t*) malloc(sizeof(struct padgpio));
    gpio_init(&((*padgpio)->h_gpio), PADS_GPIO, PADS_GPIO_LEN);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

void padgpio_deinit(padgpio_t **padgpio) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    gpio_deinit(&(*padgpio)->h_gpio);
    FREE(*padgpio);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);
}

int padgpio_setlevel(gpio_t **gpio, int level) {
    LIBRPITX_DBG_PRINTF(2, "> func: %s (file %s | line %d)\n", __func__, __FILE__, __LINE__);

    if ((*gpio) == NULL)
        LIBRPITX_DBG_PRINTF(2, "!! padgpio_setlevel(*padgpio)->h_gpio->gpioreg == NULL");
    (*gpio)->gpioreg[PADS_GPIO_0] = (0x5a << 24) | (level & 0x7) | (0 << 4) | (0 << 3);

    LIBRPITX_DBG_PRINTF(2, "< func: %s |\n", __func__);

    return 0;
}
