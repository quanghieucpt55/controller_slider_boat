################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Library_User/Network/Sim/sim.c \
../Library_User/Network/Sim/sim_io.c \
../Library_User/Network/Sim/sim_utils.c 

OBJS += \
./Library_User/Network/Sim/sim.o \
./Library_User/Network/Sim/sim_io.o \
./Library_User/Network/Sim/sim_utils.o 

C_DEPS += \
./Library_User/Network/Sim/sim.d \
./Library_User/Network/Sim/sim_io.d \
./Library_User/Network/Sim/sim_utils.d 


# Each subdirectory must supply rules for building sources it contributes
Library_User/Network/Sim/%.o Library_User/Network/Sim/%.su Library_User/Network/Sim/%.cyclo: ../Library_User/Network/Sim/%.c Library_User/Network/Sim/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/BMS_Jihong" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Display/Inc" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Led_Error" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/VCU_State" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network/Sim" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Network/Mqtt" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Storage" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Controller_Slider/Inc" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Boat/Log" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/Boat/Frame" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/ModbusRTU" -I"C:/Users/quang/STM32CubeIDE/workspace_1.19.0/Controller_Slider/Library_User/GNSS" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Library_User-2f-Network-2f-Sim

clean-Library_User-2f-Network-2f-Sim:
	-$(RM) ./Library_User/Network/Sim/sim.cyclo ./Library_User/Network/Sim/sim.d ./Library_User/Network/Sim/sim.o ./Library_User/Network/Sim/sim.su ./Library_User/Network/Sim/sim_io.cyclo ./Library_User/Network/Sim/sim_io.d ./Library_User/Network/Sim/sim_io.o ./Library_User/Network/Sim/sim_io.su ./Library_User/Network/Sim/sim_utils.cyclo ./Library_User/Network/Sim/sim_utils.d ./Library_User/Network/Sim/sim_utils.o ./Library_User/Network/Sim/sim_utils.su

.PHONY: clean-Library_User-2f-Network-2f-Sim

