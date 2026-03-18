/*
 * modbus_rtu_interface.c
 * Sử dụng UART2 cho RS485 communication
 */

#include <stdint.h>
#include "modbus_rtu_interface.h"
#include "modbus_slave_comp.h"
#include "main.h"
#include "stm32f4xx_hal.h"

// Ring buffer structure cho TX
typedef struct
{
  uint8_t * buffer;
  uint16_t bufferSize;
  volatile uint32_t head;
  volatile uint32_t tail;
} modbus_ring_buffer_t;

#define BUFFER_UART_SIZE 255
static uint8_t buf_uart_rs485[BUFFER_UART_SIZE];
static modbus_ring_buffer_t tx_ring_buffer_rs485;

// External UART handle từ main.c
extern UART_HandleTypeDef huart2;

/**
 * @brief Khởi tạo UART RS485
 */
void ModbusRTU_UartInit(void)
{
	tx_ring_buffer_rs485.buffer = buf_uart_rs485;
	tx_ring_buffer_rs485.bufferSize = BUFFER_UART_SIZE;
	tx_ring_buffer_rs485.head = 0;
	tx_ring_buffer_rs485.tail = 0;

	/* Enable the UART Data Register not empty Interrupt */
	__HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
}

/**
 * @brief Gửi một byte qua UART RS485 (sử dụng ring buffer)
 */
void ModbusRTU_UartWriteByte(uint8_t c)
{
	int i = (tx_ring_buffer_rs485.head + 1) % tx_ring_buffer_rs485.bufferSize;
	while (i == tx_ring_buffer_rs485.tail);

	tx_ring_buffer_rs485.buffer[tx_ring_buffer_rs485.head] = (uint8_t)c;
	tx_ring_buffer_rs485.head = i;

	__HAL_UART_ENABLE_IT(&huart2, UART_IT_TXE); // Enable UART transmission interrupt
}

/**
 * @brief Gửi mảng dữ liệu qua UART RS485
 */
void ModbusRTU_UartPutArray(uint8_t * frame, uint32_t len)
{
	for(uint32_t i=0;i<len;i++)
	{
		ModbusRTU_UartWriteByte(frame[i]);
	}
}

/**
 * @brief Xử lý UART interrupt cho USART2
 * Hàm này cần được gọi từ USART2_IRQHandler trong stm32f4xx_it.c
 */
void ModbusRTU_UartIsrHandle(void)
{
	UART_HandleTypeDef *huart=&huart2;
	uint32_t isrflags   = READ_REG(huart->Instance->SR);
	uint32_t cr1its     = READ_REG(huart->Instance->CR1);

    /* if DR is not empty and the Rx Int is enabled */
    if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
    {

		huart->Instance->SR;                       /* Read status register */
        uint8_t c = huart->Instance->DR;     /* Read data register */
        ModbusSlaveComp_DecodeMessage(c);

    }

    /*If interrupt is caused due to Transmit Data Register Empty */
    if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
    {
    	if(tx_ring_buffer_rs485.head == tx_ring_buffer_rs485.tail)
		{
		  // Buffer empty, so disable interrupts
		  __HAL_UART_DISABLE_IT(huart, UART_IT_TXE);

		}

    	else
		{
		  // There is more data in the output buffer. Send the next byte
		  unsigned char c = tx_ring_buffer_rs485.buffer[tx_ring_buffer_rs485.tail];
		  tx_ring_buffer_rs485.tail = (tx_ring_buffer_rs485.tail + 1) % tx_ring_buffer_rs485.bufferSize;

		  huart->Instance->SR;
		  huart->Instance->DR = c;

		}
    }
}
