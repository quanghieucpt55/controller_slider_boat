#include "gnss_uart_dma.h"
#include "gps_rmc.h"
#include <string.h>

// ====================== cấu hình ======================
#ifndef GNSS_UART_DMA_RX_BUF_SIZE
#define GNSS_UART_DMA_RX_BUF_SIZE 256u
#endif

// ====================== biến nội bộ ======================
static UART_HandleTypeDef *s_gnss_uart = NULL;
static uint8_t s_rx_buf[GNSS_UART_DMA_RX_BUF_SIZE];
static volatile uint16_t s_old_pos = 0;

UART_HandleTypeDef *GNSS_UART_DMA_GetHandle(void)
{
    return s_gnss_uart;
}

static void gnss_uart_dma_consume_new_bytes(void)
{
    UART_HandleTypeDef *huart = s_gnss_uart;
    if (huart == NULL) return;
    if (huart->hdmarx == NULL) return;

    uint16_t buf_size = (uint16_t)sizeof(s_rx_buf);
    uint16_t new_pos = (uint16_t)(buf_size - (uint16_t)__HAL_DMA_GET_COUNTER(huart->hdmarx));
    uint16_t old_pos = (uint16_t)s_old_pos;

    if (new_pos == old_pos) return;

    if (new_pos > old_pos) {
        for (uint16_t i = old_pos; i < new_pos; i++) {
            gps_rmc_feed_byte(s_rx_buf[i]);
        }
    } else {
        // wrap-around
        for (uint16_t i = old_pos; i < buf_size; i++) {
            gps_rmc_feed_byte(s_rx_buf[i]);
        }
        for (uint16_t i = 0; i < new_pos; i++) {
            gps_rmc_feed_byte(s_rx_buf[i]);
        }
    }

    s_old_pos = new_pos;
}

HAL_StatusTypeDef GNSS_UART_DMA_Start(UART_HandleTypeDef *huart)
{
    if (huart == NULL) return HAL_ERROR;
    if (huart->hdmarx == NULL) {
        return HAL_ERROR;
    }

    // Nếu đã start trước đó thì dừng để restart
    if (s_gnss_uart != NULL) {
        (void)GNSS_UART_DMA_Stop();
    }

    s_gnss_uart = huart;
    memset(s_rx_buf, 0, sizeof(s_rx_buf));
    s_old_pos = 0;

    gps_rmc_init();

    // Start RX DMA (circular)
    HAL_StatusTypeDef st = HAL_UART_Receive_DMA(huart, s_rx_buf, (uint16_t)sizeof(s_rx_buf));
    if (st != HAL_OK) return st;

    // Tắt ngắt DMA (HT/TC) để giảm tải IRQ
    if (huart->hdmarx != NULL) {
        __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT);
        __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_TC);
    }

    return HAL_OK;
}

HAL_StatusTypeDef GNSS_UART_DMA_Stop(void)
{
    if (s_gnss_uart == NULL) return HAL_OK;

    // Dừng DMA RX hiện tại
    HAL_StatusTypeDef st = HAL_UART_DMAStop(s_gnss_uart);
    s_gnss_uart = NULL;
    s_old_pos = 0;
    return st;
}

void GNSS_UART_DMA_Task(void)
{
    // Lấy byte mới từ DMA ring-buffer
    gnss_uart_dma_consume_new_bytes();

    // Parse các câu đã gom
    (void)gps_rmc_process();
}
