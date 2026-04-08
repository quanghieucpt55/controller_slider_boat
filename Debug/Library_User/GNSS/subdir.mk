################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Library_User/GNSS/example_usage.c \
../Library_User/GNSS/gnss_uart_dma.c \
../Library_User/GNSS/gps_rmc.c 

OBJS += \
./Library_User/GNSS/example_usage.o \
./Library_User/GNSS/gnss_uart_dma.o \
./Library_User/GNSS/gps_rmc.o 

C_DEPS += \
./Library_User/GNSS/example_usage.d \
./Library_User/GNSS/gnss_uart_dma.d \
./Library_User/GNSS/gps_rmc.d 


# Each subdirectory must supply rules for building sources it contributes
Library_User/GNSS/%.o Library_User/GNSS/%.su Library_User/GNSS/%.cyclo: ../Library_User/GNSS/%.c Library_User/GNSS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/BMS_Jihong" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Display/Inc" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Led_Error" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/VCU_State" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Network" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Network/Sim" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Network/Mqtt" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Storage" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Controller_Slider/Inc" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Boat/Log" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Boat/Frame" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/ModbusRTU" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/GNSS" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/DriverFaultConfig" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/ADS1115" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/IMD" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Library_User-2f-GNSS

clean-Library_User-2f-GNSS:
	-$(RM) ./Library_User/GNSS/example_usage.cyclo ./Library_User/GNSS/example_usage.d ./Library_User/GNSS/example_usage.o ./Library_User/GNSS/example_usage.su ./Library_User/GNSS/gnss_uart_dma.cyclo ./Library_User/GNSS/gnss_uart_dma.d ./Library_User/GNSS/gnss_uart_dma.o ./Library_User/GNSS/gnss_uart_dma.su ./Library_User/GNSS/gps_rmc.cyclo ./Library_User/GNSS/gps_rmc.d ./Library_User/GNSS/gps_rmc.o ./Library_User/GNSS/gps_rmc.su

.PHONY: clean-Library_User-2f-GNSS

