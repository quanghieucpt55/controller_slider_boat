#ifndef _MODBUS_MSG_HANDLE_H_
#define _MODBUS_MSG_HANDLE_H_

#include "modbus_slave_base.h"

/**
 * @brief Xử lý message nhận được từ Modbus Master
 * @param msg Con trỏ đến packet Modbus Slave
 * Hàm này được gọi khi nhận được message từ master.
 */
void SlaverSerial_MessageHandle(Packet_Modbus_Slaver * msg);

/**
 * @brief Cập nhật thông tin hệ thống trước khi gửi phản hồi
 * @param msg Con trỏ đến packet Modbus Slave
 * Hàm này được gọi trước khi chuẩn bị phản hồi cho master.
 */
void SlaverSerial_Update(Packet_Modbus_Slaver * msg);

#endif /* _MODBUS_MSG_HANDLE_H_ */
