/*
 * display_curtis.c
 *
 *  Created on: Oct 22, 2025
 *      Author: quang
 */

#include "display_curtis.h"

uint16_t throttle_value = 0;
Can_Vehicle_Mode_VCU_t throttle_vehicle_mode_temp = {0};

menu_state_t current_menu = MENU_CAN_INFO_1;

throttle_mode_t throttle_mode = THROTTLE_NAVIGATION;
throttle_param_t throttle_selected_param = THROTTLE_PARAM_SPEED;

// Hàm hiển thị thông tin CAN lên màn hình ST7565
void display_can_info_1(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);

    // Hàng 1: Forward (trái) và Reserve (phải)
    char forward_state_text[15];
    switch (can_slider.slider_1.vehicle_mode.forward)
    {
		case 0: strcpy(forward_state_text, "Forwd: OFF"); break;
		case 1: strcpy(forward_state_text, "Forwd: ON"); break;
		default: strcpy(forward_state_text, "Forwd: ERROR"); break;
    }
    ST7565_drawstring_anywhere(5, 5, forward_state_text);

    char reserve_state_text[15];
    switch (can_slider.slider_1.vehicle_mode.reserve)
    {
		case 0: strcpy(reserve_state_text, "Resv: OFF"); break;
		case 1: strcpy(reserve_state_text, "Resv: ON"); break;
		default: strcpy(reserve_state_text, "Resv: ERROR"); break;
    }
    ST7565_drawstring_anywhere(70, 5, reserve_state_text);

    char brake_state_text[15];
    switch (can_slider.slider_1.vehicle_mode.brake)
    {
		case 0: strcpy(brake_state_text, "Brake: OFF"); break;
		case 1: strcpy(brake_state_text, "Brake: ON"); break;
		default: strcpy(brake_state_text, "Brake: ERROR"); break;
    }
    ST7565_drawstring_anywhere(5, 20, brake_state_text);

    char motor_rpm_text[15];
    sprintf(motor_rpm_text, "RPM: %u", can_slider.slider_1.motor_rpm);
    ST7565_drawstring_anywhere(70, 20, motor_rpm_text);

    char motor_temp_text[15];
    sprintf(motor_temp_text, "Motor temp: %u", can_slider.slider_1.motor_temp);
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(motor_temp_text)/2)*6), 35, motor_temp_text);

    char con_temp_text[15];
    sprintf(con_temp_text, "Controller temp: %u", can_slider.slider_1.controller_temp);
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(con_temp_text)/2)*6), 50, con_temp_text);

    // Cập nhật màn hình
    updateDisplay();
}

void display_can_info_2(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);

    // Hiển thị lỗi
    char error_string[100];
    switch (can_slider.slider_1.error_code) 
    {
        case OVER_CURRENT: strcpy(error_string, "Error: Cur_Over"); break;
        case CONTROLLER_TEMP_HIGH: strcpy(error_string, "Error: Con_Temp_High"); break;
        case MOTOR_ENCODER_ERROR: strcpy(error_string, "Error: Enc_Error"); break;
        case COMMUNICATION_ERROR: strcpy(error_string, "Error: Com_Error"); break;
        case UNDER_VOLTAGE_BATTERY: strcpy(error_string, "Error: Bat_Volt_Low"); break;
        case OVER_VOLTAGE_BATTERY: strcpy(error_string, "Error: Bat_Volt_High"); break;
        case MOTOR_TEMP_HIGH: strcpy(error_string, "Error: Motor_Temp_High"); break;
        case MOTOR_TEMP_SENSOR_ERROR: strcpy(error_string, "Error: Motor_Temp_Err"); break;
        case ACCELERATOR_FAULT: strcpy(error_string, "Error: Acc_Err"); break;
        default: strcpy(error_string, "Error: ERROR"); break;
    }
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(error_string)/2)*6), 5, error_string);

    char control_mode_text[15];
    switch (can_slider.slider_2.control_mode)
    {
        case 0: strcpy(control_mode_text, "Mode: Accelerator"); break;
        case 1: strcpy(control_mode_text, "Mode: Can protocol"); break;
        default: strcpy(control_mode_text, "Mode: ERROR"); break;
    }
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(control_mode_text)/2)*6), 20, control_mode_text);

    char battery_voltage_text[15];
    sprintf(battery_voltage_text, "Bat Voltage: %.1f V", can_slider.slider_2.battery_voltage);
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(battery_voltage_text)/2)*6), 35, battery_voltage_text);

    char dc_current_text[15];
    sprintf(dc_current_text, "DC Current: %.1f A", can_slider.slider_2.dc_current);
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(dc_current_text)/2)*6), 50, dc_current_text);

    // Cập nhật màn hình
    updateDisplay();
}

// Hàm hiển thị menu throttle control
void display_throttle_control(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    // Biến để kiểm tra nhấp nháy (dựa trên tick)
    uint32_t current_time = HAL_GetTick();
    uint8_t blink_state = ((current_time / 500) % 2); // Nhấp nháy mỗi 500ms
    
    // Target throttle value
    char target_text[20];
    sprintf(target_text, "Speed: %d|%d rpm", can_slider_vcu.speed_high << 8 | can_slider_vcu.speed_low, throttle_value);
    uint8_t speed_x = (LCD_WIDTH/2)-((strlen(target_text)/2)*6);
    uint8_t speed_y = 20;
    ST7565_drawstring_anywhere(speed_x, speed_y, target_text);
    
    // Vẽ indicator cho Speed nếu đang được chọn
    if (throttle_mode == THROTTLE_EDIT_SELECT && throttle_selected_param == THROTTLE_PARAM_SPEED) {
        if (blink_state) {
            ST7565_drawrect(speed_x - 2, speed_y - 1, strlen(target_text) * 6 + 4, 10, 1);
        }
    } else if (throttle_mode == THROTTLE_EDIT_VALUE && throttle_selected_param == THROTTLE_PARAM_SPEED) {
        // Chế độ edit giá trị - luôn hiển thị viền
        ST7565_drawrect(speed_x - 2, speed_y - 1, strlen(target_text) * 6 + 4, 10, 1);
    }

    char direction_text[20];
    // Sử dụng biến trung gian nếu đang ở chế độ edit
    Can_Vehicle_Mode_VCU_t *display_vehicle_mode = (throttle_mode == THROTTLE_EDIT_VALUE) ? 
                                                     &throttle_vehicle_mode_temp : 
                                                     &can_slider_vcu.vehicle_mode;
    if (display_vehicle_mode->forward && !display_vehicle_mode->reserve) {
        strcpy(direction_text, "Direction: Forward");
    } else if (display_vehicle_mode->reserve && !display_vehicle_mode->forward) {
        strcpy(direction_text, "Direction: Reserve");
    } else {
        strcpy(direction_text, "Direction: Neutral");
    }
    uint8_t direction_x = (LCD_WIDTH/2)-((strlen(direction_text)/2)*6);
    uint8_t direction_y = 5;
    ST7565_drawstring_anywhere(direction_x, direction_y, direction_text);
    
    // Vẽ indicator cho Direction nếu đang được chọn
    if (throttle_mode == THROTTLE_EDIT_SELECT && throttle_selected_param == THROTTLE_PARAM_DIRECTION) {
        if (blink_state) {
            ST7565_drawrect(direction_x - 2, direction_y - 1, strlen(direction_text) * 6 + 4, 10, 1);
        }
    } else if (throttle_mode == THROTTLE_EDIT_VALUE && throttle_selected_param == THROTTLE_PARAM_DIRECTION) {
        ST7565_drawrect(direction_x - 2, direction_y - 1, strlen(direction_text) * 6 + 4, 10, 1);
    }
    
    char brake_text[15];
    if (display_vehicle_mode->brake) {
        strcpy(brake_text, "Brake: ON");
    } else {
        strcpy(brake_text, "Brake: OFF");
    }
    uint8_t brake_x = 5;
    uint8_t brake_y = 35;
    ST7565_drawstring_anywhere(brake_x, brake_y, brake_text);
    
    // Vẽ indicator cho Brake nếu đang được chọn
    if (throttle_mode == THROTTLE_EDIT_SELECT && throttle_selected_param == THROTTLE_PARAM_BRAKE) {
        if (blink_state) {
            ST7565_drawrect(brake_x - 2, brake_y - 1, strlen(brake_text) * 6 + 4, 10, 1);
        }
    } else if (throttle_mode == THROTTLE_EDIT_VALUE && throttle_selected_param == THROTTLE_PARAM_BRAKE) {
        ST7565_drawrect(brake_x - 2, brake_y - 1, strlen(brake_text) * 6 + 4, 10, 1);
    }

    char enable_text[15];
    if (display_vehicle_mode->enable) {
        strcpy(enable_text, "En: ON");
    } else {
        strcpy(enable_text, "En: OFF");
    }
    uint8_t enable_x = 70;
    uint8_t enable_y = 35;
    ST7565_drawstring_anywhere(enable_x, enable_y, enable_text);
    
    // Vẽ indicator cho Enable nếu đang được chọn
    if (throttle_mode == THROTTLE_EDIT_SELECT && throttle_selected_param == THROTTLE_PARAM_ENABLE) {
        if (blink_state) {
            ST7565_drawrect(enable_x - 2, enable_y - 1, strlen(enable_text) * 6 + 4, 10, 1);
        }
    } else if (throttle_mode == THROTTLE_EDIT_VALUE && throttle_selected_param == THROTTLE_PARAM_ENABLE) {
        ST7565_drawrect(enable_x - 2, enable_y - 1, strlen(enable_text) * 6 + 4, 10, 1);
    }
    
    // Mode indicator
    if (throttle_mode == THROTTLE_EDIT_SELECT) {
        // Chế độ chọn thông số
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("UP/DN:Sel EN:Edit")/2)*6), 50, "UP/DN:Sel EN:Edit");
    } else if (throttle_mode == THROTTLE_EDIT_VALUE) {
        // Chế độ chỉnh sửa giá trị
        char edit_hint[30];
        if (throttle_selected_param == THROTTLE_PARAM_SPEED) {
            strcpy(edit_hint, "UP/DN:Change EN:Save");
        } else {
            strcpy(edit_hint, "UP/DN:Toggle EN:Save");
        }
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(edit_hint)/2)*6), 50, edit_hint);
    } else {
        // Chế độ navigation
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ENTER:Edit Throttle")/2)*6), 50, "ENTER:Edit Throttle");
    }
    
    // Cập nhật màn hình
    updateDisplay();
}
