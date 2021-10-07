################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../librpitx/modulation/amdmasync.c \
../librpitx/modulation/atv.c \
../librpitx/modulation/fmdmasync.c \
../librpitx/modulation/fskburst.c \
../librpitx/modulation/iqdmasync.c \
../librpitx/modulation/ngfmdmasync.c \
../librpitx/modulation/ookburst.c \
../librpitx/modulation/phasedmasync.c \
../librpitx/modulation/serialdmasync.c 

OBJS += \
./librpitx/modulation/amdmasync.o \
./librpitx/modulation/atv.o \
./librpitx/modulation/fmdmasync.o \
./librpitx/modulation/fskburst.o \
./librpitx/modulation/iqdmasync.o \
./librpitx/modulation/ngfmdmasync.o \
./librpitx/modulation/ookburst.o \
./librpitx/modulation/phasedmasync.o \
./librpitx/modulation/serialdmasync.o 

C_DEPS += \
./librpitx/modulation/amdmasync.d \
./librpitx/modulation/atv.d \
./librpitx/modulation/fmdmasync.d \
./librpitx/modulation/fskburst.d \
./librpitx/modulation/iqdmasync.d \
./librpitx/modulation/ngfmdmasync.d \
./librpitx/modulation/ookburst.d \
./librpitx/modulation/phasedmasync.d \
./librpitx/modulation/serialdmasync.d 


# Each subdirectory must supply rules for building sources it contributes
librpitx/modulation/%.o: ../librpitx/modulation/%.c librpitx/modulation/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I"../librpitx" -I"../librpitx/core/include" -I"../librpitx/modulation/include" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


