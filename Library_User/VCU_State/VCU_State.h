/*
 * VCU_State.h
 *
 *  Created on: Nov 19, 2025
 *      Author: quang
 */

#ifndef VCU_STATE_VCU_STATE_H_
#define VCU_STATE_VCU_STATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "Can_Slider.h"
#include "jikong_can.h"
#include <stdbool.h>

typedef enum {
	VCU_STATE_INIT = 0,        // INIT & SELF-CHECK - khởi tạo lúc khởi động
	VCU_STATE_WAITING = 1,     // Waiting F/R → N, Waiting RPM <100 - chờ tín hiệu ổn định trước khi cho phép điều khiển
	VCU_STATE_CAN = 2,         // CAN MODE - giám sát hệ thống và điều khiển qua CAN
	VCU_STATE_PHYSICAL = 3,    // Physical MODE - giám sát hệ thống và điều khiển qua chân ga, cần số
	VCU_STATE_CHARGE = 4,      // CHARGER MODE - chế độ sạc pin
	VCU_STATE_IDLE = 5,        // IDLE - giám sát cơ bản khi động cơ nghỉ
	VCU_STATE_ERROR = 6,       // ERROR STATE - giám sát lỗi, mô tả lỗi
} vcu_state_t;

extern vcu_state_t vcu_state;

typedef struct {
	bool bms_ok;                  // BMS báo OK, không lỗi pin
	bool slider_ok;               // Bộ controller slider ở trạng thái cho phép chạy
	bool charger_plugged;         // Đang cắm sạc
	bool charger_error;           // Lỗi do bộ sạc báo về
	bool charger_stopped_full;    // Sạc đã dừng do đầy
	bool bms_critical_error;      // Lỗi BMS
	bool slider_critical_error;   // Lỗi bộ điều khiển
	bool system_fault;            // Lỗi hệ thống
	bool select_can_mode_request; // Yêu cầu chọn mode CAN (true) hoặc Physical (false)
	bool contactor_request;       // Yêu cầu bật contactor (true = ON, false = OFF)
} vcu_state_inputs_t;

typedef struct {
	bool contactor_on;      // Relay PE3 đóng contactor
	bool select_can_mode;   // Mức xuất ra PE2 (0: chân ga, 1: CAN)
} vcu_state_outputs_t;

typedef struct {
	vcu_state_t current_state;         // Trạng thái hiện tại
    vcu_state_inputs_t inputs;         // các tín hiệu vào
	vcu_state_outputs_t outputs;      // Lệnh xuất ra relay/driver
	uint32_t last_transition_tick;    // Mốc thời gian chuyển trạng thái gần nhất
} vcu_state_context_t;
extern vcu_state_context_t vcu_ctx;

// Menu chính - tương ứng với 7 VCU states
typedef enum {
    MENU_MAIN_INIT = 0,      // Menu INIT - Khởi tạo và tự kiểm tra
    MENU_MAIN_WAITING = 1,   // Menu WAITING - Chờ điều kiện an toàn
    MENU_MAIN_CAN = 2,       // Menu CAN MODE - Điều khiển qua CAN
    MENU_MAIN_PHYSICAL = 3,  // Menu PHYSICAL MODE - Điều khiển vật lý
    MENU_MAIN_CHARGE = 4,    // Menu CHARGE - Chế độ sạc
    MENU_MAIN_IDLE = 5,      // Menu IDLE - Chế độ chờ
    MENU_MAIN_ERROR = 6,     // Menu ERROR - Trạng thái lỗi
} menu_main_t;

// Menu chi tiết - sử dụng bitmask (mỗi menu là một bit riêng biệt)
typedef enum {
    MENU_DETAIL_CAN_INFO_1 = 0x0001,      // Bit 0: Hiển thị các thông số giám sát Slider1
	MENU_DETAIL_CAN_INFO_2 = 0x0002,      // Bit 1: Hiển thị các thông số giám sát Slider2
    MENU_DETAIL_THROTTLE_CONTROL = 0x0004, // Bit 2: Chỉnh sửa throttle command
    MENU_DETAIL_BMS_INFO = 0x0008,        // Bit 3: Hiển thị các thông số giám sát BMS
	MENU_DETAIL_ALM_BMS = 0x0010,         // Bit 4: Hiển thị cảnh báo BMS
    MENU_DETAIL_IO_INFO = 0x0020,         // Bit 5: Hiển thị các thông số giám sát IO
    MENU_DETAIL_BMS_BATT_ST2 = 0x0040,    // Bit 6: Hiển thị dung lượng và số chu kỳ
    MENU_DETAIL_BMS_ALL_TEMP = 0x0080,    // Bit 7: Hiển thị 5 cảm biến nhiệt độ
    MENU_DETAIL_BMS_ERR_INFO = 0x0100,    // Bit 8: Hiển thị lỗi bên trong BMS
    MENU_DETAIL_BMS_INFO_SYS = 0x0200,    // Bit 9: Hiển thị thông tin hệ thống (runtime, SOH)
    MENU_DETAIL_BMS_SW_STA = 0x0400,      // Bit 10: Hiển thị trạng thái MOS
    MENU_DETAIL_BMS_CELLVOL = 0x0800,    // Bit 11: Hiển thị điện áp cell 1-8
    MENU_DETAIL_BMS_CELLVOL_2 = 0x1000,  // Bit 12: Hiển thị điện áp cell 9-16
    MENU_DETAIL_BMS_CHG_INFO = 0x2000,   // Bit 13: Hiển thị thông tin yêu cầu sạc
} menu_detail_t;

typedef menu_detail_t menu_state_t;

typedef uint16_t menu_mask_t;

/**
 * @brief Lấy bitmask các menu chi tiết được phép hiển thị dựa trên VCU state hiện tại
 * 
 * Mỗi VCU state có một tập hợp menu chi tiết được phép hiển thị khi ở chế độ detail.
 * Hàm này trả về một bitmask, mỗi bit đại diện cho một menu chi tiết.
 * Bit = 1 nghĩa là menu đó được phép, Bit = 0 nghĩa là không được phép.
 * 
 * @return Bitmask các menu chi tiết được phép (có thể OR nhiều menu lại với nhau)
 */
menu_mask_t VCU_GetAllowedDetailMenuMask(void);

void VCU_StateInit(void);
void VCU_StateTask(void);
vcu_state_t VCU_StateGet(void);
const vcu_state_outputs_t *VCU_StateOutputs(void);
const char *VCU_StateToString(vcu_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* VCU_STATE_VCU_STATE_H_ */
