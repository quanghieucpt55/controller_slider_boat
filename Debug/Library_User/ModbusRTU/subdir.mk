################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Library_User/ModbusRTU/modbus_msg_handle.c \
../Library_User/ModbusRTU/modbus_rtu_interface.c \
../Library_User/ModbusRTU/modbus_slave_base.c \
../Library_User/ModbusRTU/modbus_slave_comp.c 

OBJS += \
./Library_User/ModbusRTU/modbus_msg_handle.o \
./Library_User/ModbusRTU/modbus_rtu_interface.o \
./Library_User/ModbusRTU/modbus_slave_base.o \
./Library_User/ModbusRTU/modbus_slave_comp.o 

C_DEPS += \
./Library_User/ModbusRTU/modbus_msg_handle.d \
./Library_User/ModbusRTU/modbus_rtu_interface.d \
./Library_User/ModbusRTU/modbus_slave_base.d \
./Library_User/ModbusRTU/modbus_slave_comp.d 


# Each subdirectory must supply rules for building sources it contributes
Library_User/ModbusRTU/%.o Library_User/ModbusRTU/%.su Library_User/ModbusRTU/%.cyclo: ../Library_User/ModbusRTU/%.c Library_User/ModbusRTU/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/BMS_Jihong" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Display/Inc" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Led_Error" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/VCU_State" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network/Sim" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network/Mqtt" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Storage" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Controller_Slider/Inc" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Boat/Log" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Boat/Frame" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/ModbusRTU" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/GNSS" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/DriverFaultConfig" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Library_User-2f-ModbusRTU

clean-Library_User-2f-ModbusRTU:
	-$(RM) ./Library_User/ModbusRTU/modbus_msg_handle.cyclo ./Library_User/ModbusRTU/modbus_msg_handle.d ./Library_User/ModbusRTU/modbus_msg_handle.o ./Library_User/ModbusRTU/modbus_msg_handle.su ./Library_User/ModbusRTU/modbus_rtu_interface.cyclo ./Library_User/ModbusRTU/modbus_rtu_interface.d ./Library_User/ModbusRTU/modbus_rtu_interface.o ./Library_User/ModbusRTU/modbus_rtu_interface.su ./Library_User/ModbusRTU/modbus_slave_base.cyclo ./Library_User/ModbusRTU/modbus_slave_base.d ./Library_User/ModbusRTU/modbus_slave_base.o ./Library_User/ModbusRTU/modbus_slave_base.su ./Library_User/ModbusRTU/modbus_slave_comp.cyclo ./Library_User/ModbusRTU/modbus_slave_comp.d ./Library_User/ModbusRTU/modbus_slave_comp.o ./Library_User/ModbusRTU/modbus_slave_comp.su

.PHONY: clean-Library_User-2f-ModbusRTU

