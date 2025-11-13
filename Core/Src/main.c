/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "display_curtis.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;

SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
// Button interrupt variables
volatile uint8_t button_down_pressed = 0;
volatile uint8_t button_enter_pressed = 0;
volatile uint8_t button_up_pressed = 0;

volatile uint32_t button_down_time = 0;
volatile uint32_t button_enter_time = 0;
volatile uint32_t button_up_time = 0;

// Button timing constants
#define BUTTON_LONG_PRESS_TIME_MS  300
#define ENTER_LONG_PRESS_TIME_MS  500
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART6_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Hàm debug in ra USB CDC
void debug_print(const char* message) {
  CDC_Transmit_FS((uint8_t*)message, strlen(message));
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN1_Init();
  MX_USB_DEVICE_Init();
  MX_SPI2_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
  BMS_Jikong_Init(&hcan1);
  Can_Slider_Init(&hcan1);
  // Bắt đầu CAN sau khi tất cả filter đã được config
  if (HAL_CAN_Start(&hcan1) != HAL_OK) {
    Error_Handler();
  }
  ST7565_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    static uint32_t display_time = 0;

    process_button();

    // Cập nhật màn hình mỗi 500ms
    if (HAL_GetTick() - display_time > 500) {
      display_time = HAL_GetTick();
      process_menu();
      Can_Vcu_Send_Slider(&hcan1);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 12;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_11TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = ENABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 PD10 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PD11 PD12 PD13 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// GPIO interrupt callback function
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  uint32_t current_time = HAL_GetTick();

  debug_print("*** GPIO INTERRUPT TRIGGERED ***\r\n");

  switch (GPIO_Pin) {
      case GPIO_PIN_11: // DOWN button
          if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_11) == GPIO_PIN_RESET) {
              // Button pressed (LOW)
              button_down_time = current_time;
              button_down_pressed = 1;
          } else {
              // Button released (HIGH)
              button_down_pressed = 0;
          }
          break;

      case GPIO_PIN_12: // ENTER button
          if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_12) == GPIO_PIN_RESET) {
              // Button pressed (LOW)
              button_enter_time = current_time;
              button_enter_pressed = 1;
          } else {
              // Button released (HIGH)
              button_enter_pressed = 0;
          }
          break;

      case GPIO_PIN_13: // UP button
          if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_13) == GPIO_PIN_RESET) {
              // Button pressed (LOW)
              button_up_time = current_time;
              button_up_pressed = 1;
          } else {
              // Button released (HIGH)
              button_up_pressed = 0;
          }
          break;

      default:
          break;
  }
}

void process_button(void) {
  // Xử lý button actions từ interrupt
  static uint8_t last_button_enter = 0;
  static uint8_t last_button_up = 0;
  static uint8_t last_button_down = 0;

  // UP button
  if (button_up_pressed && !last_button_up) {
      if (current_menu == MENU_THROTTLE_CONTROL) {
          if (throttle_mode == THROTTLE_EDIT_SELECT) {
              // Chế độ chọn thông số - chọn thông số trước đó
              if (throttle_selected_param > 0) {
                  throttle_selected_param--;
              } else {
                  throttle_selected_param = THROTTLE_PARAM_ENABLE; // Quay vòng
              }
              debug_print("Parameter selection changed\r\n");
          } else if (throttle_mode == THROTTLE_EDIT_VALUE) {
              // Chế độ chỉnh sửa giá trị
              if (throttle_selected_param == THROTTLE_PARAM_SPEED) {
                  // Tăng speed
                  if (throttle_value < 2000) {
                      throttle_value++;
                  }
              } else if (throttle_selected_param == THROTTLE_PARAM_DIRECTION) {
                  // Chuyển direction: Forward -> Reserve -> Forward (bỏ qua Neutral)
                  if (throttle_vehicle_mode_temp.forward && !throttle_vehicle_mode_temp.reserve) {
                      throttle_vehicle_mode_temp.forward = 0;
                      throttle_vehicle_mode_temp.reserve = 1;
                  } else if (throttle_vehicle_mode_temp.reserve && !throttle_vehicle_mode_temp.forward) {
                      throttle_vehicle_mode_temp.reserve = 0;
                      throttle_vehicle_mode_temp.forward = 0;
                  } else {
                      // Reset direction to Forward
                      throttle_vehicle_mode_temp.forward = 1;
                      throttle_vehicle_mode_temp.reserve = 0;
                  }
              } else if (throttle_selected_param == THROTTLE_PARAM_BRAKE) {
                  // Toggle brake
                  throttle_vehicle_mode_temp.brake = !throttle_vehicle_mode_temp.brake;
              } else if (throttle_selected_param == THROTTLE_PARAM_ENABLE) {
                  // Toggle enable
                  throttle_vehicle_mode_temp.enable = !throttle_vehicle_mode_temp.enable;
              }
          } else {
              // Chế độ navigation - chuyển menu forward
              current_menu = (current_menu + 1) % 3;
              debug_print("Menu switched forward\r\n");
          }
      } else {
          // Không phải menu throttle - chuyển menu forward
          current_menu = (current_menu + 1) % 3;
          debug_print("Menu switched forward\r\n");
      }
  } else if (button_up_pressed && HAL_GetTick() - button_up_time > BUTTON_LONG_PRESS_TIME_MS) {
      // Long press - tăng nhanh cho speed
      if (current_menu == MENU_THROTTLE_CONTROL && 
          throttle_mode == THROTTLE_EDIT_VALUE && 
          throttle_selected_param == THROTTLE_PARAM_SPEED && 
          throttle_value < 2000) {
          throttle_value += 10;
          if (throttle_value > 2000) throttle_value = 2000;
          button_up_time = HAL_GetTick();
      }
  }
  last_button_up = button_up_pressed;

  // DOWN button
  if (button_down_pressed && !last_button_down) {
      if (current_menu == MENU_THROTTLE_CONTROL) {
          if (throttle_mode == THROTTLE_EDIT_SELECT) {
              // Chế độ chọn thông số - chọn thông số tiếp theo
              if (throttle_selected_param < THROTTLE_PARAM_ENABLE) {
                  throttle_selected_param++;
              } else {
                  throttle_selected_param = THROTTLE_PARAM_SPEED; // Quay vòng
              }
              debug_print("Parameter selection changed\r\n");
          } else if (throttle_mode == THROTTLE_EDIT_VALUE) {
              // Chế độ chỉnh sửa giá trị
              if (throttle_selected_param == THROTTLE_PARAM_SPEED) {
                  // Giảm speed
                  if (throttle_value > 0) {
                      throttle_value--;
                  }
              } else if (throttle_selected_param == THROTTLE_PARAM_DIRECTION) {
                  // Chuyển direction: Forward -> Neutral -> Reserve -> Forward
                  if (throttle_vehicle_mode_temp.forward) {
                      throttle_vehicle_mode_temp.forward = 0;
                  } else if (throttle_vehicle_mode_temp.reserve) {
                      throttle_vehicle_mode_temp.reserve = 0;
                      throttle_vehicle_mode_temp.forward = 1;
                  } else {
                      throttle_vehicle_mode_temp.reserve = 1;
                  }
              } else if (throttle_selected_param == THROTTLE_PARAM_BRAKE) {
                  // Toggle brake
                  throttle_vehicle_mode_temp.brake = !throttle_vehicle_mode_temp.brake;
              } else if (throttle_selected_param == THROTTLE_PARAM_ENABLE) {
                  // Toggle enable
                  throttle_vehicle_mode_temp.enable = !throttle_vehicle_mode_temp.enable;
              }
          } else {
              // Chế độ navigation - chuyển menu backward
              current_menu = (current_menu - 1 + 3) % 3;
              debug_print("Menu switched backward\r\n");
          }
      } else {
          // Không phải menu throttle - chuyển menu backward
          current_menu = (current_menu - 1 + 3) % 3;
          debug_print("Menu switched backward\r\n");
      }
  } else if (button_down_pressed && HAL_GetTick() - button_down_time > BUTTON_LONG_PRESS_TIME_MS) {
      // Long press - giảm nhanh cho speed
      if (current_menu == MENU_THROTTLE_CONTROL && 
          throttle_mode == THROTTLE_EDIT_VALUE && 
          throttle_selected_param == THROTTLE_PARAM_SPEED && 
          throttle_value > 0) {
          throttle_value -= 10;
          if (throttle_value < 0) throttle_value = 0;
          button_down_time = HAL_GetTick();
      }
  }
  last_button_down = button_down_pressed;

  // ENTER button
  static uint8_t enter_long_press_handled = 0;
  
  if (button_enter_pressed && !last_button_enter) {
      enter_long_press_handled = 0; // Reset flag khi bắt đầu nhấn
      if (current_menu == MENU_THROTTLE_CONTROL) {
          if (throttle_mode == THROTTLE_NAVIGATION) {
              // Lần đầu ấn ENTER - vào chế độ chọn thông số
              throttle_mode = THROTTLE_EDIT_SELECT;
              throttle_selected_param = THROTTLE_PARAM_SPEED; // Reset về thông số đầu tiên
              debug_print("Entered throttle select mode\r\n");
          } else if (throttle_mode == THROTTLE_EDIT_SELECT) {
              // Ấn ENTER lần 2 - vào chế độ chỉnh sửa giá trị
              throttle_mode = THROTTLE_EDIT_VALUE;
              // Khởi tạo biến trung gian từ giá trị gốc
              throttle_vehicle_mode_temp = can_slider_vcu.vehicle_mode;
              // Nếu chọn speed, khởi tạo throttle_value từ speed_high và speed_low
              if (throttle_selected_param == THROTTLE_PARAM_SPEED) {
                  throttle_value = (can_slider_vcu.speed_high << 8) | can_slider_vcu.speed_low;
              }
              debug_print("Entered throttle edit value mode\r\n");
          } else if (throttle_mode == THROTTLE_EDIT_VALUE) {
              // Ấn ENTER lần 3 - lưu giá trị và quay về chế độ chọn
              if (throttle_selected_param == THROTTLE_PARAM_SPEED) {
                  can_slider_vcu.speed_high = throttle_value >> 8;
                  can_slider_vcu.speed_low = throttle_value & 0xFF;
                  debug_print("Speed saved\r\n");
              } else {
                  // Lưu vehicle_mode từ biến trung gian vào can_slider_vcu
                  can_slider_vcu.vehicle_mode = throttle_vehicle_mode_temp;
                  debug_print("Parameter saved\r\n");
              }
              throttle_mode = THROTTLE_EDIT_SELECT; // Quay về chế độ chọn
          }
      }
  } else if (button_enter_pressed && 
             HAL_GetTick() - button_enter_time > ENTER_LONG_PRESS_TIME_MS && 
             !enter_long_press_handled) {
      // Long press ENTER - thoát khỏi chế độ edit (chỉ trigger một lần)
      if (current_menu == MENU_THROTTLE_CONTROL) {
          if (throttle_mode == THROTTLE_EDIT_SELECT || throttle_mode == THROTTLE_EDIT_VALUE) {
              // Thoát về chế độ navigation
              throttle_mode = THROTTLE_NAVIGATION;
              debug_print("Exited throttle edit mode\r\n");
              enter_long_press_handled = 1; // Đánh dấu đã xử lý
          }
      }
  }
  
  if (!button_enter_pressed) {
      enter_long_press_handled = 0; // Reset flag khi thả nút
  }
  
  last_button_enter = button_enter_pressed;
}

void process_menu(void) {
  // Hiển thị menu tương ứng
  switch (current_menu)
  {
  case MENU_CAN_INFO_1:
    display_can_info_1();
    break;
  case MENU_CAN_INFO_2:
    display_can_info_2();
    break;
  case MENU_THROTTLE_CONTROL:
    display_throttle_control();
    break;
  default:
    display_can_info_1();
    break;
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
