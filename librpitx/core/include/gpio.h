/*
 * gpio.h
 *
 *  Created on: 4 oct. 2021
 *      Author: egonzalez
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

    void gpio_Cgpio(gpio_t **gpio, uint32_t base, uint32_t len);
    void gpio_Dgpio(gpio_t **gpio);
uint32_t gpio_get_hwbase(void);
uint32_t gpio_GetPeripheralBase(gpio_t **gpio);

    void dmagpio_Cdmagpio(dmagpio_t **dmagpio);

    void clkgpio_Cclkgpio(clkgpio_t **clkgpio);
    void clkgpio_Dclkgpio(clkgpio_t **clkgpio);
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
//  void clkgpio_print_clock_tree(clkgpio_t **clkgpio);
    void clkgpio_enableclk(clkgpio_t **clkgpio, int gpio);
    void clkgpio_disableclk(clkgpio_t **clkgpio, int gpio);
    void clkgpio_Setppm(clkgpio_t **clkgpio, double ppm);
    void clkgpio_SetppmFromNTP(clkgpio_t **clkgpio);

    void generalgpio_Cgeneralgpio(generalgpio_t **generalgpio);
    void generalgpio_Dgeneralgpio(generalgpio_t **generalgpio);
     int generalgpio_setmode(generalgpio_t **generalgpio, uint32_t gpio, uint32_t mode);
     int generalgpio_setpulloff(generalgpio_t **generalgpio, uint32_t gpio);

    void pwmgpio_Cpwmgpio(pwmgpio_t **pwmgpio);
    void pwmgpio_Dpwmgpio(pwmgpio_t **pwmgpio);
    void pwmgpio_enablepwm(pwmgpio_t **pwmgpio, int gpio, int PwmNumber);
    void pwmgpio_disablepwm(pwmgpio_t **pwmgpio, int gpio);
     int pwmgpio_SetPllNumber(pwmgpio_t **pwmgpio, int PllNo, int MashType);
uint64_t pwmgpio_GetPllFrequency(pwmgpio_t **pwmgpio, int PllNo);
     int pwmgpio_SetFrequency(pwmgpio_t **pwmgpio, uint64_t Frequency);
    void pwmgpio_SetMode(pwmgpio_t **pwmgpio, int Mode);
     int pwmgpio_SetPrediv(pwmgpio_t **pwmgpio, int predivisor);

    void pcmgpio_Cpcmgpio(pcmgpio_t **pcmgpio);
    void pcmgpio_Dpcmgpio(pcmgpio_t **pcmgpio);
     int pcmgpio_SetPllNumber(pcmgpio_t **pcmgpio, int PllNo, int MashType);
uint64_t pcmgpio_GetPllFrequency(pcmgpio_t **pcmgpio, int PllNo);
     int pcmgpio_ComputePrediv(pcmgpio_t **pcmgpio, uint64_t Frequency);
     int pcmgpio_SetFrequency(pcmgpio_t **pcmgpio, uint64_t Frequency);
     int pcmgpio_SetPrediv(pcmgpio_t **pcmgpio, int predivisor);

    void padgpio_Cpadgpio(padgpio_t **padgpio);
    void padgpio_Dpadgpio(padgpio_t **padgpio);
     int padgpio_setlevel(gpio_t **gpio, int level);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H_ */
