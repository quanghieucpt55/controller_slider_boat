/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern CAN_HandleTypeDef hcan1;
extern UART_HandleTypeDef huart6;
extern IWDG_HandleTypeDef hiwdg;
extern TIM_HandleTypeDef htim2;

#define uart_sim &huart6
#define htimer &htim2
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern void debug_print(const char* message);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SPI_FLASH_CS_Pin GPIO_PIN_4
#define SPI_FLASH_CS_GPIO_Port GPIOA
#define SPI1_FLASH_SCK_Pin GPIO_PIN_5
#define SPI1_FLASH_SCK_GPIO_Port GPIOA
#define SPI1_FLASH_MISO_Pin GPIO_PIN_6
#define SPI1_FLASH_MISO_GPIO_Port GPIOA
#define SPI1_FLASH_MOSI_Pin GPIO_PIN_7
#define SPI1_FLASH_MOSI_GPIO_Port GPIOA
#define PWR_KEY_Pin GPIO_PIN_15
#define PWR_KEY_GPIO_Port GPIOD
#define UART6_SIM_TX_Pin GPIO_PIN_6
#define UART6_SIM_TX_GPIO_Port GPIOC
#define UART6_SIM_RX_Pin GPIO_PIN_7
#define UART6_SIM_RX_GPIO_Port GPIOC
#define SOFT_I2C_SCL_Pin GPIO_PIN_6
#define SOFT_I2C_SCL_GPIO_Port GPIOB
#define SOFT_I2C_SDA_Pin GPIO_PIN_7
#define SOFT_I2C_SDA_GPIO_Port GPIOB
#define RST_RTC_Pin GPIO_PIN_8
#define RST_RTC_GPIO_Port GPIOB
#define SDA_RTC_Pin GPIO_PIN_9
#define SDA_RTC_GPIO_Port GPIOB
#define SCK_RTC_Pin GPIO_PIN_0
#define SCK_RTC_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */
#define RELAY_SELECT_MODE_Pin GPIO_PIN_2
#define RELAY_SELECT_MODE_GPIO_Port GPIOE
#define RELAY_CONTACTOR_Pin GPIO_PIN_3
#define RELAY_CONTACTOR_GPIO_Port GPIOE
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
