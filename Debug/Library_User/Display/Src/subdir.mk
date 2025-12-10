################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Library_User/Display/Src/Display_Curtis.c \
../Library_User/Display/Src/ST7565.c \
../Library_User/Display/Src/glcdfont.c 

OBJS += \
./Library_User/Display/Src/Display_Curtis.o \
./Library_User/Display/Src/ST7565.o \
./Library_User/Display/Src/glcdfont.o 

C_DEPS += \
./Library_User/Display/Src/Display_Curtis.d \
./Library_User/Display/Src/ST7565.d \
./Library_User/Display/Src/glcdfont.d 


# Each subdirectory must supply rules for building sources it contributes
Library_User/Display/Src/%.o Library_User/Display/Src/%.su Library_User/Display/Src/%.cyclo: ../Library_User/Display/Src/%.c Library_User/Display/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/BMS_Jihong" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Controller_Slider" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Display/Inc" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Led_Error" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/VCU_State" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Library_User-2f-Display-2f-Src

clean-Library_User-2f-Display-2f-Src:
	-$(RM) ./Library_User/Display/Src/Display_Curtis.cyclo ./Library_User/Display/Src/Display_Curtis.d ./Library_User/Display/Src/Display_Curtis.o ./Library_User/Display/Src/Display_Curtis.su ./Library_User/Display/Src/ST7565.cyclo ./Library_User/Display/Src/ST7565.d ./Library_User/Display/Src/ST7565.o ./Library_User/Display/Src/ST7565.su ./Library_User/Display/Src/glcdfont.cyclo ./Library_User/Display/Src/glcdfont.d ./Library_User/Display/Src/glcdfont.o ./Library_User/Display/Src/glcdfont.su

.PHONY: clean-Library_User-2f-Display-2f-Src

