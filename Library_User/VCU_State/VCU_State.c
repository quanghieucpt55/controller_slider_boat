/*
 * VCU_State.c
 *
 *  Created on: Nov 19, 2025
 *      Author: quang
 */

#include "VCU_State.h"

#include <string.h>

vcu_state_context_t vcu_ctx;

boat_error_count_t boat_error_count = {1};

vcu_state_t vcu_state = VCU_STATE_INIT;

static bool VCU_HasCriticalFault(void);
static void VCU_UpdateInputs(void);
static void VCU_ApplyOutputs(void);
static void VCU_UpdateOutput(void);

static void VCU_OnEnterState(vcu_state_t state);
static void VCU_OnExitState(vcu_state_t state);
static void VCU_TransitionTo(vcu_state_t next_state);

static vcu_state_t VCU_StateHandleInit(void);
static vcu_state_t VCU_StateHandleCan(void);
static vcu_state_t VCU_StateHandlePhysical(void);
static vcu_state_t VCU_StateHandleCharge(void);
static vcu_state_t VCU_StateHandleError(void);

static void VCU_ChargerModeEnter(void);
static void VCU_ErrorModeEnter(void);
static void VCU_InitModeEnter(void);

static void VCU_ErrorModeExit(void);
static void VCU_ChargerModeExit(void);
static void VCU_InitModeExit(void);

/**
 * @brief Khởi tạo VCU State Machine
 * 
 * Hàm này được gọi một lần khi hệ thống khởi động để:
 * - Xóa toàn bộ dữ liệu context về 0
 * - Đặt trạng thái ban đầu là INIT
 * - Tắt tất cả các output (contactor, motor status)
 * - Ghi nhận thời điểm khởi tạo
 */
void VCU_StateInit(void)
{
	memset(&vcu_ctx, 0, sizeof(vcu_ctx));
	vcu_state = VCU_STATE_INIT;
	vcu_ctx.current_state = VCU_STATE_INIT;
	vcu_ctx.inputs.contactor_request = false;
	vcu_ctx.inputs.disable_motor_request = true;
	vcu_ctx.last_transition_tick = HAL_GetTick();
	vcu_ctx.inputs.init_completed = false;
}

/**
 * @brief Hàm chính xử lý State Machine
 * 
 * Hàm này thực hiện các bước sau:
 * 1. Xử lý logic của trạng thái hiện tại để quyết định trạng thái tiếp theo
 * 2. Nếu có thay đổi trạng thái, thực hiện chuyển đổi
 * 3. Cập nhật các output (relay, mode selection)
 * 4. Áp dụng các output ra GPIO
 */
void VCU_StateTask(void)
{
	static uint32_t send_time = 0;
	vcu_state_t next_state = vcu_state;

	/* Cập nhật và áp dụng các input và output */
	VCU_UpdateInputs();
	VCU_UpdateOutput();
	VCU_ApplyOutputs();

	 if (HAL_GetTick() - send_time > 500) {
		send_time = HAL_GetTick();
		Can_Vcu_Send_Slider(&hcan1);
	 }

	/* Xử lý logic của trạng thái hiện tại để quyết định trạng thái tiếp theo */
	switch (vcu_state) {
	case VCU_STATE_INIT:
		next_state = VCU_StateHandleInit();
		break;
	case VCU_STATE_CAN:
		next_state = VCU_StateHandleCan();
		break;
	case VCU_STATE_PHYSICAL:
		next_state = VCU_StateHandlePhysical();
		break;
	case VCU_STATE_CHARGE:
		next_state = VCU_StateHandleCharge();
		break;
	case VCU_STATE_ERROR:
		next_state = VCU_StateHandleError();
		break;
	}

	/* Nếu trạng thái thay đổi, thực hiện chuyển đổi */
	if (next_state != vcu_state) {
		VCU_TransitionTo(next_state);
	}
}

/**
 * @brief Lấy trạng thái hiện tại của VCU
 */
vcu_state_t VCU_StateGet(void)
{
	return vcu_state;
}

/**
 * @brief Lấy con trỏ đến các output hiện tại của VCU
 * @return Con trỏ đến struct chứa các output 
 */
const vcu_state_outputs_t *VCU_StateOutputs(void)
{
	return &vcu_ctx.outputs;
}

/**
 * @brief Hàm set input từ ngoài
 */

void VCU_StateSetMotorStatus(motor_status_t status)
{
	vcu_ctx.inputs.disable_motor_request = status == DISABLE_MOTOR ? true : false;
}

motor_status_t VCU_StateGetMotorStatus(void)
{
	return vcu_ctx.inputs.disable_motor_request == true ? DISABLE_MOTOR : ENABLE_MOTOR;
}

/**
 * @brief Chuyển đổi trạng thái enum sang chuỗi ký tự để hiển thị
 */
const char *VCU_StateToString(vcu_state_t state)
{
	switch (state) {
	case VCU_STATE_INIT:
		return "INIT";
	case VCU_STATE_CAN:
		return "CAN";
	case VCU_STATE_PHYSICAL:
		return "PHYSICAL";
	case VCU_STATE_CHARGE:
		return "CHARGE";
	case VCU_STATE_ERROR:
	default:
		return "ERROR";
	}
}

static void VCU_UpdateInputs(void) {
	if (memcmp(&bms.bmsErrInfo, &(BMS_BmsErrInfo_t){0}, sizeof(BMS_BmsErrInfo_t)) == 0) {
		vcu_ctx.inputs.bms_critical_error = false;
		if (bms.batt1.voltage_V > 0 && memcmp(&bms.almInfo, &(BMS_AlmInfo_t){0}, sizeof(BMS_AlmInfo_t)) == 0) {
			vcu_ctx.inputs.bms_ok = true;
		} else {
			vcu_ctx.inputs.bms_ok = false;
		}
	} else {
		vcu_ctx.inputs.bms_critical_error = true;
	}
	if (can_slider.slider_1.error_code != 0) {
		vcu_ctx.inputs.slider_critical_error = true;	
	} else {
		vcu_ctx.inputs.slider_critical_error = false;
		if (can_slider.slider_2.battery_voltage > 0 && can_slider.slider_1.vehicle_mode.forward == 0 
			&& can_slider.slider_1.vehicle_mode.reverse == 0 && can_slider.slider_1.motor_rpm < 100) {
			vcu_ctx.inputs.slider_ok = true;
		} else {
			vcu_ctx.inputs.slider_ok = false;
		}
	}
	
	/* Đọc thông tin sạc từ BMS (BMS quản lý bộ sạc) */
	/* chgPlug = 1 : đã cắm sạc */
	vcu_ctx.inputs.charger_plugged = (bms.swSta.chgPlug == 1);
	
	/* Sạc dừng khi: chg_dev_sw = 1 (tắt) hoặc SOC >= 100% */
	vcu_ctx.inputs.charger_stopped_full = (bms.chgInfo.chg_dev_sw == 1) || 
	                                      (bms.batt1.soc_percent >= 100);
}

/**
 * @brief Kiểm tra xem có lỗi nghiêm trọng (Critical Fault) không
 */
static bool VCU_HasCriticalFault(void)
{
	if (vcu_ctx.inputs.slider_critical_error) {
		if (++boat_error_count.count_error_slider > 100) {
			boat_error_count.count_error_slider = 100;
			return true;
		}
	} else {
		if (--boat_error_count.count_error_slider == 0) {
			boat_error_count.count_error_slider = 1;
			return false;
		}
	}
	if (vcu_ctx.inputs.bms_critical_error) {
		if (++boat_error_count.count_error_bms > 100) {
			boat_error_count.count_error_bms = 100;
			return true;
		}
	} else {
		if (--boat_error_count.count_error_bms == 0) {
			boat_error_count.count_error_bms = 1;
			return false;
		}
	}
	return true;
}

/**
 * @brief Áp dụng các output ra GPIO hardware
 */
static void VCU_ApplyOutputs(void)
{
	/* Điều khiển relay PE3 để đóng/mở contactor nguồn */
	HAL_GPIO_WritePin(RELAY_CONTACTOR_GPIO_Port,
			  RELAY_CONTACTOR_Pin,
			  vcu_ctx.outputs.contactor_on ? GPIO_PIN_SET : GPIO_PIN_RESET);

	/* Điều khiển relay PE2 đóng/mở động cơ */
	HAL_GPIO_WritePin(RELAY_DISABLE_MOTOR_GPIO_Port,
			  RELAY_DISABLE_MOTOR_Pin,
			  vcu_ctx.outputs.disable_motor ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Cập nhật output từ input
 */
static void VCU_UpdateOutput(void)
{
	if (vcu_ctx.outputs.contactor_on != vcu_ctx.inputs.contactor_request) 
	{
		vcu_ctx.outputs.contactor_on = vcu_ctx.inputs.contactor_request;
	}

	if (vcu_ctx.outputs.disable_motor != vcu_ctx.inputs.disable_motor_request) 
	{
		vcu_ctx.outputs.disable_motor = vcu_ctx.inputs.disable_motor_request;
	}
}

/**
 * @brief Hàm được gọi khi vào một trạng thái mới
 */
static void VCU_OnEnterState(vcu_state_t state)
{
	if (state == VCU_STATE_CHARGE) {
		VCU_ChargerModeEnter();
	}
	if (state == VCU_STATE_ERROR) {
		VCU_ErrorModeEnter();
	}
	if (state == VCU_STATE_INIT) {
		VCU_InitModeEnter();
	}
}

/**
 * @brief Hàm được gọi khi rời khỏi một trạng thái
 */
static void VCU_OnExitState(vcu_state_t state)
{
	if (state == VCU_STATE_CHARGE) {
		VCU_ChargerModeExit();
	}
	if (state == VCU_STATE_ERROR) {
		VCU_ErrorModeExit();
	}
	if (state == VCU_STATE_INIT) {
		VCU_InitModeExit();
	}
}

/**
 * @brief Thực hiện chuyển đổi trạng thái
 * 
 */
static void VCU_TransitionTo(vcu_state_t next_state)
{
	VCU_OnExitState(vcu_state);
	vcu_state = next_state;
	vcu_ctx.current_state = next_state;
	vcu_ctx.last_transition_tick = HAL_GetTick();
	VCU_OnEnterState(next_state);
}

/**
 * @brief Xử lý logic của trạng thái INIT & SELF-CHECK
 * 
 * Đây là trạng thái đầu tiên khi hệ thống khởi động.
 * Thực hiện kiểm tra tự động:
 * - Kiểm tra lỗi nghiêm trọng (BMS, Slider, System)
 * - Kiểm tra trạng thái BMS và Slider
 * - Kiểm tra có sạc không
 * 
 * Quyết định chuyển trạng thái:
 * 1. Có lỗi nghiêm trọng → ERROR
 * 2. BMS OK + Slider OK + Contactor ON + Không sạc → PHYSICAL (chạy ngay)
 * 3. Có sạc → CHARGE
 * 
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandleInit(void)
{
	/* Kiểm tra Critical Error - nếu có lỗi nghiêm trọng, chuyển ngay sang ERROR */
	if (VCU_HasCriticalFault()) {
		return VCU_STATE_ERROR;
	}
	/* Nếu có charger → chuyển sang CHARGER MODE để sạc pin */
	if (vcu_ctx.inputs.charger_plugged) {
		return VCU_STATE_CHARGE;
	}
	/* Nếu BMS OK, Contactor ON + Slider OK, No Charger → chuyển thẳng sang Physical MODE */
	// if (vcu_ctx.inputs.bms_ok &&
	//     !vcu_ctx.inputs.charger_plugged &&
	if	(HAL_GetTick() - vcu_ctx.last_transition_tick > 3000) {
		vcu_ctx.inputs.contactor_request = true;
		if (vcu_ctx.inputs.slider_ok) {
			vcu_ctx.inputs.init_completed = true;
			return VCU_STATE_PHYSICAL;
		}
	}
	return VCU_STATE_INIT;
}

/**
 * @brief Xử lý logic của trạng thái CAN MODE
 * 
 * Trạng thái điều khiển boat qua giao thức CAN:
 * - Đọc tham số từ CAN (Parameter, ALM/ERROR, I/O Status)
 * - Điều khiển phanh, hướng, tốc độ động cơ qua CAN
 * 
 * Quyết định chuyển trạng thái:
 * 1. Có lỗi nghiêm trọng → ERROR
 * 2. Có sạc → CHARGE (ưu tiên sạc)
 * 3. Driver gửi trạng thái Physical mode → PHYSICAL
 * 
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandleCan(void)
{
	if (vcu_ctx.inputs.charger_plugged) {
		return VCU_STATE_CHARGE;
	}

	if (can_slider.slider_2.control_mode == 0) {
		return VCU_STATE_PHYSICAL;
	}

	/* CAN MODE: Parameter, ALM/ERROR; I/O Status; Control Brake, Direction; Control Motor Speed */
	if (VCU_HasCriticalFault()) {
		return VCU_STATE_ERROR;
	}

	return VCU_STATE_CAN;
}

/**
 * @brief Xử lý logic của trạng thái PHYSICAL MODE
 * 
 * Trạng thái điều khiển xe qua chân ga vật lý (không qua CAN):
 * - Đọc tham số từ BMS và Motor
 * - Giám sát I/O Status và ALM/ERROR
 * - Contactor luôn ON để cấp nguồn
 * 
 * Quyết định chuyển trạng thái:
 * 1. Có lỗi nghiêm trọng → ERROR
 * 2. Có sạc → CHARGE (ưu tiên sạc, sẽ tắt contactor)
 * 
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandlePhysical(void)
{
	if (vcu_ctx.inputs.charger_plugged) {
		return VCU_STATE_CHARGE;
	}

	if (can_slider.slider_2.control_mode == 1) {
		return VCU_STATE_CAN;
	}

	if (VCU_HasCriticalFault()) {
		return VCU_STATE_ERROR;
	}

	return VCU_STATE_PHYSICAL;
}

/**
 * @brief Xử lý logic của trạng thái CHARGE (CHARGER MODE)
 * 
 * Trạng thái sạc pin:
 * - Contactor luôn OFF (ngắt nguồn động cơ để sạc)
 * - Chỉ đọc thông tin sạc từ BMS (BMS quản lý bộ sạc)
 * - Giám sát quá trình sạc thông qua BMS
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandleCharge(void)
{
	if (VCU_HasCriticalFault()) {
		return VCU_STATE_ERROR;
	}

	if (!vcu_ctx.inputs.charger_plugged) {
		return VCU_STATE_PHYSICAL;
	}

	return VCU_STATE_CHARGE;
}

/**
 * @brief Xử lý logic của trạng thái ERROR
 * 
 * Trạng thái lỗi nghiêm trọng:
 * - Disable Motor
 * - Hiển thị trạng thái lỗi và mô tả lỗi
 * - VCU thực hiện các hành động an toàn
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandleError(void)
{
	if (!VCU_HasCriticalFault()) {
		if (vcu_ctx.inputs.init_completed) {
			return VCU_STATE_PHYSICAL;
		} else {
			return VCU_STATE_INIT;
		}
	}
	return VCU_STATE_ERROR;
}

/**
 * @brief Hàm được gọi khi vào trạng thái CHARGE
 * 
 * Thực hiện các bước khởi tạo khi bắt đầu sạc:
 * - Disable Motor
 * - BMS sẽ tự động quản lý bộ sạc, VCU chỉ đọc thông tin
 * 
 * Lưu ý: BMS quản lý bộ sạc, VCU chỉ giám sát thông tin từ BMS
 */
static void VCU_ChargerModeEnter(void)
{
	/* Tắt contactor khi vào chế độ sạc */
	vcu_ctx.inputs.contactor_request = false;
	vcu_ctx.inputs.disable_motor_request = true;
}

static void VCU_ErrorModeEnter(void)
{
	vcu_ctx.inputs.disable_motor_request = true;
}

static void VCU_InitModeEnter(void)
{
	vcu_ctx.inputs.disable_motor_request = true;
}
/**
 * @brief Hàm được gọi khi rời khỏi trạng thái CHARGE
 * 
 * Thực hiện các bước dọn dẹp khi kết thúc sạc:
 * - BMS sẽ tự động quản lý việc dừng sạc
 * - VCU chỉ cần reset các biến trạng thái nội bộ
 * 
 * Lưu ý: BMS quản lý bộ sạc, VCU không cần gửi lệnh dừng sạc
 */
static void VCU_ChargerModeExit(void)
{
	/* BMS tự động quản lý việc dừng sạc */
	/* VCU chỉ cần đảm bảo contactor vẫn tắt khi rời khỏi CHARGE state */
	vcu_ctx.inputs.contactor_request = true;
}

static void VCU_ErrorModeExit(void)
{
	vcu_ctx.inputs.disable_motor_request = false;
}

static void VCU_InitModeExit(void) 
{
	vcu_ctx.inputs.disable_motor_request = false;
}

/**
 * @brief Lấy bitmask các menu được phép hiển thị dựa trên VCU state hiện tại
 * 
 * Sử dụng bitmask để kiểm tra nhanh menu nào được phép thay vì nhiều điều kiện if/else.
 * Mỗi VCU state có một tập hợp menu riêng được phép hiển thị.
 * 
 * @return Bitmask các menu được phép (có thể OR nhiều menu lại)
 */
menu_mask_t VCU_GetAllowedDetailMenuMask(void)
{
	vcu_state_t state = VCU_StateGet();
	menu_mask_t mask = 0;
	
	switch (state) {
	case VCU_STATE_INIT:
		/* INIT: Chỉ menu thông tin cơ bản */
		mask = MENU_DETAIL_BMS_INFO | MENU_DETAIL_ALM_BMS | MENU_DETAIL_IO_INFO|
			   MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_CAN_INFO_2 | MENU_DETAIL_NETWORK;
		break;
		
	case VCU_STATE_PHYSICAL:
		/* WAITING & PHYSICAL: Tất cả menu trừ THROTTLE_CONTROL */
		mask = MENU_DETAIL_CAN_INFO_1 | MENU_DETAIL_CAN_INFO_2 | MENU_DETAIL_BMS_INFO | 
		       MENU_DETAIL_ALM_BMS | MENU_DETAIL_IO_INFO | MENU_DETAIL_BMS_BATT_ST2 | 
		       MENU_DETAIL_BMS_ALL_TEMP | MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_BMS_INFO_SYS | 
		       MENU_DETAIL_BMS_SW_STA | MENU_DETAIL_BMS_CELLVOL | MENU_DETAIL_BMS_CELLVOL_2 | 
		       MENU_DETAIL_BMS_CHG_INFO | MENU_DETAIL_NETWORK;
		break;
		
	case VCU_STATE_CAN:
		/* CAN: Tất cả menu */
		mask = MENU_DETAIL_CAN_INFO_1 | MENU_DETAIL_CAN_INFO_2 | MENU_DETAIL_THROTTLE_CONTROL | 
		       MENU_DETAIL_BMS_INFO | MENU_DETAIL_ALM_BMS | MENU_DETAIL_IO_INFO | MENU_DETAIL_BMS_BATT_ST2 | 
		       MENU_DETAIL_BMS_ALL_TEMP | MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_BMS_INFO_SYS | 
		       MENU_DETAIL_BMS_SW_STA | MENU_DETAIL_BMS_CELLVOL | MENU_DETAIL_BMS_CELLVOL_2 | 
		       MENU_DETAIL_BMS_CHG_INFO | MENU_DETAIL_NETWORK;
		break;
		
	case VCU_STATE_CHARGE:
		/* CHARGE: Ưu tiên menu sạc và BMS */
		mask = MENU_DETAIL_BMS_CHG_INFO | MENU_DETAIL_BMS_INFO | MENU_DETAIL_BMS_BATT_ST2 | 
		       MENU_DETAIL_BMS_ALL_TEMP | MENU_DETAIL_BMS_SW_STA | MENU_DETAIL_BMS_CELLVOL | 
		       MENU_DETAIL_BMS_CELLVOL_2 | MENU_DETAIL_ALM_BMS | MENU_DETAIL_BMS_ERR_INFO | 
		       MENU_DETAIL_BMS_INFO_SYS | MENU_DETAIL_IO_INFO | MENU_DETAIL_NETWORK;
		break;
		
	case VCU_STATE_ERROR:
		/* ERROR: Chỉ menu lỗi và thông tin cơ bản */
		mask = MENU_DETAIL_ALM_BMS | MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_CAN_INFO_2 | MENU_DETAIL_IO_INFO | MENU_DETAIL_NETWORK;
		break;
		
	default:
		/* Mặc định: Tất cả menu */
		mask = 0x7FFF; /* Tất cả 15 bit */
		break;
	}
	
	return mask;
}
