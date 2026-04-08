################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Library_User/DriverFaultConfig/driver_fault_config.c 

OBJS += \
./Library_User/DriverFaultConfig/driver_fault_config.o 

C_DEPS += \
./Library_User/DriverFaultConfig/driver_fault_config.d 


# Each subdirectory must supply rules for building sources it contributes
Library_User/DriverFaultConfig/%.o Library_User/DriverFaultConfig/%.su Library_User/DriverFaultConfig/%.cyclo: ../Library_User/DriverFaultConfig/%.c Library_User/DriverFaultConfig/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/BMS_Jihong" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Display/Inc" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Led_Error" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/VCU_State" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Network" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Network/Sim" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Network/Mqtt" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Storage" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Controller_Slider/Inc" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Boat/Log" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/Boat/Frame" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/ModbusRTU" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/GNSS" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/DriverFaultConfig" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/ADS1115" -I"C:/Users/Admin/STM32CubeIDE/workspace_2.1.1/Controller_Slider/Library_User/IMD" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Library_User-2f-DriverFaultConfig

clean-Library_User-2f-DriverFaultConfig:
	-$(RM) ./Library_User/DriverFaultConfig/driver_fault_config.cyclo ./Library_User/DriverFaultConfig/driver_fault_config.d ./Library_User/DriverFaultConfig/driver_fault_config.o ./Library_User/DriverFaultConfig/driver_fault_config.su

.PHONY: clean-Library_User-2f-DriverFaultConfig

