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

#include <stdint.h>
#include <stdbool.h>

struct gpio {
                 bool pi_is_2711;
             uint64_t xosc_frequency;
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
              int mash;
         uint64_t pll_frequency;
             bool modulate_from_master_pll;
         uint64_t central_frequency;
    generalgpio_t *gengpio;
           double clk_ppm;
              int pll_fix_divider;
};
typedef struct clkgpio clkgpio_t;

struct pwmgpio {
           gpio_t *h_gpio;

        clkgpio_t *clk;
              int pllnumber;
              int mash;
              int prediv; // Range of PWM
         uint64_t pll_frequency;
             bool modulate_from_master_pll;
              int mode_pwm;
    generalgpio_t *gengpio;
};
typedef struct pwmgpio pwmgpio_t;

struct pcmgpio {
       gpio_t *h_gpio;

    clkgpio_t *clk;
          int pllnumber;
          int mash;
          int prediv; // Range of PCM
     uint64_t pll_frequency;
};
typedef struct pcmgpio pcmgpio_t;

struct padgpio {
    gpio_t *h_gpio;
};
typedef struct padgpio padgpio_t;

    void gpio_init(gpio_t **gpio, uint32_t base, uint32_t len);
    void gpio_deinit(gpio_t **gpio);
uint32_t gpio_get_hwbase(void);
uint32_t gpio_get_peripheral_base(gpio_t **gpio);

    void dmagpio_init(dmagpio_t **dmagpio);

    void clkgpio_init(clkgpio_t **clkgpio);
    void clkgpio_deinit(clkgpio_t **clkgpio);
     int clkgpio_set_pll_number(clkgpio_t **clkgpio, int pll_no, int mash_type);
uint64_t clkgpio_get_pll_frequency(clkgpio_t **clkgpio, int pll_no);
     int clkgpio_set_clk_div_frac(clkgpio_t **clkgpio, uint32_t div, uint32_t frac);
     int clkgpio_set_master_mult_frac(clkgpio_t **clkgpio, uint32_t mult, uint32_t frac);
     int clkgpio_set_frequency(clkgpio_t **clkgpio, double frequency);
uint32_t clkgpio_get_master_frac(clkgpio_t **clkgpio, double frequency);
     int clkgpio_compute_best_lo(clkgpio_t **clkgpio, uint64_t frequency, int bandwidth);
  double clkgpio_GetFrequencyResolution(clkgpio_t **clkgpio);
  double clkgpio_get_real_frequency(clkgpio_t **clkgpio, double frequency);
     int clkgpio_set_center_frequency(clkgpio_t **clkgpio, uint64_t frequency, int bandwidth);
    void clkgpio_set_phase(clkgpio_t **clkgpio, bool inversed);
    void clkgpio_set_advanced_pll_mode(clkgpio_t **clkgpio, bool advanced);
    void clkgpio_set_pll_master_loop(clkgpio_t **clkgpio, int ki, int kp, int ka);
    void clkgpio_print_clock_tree(clkgpio_t **clkgpio);
    void clkgpio_enableclk(clkgpio_t **clkgpio, int gpio);
    void clkgpio_disableclk(clkgpio_t **clkgpio, int gpio);
    void clkgpio_set_ppm(clkgpio_t **clkgpio, double ppm);
    void clkgpio_set_ppm_from_ntp(clkgpio_t **clkgpio);

    void generalgpio_init(generalgpio_t **generalgpio);
    void generalgpio_deinit(generalgpio_t **generalgpio);
     int generalgpio_setmode(generalgpio_t **generalgpio, uint32_t gpio, uint32_t mode);
     int generalgpio_setpulloff(generalgpio_t **generalgpio, uint32_t gpio);

    void pwmgpio_init(pwmgpio_t **pwmgpio);
    void pwmgpio_deinit(pwmgpio_t **pwmgpio);
    void pwmgpio_enablepwm(pwmgpio_t **pwmgpio, int gpio, int pwm_number);
    void pwmgpio_disablepwm(pwmgpio_t **pwmgpio, int gpio);
     int pwmgpio_set_pll_number(pwmgpio_t **pwmgpio, int pll_no, int mash_type);
uint64_t pwmgpio_get_pll_frequency(pwmgpio_t **pwmgpio, int pll_no);
     int pwmgpio_set_frequency(pwmgpio_t **pwmgpio, uint64_t frequency);
    void pwmgpio_set_mode(pwmgpio_t **pwmgpio, int mode);
     int pwmgpio_set_prediv(pwmgpio_t **pwmgpio, int predivisor);

    void pcmgpio_init(pcmgpio_t **pcmgpio);
    void pcmgpio_deinit(pcmgpio_t **pcmgpio);
     int pcmgpio_set_pll_number(pcmgpio_t **pcmgpio, int pll_no, int mash_type);
uint64_t pcmgpio_get_pll_frequency(pcmgpio_t **pcmgpio, int pll_no);
     int pcmgpio_compute_prediv(pcmgpio_t **pcmgpio, uint64_t frequency);
     int pcmgpio_set_frequency(pcmgpio_t **pcmgpio, uint64_t frequency);
     int pcmgpio_set_prediv(pcmgpio_t **pcmgpio, int predivisor);

    void padgpio_init(padgpio_t **padgpio);
    void padgpio_deinit(padgpio_t **padgpio);
     int padgpio_setlevel(gpio_t **gpio, int level);

#endif /* GPIO_H_ */
