/*
 * modbus_slave_comp.h
 *
 *  Created on: Jan 13, 2025
 *      Author: trank
 */

#ifndef MODBUS_SLAVE_COMP_H_
#define MODBUS_SLAVE_COMP_H_

#include <stdint.h>

/**
 * @brief Trạng thái Modbus RS485 Slave
 */
typedef enum e_modbus_status e_modbus_status;
enum e_modbus_status
{
    Modbus_Init_Status = 0,      // Khởi tạo
    Modbus_Idle_Status,          // Chờ dữ liệu
    Modbus_Receiving_Status,     // Đang nhận dữ liệu
    Modbus_Processing_Status,    // Đang xử lý
    Modbus_Responding_Status,    // Đang phản hồi
    Modbus_Error_Status,         // Lỗi
    Modbus_Ready_Status          // Sẵn sàng
};

/**
 * @brief Lấy trạng thái hiện tại của Modbus RS485
 * @return Trạng thái Modbus
 */
e_modbus_status ModbusSlaveComp_GetStatus(void);

/**
 * @brief Giải mã byte nhận được từ UART
 * @param byte Byte nhận được từ UART RX interrupt
 * Hàm này cần được gọi trong UART RX interrupt handler
 */
void ModbusSlaveComp_DecodeMessage(uint8_t byte);

/**
 * @brief Kiểm tra xem đã nhận đủ message chưa
 */
void ModbusComp_checkIfEndOfMessage(void);

/**
 * @brief Khởi tạo Modbus Slave
 */
void ModbusSlaveComp_Init(void);

/**
 * @brief Chạy Modbus Slave 
 */
void ModbusSlaveComp_Run(void);

#endif /* MODBUS_SLAVE_COMP_H_ */
