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

int16_t throttle_value = 0;
Can_Slider_VCU_t throttle_vehicle_mode_temp = {0};

// Menu chính hiện tại (tương ứng với VCU state)
menu_main_t current_main_menu = MENU_MAIN_INIT;

// Chế độ hiển thị: overview (tổng quan) hoặc detail (chi tiết)
menu_display_mode_t menu_display_mode = MENU_MODE_OVERVIEW;

// Menu chi tiết hiện tại (khi ở chế độ detail)
menu_detail_t current_detail_menu = MENU_DETAIL_CAN_INFO_1;

// VCU state trước đó để phát hiện thay đổi
static vcu_state_t last_vcu_state = VCU_STATE_INIT;

// Hàm chuyển đổi VCU state sang menu chính tương ứng
static menu_main_t vcu_state_to_main_menu(vcu_state_t state)
{
	switch (state) {
	case VCU_STATE_INIT:
		return MENU_MAIN_INIT;
	case VCU_STATE_CAN:
		return MENU_MAIN_CAN;
	case VCU_STATE_PHYSICAL:
		return MENU_MAIN_PHYSICAL;
	case VCU_STATE_CHARGE:
		return MENU_MAIN_CHARGE;
	case VCU_STATE_ERROR:
		return MENU_MAIN_ERROR;
	default:
		return MENU_MAIN_INIT;
	}
}

// Hàm kiểm tra menu chi tiết có được phép hiển thị dựa trên VCU state (sử dụng bitmask)
static bool is_detail_menu_allowed(menu_detail_t menu)
{
	menu_mask_t allowed_mask = VCU_GetAllowedDetailMenuMask();
	/* Kiểm tra bit tương ứng với menu có được set không */
	return (allowed_mask & menu) != 0;
}

// Hàm tìm menu chi tiết hợp lệ tiếp theo (forward) - sử dụng bitmask
static menu_detail_t find_next_allowed_detail_menu(menu_detail_t start_menu)
{
	menu_mask_t allowed_mask = VCU_GetAllowedDetailMenuMask();
	
	// Duyệt qua tất cả các menu chi tiết (15 menu)
	menu_detail_t menu_list[] = {
		MENU_DETAIL_CAN_INFO_1, MENU_DETAIL_CAN_INFO_2, MENU_DETAIL_THROTTLE_CONTROL,
		MENU_DETAIL_BMS_INFO, MENU_DETAIL_ALM_BMS, MENU_DETAIL_IO_INFO,
		MENU_DETAIL_BMS_BATT_ST2, MENU_DETAIL_BMS_ALL_TEMP, MENU_DETAIL_BMS_ERR_INFO,
		MENU_DETAIL_BMS_INFO_SYS, MENU_DETAIL_BMS_SW_STA, MENU_DETAIL_BMS_CELLVOL,
		MENU_DETAIL_BMS_CELLVOL_2, MENU_DETAIL_BMS_CHG_INFO, MENU_DETAIL_NETWORK
	};
	
	// Tìm vị trí menu hiện tại
	int start_idx = -1;
	for (int i = 0; i < 15; i++) {
		if (menu_list[i] == start_menu) {
			start_idx = i;
			break;
		}
	}
	
	if (start_idx == -1) {
		start_idx = 0; // Nếu không tìm thấy, bắt đầu từ đầu
	}
	
	// Tìm menu hợp lệ tiếp theo
	for (int i = 1; i < 15; i++) {
		int idx = (start_idx + i) % 15;
		if (allowed_mask & menu_list[idx]) {
			return menu_list[idx];
		}
	}
	
	return start_menu; // Nếu không tìm thấy, giữ nguyên
}

// Hàm tìm menu chi tiết hợp lệ trước đó (backward) - sử dụng bitmask
static menu_detail_t find_prev_allowed_detail_menu(menu_detail_t start_menu)
{
	menu_mask_t allowed_mask = VCU_GetAllowedDetailMenuMask();
	
	// Duyệt qua tất cả các menu chi tiết (15 menu)
	menu_detail_t menu_list[] = {
		MENU_DETAIL_CAN_INFO_1, MENU_DETAIL_CAN_INFO_2, MENU_DETAIL_THROTTLE_CONTROL,
		MENU_DETAIL_BMS_INFO, MENU_DETAIL_ALM_BMS, MENU_DETAIL_IO_INFO,
		MENU_DETAIL_BMS_BATT_ST2, MENU_DETAIL_BMS_ALL_TEMP, MENU_DETAIL_BMS_ERR_INFO,
		MENU_DETAIL_BMS_INFO_SYS, MENU_DETAIL_BMS_SW_STA, MENU_DETAIL_BMS_CELLVOL,
		MENU_DETAIL_BMS_CELLVOL_2, MENU_DETAIL_BMS_CHG_INFO, MENU_DETAIL_NETWORK
	};
	
	// Tìm vị trí menu hiện tại
	int start_idx = -1;
	for (int i = 0; i < 15; i++) {
		if (menu_list[i] == start_menu) {
			start_idx = i;
			break;
		}
	}
	
	if (start_idx == -1) {
		start_idx = 0; // Nếu không tìm thấy, bắt đầu từ đầu
	}
	
	// Tìm menu hợp lệ trước đó
	for (int i = 1; i < 15; i++) {
		int idx = (start_idx - i + 15) % 15;
		if (allowed_mask & menu_list[idx]) {
			return menu_list[idx];
		}
	}
	
	return start_menu; // Nếu không tìm thấy, giữ nguyên
}

throttle_mode_t throttle_mode = THROTTLE_NAVIGATION;
throttle_param_t throttle_selected_param = THROTTLE_PARAM_BRAKE;

io_mode_t io_mode = IO_NAVIGATION;
io_param_t io_selected_param = IO_PARAM_MOTOR_STATUS;
// Biến trung gian để lưu trạng thái relay khi đang chỉnh sửa
struct {
    uint8_t disable_motor;
    uint8_t contactor_state;
    uint8_t light_state;
} io_states_temp = {0};

// Hàm hiển thị thông tin CAN lên màn hình ST7565
void display_can_info_1(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);

    // Hàng 1: Forward (trái) và Reverse (phải)
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
    switch (can_slider.raw_err_code) 
    {
        case 0: strcpy(error_string, "Error: None"); break;
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
    uint8_t speed_y = 5;
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
        strcpy(direction_text, "Direction: Reverse");
    } else {
        strcpy(direction_text, "Direction: Neutral");
    }
    uint8_t direction_x = (LCD_WIDTH/2)-((strlen(direction_text)/2)*6);
    uint8_t direction_y = 20;
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
    
    char max_cell_text[20];
    sprintf(max_cell_text, "MaxCell:%dmV", bms.cellVolt.max_cell_mV);
    ST7565_drawstring_anywhere(5, 5, max_cell_text);

    char max_cell_no_text[20];
    sprintf(max_cell_no_text, "No.%d", bms.cellVolt.max_cell_no);
    ST7565_drawstring_anywhere(95, 5, max_cell_no_text);

    char min_cell_text[20];
    sprintf(min_cell_text, "MinCell:%dmV", bms.cellVolt.min_cell_mV);
    ST7565_drawstring_anywhere(5, 20, min_cell_text);

    char min_cell_no_text[20];
    sprintf(min_cell_no_text, "No.%d", bms.cellVolt.min_cell_no);
    ST7565_drawstring_anywhere(95, 20, min_cell_no_text);
    
    // Hàng 1: Điện áp tổng (V)
    char voltage_text[20];
    sprintf(voltage_text, "Vol:%.1fV", bms.batt1.voltage_V);
    ST7565_drawstring_anywhere(5, 35, voltage_text);
    
    // Hàng 2: Dòng điện (A)
    char current_text[20];
    sprintf(current_text, "Cur:%.1fA", bms.batt1.current_A);
    ST7565_drawstring_anywhere(65, 35, current_text);
    
    // Hàng 3: SOC (%)
    char soc_text[20];
    sprintf(soc_text, "SOC:%u", bms.batt1.soc_percent);
    ST7565_drawstring_anywhere(90, 50, soc_text);
    
    // Hàng 4: Nhiệt độ trung bình
    char temp_text[25];
    sprintf(temp_text, "Temp:%d/%d/%d", 
            bms.cellTemp.max_temp_C, 
            bms.cellTemp.min_temp_C, 
            bms.cellTemp.avg_temp_C);
    ST7565_drawstring_anywhere(5, 50, temp_text);

    // Cập nhật màn hình
    updateDisplay();
}

void display_alm_bms(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);

    // Vẽ viền xung quanh màn hình
    ST7565_drawrect(2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, 1);

    uint8_t count_alm = 0;
    uint8_t name_y[2] = {50, 20};  // Tọa độ y cho tên lỗi (lỗi 1, lỗi 2)
    uint8_t level_y[2] = {35, 5};  // Tọa độ y cho level lỗi (lỗi 1, lỗi 2)

     //Hiển thị cảnh báo nếu có (tối đa 2 lỗi)
     if (count_alm < 2 && bms.almInfo.cell_overvolt) {
    	 ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ALM: Over Voltage")/2)*6), name_y[count_alm], "ALM: Over Voltage");
         char level_text[15];
         sprintf(level_text, "Level: %d", bms.almInfo.cell_overvolt);
         ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(level_text)/2)*6), level_y[count_alm], level_text);
         count_alm++;
     }
     if (count_alm < 2 && bms.almInfo.cell_undervolt) {
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ALM: Under Voltage")/2)*6), name_y[count_alm], "ALM: Under Voltage");
        char level_text[15];
        sprintf(level_text, "Level: %d", bms.almInfo.cell_undervolt);
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(level_text)/2)*6), level_y[count_alm], level_text);
        count_alm++;
     }
     if (count_alm < 2 && bms.almInfo.temp_high) {
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ALM: High Temperature")/2)*6), name_y[count_alm], "ALM: High Temperature");
        char level_text[15];
        sprintf(level_text, "Level: %d", bms.almInfo.temp_high);
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(level_text)/2)*6), level_y[count_alm], level_text);
        count_alm++;
     }
     if (count_alm < 2 && bms.almInfo.temp_low) {
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ALM: Low Temperature")/2)*6), name_y[count_alm], "ALM: Low Temperature");
        char level_text[15];
        sprintf(level_text, "Level: %d", bms.almInfo.temp_low);
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(level_text)/2)*6), level_y[count_alm], level_text);
        count_alm++;
     }
     if (count_alm < 2 && bms.almInfo.delta_over) {
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ALM: Delta Over")/2)*6), name_y[count_alm], "ALM: Delta Over");
        char level_text[15];
        sprintf(level_text, "Level: %d", bms.almInfo.delta_over);
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(level_text)/2)*6), level_y[count_alm], level_text);
        count_alm++;
     }
     if (count_alm < 2 && bms.almInfo.dchg_oc) {
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ALM: Discharge Over Current")/2)*6), name_y[count_alm], "ALM: Discharge Over Current");
        char level_text[15];
        sprintf(level_text, "Level: %d", bms.almInfo.dchg_oc);
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(level_text)/2)*6), level_y[count_alm], level_text);
        count_alm++;
     }
     if (count_alm < 2 && bms.almInfo.chg_oc) {
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ALM: Charge Over Current")/2)*6), name_y[count_alm], "ALM: Charge Over Current");
        char level_text[15];
        sprintf(level_text, "Level: %d", bms.almInfo.chg_oc);
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(level_text)/2)*6), level_y[count_alm], level_text);
        count_alm++;
     }
     if (count_alm < 2 && bms.almInfo.soc_low) {
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ALM: SOC Low")/2)*6), name_y[count_alm], "ALM: SOC Low");
        char level_text[15];
        sprintf(level_text, "Level: %d", bms.almInfo.soc_low);
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(level_text)/2)*6), level_y[count_alm], level_text);
        count_alm++;
     }
     if (count_alm < 2 && bms.almInfo.comm_fault) {
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ALM: Communication Fault")/2)*6), name_y[count_alm], "ALM: Communication Fault");
        char level_text[15];
        sprintf(level_text, "Level: %d", bms.almInfo.comm_fault);
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(level_text)/2)*6), level_y[count_alm], level_text);
        count_alm++;
     }

     if (count_alm == 0) {
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ALM: No Alarm")/2)*6), 25, "ALM: No Alarm");
     }

    // Cập nhật màn hình
    updateDisplay();
}

void display_io_info(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    // Biến để kiểm tra nhấp nháy (dựa trên tick)
    uint32_t current_time = HAL_GetTick();
    uint8_t blink_state = ((current_time / 500) % 2); // Nhấp nháy mỗi 500ms
    
    // Sử dụng biến trung gian nếu đang ở chế độ edit
    uint8_t disbale_motor_state_display, contactor_state_display, light_state_display;
    if (io_mode == IO_EDIT_VALUE) {
        disbale_motor_state_display = io_states_temp.disable_motor;
        contactor_state_display = io_states_temp.contactor_state;
        light_state_display = io_states_temp.light_state;
    } else {
        disbale_motor_state_display = (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_SET) ? 1 : 0;
        contactor_state_display = (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3) == GPIO_PIN_SET) ? 1 : 0;
        light_state_display = (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_6) == GPIO_PIN_SET) ? 1 : 0;
    }
    
    // Hiển thị Disable Motor
    char disable_motor_text[20];
    if (disbale_motor_state_display) {
        strcpy(disable_motor_text, "Disable Motor");
    } else {
        strcpy(disable_motor_text, "Enable Motor");
    }
    uint8_t disable_motor_x = (LCD_WIDTH/2)-((strlen(disable_motor_text)/2)*6);
    uint8_t disable_motor_y = 35;
    ST7565_drawstring_anywhere(disable_motor_x, disable_motor_y, disable_motor_text);
    
    // Vẽ indicator cho Disable Motor nếu đang được chọn
    if (io_mode == IO_EDIT_SELECT && io_selected_param == IO_PARAM_MOTOR_STATUS) {
        if (blink_state) {
            ST7565_drawrect(disable_motor_x - 2, disable_motor_y - 1, strlen(disable_motor_text) * 6 + 4, 10, 1);
        }
    } else if (io_mode == IO_EDIT_VALUE && io_selected_param == IO_PARAM_MOTOR_STATUS) {
        // Chế độ edit giá tri, hiển thị viền
        ST7565_drawrect(disable_motor_x - 2, disable_motor_y - 1, strlen(disable_motor_text) * 6 + 4, 10, 1);
    }
    
    // Hiển thị Contactor
    char contactor_text[20];
    if (contactor_state_display) {
        strcpy(contactor_text, "Contactor: ON");
    } else {
        strcpy(contactor_text, "Contactor: OFF");
    }
    uint8_t contactor_x = (LCD_WIDTH/2)-((strlen(contactor_text)/2)*6);
    uint8_t contactor_y = 20;
    ST7565_drawstring_anywhere(contactor_x, contactor_y, contactor_text);
    
    // Vẽ indicator cho Contactor nếu đang được chọn
    if (io_mode == IO_EDIT_SELECT && io_selected_param == IO_PARAM_CONTACTOR) {
        if (blink_state) {
            ST7565_drawrect(contactor_x - 2, contactor_y - 1, strlen(contactor_text) * 6 + 4, 10, 1);
        }
    } else if (io_mode == IO_EDIT_VALUE && io_selected_param == IO_PARAM_CONTACTOR) {
        ST7565_drawrect(contactor_x - 2, contactor_y - 1, strlen(contactor_text) * 6 + 4, 10, 1);
    }
    
    // Hiển thị Light
    char light_text[20];
    if (light_state_display) {
        strcpy(light_text, "Light: ON");
    } else {
        strcpy(light_text, "Light: OFF");
    }
    uint8_t light_x = (LCD_WIDTH/2)-((strlen(light_text)/2)*6);
    uint8_t light_y = 5;
    ST7565_drawstring_anywhere(light_x, light_y, light_text);
    
    // Vẽ indicator cho Light nếu đang được chọn
    if (io_mode == IO_EDIT_SELECT && io_selected_param == IO_PARAM_LIGHT) {
        if (blink_state) {
            ST7565_drawrect(light_x - 2, light_y - 1, strlen(light_text) * 6 + 4, 10, 1);
        }
    } else if (io_mode == IO_EDIT_VALUE && io_selected_param == IO_PARAM_LIGHT) {
        ST7565_drawrect(light_x - 2, light_y - 1, strlen(light_text) * 6 + 4, 10, 1);
    }
    
    // Mode indicator
    if (io_mode == IO_EDIT_SELECT) {
        // Chế độ chọn relay
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("UP/DN:Sel EN:Edit")/2)*6), 50, "UP/DN:Sel EN:Edit");
    } else if (io_mode == IO_EDIT_VALUE) {
        // Chế độ chỉnh sửa trạng thái
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("UP/DN:Toggle EN:Save")/2)*6), 50, "UP/DN:Toggle EN:Save");
    } else {
        // Chế độ navigation
        ST7565_drawrect((LCD_WIDTH/2)-((strlen("ENTER: Edit Mode")/2)*6) - 2, 48, strlen("ENTER: Edit Mode") * 6 + 4, 12, 1);
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen("ENTER: Edit Mode")/2)*6), 50, "ENTER: Edit Mode");
    }
    
    // Cập nhật màn hình
    updateDisplay();
}

void display_bms_batt_st2(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    // Vẽ viền xung quanh màn hình
    ST7565_drawrect(2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, 1);
    
    // Dung lượng còn lại
    char cap_remain_text[25];
    sprintf(cap_remain_text, "Cap Remain: %.1f Ah", bms.batt2.cap_remain_Ah);
    ST7565_drawstring_anywhere(5, 5, cap_remain_text);
    
    // Dung lượng đầy
    char cap_full_text[25];
    sprintf(cap_full_text, "Cap Full: %.1f Ah", bms.batt2.cap_full_Ah);
    ST7565_drawstring_anywhere(5, 20, cap_full_text);
    
    // Dung lượng đã xả tích lũy
    char cycle_cap_text[30];
    sprintf(cycle_cap_text, "Cycle Cap: %.1f Ah", bms.batt2.cycle_cap_Ah);
    ST7565_drawstring_anywhere(5, 35, cycle_cap_text);
    
    // Số chu kỳ sạc/xả
    char cycle_count_text[20];
    sprintf(cycle_count_text, "Cycles: %u", bms.batt2.cycle_count);
    ST7565_drawstring_anywhere(5, 50, cycle_count_text);
    
    // Cập nhật màn hình
    updateDisplay();
}

void display_bms_all_temp(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    // Vẽ viền xung quanh màn hình
    ST7565_drawrect(2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, 1);
    
    // Hiển thị 5 cảm biến nhiệt độ
    char temp_text[30];
    uint8_t y_pos = 5;
    uint8_t sensor_count = 0;
    
    for (int i = 0; i < 5; i++) {
        if (bms.allTemp.mask & (1 << i)) {  // Kiểm tra bitmask
            if (bms.allTemp.temp[i] != -127) {  // Cảm biến tồn tại
                sprintf(temp_text, "Temp%d: %d C", i + 1, bms.allTemp.temp[i]);
                ST7565_drawstring_anywhere(5, y_pos, temp_text);
                y_pos += 15;
                sensor_count++;
                if (sensor_count >= 3) break;  // Tối đa 3 dòng trên màn hình
            } 
        } else {
            char not_available_text[20];
            sprintf(not_available_text, "Temp%d: N/A", i + 1);
            ST7565_drawstring_anywhere(5, y_pos, not_available_text);
            y_pos += 15;
            sensor_count++;
            if (sensor_count >= 3) break;  // Tối đa 3 dòng trên màn hình
        }
    }
    
    char mask_text[20];
    sprintf(mask_text, "Mask: 0x%02X", bms.allTemp.mask);
    ST7565_drawstring_anywhere(5, 50, mask_text);
    
    // Cập nhật màn hình
    updateDisplay();
}

void display_bms_err_info(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    // Vẽ viền xung quanh màn hình
    ST7565_drawrect(2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, 1);
    
    uint8_t y_pos = 5;
    uint8_t error_count = 0;
    char error_text[30];
    
    // Hiển thị các lỗi (tối đa 4 lỗi trên màn hình)
    if (bms.bmsErrInfo.line_res_high) {
        strcpy(error_text, "Line Res High");
        ST7565_drawstring_anywhere(5, y_pos, error_text);
        y_pos += 12;
        error_count++;
    }
    if (bms.bmsErrInfo.mos_overtemp && error_count < 4) {
        strcpy(error_text, "MOS Over Temp");
        ST7565_drawstring_anywhere(5, y_pos, error_text);
        y_pos += 12;
        error_count++;
    }
    if (bms.bmsErrInfo.cell_count_mismatch && error_count < 4) {
        strcpy(error_text, "Cell Count Err");
        ST7565_drawstring_anywhere(5, y_pos, error_text);
        y_pos += 12;
        error_count++;
    }
    if (bms.bmsErrInfo.cur_sensor_fault && error_count < 4) {
        strcpy(error_text, "Cur Sensor Err");
        ST7565_drawstring_anywhere(5, y_pos, error_text);
        y_pos += 12;
        error_count++;
    }
    if (bms.bmsErrInfo.chg_mos_fault && error_count < 4) {
        strcpy(error_text, "Chg MOS Fault");
        ST7565_drawstring_anywhere(5, y_pos, error_text);
        y_pos += 12;
        error_count++;
    }
    if (bms.bmsErrInfo.dchg_mos_fault && error_count < 4) {
        strcpy(error_text, "Dchg MOS Fault");
        ST7565_drawstring_anywhere(5, y_pos, error_text);
        y_pos += 12;
        error_count++;
    }
    
    // Nếu không có lỗi
    if (error_count == 0) {
        strcpy(error_text, "No Errors");
        ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(error_text)/2)*6), 25, error_text);
    }
    
    // Cập nhật màn hình
    updateDisplay();
}

void display_bms_info_sys(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    // Vẽ viền xung quanh màn hình
    ST7565_drawrect(2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, 1);

    char tile_text[] = "BMS INFO";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(tile_text)/2)*6), 50, tile_text);
    
    // Thời gian chạy (giây)
    char runtime_text[30];
    sprintf(runtime_text, "BMSRuntime: %u", bms.info.runtime_s);
    ST7565_drawstring_anywhere(5, 5, runtime_text);
    
    // Dòng sưởi
    char heat_current_text[25];
    sprintf(heat_current_text, "Heat Current: %.2f A", bms.info.heat_current_A);
    ST7565_drawstring_anywhere(5, 20, heat_current_text);
    
    // SOH (State of Health)
    char soh_text[20];
    sprintf(soh_text, "SOH: %u %%", bms.info.soh_percent);
    ST7565_drawstring_anywhere(5, 35, soh_text);
    
    // Cập nhật màn hình
    updateDisplay();
}

void display_bms_sw_sta(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    // Vẽ viền xung quanh màn hình
    ST7565_drawrect(2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, 1);
    
    // Trạng thái MOS sạc
    char chg_mos_text[20];
    strcpy(chg_mos_text, bms.swSta.chgMOS ? "ChgMOS:ON" : "ChgMOS:OFF");
    ST7565_drawstring_anywhere(5, 5, chg_mos_text);
    
    // Trạng thái MOS xả
    char dchg_mos_text[20];
    strcpy(dchg_mos_text, bms.swSta.dchgMOS ? "DchgMOS: ON" : "DchgMOS: OFF");
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(dchg_mos_text)/2)*6), 50, dchg_mos_text);
    
    // Trạng thái cân bằng
    char balance_text[20];
    strcpy(balance_text, bms.swSta.balance ? "Balance:ON" : "Balance:OFF");
    ST7565_drawstring_anywhere(5, 35, balance_text);
    
    // Trạng thái sưởi
    char heat_text[20];
    strcpy(heat_text, bms.swSta.heat ? "Heat:ON" : "Heat:OFF");
    ST7565_drawstring_anywhere(70, 5, heat_text);
    
    // Trạng thái sạc đã cắm
    char chg_plug_text[20];
    strcpy(chg_plug_text, bms.swSta.chgPlug ? "ChgPlug: ON" : "ChgPlug: OFF");
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(chg_plug_text)/2)*6), 20, chg_plug_text);
    
    // Trạng thái ACC
    char acc_text[20];
    strcpy(acc_text, bms.swSta.acc ? "ACC:ON" : "ACC:OFF");
    ST7565_drawstring_anywhere(80, 35, acc_text);
    
    // Cập nhật màn hình
    updateDisplay();
}

static void draw_cellvol_page(uint8_t start_cell)
{
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);

    for (uint8_t row = 0; row < 4; row++) {
        uint8_t cell_a = start_cell + row * 2;
        uint8_t cell_b = cell_a + 1;

        uint16_t mv_a = (cell_a < 25) ? bms.cellArray.cell_mV[cell_a] : 0;
        uint16_t mv_b = (cell_b < 25) ? bms.cellArray.cell_mV[cell_b] : 0;

        char line[30];
        if (mv_a == 0)
            sprintf(line, "C%02u:----mV", cell_a + 1);
        else
            sprintf(line, "C%02u:%4dmV", cell_a + 1, mv_a);

        char line_b[30];
        if (mv_b == 0)
            sprintf(line_b, "C%02u:----mV", cell_b + 1);
        else
            sprintf(line_b, "C%02u:%4dmV", cell_b + 1, mv_b);

        ST7565_drawstring_anywhere(5, 8 + row * 14, line);
        ST7565_drawstring_anywhere(68, 8 + row * 14, line_b);
    }

    updateDisplay();
}

void display_bms_cellvol(void) {
    draw_cellvol_page(0);   // Cell 1-8
}

void display_bms_cellvol_2(void) {
    draw_cellvol_page(8);   // Cell 9-16
}

void display_bms_chg_info(void) {
    // Clear display buffer
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    // Vẽ viền xung quanh màn hình
    ST7565_drawrect(2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, 1);
    
    // Điện áp yêu cầu sạc
    char chg_volt_text[25];
    sprintf(chg_volt_text, "Chg Volt: %.1f V", bms.chgInfo.chg_volt_V);
    ST7565_drawstring_anywhere(5, 5, chg_volt_text);
    
    // Dòng yêu cầu sạc
    char chg_curr_text[25];
    sprintf(chg_curr_text, "Chg Curr: %.1f A", bms.chgInfo.chg_curr_A);
    ST7565_drawstring_anywhere(5, 20, chg_curr_text);
    
    // Trạng thái bộ sạc
    char chg_dev_text[25];
    sprintf(chg_dev_text, "Chg Dev: %s", bms.chgInfo.chg_dev_sw ? "OFF" : "ON");
    ST7565_drawstring_anywhere(5, 35, chg_dev_text);
    
    // Chế độ sạc/sưởi
    char mode_text[25];
    sprintf(mode_text, "Mode: %s", bms.chgInfo.chg_and_heat ? "Heat" : "Charge");
    ST7565_drawstring_anywhere(5, 50, mode_text);
    
    // Cập nhật màn hình
    updateDisplay();
}

// ========== CÁC HÀM HIỂN THỊ MENU CHÍNH (OVERVIEW) ==========

/**
 * @brief Hiển thị menu INIT - Khởi tạo và tự kiểm tra
 * Hiển thị ngắn gọn: trạng thái lỗi, trạng thái I/O
 */
void display_main_init(void) {
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    // Tiêu đề
    char title_text[] = "INIT & SELF-CHECK";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(title_text)/2)*6), 5, title_text);
    
    // Lấy outputs từ VCU
    const vcu_state_outputs_t *outputs = VCU_StateOutputs();
    
    // Trạng thái I/O
    char io_text[30];
    sprintf(io_text, "Contactor: %s", outputs->contactor_on ? "ON" : "OFF");
    ST7565_drawstring_anywhere(5, 20, io_text);
    
    char mode_text[30];
    sprintf(mode_text, "Mode: %s", can_slider.slider_2.control_mode ? "CAN" : "PHYS");
    ST7565_drawstring_anywhere(5, 35, mode_text);
    
    // Hướng dẫn xem chi tiết
    char hint_text[] = "ENTER: Details ERROR";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(hint_text)/2)*6), 50, hint_text);
    
    updateDisplay();
}

/**
 * @brief Hiển thị menu WAITING - Chờ điều kiện an toàn
 */
void display_main_waiting(void) {
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    char title_text[] = "WAITING";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(title_text)/2)*6), 5, title_text);

    char direction_text[20];
    if (can_slider.slider_1.vehicle_mode.forward && !can_slider.slider_1.vehicle_mode.reverse) {
        strcpy(direction_text, "Direction: Forward");
    } else if (can_slider.slider_1.vehicle_mode.reverse && !can_slider.slider_1.vehicle_mode.forward) {
        strcpy(direction_text, "Direction: Reverse");
    } else {
        strcpy(direction_text, "Direction: Neutral");
    }
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(direction_text)/2)*6), 20, direction_text);

    char rpm_text[20];
    sprintf(rpm_text, "RPM: %u", can_slider.slider_1.motor_rpm);
    ST7565_drawstring_anywhere(70, 35, rpm_text);
    
    char mode_text[30];
    sprintf(mode_text, "Motor: %s", vcu_ctx.outputs.disable_motor ? "DISABLE" : "ENABLE");
    ST7565_drawstring_anywhere(5, 35, mode_text);
    
    char hint_text[] = "ENTER: Details";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(hint_text)/2)*6), 50, hint_text);
    
    updateDisplay();
}

/**
 * @brief Hiển thị menu CAN MODE - Điều khiển qua CAN
 */
void display_main_can(void) {
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    char title_text[] = "CAN MODE";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(title_text)/2)*6), 5, title_text);
    
    // Thông tin cơ bản
    char rpm_text[20];
    sprintf(rpm_text, "RPM: %u", can_slider.slider_1.motor_rpm);
    ST7565_drawstring_anywhere(5, 20, rpm_text);
    
    char volt_text[20];
    sprintf(volt_text, "Volt: %.1fV", can_slider.slider_2.battery_voltage);
    ST7565_drawstring_anywhere(5, 35, volt_text);
    
    char hint_text[] = "ENTER: Details";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(hint_text)/2)*6), 50, hint_text);
    
    updateDisplay();
}

/**
 * @brief Hiển thị menu PHYSICAL MODE - Điều khiển vật lý
 */
void display_main_physical(void) {
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    char title_text[] = "PHYSICAL MODE";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(title_text)/2)*6), 5, title_text);
    
    // Thông tin cơ bản
    const vcu_state_outputs_t *outputs = VCU_StateOutputs();
    char contactor_text[20];
    sprintf(contactor_text, "Contactor: %s", outputs->contactor_on ? "ON" : "OFF");
    ST7565_drawstring_anywhere(5, 20, contactor_text);
    
    char select_text[20];
    sprintf(select_text, "Motor: %s", outputs->disable_motor ? "DISABLE" : "ENABLE");
    ST7565_drawstring_anywhere(5, 35, select_text);
    
    char hint_text[] = "ENTER: Details";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(hint_text)/2)*6), 50, hint_text);
    
    updateDisplay();
}

/**
 * @brief Hiển thị menu CHARGE - Chế độ sạc
 */
void display_main_charge(void) {
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    char title_text[] = "CHARGER MODE";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(title_text)/2)*6), 5, title_text);
    
    // Thông tin sạc cơ bản
    char volt_text[25];
    sprintf(volt_text, "Voltage: %.1f V", bms.batt1.voltage_V);
    ST7565_drawstring_anywhere(5, 20, volt_text);
    
    char curr_text[25];
    sprintf(curr_text, "Current: %.1f A", bms.batt1.current_A);
    ST7565_drawstring_anywhere(70, 20, curr_text);
    
    char soc_text[20];
    sprintf(soc_text, "SOC: %u%%", bms.batt1.soc_percent);
    ST7565_drawstring_anywhere(5, 35, soc_text);

    const vcu_state_outputs_t *outputs = VCU_StateOutputs();
    char contactor_text[20];
    sprintf(contactor_text, "Contactor: %s", outputs->contactor_on ? "ON" : "OFF");
    ST7565_drawstring_anywhere(70, 35, contactor_text);
    
    char hint_text[] = "ENTER: Details";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(hint_text)/2)*6), 50, hint_text);
    
    updateDisplay();
}

/**
 * @brief Hiển thị menu IDLE - Chế độ chờ
 */
void display_main_idle(void) {
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    char title_text[] = "IDLE";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(title_text)/2)*6), 5, title_text);
    
    // Thông tin cơ bản
    const vcu_state_outputs_t *outputs = VCU_StateOutputs();
    char contactor_text[20];
    sprintf(contactor_text, "Contactor: %s", outputs->contactor_on ? "ON" : "OFF");
    ST7565_drawstring_anywhere(5, 20, contactor_text);
    
    char bms_text[20];
    sprintf(bms_text, "Voltage: %.1fV", bms.batt1.voltage_V);
    ST7565_drawstring_anywhere(5, 35, bms_text);
    
    char hint_text[] = "ENTER: Details";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(hint_text)/2)*6), 50, hint_text);
    
    updateDisplay();
}

/**
 * @brief Hiển thị menu ERROR - Trạng thái lỗi
 */
void display_main_error(void) {
    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    char title_text[] = "ERROR STATE";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(title_text)/2)*6), 5, title_text);
    
    // Hiển thị lỗi chính
    char error_text[] = "Critical Error!";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(error_text)/2)*6), 20, error_text);
    
    char select_text[20];
    const vcu_state_outputs_t *outputs = VCU_StateOutputs();
    sprintf(select_text, "Motor: %s", outputs->disable_motor ? "DISABLE" : "ENABLE");
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(select_text)/2)*6), 35, select_text);
    
    char hint_text[] = "ENTER: Details";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(hint_text)/2)*6), 50, hint_text);
    
    updateDisplay();
}

/**
 * @brief Hiển thị trạng thái mạng
 */
void display_network(void)
{

    memset(displayBuffer, 0, LCD_BUFFER_SIZE);
    
    char title_text[] = "NETWORK";
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(title_text)/2)*6), 5, title_text);

    char simStatus_text[20];
    switch(simStatus)
    {
        case Sim_StartUp_Status:
        {
            strcpy(simStatus_text, "Starting...");
        }
        break;
        case Sim_NoSim_Status:
        {
            strcpy(simStatus_text, "No SIM...");
        }
        break;
        case Sim_CheckSignal_Status:
        {
            strcpy(simStatus_text, "Checking signal...");
        }
        break;
        case Sim_CheckNetwork_Status:
        {
            strcpy(simStatus_text, "Checking network...");
        }
        break;
        case Sim_Connecting_Status:
        {
            strcpy(simStatus_text, "Connecting...");

        }
        break;
        case Sim_Connected_Status:
        {
            strcpy(simStatus_text, "Connected: 4G");

        }
        break;
        case Sim_Disconnected_Status:
        {
            strcpy(simStatus_text, "Disconnecting...");
        }
        break;
        case Sim_Sleep_Status:
        {
            strcpy(simStatus_text, "Sleeping");
        }
        break;
        default:
        {
            strcpy(simStatus_text, "Unknown");
        }
        break;
    }
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(simStatus_text)/2)*6), 20, simStatus_text);

    char signal_text[20];
    sprintf(signal_text, "Signal: %d%%", Sim_SignalQuanlityPercent());
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(signal_text)/2)*6), 35, signal_text);

    char id_text[20];
    sprintf(id_text, "ID Topic: %d", ID_DEVICE);
    ST7565_drawstring_anywhere((LCD_WIDTH/2)-((strlen(id_text)/2)*6), 50, id_text);

    updateDisplay();
}

void process_button(void) {
    // Xử lý button actions từ interrupt
    static uint8_t last_button_enter = 0;
    static uint8_t last_button_up = 0;
    static uint8_t last_button_down = 0;

    // UP button
    if (button_up_pressed && !last_button_up) {
        if (menu_display_mode == MENU_MODE_OVERVIEW) {
            // Ở overview mode, không làm gì
        } else if (menu_display_mode == MENU_MODE_DETAIL) {
            // Ở detail mode, xử lý menu chi tiết
            if (current_detail_menu == MENU_DETAIL_THROTTLE_CONTROL) {
                if (throttle_mode == THROTTLE_EDIT_SELECT) {
                    // Chế độ chọn thông số - chọn thông số trước đó
                    if (throttle_selected_param > 0) {
                        throttle_selected_param--;
                    } else {
                        throttle_selected_param = THROTTLE_PARAM_SPEED; // Quay vòng
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
                        // Chuyển direction: Forward -> Reverse -> Forward
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
                    // Chế độ navigation - chuyển menu forward (chỉ menu hợp lệ)
                    current_detail_menu = find_next_allowed_detail_menu(current_detail_menu);
                    debug_print("Detail menu switched forward\r\n");
                }
            } else if (current_detail_menu == MENU_DETAIL_IO_INFO) {
                if (io_mode == IO_EDIT_SELECT) {
                    // Chế độ chọn relay - chọn relay trước đó
                    if (io_selected_param > 0) {
                        io_selected_param--;
                    } else {
                        io_selected_param = IO_PARAM_LIGHT; // Quay vòng
                    }
                    debug_print("IO parameter selection changed\r\n");
                } else if (io_mode == IO_EDIT_VALUE) {
                    // Chế độ chỉnh sửa trạng thái - toggle relay
                    if (io_selected_param == IO_PARAM_MOTOR_STATUS) {
                        io_states_temp.disable_motor = !io_states_temp.disable_motor;
                    } else if (io_selected_param == IO_PARAM_CONTACTOR) {
                        io_states_temp.contactor_state = !io_states_temp.contactor_state;
                    } else if (io_selected_param == IO_PARAM_LIGHT) {
                        io_states_temp.light_state = !io_states_temp.light_state;
                    }
                } else {
                    // Chế độ navigation - chuyển menu forward (chỉ menu hợp lệ)
                    current_detail_menu = find_next_allowed_detail_menu(current_detail_menu);
                    debug_print("Detail menu switched forward\r\n");
                }
            } else {
                // Không phải menu throttle hoặc IO - chuyển menu forward (chỉ menu hợp lệ)
                current_detail_menu = find_next_allowed_detail_menu(current_detail_menu);
                debug_print("Detail menu switched forward\r\n");
            }
        }
    } else if (button_up_pressed && HAL_GetTick() - button_up_time > BUTTON_LONG_PRESS_TIME_MS) {
        // Long press - tăng nhanh cho speed
        if (menu_display_mode == MENU_MODE_DETAIL &&
            current_detail_menu == MENU_DETAIL_THROTTLE_CONTROL && 
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
        if (menu_display_mode == MENU_MODE_OVERVIEW) {
            // Ở overview mode, không làm gì
        } else if (menu_display_mode == MENU_MODE_DETAIL) {
            // Ở detail mode, xử lý menu chi tiết
            if (current_detail_menu == MENU_DETAIL_THROTTLE_CONTROL) {
            if (throttle_mode == THROTTLE_EDIT_SELECT) {
                // Chế độ chọn thông số - chọn thông số tiếp theo
                if (throttle_selected_param < THROTTLE_PARAM_SPEED) {
                    throttle_selected_param++;
                } else {
                    throttle_selected_param = THROTTLE_PARAM_BRAKE; // Quay vòng
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
                    // Chuyển direction: Forward -> Neutral -> Reverse -> Forward
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
                // Chế độ navigation - chuyển menu backward (chỉ menu hợp lệ)
                current_detail_menu = find_prev_allowed_detail_menu(current_detail_menu);
                debug_print("Detail menu switched backward\r\n");
            }
        } else if (current_detail_menu == MENU_DETAIL_IO_INFO) {
            if (io_mode == IO_EDIT_SELECT) {
                // Chế độ chọn relay - chọn relay tiếp theo
                if (io_selected_param < IO_PARAM_LIGHT) {
                    io_selected_param++;
                } else {
                    io_selected_param = IO_PARAM_MOTOR_STATUS; // Quay vòng
                }
                debug_print("IO parameter selection changed\r\n");
            } else if (io_mode == IO_EDIT_VALUE) {
                // Chế độ chỉnh sửa trạng thái - toggle relay
                if (io_selected_param == IO_PARAM_MOTOR_STATUS) {
                    io_states_temp.disable_motor = !io_states_temp.disable_motor;
                } else if (io_selected_param == IO_PARAM_CONTACTOR) {
                    io_states_temp.contactor_state = !io_states_temp.contactor_state;
                } else if (io_selected_param == IO_PARAM_LIGHT) {
                    io_states_temp.light_state = !io_states_temp.light_state;
                }
            } else {
                // Chế độ navigation - chuyển menu backward (chỉ menu hợp lệ)
                current_detail_menu = find_prev_allowed_detail_menu(current_detail_menu);
                debug_print("Detail menu switched backward\r\n");
            }
        } else {
            // Không phải menu throttle hoặc IO - chuyển menu backward (chỉ menu hợp lệ)
            current_detail_menu = find_prev_allowed_detail_menu(current_detail_menu);
            debug_print("Detail menu switched backward\r\n");
        }
        }
    } else if (button_down_pressed && HAL_GetTick() - button_down_time > BUTTON_LONG_PRESS_TIME_MS) {
        // Long press - giảm nhanh cho speed
        if (menu_display_mode == MENU_MODE_DETAIL &&
            current_detail_menu == MENU_DETAIL_THROTTLE_CONTROL && 
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
    static uint8_t last_enter_long_press_handled = 0;

    if (button_enter_pressed && !last_button_enter) {
        enter_long_press_handled = 0;
        if (menu_display_mode == MENU_MODE_OVERVIEW) {
            // Ở overview mode, nhấn ENTER chuyển sang detail mode
            menu_display_mode = MENU_MODE_DETAIL;
            // Tìm menu chi tiết hợp lệ đầu tiên
            current_detail_menu = find_next_allowed_detail_menu(MENU_DETAIL_BMS_INFO);
            debug_print("Switched to detail mode\r\n");
        }
    } else if (!button_enter_pressed && last_button_enter && !last_enter_long_press_handled) {
        enter_long_press_handled = 0; // Reset flag khi bắt đầu nhấn
        if (menu_display_mode == MENU_MODE_DETAIL) {
            // Ở detail mode, xử lý menu chi tiết
            if (current_detail_menu == MENU_DETAIL_THROTTLE_CONTROL) {
                if (throttle_mode == THROTTLE_NAVIGATION) {
                    // Lần đầu ấn ENTER - vào chế độ chọn thông số
                    throttle_mode = THROTTLE_EDIT_SELECT;
                    throttle_selected_param = THROTTLE_PARAM_BRAKE; // Reset về thông số đầu tiên
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
                    can_slider_vcu = throttle_vehicle_mode_temp;
                    throttle_mode = THROTTLE_EDIT_SELECT; // Quay về chế độ chọn
                }
            } else if (current_detail_menu == MENU_DETAIL_IO_INFO) {
                if (io_mode == IO_NAVIGATION) {
                    // Lần đầu ấn ENTER - vào chế độ chọn relay
                    io_mode = IO_EDIT_SELECT;
                    io_selected_param = IO_PARAM_MOTOR_STATUS; // Reset về relay đầu tiên
                    debug_print("Entered IO select mode\r\n");
                } else if (io_mode == IO_EDIT_SELECT) {
                    // Ấn ENTER lần 2 - vào chế độ chỉnh sửa trạng thái
                    io_mode = IO_EDIT_VALUE;
                    // Khởi tạo biến trung gian từ giá trị GPIO hiện tại
                    io_states_temp.disable_motor = (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_SET) ? 1 : 0;
                    io_states_temp.contactor_state = (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3) == GPIO_PIN_SET) ? 1 : 0;
                    io_states_temp.light_state = (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_6) == GPIO_PIN_SET) ? 1 : 0;
                    debug_print("Entered IO edit value mode\r\n");
                } else if (io_mode == IO_EDIT_VALUE) {
                    // Ấn ENTER lần 3 - lưu giá trị và áp dụng vào GPIO
                    if (io_selected_param == IO_PARAM_MOTOR_STATUS) {
                        vcu_ctx.inputs.disable_motor_request = io_states_temp.disable_motor;
                        debug_print("Motor state saved\r\n");
                    } else if (io_selected_param == IO_PARAM_CONTACTOR) {
                        vcu_ctx.inputs.contactor_request = io_states_temp.contactor_state;
                        debug_print("Contactor state saved\r\n");
                    } else if (io_selected_param == IO_PARAM_LIGHT) {
                        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, io_states_temp.light_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
                        debug_print("Light state saved\r\n");
                    }
                    io_mode = IO_EDIT_SELECT; // Quay về chế độ chọn
                }
            }
        }
    } else if (button_enter_pressed && 
                HAL_GetTick() - button_enter_time > ENTER_LONG_PRESS_TIME_MS && 
                !enter_long_press_handled) {
        // Long press ENTER - quay về overview mode hoặc thoát khỏi chế độ edit
        if (menu_display_mode == MENU_MODE_DETAIL) {
            enter_long_press_handled = 1;
            if (current_detail_menu == MENU_DETAIL_THROTTLE_CONTROL) {
                if (throttle_mode == THROTTLE_EDIT_SELECT || throttle_mode == THROTTLE_EDIT_VALUE) {
                    // Thoát về chế độ điều hướng
                    throttle_mode = THROTTLE_NAVIGATION;
                    debug_print("Exited throttle edit mode\r\n");
                } else {
                    // Quay về overview mode
                    menu_display_mode = MENU_MODE_OVERVIEW;
                    debug_print("Returned to overview mode\r\n");
                }
            } else if (current_detail_menu == MENU_DETAIL_IO_INFO) {
                if (io_mode == IO_EDIT_SELECT || io_mode == IO_EDIT_VALUE) {
                    // Thoát về chế độ điều hướng
                    io_mode = IO_NAVIGATION;
                    debug_print("Exited IO edit mode\r\n");
                } else {
                    // Quay về overview mode
                    menu_display_mode = MENU_MODE_OVERVIEW;
                    debug_print("Returned to overview mode\r\n");
                }
            } else {
                // Quay về overview mode
                menu_display_mode = MENU_MODE_OVERVIEW;
                debug_print("Returned to overview mode\r\n");
            }
        }
    }

    last_enter_long_press_handled = enter_long_press_handled;

    if (!button_enter_pressed) {
        enter_long_press_handled = 0; // Reset flag khi thả nút
    }

    last_button_enter = button_enter_pressed;
}
  
void process_menu(void) {
	vcu_state_t current_vcu_state = VCU_StateGet();
	
	// Tự động chuyển menu chính theo VCU state
	if (current_vcu_state != last_vcu_state) {
		current_main_menu = vcu_state_to_main_menu(current_vcu_state);
		last_vcu_state = current_vcu_state;
		menu_display_mode = MENU_MODE_OVERVIEW; // Reset về overview khi chuyển state
	}
	
	// Nếu ở chế độ overview, hiển thị menu chính
	if (menu_display_mode == MENU_MODE_OVERVIEW) {
        throttle_mode = THROTTLE_NAVIGATION;
        io_mode = IO_NAVIGATION;
		switch (current_main_menu) {
		case MENU_MAIN_INIT:
			display_main_init();
			break;
		case MENU_MAIN_CAN:
			display_main_can();
			break;
		case MENU_MAIN_PHYSICAL:
			display_main_physical();
			break;
		case MENU_MAIN_CHARGE:
			display_main_charge();
			break;
		case MENU_MAIN_ERROR:
			display_main_error();
			break;
		default:
			display_main_init();
			break;
		}
	} else {
		// Chế độ detail: hiển thị menu chi tiết
		// Kiểm tra menu chi tiết có được phép không
		if (!is_detail_menu_allowed(current_detail_menu)) {
			// Nếu menu không được phép, chuyển sang menu hợp lệ đầu tiên
			current_detail_menu = find_next_allowed_detail_menu(MENU_DETAIL_BMS_INFO);
		}
		
		// Hiển thị menu chi tiết tương ứng
		switch (current_detail_menu) {
		case MENU_DETAIL_CAN_INFO_1:
			display_can_info_1();
			break;
		case MENU_DETAIL_CAN_INFO_2:
			display_can_info_2();
			break;
		case MENU_DETAIL_THROTTLE_CONTROL:
			display_throttle_control();
			break;
		case MENU_DETAIL_BMS_INFO:
			display_bms_info();
			break;
		case MENU_DETAIL_ALM_BMS:
			display_alm_bms();
			break;
		case MENU_DETAIL_IO_INFO:
			display_io_info();
			break;
		case MENU_DETAIL_BMS_BATT_ST2:
			display_bms_batt_st2();
			break;
		case MENU_DETAIL_BMS_ALL_TEMP:
			display_bms_all_temp();
			break;
		case MENU_DETAIL_BMS_ERR_INFO:
			display_bms_err_info();
			break;
		case MENU_DETAIL_BMS_INFO_SYS:
			display_bms_info_sys();
			break;
		case MENU_DETAIL_BMS_SW_STA:
			display_bms_sw_sta();
			break;
		case MENU_DETAIL_BMS_CELLVOL:
			display_bms_cellvol();
			break;
		case MENU_DETAIL_BMS_CELLVOL_2:
			display_bms_cellvol_2();
			break;
		case MENU_DETAIL_BMS_CHG_INFO:
			display_bms_chg_info();
			break;
        case MENU_DETAIL_NETWORK:
            display_network();
            break;
		default:
			display_bms_info();
			break;
		}
	}
}
