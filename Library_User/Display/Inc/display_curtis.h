/*
 * dipslay_curtis.h
 *
 *  Created on: Oct 22, 2025
 */

#ifndef DISPLAY_INC_DIPSLAY_CURTIS_H_
#define DISPLAY_INC_DIPSLAY_CURTIS_H_

#include "ST7565.h"
#include "Can_Slider.h"
#include "jikong_can.h"


// Button timing constants
#define BUTTON_LONG_PRESS_TIME_MS  300
#define ENTER_LONG_PRESS_TIME_MS  500

extern volatile uint8_t button_down_pressed;
extern volatile uint8_t button_enter_pressed;
extern volatile uint8_t button_up_pressed;

extern volatile uint32_t button_down_time;
extern volatile uint32_t button_enter_time;
extern volatile uint32_t button_up_time;

// Menu hệ thống
typedef enum {
    MENU_CAN_INFO_1 = 0, // Hiển thị các thông số giám sát Slider1
	MENU_CAN_INFO_2 = 1, // Hiển thị các thông số giám sát Slider2
    MENU_THROTTLE_CONTROL = 2, // Chỉnh sửa throttle command
    MENU_BMS_INFO = 3, // Hiển thị các thông số giám sát BMS (BATT_ST1, CELL_VOLT, CELL_TEMP)
	MENU_ALM_BMS = 4, // Hiển thị cảnh báo BMS
    MENU_IO_INFO = 5, // Hiển thị các thông số giám sát IO
    MENU_BMS_BATT_ST2 = 6, // Hiển thị dung lượng và số chu kỳ
    MENU_BMS_ALL_TEMP = 7, // Hiển thị 5 cảm biến nhiệt độ
    MENU_BMS_ERR_INFO = 8, // Hiển thị lỗi bên trong BMS
    MENU_BMS_INFO_SYS = 9, // Hiển thị thông tin hệ thống (runtime, SOH)
    MENU_BMS_SW_STA = 10, // Hiển thị trạng thái MOS
    MENU_BMS_CELLVOL = 11, // Hiển thị điện áp cell 1-8
    MENU_BMS_CELLVOL_2 = 12, // Hiển thị điện áp cell 9-16
    MENU_BMS_CHG_INFO = 13, // Hiển thị thông tin yêu cầu sạc
} menu_state_t;

typedef enum {
    THROTTLE_NAVIGATION = 0,  // Chế độ điều hướng
    THROTTLE_EDIT_SELECT = 1, // Chế độ chọn thông số để chỉnh sửa
    THROTTLE_EDIT_VALUE = 2   // Chế độ chỉnh sửa giá trị thông số
} throttle_mode_t;

typedef enum {
    THROTTLE_PARAM_BRAKE = 0,      // Thông số Brake
    THROTTLE_PARAM_ENABLE = 1,      // Thông số Enable
    THROTTLE_PARAM_DIRECTION = 2,  // Thông số Direction
    THROTTLE_PARAM_SPEED = 3      // Thông số Speed
} throttle_param_t;

typedef enum {
    IO_NAVIGATION = 0,  // Chế độ điều hướng
    IO_EDIT_SELECT = 1, // Chế độ chọn relay để chỉnh sửa
    IO_EDIT_VALUE = 2   // Chế độ chỉnh sửa trạng thái relay
} io_mode_t;

typedef enum {
    IO_PARAM_AC = 0,    // Relay A/C (PE2)
    IO_PARAM_FAN = 1,   // Relay Fan (PE3)
    IO_PARAM_LIGHT = 2  // Relay Light (PE6)
} io_param_t;

// Các hàm display
void display_can_info_1(void);
void display_can_info_2(void);
void display_throttle_control(void);
void display_bms_info(void);
void display_alm_bms(void);
void display_io_info(void);
void display_bms_batt_st2(void);
void display_bms_all_temp(void);
void display_bms_err_info(void);
void display_bms_info_sys(void);
void display_bms_sw_sta(void);
void display_bms_cellvol(void);
void display_bms_cellvol_2(void);
void display_bms_chg_info(void);

#endif /* DISPLAY_INC_DIPSLAY_CURTIS_H_ */
