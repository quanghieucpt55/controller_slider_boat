/*
 * Boat_param.h
 *
 *  Created on: Dec 15, 2025
 *      Author: quang
 */

#ifndef CONTROLLER_SLIDER_INC_BOAT_PARAM_H_
#define CONTROLLER_SLIDER_INC_BOAT_PARAM_H_

#include "cytypes.h"

#define BOAT_CMD_COUNT 11

typedef struct {
	uint8_t cycle_cmd[BOAT_CMD_COUNT];  // Mảng chu kỳ cho 11 CMD (đơn vị: giây)
	int32_t lastTimeCmd[BOAT_CMD_COUNT];  // Thời gian cuối cùng gửi của mỗi CMD (milliseconds)
	uint8_t cmdReadyFlag[BOAT_CMD_COUNT];  // Cờ báo CMD sẵn sàng gửi (1 = sẵn sàng, 0 = chưa)
} Boat_cycle_param_t;
extern Boat_cycle_param_t boat_cycle_param;

// Hàm khởi tạo và quản lý timer CMD
void Boat_param_Init(void);
void Boat_CmdTimer_Reset(void);  // Reset timer và clear cờ cho tất cả các CMD
void Boat_CmdTimer_Check(void);   // Kiểm tra timer và đặt cờ cho các CMD đã đủ thời gian chu kỳ

#endif /* CONTROLLER_SLIDER_INC_BOAT_PARAM_H_ */
