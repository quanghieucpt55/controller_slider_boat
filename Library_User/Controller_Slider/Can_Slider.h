/*
 * Can_Slider.h
 *
 *  Created on: Nov 12, 2025
 *      Author: quang
 */

#ifndef CONTROLLER_SLIDER_CAN_SLIDER_H_
#define CONTROLLER_SLIDER_CAN_SLIDER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>

// Node ID của Controller Slider
#define CAN_SLIDER_ID 0x01

// Frame ID VCU -> Controller Slider
#define CAN_SLIDER_VCU_ID 0x200 + CAN_SLIDER_ID
// Frmae ID Controller Slider -> VCU
#define CAN_SLIDER_1_ID 0x180 + CAN_SLIDER_ID
#define CAN_SLIDER_2_ID 0x280 + CAN_SLIDER_ID

/* Các trạng thái yêu cầu từ VCU */
typedef union {
    uint8_t raw;
    struct {
        uint8_t forward : 1;  // Bit0
        uint8_t reserve : 1;  // Bit1
        uint8_t brake   : 1;  // Bit2
        uint8_t enable  : 1;  // Bit3
        uint8_t reserved: 4;  // Bit4-7
    };
} Can_Vehicle_Mode_VCU_t;

/* Gói tin VCU gửi tới Controller Slider */
typedef struct {
	Can_Vehicle_Mode_VCU_t vehicle_mode;
	uint8_t speed_high;
	uint8_t speed_low;
} Can_Slider_VCU_t;
extern Can_Slider_VCU_t can_slider_vcu;

/* Các trạng thái hiện tại nhận được từ Controller Slider */
typedef struct {
	uint8_t forward;
	uint8_t reserve;
	uint8_t brake;
} Can_Vehicle_Mode_Slider_t;

/* Gói tin Can Slider 1 */
typedef struct {
	Can_Vehicle_Mode_Slider_t vehicle_mode;
	uint16_t motor_rpm;
	uint8_t motor_temp;
	uint8_t controller_temp;
	uint8_t error_code;
} Can_Slider_1_t;

/* Gói tin Can Slider 2 */
typedef struct {
	float battery_voltage;
	float dc_current;
	uint8_t control_mode; // 0: Accelerator, 1: Can protocol
} Can_Slider_2_t;

/* Gói tin Can Slider */
typedef struct {
	Can_Slider_1_t slider_1;
	Can_Slider_2_t slider_2;
} Can_Slider_t;
extern Can_Slider_t can_slider;

/* Error Code */
typedef enum {
	OVER_CURRENT = 3,
	CONTROLLER_TEMP_HIGH = 4,
	MOTOR_ENCODER_ERROR = 7,
	COMMUNICATION_ERROR = 8,
	UNDER_VOLTAGE_BATTERY = 9,
	OVER_VOLTAGE_BATTERY = 10,
	MOTOR_TEMP_HIGH = 11,
	MOTOR_TEMP_SENSOR_ERROR = 12,
	ACCELERATOR_FAULT = 13
} Can_Slider_Error_Code_t;

/* API Functions */
void Can_Slider_Init(CAN_HandleTypeDef *hcan);
void Can_Slider_Process(Can_Slider_t *can_slider, CAN_RxHeaderTypeDef *hdr, uint8_t *d);
Can_Slider_Error_Code_t Can_Slider_1_Process(Can_Slider_1_t *slider_1, CAN_RxHeaderTypeDef *hdr, uint8_t *d);
void Can_Slider_2_Process(Can_Slider_2_t *slider_2, CAN_RxHeaderTypeDef *hdr, uint8_t *d);
void Can_Vcu_Send_Slider(CAN_HandleTypeDef *hcan);

#ifdef __cplusplus
}
#endif

#endif /* CONTROLLER_SLIDER_CAN_SLIDER_H_ */
