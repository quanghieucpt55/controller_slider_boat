/*
 * Led_Error.h
 *
 *  Created on: Nov 14, 2025
 *      Author: quang
 */

#ifndef LED_ERROR_LED_ERROR_H_
#define LED_ERROR_LED_ERROR_H_

#include "main.h"
#include "jikong_can.h"
#include "Can_Slider.h"

// State machine cho LED error display
typedef enum {
	LED_ERROR_IDLE,
	LED_ERROR_SHOW_GREEN_1,           // Xanh nháy 1 lần (báo hiệu chữ số đầu)
	LED_ERROR_SHOW_RED_FIRST_DIGIT,   // Đỏ nháy theo chữ số đầu
	LED_ERROR_PAUSE_AFTER_FIRST,      // Tạm dừng sau chữ số đầu
	LED_ERROR_SHOW_GREEN_2,           // Xanh nháy 2 lần (báo hiệu chữ số thứ hai)
	LED_ERROR_SHOW_RED_SECOND_DIGIT,  // Đỏ nháy theo chữ số thứ hai
	LED_ERROR_PAUSE_BETWEEN_ERRORS   // Tạm dừng giữa các lỗi
} led_error_state_t;
  
typedef struct {
	led_error_state_t state;
	uint8_t error_list[18];        // Danh sách lỗi (mã 1-18)
	uint8_t error_count;           // Số lượng lỗi
	uint8_t current_error_idx;     // Chỉ số lỗi đang hiển thị
	uint8_t blink_count;            // Số lần nháy đã thực hiện
	uint32_t last_blink_time;       // Thời gian nháy cuối cùng
	uint32_t pause_start_time;      // Thời gian bắt đầu tạm dừng
} led_error_ctrl_t;
  
static led_error_ctrl_t led_error_ctrl = {0};

#define LED_BLINK_ON_TIME_MS    200   // Thời gian LED sáng (ms)
#define LED_BLINK_OFF_TIME_MS   200   // Thời gian LED tắt (ms)
#define LED_PAUSE_AFTER_FIRST_MS 500  // Tạm dừng sau chữ số đầu
#define LED_PAUSE_BETWEEN_ERRORS_MS  1000 // Tạm dừng giữa các lỗi

#endif /* LED_ERROR_LED_ERROR_H_ */
