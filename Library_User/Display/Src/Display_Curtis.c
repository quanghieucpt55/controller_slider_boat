/*
 * display_curtis.c
 *
 *  Created on: Oct 22, 2025
 *      Author: quang
 */

#include "display_curtis.h"

// Button interrupt variables
volatile uint8_t button_down_pressed = 0;
volatile uint8_t button_enter_pressed = 0;
volatile uint8_t button_up_pressed = 0;

volatile uint32_t button_down_time = 0;
volatile uint32_t button_enter_time = 0;
volatile uint32_t button_up_time = 0;

uint16_t throttle_value = 0;
Can_Slider_VCU_t throttle_vehicle_mode_temp = {0};

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

    char reverse_state_text[15];
    switch (can_slider.slider_1.vehicle_mode.reverse)
    {
		case 0: strcpy(reverse_state_text, "Revs: OFF"); break;
		case 1: strcpy(reverse_state_text, "Revs: ON"); break;
		default: strcpy(reverse_state_text, "Revs: ERROR"); break;
    }
    ST7565_drawstring_anywhere(70, 5, reverse_state_text);

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
        case CONTROLLER_TEMP_HIGH: strcpy(error_string, "Error: Con_Temp_Hi"); break;
        case MOTOR_ENCODER_ERROR: strcpy(error_string, "Error: Enc_Error"); break;
        case COMMUNICATION_ERROR: strcpy(error_string, "Error: Com_Error"); break;
        case UNDER_VOLTAGE_BATTERY: strcpy(error_string, "Error: Bat_Volt_Low"); break;
        case OVER_VOLTAGE_BATTERY: strcpy(error_string, "Error: Bat_Volt_Hi"); break;
        case MOTOR_TEMP_HIGH: strcpy(error_string, "Error: Motor_Temp_Hi"); break;
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

    // Sử dụng biến trung gian nếu đang ở chế độ edit
    Can_Slider_VCU_t *display_vehicle_mode = (throttle_mode == THROTTLE_EDIT_VALUE) ? 
    &throttle_vehicle_mode_temp : 
    &can_slider_vcu;
    
    // Target throttle value
    char target_text[20];
    sprintf(target_text, "Speed: %d rpm", (display_vehicle_mode->speed_high << 8) | display_vehicle_mode->speed_low);
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
    if (display_vehicle_mode->vehicle_mode.forward && !display_vehicle_mode->vehicle_mode.reverse) {
        strcpy(direction_text, "Direction: Forward");
    } else if (display_vehicle_mode->vehicle_mode.reverse && !display_vehicle_mode->vehicle_mode.forward) {
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
    if (display_vehicle_mode->vehicle_mode.brake) {
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
    if (display_vehicle_mode->vehicle_mode.enable) {
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
        ST7565_drawrect((LCD_WIDTH/2)-((strlen("ENTER: Edit Mode")/2)*6) - 2, 48, strlen("ENTER: Edit Mode") * 6 + 4, 12, 1);
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ENTER: Edit Mode")/2)*6), 50, "ENTER: Edit Mode");
    }
    
    // Cập nhật màn hình
    updateDisplay();
}

void display_bms_info(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);

    uint32_t current_time = HAL_GetTick();
    uint8_t blink_state = ((current_time / 500) % 2);
    
    // Vẽ viền xung quanh màn hình
    ST7565_drawrect(2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, 1);
    
    // Tiêu đề
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("BMS Info")/2)*6), 8, "BMS Info");
    
    // Hàng 1: Điện áp tổng (V)
    char voltage_text[20];
    sprintf(voltage_text, "Voltage: %.1f V", bms.batt1.voltage_V);
    ST7565_drawstring_anywhere(5, 20, voltage_text);
    
    // Hàng 2: Dòng điện (A)
    char current_text[20];
    sprintf(current_text, "Current: %.1f A", bms.batt1.current_A);
    ST7565_drawstring_anywhere(5, 30, current_text);
    
    // Hàng 3: SOC (%)
    char soc_text[20];
    sprintf(soc_text, "SOC: %u%%", bms.batt1.soc_percent);
    ST7565_drawstring_anywhere(5, 40, soc_text);
    
    // Hàng 4: Nhiệt độ trung bình
    char temp_text[25];
    sprintf(temp_text, "Temp: %d/%d/%d C", 
            bms.cellTemp.max_temp_C, 
            bms.cellTemp.min_temp_C, 
            bms.cellTemp.avg_temp_C);
    ST7565_drawstring_anywhere(5, 50, temp_text);
    
    // Hiển thị cảnh báo nếu có (ở góc phải trên)
    if (bms.almInfo.cell_overvolt > 0 || 
        bms.almInfo.cell_undervolt > 0 || 
        bms.almInfo.temp_high > 0 || 
        bms.almInfo.soc_low > 0 ||
        bms.almInfo.comm_fault > 0 ||
        bms.almInfo.dchg_oc > 0 ||
        bms.almInfo.chg_oc > 0 ||
        bms.almInfo.temp_low > 0 ||
        bms.almInfo.delta_over > 0) {
        if (blink_state) {
            ST7565_drawstring_anywhere(100, 5, "ALM!");
        }
    }
    
    // Cập nhật màn hình
    updateDisplay();
}

void process_button(void) {
    // Xử lý button actions từ interrupt
    static uint8_t last_button_enter = 0;
    static uint8_t last_button_up = 0;
    static uint8_t last_button_down = 0;

    // UP button
    if (button_up_pressed && !last_button_up) {
        if (current_menu == MENU_THROTTLE_CONTROL) {
            if (throttle_mode == THROTTLE_EDIT_SELECT) {
                // Chế độ chọn thông số - chọn thông số trước đó
                if (throttle_selected_param > 0) {
                    throttle_selected_param--;
                } else {
                    throttle_selected_param = THROTTLE_PARAM_ENABLE; // Quay vòng
                }
                debug_print("Parameter selection changed\r\n");
            } else if (throttle_mode == THROTTLE_EDIT_VALUE) {
                // Chế độ chỉnh sửa giá trị
                if (throttle_selected_param == THROTTLE_PARAM_SPEED) {
                    // Tăng speed
                    if (throttle_value < 2000) {
                        throttle_value++;
                        throttle_vehicle_mode_temp.speed_low = throttle_value & 0xFF;
                        throttle_vehicle_mode_temp.speed_high = throttle_value >> 8;
                    }
                } else if (throttle_selected_param == THROTTLE_PARAM_DIRECTION) {
                    // Chuyển direction: Forward -> Reserve -> Forward (bỏ qua Neutral)
                    if (throttle_vehicle_mode_temp.vehicle_mode.forward && !throttle_vehicle_mode_temp.vehicle_mode.reverse) {
                        throttle_vehicle_mode_temp.vehicle_mode.forward = 0;
                        throttle_vehicle_mode_temp.vehicle_mode.reverse = 1;
                    } else if (throttle_vehicle_mode_temp.vehicle_mode.reverse && !throttle_vehicle_mode_temp.vehicle_mode.forward) {
                        throttle_vehicle_mode_temp.vehicle_mode.reverse = 0;
                        throttle_vehicle_mode_temp.vehicle_mode.forward = 0;
                    } else {
                        // Reset direction to Forward
                        throttle_vehicle_mode_temp.vehicle_mode.forward = 1;
                        throttle_vehicle_mode_temp.vehicle_mode.reverse = 0;
                    }
                } else if (throttle_selected_param == THROTTLE_PARAM_BRAKE) {
                    // Toggle brake
                    throttle_vehicle_mode_temp.vehicle_mode.brake = !throttle_vehicle_mode_temp.vehicle_mode.brake;
                } else if (throttle_selected_param == THROTTLE_PARAM_ENABLE) {
                    // Toggle enable
                    throttle_vehicle_mode_temp.vehicle_mode.enable = !throttle_vehicle_mode_temp.vehicle_mode.enable;
                }
            } else {
                // Chế độ navigation - chuyển menu forward
                current_menu = (current_menu + 1) % 4;
                debug_print("Menu switched forward\r\n");
            }
        } else {
            // Không phải menu throttle - chuyển menu forward
            current_menu = (current_menu + 1) % 4;
            debug_print("Menu switched forward\r\n");
        }
    } else if (button_up_pressed && HAL_GetTick() - button_up_time > BUTTON_LONG_PRESS_TIME_MS) {
        // Long press - tăng nhanh cho speed
        if (current_menu == MENU_THROTTLE_CONTROL && 
            throttle_mode == THROTTLE_EDIT_VALUE && 
            throttle_selected_param == THROTTLE_PARAM_SPEED && 
            throttle_value < 2000) {
            throttle_value += 10;
            if (throttle_value > 2000) throttle_value = 2000;
            throttle_vehicle_mode_temp.speed_low = throttle_value & 0xFF;
            throttle_vehicle_mode_temp.speed_high = throttle_value >> 8;
            button_up_time = HAL_GetTick();
        }
    }
    last_button_up = button_up_pressed;

    // DOWN button
    if (button_down_pressed && !last_button_down) {
        if (current_menu == MENU_THROTTLE_CONTROL) {
            if (throttle_mode == THROTTLE_EDIT_SELECT) {
                // Chế độ chọn thông số - chọn thông số tiếp theo
                if (throttle_selected_param < THROTTLE_PARAM_ENABLE) {
                    throttle_selected_param++;
                } else {
                    throttle_selected_param = THROTTLE_PARAM_SPEED; // Quay vòng
                }
                debug_print("Parameter selection changed\r\n");
            } else if (throttle_mode == THROTTLE_EDIT_VALUE) {
                // Chế độ chỉnh sửa giá trị
                if (throttle_selected_param == THROTTLE_PARAM_SPEED) {
                    // Giảm speed
                    if (throttle_value > 0) {
                        throttle_value--;
                        throttle_vehicle_mode_temp.speed_low = throttle_value & 0xFF;
                        throttle_vehicle_mode_temp.speed_high = throttle_value >> 8;
                    }
                } else if (throttle_selected_param == THROTTLE_PARAM_DIRECTION) {
                    // Chuyển direction: Forward -> Neutral -> Reserve -> Forward
                    if (throttle_vehicle_mode_temp.vehicle_mode.forward && !throttle_vehicle_mode_temp.vehicle_mode.reverse) {
                        throttle_vehicle_mode_temp.vehicle_mode.forward = 0;
                        throttle_vehicle_mode_temp.vehicle_mode.reverse = 1;
                    } else if (throttle_vehicle_mode_temp.vehicle_mode.reverse && !throttle_vehicle_mode_temp.vehicle_mode.forward) {
                        throttle_vehicle_mode_temp.vehicle_mode.reverse = 0;
                        throttle_vehicle_mode_temp.vehicle_mode.forward = 0;
                    } else {
                        // Reset direction to Forward
                        throttle_vehicle_mode_temp.vehicle_mode.forward = 1;
                        throttle_vehicle_mode_temp.vehicle_mode.reverse = 0;
                    }
                } else if (throttle_selected_param == THROTTLE_PARAM_BRAKE) {
                    // Toggle brake
                    throttle_vehicle_mode_temp.vehicle_mode.brake = !throttle_vehicle_mode_temp.vehicle_mode.brake;
                } else if (throttle_selected_param == THROTTLE_PARAM_ENABLE) {
                    // Toggle enable
                    throttle_vehicle_mode_temp.vehicle_mode.enable = !throttle_vehicle_mode_temp.vehicle_mode.enable;
                }
            } else {
                // Chế độ navigation - chuyển menu backward
                current_menu = (current_menu - 1 + 4) % 4;
                debug_print("Menu switched backward\r\n");
            }
        } else {
            // Không phải menu throttle - chuyển menu backward
            current_menu = (current_menu - 1 + 4) % 4;
            debug_print("Menu switched backward\r\n");
        }
    } else if (button_down_pressed && HAL_GetTick() - button_down_time > BUTTON_LONG_PRESS_TIME_MS) {
        // Long press - giảm nhanh cho speed
        if (current_menu == MENU_THROTTLE_CONTROL && 
            throttle_mode == THROTTLE_EDIT_VALUE && 
            throttle_selected_param == THROTTLE_PARAM_SPEED && 
            throttle_value > 0) {
            throttle_value -= 10;
            if (throttle_value < 0) throttle_value = 0;
            throttle_vehicle_mode_temp.speed_low = throttle_value & 0xFF;
            throttle_vehicle_mode_temp.speed_high = throttle_value >> 8;
            button_down_time = HAL_GetTick();
        }
    }
    last_button_down = button_down_pressed;

    // ENTER button
    static uint8_t enter_long_press_handled = 0;

    if (button_enter_pressed && !last_button_enter) {
        enter_long_press_handled = 0; // Reset flag khi bắt đầu nhấn
        if (current_menu == MENU_THROTTLE_CONTROL) {
            if (throttle_mode == THROTTLE_NAVIGATION) {
                // Lần đầu ấn ENTER - vào chế độ chọn thông số
                throttle_mode = THROTTLE_EDIT_SELECT;
                throttle_selected_param = THROTTLE_PARAM_SPEED; // Reset về thông số đầu tiên
                debug_print("Entered throttle select mode\r\n");
            } else if (throttle_mode == THROTTLE_EDIT_SELECT) {
                // Ấn ENTER lần 2 - vào chế độ chỉnh sửa giá trị
                throttle_mode = THROTTLE_EDIT_VALUE;
                // Khởi tạo biến trung gian từ giá trị gốc
                throttle_vehicle_mode_temp = can_slider_vcu;
                // Nếu chọn speed, khởi tạo throttle_value từ speed_high và speed_low
                if (throttle_selected_param == THROTTLE_PARAM_SPEED) {
                    throttle_value = (can_slider_vcu.speed_high << 8) | can_slider_vcu.speed_low;
                }
                debug_print("Entered throttle edit value mode\r\n");
            } else if (throttle_mode == THROTTLE_EDIT_VALUE) {
                // Ấn ENTER lần 3 - lưu giá trị và quay về chế độ chọn
                if (throttle_selected_param == THROTTLE_PARAM_SPEED) {
                    can_slider_vcu.speed_high = throttle_value >> 8;
                    can_slider_vcu.speed_low = throttle_value & 0xFF;
                    debug_print("Speed saved\r\n");
                } else {
                    // Lưu vehicle_mode từ biến trung gian vào can_slider_vcu
                    can_slider_vcu = throttle_vehicle_mode_temp;
                    debug_print("Parameter saved\r\n");
                }
                throttle_mode = THROTTLE_EDIT_SELECT; // Quay về chế độ chọn
            }
        }
    } else if (button_enter_pressed && 
                HAL_GetTick() - button_enter_time > ENTER_LONG_PRESS_TIME_MS && 
                !enter_long_press_handled) {
        // Long press ENTER - thoát khỏi chế độ edit (chỉ trigger một lần)
        if (current_menu == MENU_THROTTLE_CONTROL) {
            if (throttle_mode == THROTTLE_EDIT_SELECT || throttle_mode == THROTTLE_EDIT_VALUE) {
                // Thoát về chế độ navigation
                throttle_mode = THROTTLE_NAVIGATION;
                debug_print("Exited throttle edit mode\r\n");
                enter_long_press_handled = 1; // Đánh dấu đã xử lý
            }
        }
    }

    if (!button_enter_pressed) {
        enter_long_press_handled = 0; // Reset flag khi thả nút
    }

    last_button_enter = button_enter_pressed;
}
  
void process_menu(void) {
// Hiển thị menu tương ứng
switch (current_menu)
{
case MENU_CAN_INFO_1:
    display_can_info_1();
    break;
case MENU_CAN_INFO_2:
    display_can_info_2();
    break;
case MENU_THROTTLE_CONTROL:
    display_throttle_control();
    break;
case MENU_BMS_INFO:
    display_bms_info();
    break;
default:
    display_can_info_1();
    break;
}
}