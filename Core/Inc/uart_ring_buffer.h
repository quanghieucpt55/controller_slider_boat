/*
 * uart_ring_buffer.h
 *
 *  Created on: Jan 14, 2025
 *      Author: trank
 */

#ifndef UART_RING_BUFFER_H_
#define UART_RING_BUFFER_H_

#include "cytypes.h"

typedef struct
{
  uint8_t * buffer;
  uint16_t bufferSize;
  volatile uint32_t head;
  volatile uint32_t tail;
} ring_buffer_t;



#endif /* UART_RING_BUFFER_H_ */
