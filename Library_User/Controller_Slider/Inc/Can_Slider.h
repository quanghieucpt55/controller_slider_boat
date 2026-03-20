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
#define CAN_SLIDER_ID 0x00

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
        uint8_t reverse : 1;  // Bit1
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
	uint8_t reverse;
	uint8_t brake;
} Can_Vehicle_Mode_Slider_t;

/* Gói tin Can Slider 1 */
typedef struct {
	Can_Vehicle_Mode_Slider_t vehicle_mode;
	uint16_t motor_rpm;
	uint8_t motor_temp;
	uint8_t controller_temp;
	uint32_t error_code;
} Can_Slider_1_t;

/* Gói tin Can Slider 2 */
typedef struct {
	float battery_voltage;
	float dc_current;
	uint8_t control_mode; // 0: Accelerator, 1: Can protocol
	uint8_t thr_Val;
} Can_Slider_2_t;

/* Gói tin Can Slider */
typedef struct {
	Can_Slider_1_t slider_1;
	Can_Slider_2_t slider_2;
	uint8_t motor_direc; // 0: Neutral, 1: Forward, 2: Reverse
	uint32_t last_motor_rpm;
	uint32_t rpm_accel;
	uint32_t last_time_accel;
	uint8_t raw_err_code;           // mã lỗi thô đang nhận trực tiếp từ controller
	uint32_t effective_error_code;  // bit lỗi hiệu dụng sau khi VCU gộp lỗi trung tâm
	uint32_t effective_warning_code; // bit cảnh báo hiệu dụng do VCU tính từ ngưỡng cảnh báo
	uint8_t effective_raw_err_code; // mã lỗi hiệu dụng ưu tiên cao nhất cho HMI/Display
} Can_Slider_t;
extern Can_Slider_t can_slider;

/* Error Code */
typedef enum {
	/*Nếu báo động vẫn còn sau khi động cơ đã được tháo ra, hãy trả lại nhà máy để sửa chữa. 
	Nếu không, hãy thay thế động cơ.*/
	OVER_CURRENT = 3, //Quá dòng
	/*Vui lòng kiểm tra xem quạt có hoạt động bình thường không và thông gió có thông suốt không.*/
	CONTROLLER_TEMP_HIGH = 4,  //Nhiệt độ bộ điều khiển cao
	/*Bộ mã hóa động cơ đã kích hoạt báo động.
	Động cơ hoặc mạch mã hóa của bộ điều khiển bị hở, hoặc bộ mã hóa động cơ bị hỏng.*/
	MOTOR_ENCODER_ERROR = 7, //Lỗi bộ mã hóa động cơ
	/*Đường truyền thông CAN bất thường.
	Vui lòng kiểm tra xem kết nối của đường truyền thông CAN có chính xác không và
	các tin nhắn điều khiển do VCU gửi đi có chính xác không.*/
	COMMUNICATION_ERROR = 8, //Lỗi truyền thông CAN
	/*Pin yếu, cần sạc ngay*/
	UNDER_VOLTAGE_BATTERY = 9, //Điện áp quá thấp
	/*1. Vui lòng kiểm tra xem bộ pin có bình thường không;
	2. Vui lòng cố gắng giảm chức năng
	phanh tái tạo càng nhiều càng tốt.*/
	OVER_VOLTAGE_BATTERY = 10, //Điện áp quá cao
	/*Đóng động cơ để nguội hoặc
	Tăng chế độ làm mát động cơ.*/
	MOTOR_TEMP_HIGH = 11, //Nhiệt độ động cơ cao
	/*Báo lỗi nhiệt độ động cơ (mạch dây cảm biến nhiệt độ động cơ bị đứt hoặc bị chạm chập).*/
	MOTOR_TEMP_SENSOR_ERROR = 12, //Lỗi cảm biến nhiệt độ động cơ
	/*Vui lòng kiểm tra xem kết nối chân ga có chính xác không; nếu bị hỏng, cần phải mang đi sửa chữa.*/
	ACCELERATOR_FAULT = 13 //Lỗi chân ga
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
