#ifndef _MODBUS_RTU_INTERFACE_H_
#define _MODBUS_RTU_INTERFACE_H_

#include <stdint.h>

/*
 * Modbus RTU Interface
 */

/**
 * @brief Khởi tạo UART RS485
 */
void ModbusRTU_UartInit(void);

/**
 * @brief Gửi một byte qua UART RS485
 * @param c Byte cần gửi
 */
void ModbusRTU_UartWriteByte(uint8_t c);

/**
 * @brief Gửi mảng dữ liệu qua UART RS485
 * @param frame Con trỏ đến mảng dữ liệu cần gửi
 * @param len Độ dài dữ liệu cần gửi
 */
void ModbusRTU_UartPutArray(uint8_t * frame, uint32_t len);

/**
 * @brief Xử lý interrupt UART (Rx và Tx interrupt)
 * Hàm này sẽ tự động xử lý cả RX và TX interrupt
 */
void ModbusRTU_UartIsrHandle(void);

#endif /* _MODBUS_RTU_INTERFACE_H_ */
