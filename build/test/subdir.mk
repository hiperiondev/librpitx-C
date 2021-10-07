################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../test/test_tx.c 

OBJS += \
./test/test_tx.o 

C_DEPS += \
./test/test_tx.d 


# Each subdirectory must supply rules for building sources it contributes
test/%.o: ../test/%.c test/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I"../librpitx" -I"../librpitx/core/include" -I"../librpitx/modulation/include" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


