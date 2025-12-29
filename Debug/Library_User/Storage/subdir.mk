################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Library_User/Storage/24Cxx.c \
../Library_User/Storage/extern_flash.c \
../Library_User/Storage/extern_rom.c \
../Library_User/Storage/soft_i2c_1.c \
../Library_User/Storage/spif.c 

OBJS += \
./Library_User/Storage/24Cxx.o \
./Library_User/Storage/extern_flash.o \
./Library_User/Storage/extern_rom.o \
./Library_User/Storage/soft_i2c_1.o \
./Library_User/Storage/spif.o 

C_DEPS += \
./Library_User/Storage/24Cxx.d \
./Library_User/Storage/extern_flash.d \
./Library_User/Storage/extern_rom.d \
./Library_User/Storage/soft_i2c_1.d \
./Library_User/Storage/spif.d 


# Each subdirectory must supply rules for building sources it contributes
Library_User/Storage/%.o Library_User/Storage/%.su Library_User/Storage/%.cyclo: ../Library_User/Storage/%.c Library_User/Storage/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/BMS_Jihong" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Display/Inc" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Led_Error" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/VCU_State" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network/Sim" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network/Mqtt" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Storage" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Controller_Slider/Inc" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Boat/Log" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Boat/Frame" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Library_User-2f-Storage

clean-Library_User-2f-Storage:
	-$(RM) ./Library_User/Storage/24Cxx.cyclo ./Library_User/Storage/24Cxx.d ./Library_User/Storage/24Cxx.o ./Library_User/Storage/24Cxx.su ./Library_User/Storage/extern_flash.cyclo ./Library_User/Storage/extern_flash.d ./Library_User/Storage/extern_flash.o ./Library_User/Storage/extern_flash.su ./Library_User/Storage/extern_rom.cyclo ./Library_User/Storage/extern_rom.d ./Library_User/Storage/extern_rom.o ./Library_User/Storage/extern_rom.su ./Library_User/Storage/soft_i2c_1.cyclo ./Library_User/Storage/soft_i2c_1.d ./Library_User/Storage/soft_i2c_1.o ./Library_User/Storage/soft_i2c_1.su ./Library_User/Storage/spif.cyclo ./Library_User/Storage/spif.d ./Library_User/Storage/spif.o ./Library_User/Storage/spif.su

.PHONY: clean-Library_User-2f-Storage

