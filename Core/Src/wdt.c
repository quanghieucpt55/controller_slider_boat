/*
 * wdt.c
 *
 *  Created on: Feb 24, 2025
 *      Author: trank
 */


#include "stm32f4xx_hal.h"
#include "main.h"

void Wdt_Reset(void)
{
	HAL_IWDG_Refresh(&hiwdg);
}
