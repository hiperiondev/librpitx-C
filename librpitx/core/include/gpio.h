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

#ifndef GPIO_H_
#define GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

struct gpio {
                 bool pi_is_2711;
             uint64_t XOSC_FREQUENCY;
    volatile uint32_t *gpioreg;
             uint32_t gpiolen;
};
typedef struct gpio gpio_t;

struct dmagpio {
    gpio_t *h_gpio;
};
typedef struct dmagpio dmagpio_t;

struct generalgpio {
    gpio_t *h_gpio;
};
typedef struct generalgpio generalgpio_t;

struct clkgpio {
           gpio_t *h_gpio;

              int pllnumber;
              int Mash;
         uint64_t Pllfrequency;
             bool ModulateFromMasterPLL;
         uint64_t CentralFrequency;
    generalgpio_t *gengpio;
           double clk_ppm;
              int PllFixDivider;
};
typedef struct clkgpio clkgpio_t;

struct pwmgpio {
           gpio_t *h_gpio;

        clkgpio_t *clk;
              int pllnumber;
              int Mash;
              int Prediv; // Range of PWM
         uint64_t Pllfrequency;
             bool ModulateFromMasterPLL;
              int ModePwm;
    generalgpio_t *gengpio;
};
typedef struct pwmgpio pwmgpio_t;

struct pcmgpio {
       gpio_t *h_gpio;

    clkgpio_t *clk;
          int pllnumber;
          int Mash;
          int Prediv; // Range of PCM
     uint64_t Pllfrequency;
};
typedef struct pcmgpio pcmgpio_t;

struct padgpio {
    gpio_t *h_gpio;
};
typedef struct padgpio padgpio_t;

    void gpio_init(gpio_t **gpio, uint32_t base, uint32_t len);
    void gpio_deinit(gpio_t **gpio);
uint32_t gpio_get_hwbase(void);
uint32_t gpio_GetPeripheralBase(gpio_t **gpio);

    void dmagpio_init(dmagpio_t **dmagpio);

    void clkgpio_init(clkgpio_t **clkgpio);
    void clkgpio_deinit(clkgpio_t **clkgpio);
     int clkgpio_SetPllNumber(clkgpio_t **clkgpio, int PllNo, int MashType);
uint64_t clkgpio_GetPllFrequency(clkgpio_t **clkgpio, int PllNo);
     int clkgpio_SetClkDivFrac(clkgpio_t **clkgpio, uint32_t Div, uint32_t Frac);
     int clkgpio_SetMasterMultFrac(clkgpio_t **clkgpio, uint32_t Mult, uint32_t Frac);
     int clkgpio_SetFrequency(clkgpio_t **clkgpio, double Frequency);
uint32_t clkgpio_GetMasterFrac(clkgpio_t **clkgpio, double Frequency);
     int clkgpio_ComputeBestLO(clkgpio_t **clkgpio, uint64_t Frequency, int Bandwidth);
  double clkgpio_GetFrequencyResolution(clkgpio_t **clkgpio);
  double clkgpio_GetRealFrequency(clkgpio_t **clkgpio, double Frequency);
     int clkgpio_SetCenterFrequency(clkgpio_t **clkgpio, uint64_t Frequency, int Bandwidth);
    void clkgpio_SetPhase(clkgpio_t **clkgpio, bool inversed);
    void clkgpio_SetAdvancedPllMode(clkgpio_t **clkgpio, bool Advanced);
    void clkgpio_SetPLLMasterLoop(clkgpio_t **clkgpio, int Ki, int Kp, int Ka);
    void clkgpio_print_clock_tree(clkgpio_t **clkgpio);
    void clkgpio_enableclk(clkgpio_t **clkgpio, int gpio);
    void clkgpio_disableclk(clkgpio_t **clkgpio, int gpio);
    void clkgpio_Setppm(clkgpio_t **clkgpio, double ppm);
    void clkgpio_SetppmFromNTP(clkgpio_t **clkgpio);

    void generalgpio_init(generalgpio_t **generalgpio);
    void generalgpio_deinit(generalgpio_t **generalgpio);
     int generalgpio_setmode(generalgpio_t **generalgpio, uint32_t gpio, uint32_t mode);
     int generalgpio_setpulloff(generalgpio_t **generalgpio, uint32_t gpio);

    void pwmgpio_init(pwmgpio_t **pwmgpio);
    void pwmgpio_deinit(pwmgpio_t **pwmgpio);
    void pwmgpio_enablepwm(pwmgpio_t **pwmgpio, int gpio, int PwmNumber);
    void pwmgpio_disablepwm(pwmgpio_t **pwmgpio, int gpio);
     int pwmgpio_SetPllNumber(pwmgpio_t **pwmgpio, int PllNo, int MashType);
uint64_t pwmgpio_GetPllFrequency(pwmgpio_t **pwmgpio, int PllNo);
     int pwmgpio_SetFrequency(pwmgpio_t **pwmgpio, uint64_t Frequency);
    void pwmgpio_SetMode(pwmgpio_t **pwmgpio, int Mode);
     int pwmgpio_SetPrediv(pwmgpio_t **pwmgpio, int predivisor);

    void pcmgpio_init(pcmgpio_t **pcmgpio);
    void pcmgpio_deinit(pcmgpio_t **pcmgpio);
     int pcmgpio_SetPllNumber(pcmgpio_t **pcmgpio, int PllNo, int MashType);
uint64_t pcmgpio_GetPllFrequency(pcmgpio_t **pcmgpio, int PllNo);
     int pcmgpio_ComputePrediv(pcmgpio_t **pcmgpio, uint64_t Frequency);
     int pcmgpio_SetFrequency(pcmgpio_t **pcmgpio, uint64_t Frequency);
     int pcmgpio_SetPrediv(pcmgpio_t **pcmgpio, int predivisor);

    void padgpio_init(padgpio_t **padgpio);
    void padgpio_deinit(padgpio_t **padgpio);
     int padgpio_setlevel(gpio_t **gpio, int level);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H_ */
