#pragma once
/**
 * Tích hợp GNSS UART RX DMA CIRCULAR + polling NDTR (HAL).
 * - DMA RX chạy vòng liên tục vào 1 buffer (ring-buffer)
 * - Không dùng UART IDLE interrupt
 * - Main/task: đọc phần byte mới theo NDTR, feed parser và parse RMC
 */

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Gắn UART GNSS và start RX DMA (circular).
 * Tự gọi gps_rmc_init() để reset parser.
 */
HAL_StatusTypeDef GNSS_UART_DMA_Start(UART_HandleTypeDef *huart);

/** @brief Dừng DMA RX UART GNSS (nếu đang chạy). */
HAL_StatusTypeDef GNSS_UART_DMA_Stop(void);

/**
 * @brief Gọi trong main/task để lấy byte mới + parse các câu đã gom.
 */
void GNSS_UART_DMA_Task(void);

/** @brief Lấy UART GNSS hiện tại (NULL nếu chưa start). */
UART_HandleTypeDef *GNSS_UART_DMA_GetHandle(void);

#ifdef __cplusplus
}
#endif

