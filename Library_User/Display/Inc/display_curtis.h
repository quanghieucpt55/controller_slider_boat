/*
 * dipslay_curtis.h
 *
 *  Created on: Oct 22, 2025
 */

#ifndef DISPLAY_INC_DIPSLAY_CURTIS_H_
#define DISPLAY_INC_DIPSLAY_CURTIS_H_

#include "ST7565.h"
#include "VCU_State.h" 

// Button timing constants
#define BUTTON_LONG_PRESS_TIME_MS  300
#define ENTER_LONG_PRESS_TIME_MS  500

extern volatile uint8_t button_down_pressed;
extern volatile uint8_t button_enter_pressed;
extern volatile uint8_t button_up_pressed;

extern volatile uint32_t button_down_time;
extern volatile uint32_t button_enter_time;
extern volatile uint32_t button_up_time;

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
    IO_PARAM_SELECT_MODE = 0,    // Relay Select Mode (PE2)
    IO_PARAM_CONTACTOR = 1,   // Relay Contactor (PE3)
    IO_PARAM_LIGHT = 2  // Relay Light (PE6)
} io_param_t;

// Chế độ hiển thị menu
typedef enum {
    MENU_MODE_OVERVIEW = 0,  // Chế độ tổng quan (hiển thị thông tin cơ bản)
    MENU_MODE_DETAIL = 1     // Chế độ chi tiết (hiển thị đầy đủ các menu con)
} menu_display_mode_t;

// Các hàm display menu chính (overview)
void display_main_init(void);
void display_main_waiting(void);
void display_main_can(void);
void display_main_physical(void);
void display_main_charge(void);
void display_main_idle(void);
void display_main_error(void);

// Các hàm display menu chi tiết (detail)
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
