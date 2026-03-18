################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/bootloader.c \
../Core/Src/clock.c \
../Core/Src/ds1302.c \
../Core/Src/dwt_delay.c \
../Core/Src/global.c \
../Core/Src/main.c \
../Core/Src/realtime.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/system_update.c \
../Core/Src/wdt.c 

OBJS += \
./Core/Src/bootloader.o \
./Core/Src/clock.o \
./Core/Src/ds1302.o \
./Core/Src/dwt_delay.o \
./Core/Src/global.o \
./Core/Src/main.o \
./Core/Src/realtime.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/system_update.o \
./Core/Src/wdt.o 

C_DEPS += \
./Core/Src/bootloader.d \
./Core/Src/clock.d \
./Core/Src/ds1302.d \
./Core/Src/dwt_delay.d \
./Core/Src/global.d \
./Core/Src/main.d \
./Core/Src/realtime.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/system_update.d \
./Core/Src/wdt.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/BMS_Jihong" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Display/Inc" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Led_Error" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/VCU_State" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network/Sim" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network/Mqtt" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Storage" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Controller_Slider/Inc" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Boat/Log" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Boat/Frame" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/ModbusRTU" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/GNSS" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/bootloader.cyclo ./Core/Src/bootloader.d ./Core/Src/bootloader.o ./Core/Src/bootloader.su ./Core/Src/clock.cyclo ./Core/Src/clock.d ./Core/Src/clock.o ./Core/Src/clock.su ./Core/Src/ds1302.cyclo ./Core/Src/ds1302.d ./Core/Src/ds1302.o ./Core/Src/ds1302.su ./Core/Src/dwt_delay.cyclo ./Core/Src/dwt_delay.d ./Core/Src/dwt_delay.o ./Core/Src/dwt_delay.su ./Core/Src/global.cyclo ./Core/Src/global.d ./Core/Src/global.o ./Core/Src/global.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/realtime.cyclo ./Core/Src/realtime.d ./Core/Src/realtime.o ./Core/Src/realtime.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/system_update.cyclo ./Core/Src/system_update.d ./Core/Src/system_update.o ./Core/Src/system_update.su ./Core/Src/wdt.cyclo ./Core/Src/wdt.d ./Core/Src/wdt.o ./Core/Src/wdt.su

.PHONY: clean-Core-2f-Src

