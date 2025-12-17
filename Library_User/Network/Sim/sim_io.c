#include "stm32f4xx_hal.h"
#include "main.h"

void PWRKEY_Write(uint8_t v)
{
	HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, v);
}
