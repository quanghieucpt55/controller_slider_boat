/*
 * dipslay_curtis.h
 *
 *  Created on: Oct 22, 2025
 */

#ifndef DISPLAY_INC_DIPSLAY_CURTIS_H_
#define DISPLAY_INC_DIPSLAY_CURTIS_H_

#include "ST7565.h"
#include "Can_Slider.h"

// Menu hệ thống
typedef enum {
    MENU_CAN_INFO_1 = 0, // Hiển thị các thông số giám sát Slider1
	MENU_CAN_INFO_2 = 1, // Hiển thị các thông số giám sát Slider2
    MENU_THROTTLE_CONTROL = 2, // Chỉnh sửa throttle command
} menu_state_t;

typedef enum {
    THROTTLE_NAVIGATION = 0,  // Chế độ điều hướng
    THROTTLE_EDIT_SELECT = 1, // Chế độ chọn thông số để chỉnh sửa
    THROTTLE_EDIT_VALUE = 2   // Chế độ chỉnh sửa giá trị thông số
} throttle_mode_t;

typedef enum {
    THROTTLE_PARAM_SPEED = 0,      // Thông số Speed
    THROTTLE_PARAM_DIRECTION = 1,  // Thông số Direction
    THROTTLE_PARAM_BRAKE = 2,      // Thông số Brake
    THROTTLE_PARAM_ENABLE = 3      // Thông số Enable
} throttle_param_t;

extern menu_state_t current_menu;
extern throttle_mode_t throttle_mode;
extern throttle_param_t throttle_selected_param;

// Biến lưu giá trị chỉnh tạm thời
extern uint16_t throttle_value;
extern Can_Vehicle_Mode_VCU_t throttle_vehicle_mode_temp; // Biến trung gian cho vehicle_mode

// Các hàm display
void display_can_info_1(void);
void display_can_info_2(void);
void display_throttle_control(void);

#endif /* DISPLAY_INC_DIPSLAY_CURTIS_H_ */
