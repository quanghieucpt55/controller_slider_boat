/*
 * Led_Error.c
 *
 *  Created on: Nov 14, 2025
 *      Author: quang
 */

#include "Led_Error.h"

led_error_ctrl_t led_error_ctrl = {0};

/**
 * Thu thập tất cả lỗi từ BMS và Slider vào mảng
 * error_list: Mảng để lưu danh sách mã lỗi (1-18)
 * Số lượng lỗi tìm được
 * return: Số lượng lỗi tìm được
 */
static uint8_t collect_errors(uint8_t *error_list) {
	uint8_t count = 0;
	
	// Thu thập lỗi BMS (mã 1-10)
	if (bms.almInfo.cell_overvolt > 0) error_list[count++] = 1;
	if (bms.almInfo.cell_undervolt > 0) error_list[count++] = 2;
	if (bms.almInfo.delta_over > 0) error_list[count++] = 3;
	if (bms.almInfo.dchg_oc > 0) error_list[count++] = 4;
	if (bms.almInfo.chg_oc > 0) error_list[count++] = 5;
	if (bms.almInfo.temp_high > 0) error_list[count++] = 6;
	if (bms.almInfo.temp_low > 0) error_list[count++] = 7;
	if (bms.almInfo.soc_low > 0) error_list[count++] = 8;
	if (bms.almInfo.comm_fault > 0) error_list[count++] = 9;
	
	// Thu thập lỗi Slider (mã 10-18)
	if (can_slider.effective_raw_err_code > 0) {
	  uint8_t slider_error = can_slider.effective_raw_err_code;
	  uint8_t mapped_error = 0;
  
	  switch (slider_error) {
		case 3:  mapped_error = 10; break; // OVER_CURRENT -> Lỗi 10
		case 4:  mapped_error = 11; break; // CONTROLLER_TEMP_HIGH -> Lỗi 11
		case 7:  mapped_error = 12; break; // MOTOR_ENCODER_ERROR -> Lỗi 12
		case 8:  mapped_error = 13; break; // COMMUNICATION_ERROR -> Lỗi 13
		case 9:  mapped_error = 14; break; // UNDER_VOLTAGE_BATTERY -> Lỗi 14
		case 10: mapped_error = 15; break; // OVER_VOLTAGE_BATTERY -> Lỗi 15
		case 11: mapped_error = 16; break; // MOTOR_TEMP_HIGH -> Lỗi 16
		case 12: mapped_error = 17; break; // MOTOR_TEMP_SENSOR_ERROR -> Lỗi 17
		case 13: mapped_error = 18; break; // ACCELERATOR_FAULT -> Lỗi 18
		default: break;
	  }
	  
	  if (mapped_error > 0) {
		error_list[count++] = mapped_error;
	  }
	}
	
	return count;
}

/*
* Nháy LED một số lần
* port GPIO port của LED
* pin GPIO pin của LED
* times: Số lần nháy (0 = không nháy, bỏ qua)
* ctrl: Con trỏ đến control structure
* 1: nếu đang nháy, 0: nếu đã hoàn thành
*/
static uint8_t blink_led(GPIO_TypeDef* port, uint16_t pin, uint8_t times, led_error_ctrl_t *ctrl) {
uint32_t current_time = HAL_GetTick();

// k nháy, bỏ qua hàm
if (times == 0) {
	return 0;
}

if (ctrl->blink_count < times * 2) { // Mỗi lần nháy = sáng + tắt
	if (ctrl->blink_count % 2 == 0) {
	// LED sáng
	// Lần đầu tiên (blink_count = 0) hoặc đã đủ thời gian tắt --> chuyển sang bật
	if (ctrl->blink_count == 0 || (current_time - ctrl->last_blink_time >= LED_BLINK_OFF_TIME_MS)) {
		HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
		ctrl->last_blink_time = current_time;
		ctrl->blink_count++;
	}
	} else {
	// LED tắt
	if (current_time - ctrl->last_blink_time >= LED_BLINK_ON_TIME_MS) {
		HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
		ctrl->last_blink_time = current_time;
		ctrl->blink_count++;
	}
	}
	return 1; // Đang nháy
}
return 0; // Đã hoàn thành
}

/*Xử lý state machine hiển thị lỗi bằng LED*/
void led_process(void) {
uint32_t current_time = HAL_GetTick();
uint8_t error_list[18];
uint8_t error_count = collect_errors(error_list);

// Nếu không có lỗi, tắt cả 2 LED và reset state machine
if (error_count == 0) {
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET); // LED đỏ
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);  // LED xanh
	led_error_ctrl.state = LED_ERROR_IDLE;
	led_error_ctrl.error_count = 0;
	return;
}

// Cập nhật danh sách lỗi nếu có thay đổi
if (error_count != led_error_ctrl.error_count) {
	led_error_ctrl.error_count = error_count;
	for (uint8_t i = 0; i < error_count; i++) {
	led_error_ctrl.error_list[i] = error_list[i];
	}
	// Reset về lỗi đầu tiên nếu danh sách thay đổi
	if (led_error_ctrl.current_error_idx >= error_count) {
	led_error_ctrl.current_error_idx = 0;
	}
}

// Xử lý state machine
switch (led_error_ctrl.state) {
	case LED_ERROR_IDLE:
	// Bắt đầu hiển thị lỗi đầu tiên - Xanh nháy 1 lần
	led_error_ctrl.current_error_idx = 0;
	led_error_ctrl.blink_count = 0;
	led_error_ctrl.last_blink_time = current_time;
	led_error_ctrl.state = LED_ERROR_SHOW_GREEN_1;
	break;
	
	case LED_ERROR_SHOW_GREEN_1:
	// Xanh nháy 1 lần (báo hiệu sắp hiển thị chữ số đầu)
	if (!blink_led(GPIOC, GPIO_PIN_5, 1, &led_error_ctrl)) {
		// Đã hoàn thành nháy xanh 1 lần
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
		led_error_ctrl.blink_count = 0; // reset count cho lần nháy sau
		led_error_ctrl.last_blink_time = current_time;
		led_error_ctrl.state = LED_ERROR_SHOW_RED_FIRST_DIGIT;
	}
	break;
	
	case LED_ERROR_SHOW_RED_FIRST_DIGIT: {
	// Đỏ nháy theo chữ số đầu
	uint8_t error_code = led_error_ctrl.error_list[led_error_ctrl.current_error_idx];
	uint8_t first_digit = error_code / 10; // Chữ số hàng chục
	
	if (!blink_led(GPIOD, GPIO_PIN_10, first_digit, &led_error_ctrl)) {
		// Đã hoàn thành nháy đỏ chữ số đầu
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET);
		led_error_ctrl.state = LED_ERROR_PAUSE_AFTER_FIRST;
		led_error_ctrl.pause_start_time = current_time;
	}
	break;
	}
	
	case LED_ERROR_PAUSE_AFTER_FIRST:
	// Tạm dừng sau chữ số đầu
	if (current_time - led_error_ctrl.pause_start_time >= LED_PAUSE_AFTER_FIRST_MS) {
		led_error_ctrl.blink_count = 0;
		led_error_ctrl.last_blink_time = current_time;
		led_error_ctrl.state = LED_ERROR_SHOW_GREEN_2;
	}
	break;
	
	case LED_ERROR_SHOW_GREEN_2:
	// Xanh nháy 2 lần (báo hiệu sắp hiển thị chữ số thứ hai)
	if (!blink_led(GPIOC, GPIO_PIN_5, 2, &led_error_ctrl)) {
		// Đã hoàn thành nháy xanh 2 lần
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
		led_error_ctrl.blink_count = 0;
		led_error_ctrl.last_blink_time = current_time;
		led_error_ctrl.state = LED_ERROR_SHOW_RED_SECOND_DIGIT;
	}
	break;
	
	case LED_ERROR_SHOW_RED_SECOND_DIGIT: {
	// Đỏ nháy theo chữ số thứ hai
	uint8_t error_code = led_error_ctrl.error_list[led_error_ctrl.current_error_idx];
	uint8_t second_digit = error_code % 10; // Chữ số hàng đơn vị
	
	if (!blink_led(GPIOD, GPIO_PIN_10, second_digit, &led_error_ctrl)) {
		// Đã hoàn thành nháy đỏ chữ số thứ hai
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET);
		led_error_ctrl.state = LED_ERROR_PAUSE_BETWEEN_ERRORS;
		led_error_ctrl.pause_start_time = current_time;
	}
	break;
	}
	
	case LED_ERROR_PAUSE_BETWEEN_ERRORS:
	// Tạm dừng giữa các lỗi
	if (current_time - led_error_ctrl.pause_start_time >= LED_PAUSE_BETWEEN_ERRORS_MS) {
		// Chuyển sang lỗi tiếp theo
		led_error_ctrl.current_error_idx++;
		if (led_error_ctrl.current_error_idx >= led_error_ctrl.error_count) {
		// Đã hiển thị hết tất cả lỗi, quay lại lỗi đầu tiên
		led_error_ctrl.current_error_idx = 0;
		}
		led_error_ctrl.blink_count = 0;
		led_error_ctrl.last_blink_time = current_time;
		led_error_ctrl.state = LED_ERROR_SHOW_GREEN_1;
	}
	break;
}
}
