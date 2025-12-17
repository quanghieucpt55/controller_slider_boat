/*
 * Boat_param.c
 *
 *  Created on: Dec 15, 2025
 *      Author: quang
 */

#include "Boat_param.h"
#include "clock.h"

#define BOAT_CYCLE_NORMAL 5
#define BOAT_CYCLE_FAST 3
#define BOAT_CYCLE_VERY_FAST 1
#define BOAT_CYCLE_SLOW 15
#define BOAT_CYCLE_VERY_SLOW 30

Boat_cycle_param_t boat_cycle_param;

void Boat_param_Init(void)
{
	boat_cycle_param.cycle_cmd[0] = BOAT_CYCLE_FAST;  // CMD1
	boat_cycle_param.cycle_cmd[1] = BOAT_CYCLE_SLOW;     // CMD2
	boat_cycle_param.cycle_cmd[2] = BOAT_CYCLE_SLOW;     // CMD3
	boat_cycle_param.cycle_cmd[3] = BOAT_CYCLE_SLOW;     // CMD4
	boat_cycle_param.cycle_cmd[4] = BOAT_CYCLE_SLOW;     // CMD5
	boat_cycle_param.cycle_cmd[5] = BOAT_CYCLE_SLOW;     // CMD6
	boat_cycle_param.cycle_cmd[6] = BOAT_CYCLE_NORMAL;       // CMD7
	boat_cycle_param.cycle_cmd[7] = BOAT_CYCLE_NORMAL;       // CMD8
	boat_cycle_param.cycle_cmd[8] = BOAT_CYCLE_NORMAL;        // CMD9
	boat_cycle_param.cycle_cmd[9] = BOAT_CYCLE_VERY_SLOW;       // CMD10
	boat_cycle_param.cycle_cmd[10] = BOAT_CYCLE_FAST; // CMD11
	
	// Khởi tạo timer và cờ
	Boat_CmdTimer_Reset();
}

void Boat_CmdTimer_Reset(void)
{
	// Reset timer và clear cờ cho tất cả các CMD
	for (uint8_t i = 0; i < BOAT_CMD_COUNT; i++) {
		boat_cycle_param.lastTimeCmd[i] = millis();
		boat_cycle_param.cmdReadyFlag[i] = 0;
	}
}

// Hàm kiểm tra timer và đặt cờ cho các CMD đã đủ thời gian chu kỳ
void Boat_CmdTimer_Check(void)
{
	int32_t currentTime = millis();
	for (uint8_t idx = 0; idx < BOAT_CMD_COUNT; idx++) {
		int32_t delay_ms = (int32_t)(boat_cycle_param.cycle_cmd[idx] * 1000);
		if (currentTime - boat_cycle_param.lastTimeCmd[idx] >= delay_ms) {
			boat_cycle_param.cmdReadyFlag[idx] = 1;  // Đặt cờ khi đủ thời gian
			boat_cycle_param.lastTimeCmd[idx] = currentTime;  // Reset timer
		}
	}
}
