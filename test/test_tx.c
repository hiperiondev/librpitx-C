/*
 * Copyright 2021 Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/librpitx-C *
 *
 * This is based on other projects:
 *    librpitx (https://github.com/F5OEO/librpitx)
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

#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <complex.h>
#include <ctype.h>

#include "librpitx.h"
#define PROGRAM_VERSION "2.0"

bool running = true;

void print_usage(void) {
    fprintf(stderr, "Warning : rpitx V2 is only to try to be compatible with version 1\n");

    fprintf(stderr,
            "\nrpitx -%s\n\
Usage:\nrpitx [-i File Input][-m ModeInput] [-f frequency output] [-s Samplerate] [-l] [-p ppm] [-h] \n\
-m            {IQ(FileInput is a Stereo Wav contains I on left Channel, Q on right channel)}\n\
              {IQFLOAT(FileInput is a Raw float interlaced I,Q)}\n\
              {RF(FileInput is a (double)Frequency,Time in nanoseconds}\n\
              {RFA(FileInput is a (double)Frequency,(int)Time in nanoseconds,(float)Amplitude}\n\
          {VFO (constant frequency)}\n\
-i            path to File Input \n\
-f float      frequency to output on GPIO_4 pin 7 in khz : (130 kHz to 750 MHz),\n\
-l            loop mode for file input\n\
-p float      frequency correction in parts per million (ppm), positive or negative, for calibration, default 0.\n\
-h            help (this help).\n\
\n",

            PROGRAM_VERSION);

}

static void terminate(int num) {
    running = false;
    fprintf(stderr, "Caught signal - Terminating %x\n", num);

}

static void fatal(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    terminate(0);
}

int main(int argc, char *argv[]) {
    librpitx_dbg_setlevel(1);

    enum {
        MODE_RPITX_IQ = 0,
        MODE_RPITX_RF,
        MODE_RPITX_RFA,
        MODE_RPITX_IQ_FLOAT,
        MODE_RPITX_VFO
    };

    int a;
    int anyargs = 0;
    char Mode = MODE_IQ;  // By default
    int SampleRate = 48000;
    float SetFrequency = 1e6;  //1MHZ
    float ppmpll = 0.0;
    char *FileName = NULL;
    FILE *FileInHandle = NULL;
    bool loop_mode_flag = false;
    bool useStdin;
    int Harmonic = 1;
    while (1) {
        a = getopt(argc, argv, "i:f:m:s:p:hld:w:c:ra:");

        if (a == -1) {
            if (anyargs)
                break;
            else
                a = 'h'; //print usage and exit
        }
        anyargs = 1;

        switch (a) {
            case 'i': // File name
                FileName = optarg;
                break;
            case 'f': // Frequency
                SetFrequency = atof(optarg) * 1e3;
                break;
            case 'm': // Mode (IQ,IQFLOAT,RF,RFA)
                if (strcmp("IQ", optarg) == 0)
                    Mode = MODE_RPITX_IQ;
                if (strcmp("RF", optarg) == 0)
                    Mode = MODE_RPITX_RF;
                if (strcmp("RFA", optarg) == 0)
                    Mode = MODE_RPITX_RFA;
                if (strcmp("IQFLOAT", optarg) == 0)
                    Mode = MODE_RPITX_IQ_FLOAT;
                if (strcmp("VFO", optarg) == 0)
                    Mode = MODE_RPITX_VFO;
                break;
            case 's': // SampleRate (Only needeed in IQ mode)
                SampleRate = atoi(optarg);
                break;
            case 'p':  // ppmcorrection
                ppmpll = atof(optarg);

                break;
            case 'h': // help
                print_usage();
                exit(1);
                break;
            case 'l': // loop mode
                loop_mode_flag = true;
                break;
            case 'd': // Dma Sample Burst
                fprintf(stderr, "Warning : 'd' parameter not used in this version\n");
                break;
            case 'c':
                fprintf(stderr, "Warning : 'c' parameter not used in this version\n");
                break;
            case 'w': // No use pwmfrequency
                fprintf(stderr, "Warning : 'w' parameter not used in this version\n");
                break;
            case 'r': // Randomize PWM frequency
                fprintf(stderr, "Warning : 'r' parameter not used in this version\n");

                break;
            case 'a': // DMA Channel 1-14
                fprintf(stderr, "Warning : 'a' parameter not used in this version\n");
                break;
            case -1:
                break;
            case '?':
                if (isprint(optopt)) {
                    fprintf(stderr, "rpitx: unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "rpitx: unknown option character `\\x%x'.\n", optopt);
                }
                print_usage();

                exit(1);
                break;
            default:
                print_usage();
                exit(1);
                break;
        }
    }

    if ((Mode == MODE_RPITX_IQ) || (Mode == MODE_RPITX_IQ_FLOAT) || (Mode == MODE_RPITX_RF) || (Mode == MODE_RPITX_RFA)) {
        if (FileName && strcmp(FileName, "-") == 0) {
            FileInHandle = stdin;
            useStdin = true;
        } else
            FileInHandle = fopen(FileName, "rb");
        if (FileInHandle == NULL) {
            fatal("Failed to read Filein %s\n", FileName);
        }
    }

    for (int i = 0; i < 64; i++) {
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate;
        sigaction(i, &sa, NULL);
    }

    fprintf(stderr, "Warning : rpitx V2 is only to try to be compatible with version 1\n");

#define IQBURST 4000 // For IQ

    iqdmasync_t *iqsender;
    amdmasync_t *amsender;
    ngfmdmasync_t *fmsender;
    float AmOrFmBuffer[IQBURST];
    float _Complex CIQBuffer_[IQBURST];
    int Decimation = 1;
    int FifoSize = IQBURST * 4;

    switch (Mode) {
        case MODE_RPITX_IQ:
        case MODE_RPITX_IQ_FLOAT: {
            iqdmasync_init(&iqsender, SetFrequency, SampleRate, 14, FifoSize, MODE_IQ);
            iqdmasync_Setppm(&iqsender, ppmpll);
        }
            break;

        case MODE_RPITX_RFA: //Amplitude
        {
            amdmasync_init(&amsender, SetFrequency, SampleRate, 14, FifoSize);
        }
            break;

        case MODE_RPITX_RF: //Frequency
        {
            ngfmdmasync_init(&fmsender, SetFrequency, SampleRate, 14, FifoSize, false);
        }
    }

    while (running) {
        switch (Mode) {
            case MODE_RPITX_IQ:
            case MODE_RPITX_IQ_FLOAT: {
                int CplxSampleNumber = 0;

                switch (Mode) {
                    case MODE_RPITX_IQ:    //I16
                    {
                        static short IQBuffer[IQBURST * 2];

                        int nbread = fread(IQBuffer, sizeof(short), IQBURST * 2, FileInHandle);
                        if (nbread > 0) {
                            for (int i = 0; i < nbread / 2; i++) {
                                if (i % Decimation == 0) {
                                    CIQBuffer_[CplxSampleNumber++] = (IQBuffer[i * 2] / 32768.0) + (IQBuffer[i * 2 + 1] / 32768.0) * I;
                                }
                            }
                        } else {
                            printf("End of file\n");
                            if (loop_mode_flag && !useStdin)
                                fseek(FileInHandle, 0, SEEK_SET);
                            else
                                running = false;
                        }

                    }
                        break;

                    case MODE_RPITX_IQ_FLOAT: {
                        static float IQBuffer[IQBURST * 2];
                        int nbread = fread(IQBuffer, sizeof(float), IQBURST * 2, FileInHandle);
                        if (nbread > 0) {
                            for (int i = 0; i < nbread / 2; i++) {
                                if (i % Decimation == 0) {
                                    CIQBuffer_[CplxSampleNumber++] = (IQBuffer[i * 2]) + (IQBuffer[i * 2 + 1]) * I;

                                }
                            }
                        } else {
                            printf("End of file\n");
                            if (loop_mode_flag && useStdin)
                                fseek(FileInHandle, 0, SEEK_SET);
                            else
                                running = false;
                        }
                    }
                        break;

                }
                iqdmasync_SetIQSamples(&iqsender, CIQBuffer_, CplxSampleNumber, Harmonic);

            }
                break;

            case MODE_RPITX_RFA:    //Amplitude
            case MODE_RPITX_RF:    //Frequence
            {

                typedef struct {
                    double Frequency;
                    uint32_t WaitForThisSample;
                } samplerf_t;

                int SampleNumber = 0;
                static samplerf_t RfBuffer[IQBURST];
                int nbread = fread(RfBuffer, sizeof(samplerf_t), IQBURST, FileInHandle);
                //if(nbread==0) continue;
                if (nbread > 0) {
                    for (int i = 0; i < nbread; i++) {
                        AmOrFmBuffer[SampleNumber++] = (float) (RfBuffer[i].Frequency);

                    }
                } else {
                    printf("End of file\n");
                    if (loop_mode_flag && useStdin)
                        fseek(FileInHandle, 0, SEEK_SET);
                    else
                        running = false;
                }
                switch (Mode) {
                    case MODE_RPITX_RFA: {
                        amdmasync_SetAmSamples(&amsender, AmOrFmBuffer, SampleNumber);
                    }
                        break;
                    case MODE_RPITX_RF: {
                        ngfmdmasync_SetFrequencySamples(&fmsender, AmOrFmBuffer, SampleNumber);
                    }
                        break;
                }

            }
                break;

            case MODE_RPITX_VFO: {

            }
                break;
        }
    }

    switch (Mode) {
        case MODE_RPITX_IQ:
        case MODE_RPITX_IQ_FLOAT:
            iqdmasync_deinit(&iqsender);
            break;

        case MODE_RPITX_RFA:
            amdmasync_deinit(&amsender);
            break;

        case MODE_RPITX_RF:
            ngfmdmasync_deinit(&fmsender);
            break;
    }
}
