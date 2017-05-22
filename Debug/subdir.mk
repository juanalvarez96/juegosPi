################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../arkanoPiLib.c \
../fsm.c \
../juegosPi.c \
../kbhit.c \
../pongPiLib.c \
../tmr.c 

OBJS += \
./arkanoPiLib.o \
./fsm.o \
./juegosPi.o \
./kbhit.o \
./pongPiLib.o \
./tmr.o 

C_DEPS += \
./arkanoPiLib.d \
./fsm.d \
./juegosPi.d \
./kbhit.d \
./pongPiLib.d \
./tmr.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I"C:\SysGCC\Raspberry\include" -I"C:\SysGCC\Raspberry\include\wiringPi" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


