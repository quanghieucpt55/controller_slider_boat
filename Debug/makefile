################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Library_User/VCU_State/subdir.mk
-include Library_User/Storage/subdir.mk
-include Library_User/Network/Sim/subdir.mk
-include Library_User/Network/Mqtt/subdir.mk
-include Library_User/Network/subdir.mk
-include Library_User/ModbusRTU/subdir.mk
-include Library_User/Led_Error/subdir.mk
-include Library_User/GNSS/subdir.mk
-include Library_User/Display/Src/subdir.mk
-include Library_User/Controller_Slider/Src/subdir.mk
-include Library_User/Boat/Log/subdir.mk
-include Library_User/Boat/Frame/subdir.mk
-include Library_User/BMS_Jihong/subdir.mk
-include Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Controller_Slider
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
Controller_Slider.elf \

MAP_FILES += \
Controller_Slider.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
Controller_Slider.list \

OBJCOPY_HEX += \
Controller_Slider.hex \


# All Target
all: main-build

# Main-build Target
main-build: Controller_Slider.elf secondary-outputs

# Tool invocations
Controller_Slider.elf Controller_Slider.map: $(OBJS) $(USER_OBJS) C:\Users\quang\STM32CubeIDE\workspace_1.19.0\Controller_Slider\STM32F407VETX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "Controller_Slider.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"C:\Users\quang\STM32CubeIDE\workspace_1.19.0\Controller_Slider\STM32F407VETX_FLASH.ld" --specs=nosys.specs -Wl,-Map="Controller_Slider.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -u _printf_float -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

Controller_Slider.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "Controller_Slider.list"
	@echo 'Finished building: $@'
	@echo ' '

Controller_Slider.hex: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "Controller_Slider.hex"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Controller_Slider.elf Controller_Slider.hex Controller_Slider.list Controller_Slider.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
