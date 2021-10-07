################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../librpitx/core/dma.c \
../librpitx/core/dsp.c \
../librpitx/core/gpio.c \
../librpitx/core/mailbox.c \
../librpitx/core/raspberry_pi_revision.c \
../librpitx/core/rpi.c \
../librpitx/core/util.c 

OBJS += \
./librpitx/core/dma.o \
./librpitx/core/dsp.o \
./librpitx/core/gpio.o \
./librpitx/core/mailbox.o \
./librpitx/core/raspberry_pi_revision.o \
./librpitx/core/rpi.o \
./librpitx/core/util.o 

C_DEPS += \
./librpitx/core/dma.d \
./librpitx/core/dsp.d \
./librpitx/core/gpio.d \
./librpitx/core/mailbox.d \
./librpitx/core/raspberry_pi_revision.d \
./librpitx/core/rpi.d \
./librpitx/core/util.d 


# Each subdirectory must supply rules for building sources it contributes
librpitx/core/%.o: ../librpitx/core/%.c librpitx/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I"../librpitx" -I"../librpitx/core/include" -I"../librpitx/modulation/include" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


