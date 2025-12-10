/*
 * VCU_State.c
 *
 *  Created on: Nov 19, 2025
 *      Author: quang
 */

#include "VCU_State.h"

#include <string.h>

vcu_state_context_t vcu_ctx;

vcu_state_t vcu_state = VCU_STATE_INIT;

static bool VCU_HasCriticalFault(void);
static void VCU_UpdateInputs(void);
static void VCU_ApplyOutputs(void);
static void VCU_UpdateSelectModeOutput(void);
static void VCU_OnEnterState(vcu_state_t state);
static void VCU_OnExitState(vcu_state_t state);
static void VCU_TransitionTo(vcu_state_t next_state);
static vcu_state_t VCU_StateHandleInit(void);
static vcu_state_t VCU_StateHandleWaiting(void);
static vcu_state_t VCU_StateHandleCan(void);
static vcu_state_t VCU_StateHandlePhysical(void);
static vcu_state_t VCU_StateHandleCharge(void);
static vcu_state_t VCU_StateHandleIdle(void);
static vcu_state_t VCU_StateHandleError(void);

static void VCU_ChargerModeEnter(void);
static void VCU_ChargerModeProcess(void);
static void VCU_ChargerModeExit(void);

/**
 * @brief Khởi tạo VCU State Machine
 * 
 * Hàm này được gọi một lần khi hệ thống khởi động để:
 * - Xóa toàn bộ dữ liệu context về 0
 * - Đặt trạng thái ban đầu là INIT
 * - Tắt tất cả các output (contactor, select mode)
 * - Ghi nhận thời điểm khởi tạo
 */
void VCU_StateInit(void)
{
	memset(&vcu_ctx, 0, sizeof(vcu_ctx));
	vcu_state = VCU_STATE_INIT;
	vcu_ctx.current_state = VCU_STATE_INIT;
	vcu_ctx.inputs.contactor_request = false;
	vcu_ctx.inputs.select_can_mode_request = false;
	vcu_ctx.last_transition_tick = HAL_GetTick();
}

/**
 * @brief Cập nhật các tín hiệu đầu vào cho VCU State Machine
 * 
 * Hàm này được gọi từ bên ngoài (ví dụ từ main.c) để cập nhật các tín hiệu:
 * - BMS status, Slider status
 * - Charger status
 * - RPM, F/R position
 * - Các lỗi và yêu cầu điều khiển
 * 
 * @param inputs Con trỏ đến struct chứa tất cả các tín hiệu đầu vào
 */

/**
 * @brief Hàm chính xử lý State Machine - được gọi định kỳ trong vòng lặp chính
 * 
 * Hàm này thực hiện các bước sau:
 * 1. Xử lý logic của trạng thái hiện tại để quyết định trạng thái tiếp theo
 * 2. Nếu có thay đổi trạng thái, thực hiện chuyển đổi
 * 3. Cập nhật các output (relay, mode selection)
 * 4. Áp dụng các output ra GPIO
 * 
 * Lưu ý: Hàm này nên được gọi định kỳ (ví dụ mỗi 10-100ms) trong vòng lặp chính
 */
void VCU_StateTask(void)
{
	static uint32_t send_time = 0;
	vcu_state_t next_state = vcu_state;

	/* Xử lý logic của trạng thái hiện tại để quyết định trạng thái tiếp theo */
	switch (vcu_state) {
	case VCU_STATE_INIT:
		next_state = VCU_StateHandleInit();
		break;
	case VCU_STATE_WAITING:
		next_state = VCU_StateHandleWaiting();
		break;
	case VCU_STATE_CAN:
		if (HAL_GetTick() - send_time > 500) {
			send_time = HAL_GetTick();
			Can_Vcu_Send_Slider(&hcan1);
		}
		next_state = VCU_StateHandleCan();
		break;
	case VCU_STATE_PHYSICAL:
		next_state = VCU_StateHandlePhysical();
		break;
	case VCU_STATE_CHARGE:
		next_state = VCU_StateHandleCharge();
		break;
	case VCU_STATE_IDLE:
		next_state = VCU_StateHandleIdle();
		break;
	case VCU_STATE_ERROR:
	default:
		next_state = VCU_StateHandleError();
		break;
	}

	/* Nếu trạng thái thay đổi, thực hiện chuyển đổi */
	if (next_state != vcu_state) {
		VCU_TransitionTo(next_state);
	}

	/* Cập nhật và áp dụng các input và output */
	VCU_UpdateInputs();
	VCU_UpdateSelectModeOutput();
	VCU_ApplyOutputs();
}

/**
 * @brief Lấy trạng thái hiện tại của VCU
 * 
 * @return Trạng thái hiện tại (INIT, WAITING, CAN, PHYSICAL, CHARGE, IDLE, ERROR)
 */
vcu_state_t VCU_StateGet(void)
{
	return vcu_state;
}

/**
 * @brief Lấy con trỏ đến các output hiện tại của VCU
 * 
 * Hàm này cho phép các module khác (ví dụ Display) đọc trạng thái output
 * mà không cần truy cập trực tiếp vào GPIO
 * 
 * @return Con trỏ đến struct chứa các output (contactor_on, select_can_mode)
 */
const vcu_state_outputs_t *VCU_StateOutputs(void)
{
	return &vcu_ctx.outputs;
}

/**
 * @brief Chuyển đổi trạng thái enum sang chuỗi ký tự để hiển thị
 * 
 * Hữu ích cho debug và hiển thị trên LCD
 * 
 * @param state Trạng thái cần chuyển đổi
 * @return Chuỗi ký tự mô tả trạng thái
 */
const char *VCU_StateToString(vcu_state_t state)
{
	switch (state) {
	case VCU_STATE_INIT:
		return "INIT";
	case VCU_STATE_WAITING:
		return "WAITING";
	case VCU_STATE_CAN:
		return "CAN";
	case VCU_STATE_PHYSICAL:
		return "PHYSICAL";
	case VCU_STATE_CHARGE:
		return "CHARGE";
	case VCU_STATE_IDLE:
		return "IDLE";
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
}

/**
 * @brief Kiểm tra xem có lỗi nghiêm trọng (Critical Fault) không
 * 
 * Lỗi nghiêm trọng bao gồm:
 * - System fault: Lỗi hệ thống chung
 * - Charger error: Lỗi bộ sạc
 * - BMS critical error: Lỗi nghiêm trọng từ BMS
 * - Slider critical error: Lỗi nghiêm trọng từ bộ điều khiển slider
 * 
 * Khi có lỗi nghiêm trọng, hệ thống sẽ chuyển sang ERROR state
 * 
 * @return true nếu có lỗi nghiêm trọng, false nếu không
 */
static bool VCU_HasCriticalFault(void)
{
	return (vcu_ctx.inputs.system_fault ||
		vcu_ctx.inputs.charger_error ||
		vcu_ctx.inputs.bms_critical_error ||
		vcu_ctx.inputs.slider_critical_error);
}

/**
 * @brief Áp dụng các output ra GPIO hardware
 * 
 * Hàm này ghi trực tiếp các giá trị output ra các chân GPIO:
 * - PE3 (RELAY_CONTACTOR): Điều khiển contactor nguồn
 *   + GPIO_PIN_SET = Contactor ON (đóng nguồn)
 *   + GPIO_PIN_RESET = Contactor OFF (ngắt nguồn)
 * 
 * - PE2 (RELAY_SELECT_MODE): Chọn nguồn điều khiển
 *   + GPIO_PIN_SET = CAN mode (điều khiển qua CAN)
 *   + GPIO_PIN_RESET = Physical mode (điều khiển qua chân ga vật lý)
 */
static void VCU_ApplyOutputs(void)
{
	/* Điều khiển relay PE3 để đóng/mở contactor nguồn */
	HAL_GPIO_WritePin(RELAY_CONTACTOR_GPIO_Port,
			  RELAY_CONTACTOR_Pin,
			  vcu_ctx.outputs.contactor_on ? GPIO_PIN_SET : GPIO_PIN_RESET);

	/* Điều khiển relay PE2 chọn nguồn điều khiển (0: chân ga, 1: CAN) */
	HAL_GPIO_WritePin(RELAY_SELECT_MODE_GPIO_Port,
			  RELAY_SELECT_MODE_Pin,
			  vcu_ctx.outputs.select_can_mode ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Cập nhật output chọn mode (CAN/Physical) từ input
 * 
 * Hàm này map trực tiếp yêu cầu chọn mode từ input sang output.
 * Khi input.select_can_mode = true → output.select_can_mode = true → Relay PE2 = SET (CAN mode)
 * Khi input.select_can_mode = false → output.select_can_mode = false → Relay PE2 = RESET (Physical mode)
 */
static void VCU_UpdateSelectModeOutput(void)
{
	if (vcu_ctx.current_state != VCU_STATE_ERROR && vcu_ctx.current_state != VCU_STATE_CHARGE) {
		vcu_ctx.outputs.contactor_on = vcu_ctx.inputs.contactor_request;
	}
	/* Map thẳng yêu cầu chọn mode từ input sang output relay */
	if (vcu_ctx.current_state == VCU_STATE_PHYSICAL || vcu_ctx.current_state == VCU_STATE_CAN) {
		vcu_ctx.outputs.select_can_mode = vcu_ctx.inputs.select_can_mode_request;
	}
}

/**
 * @brief Hàm được gọi khi vào một trạng thái mới
 * 
 * Hàm này được gọi ngay sau khi chuyển sang trạng thái mới.
 * Dùng để thực hiện các hành động khởi tạo khi vào trạng thái:
 * - CHARGE: Khởi tạo giao tiếp với bộ sạc
 * - Các trạng thái khác: Có thể thêm logic khởi tạo sau
 * 
 * @param state Trạng thái mới vừa chuyển vào
 */
static void VCU_OnEnterState(vcu_state_t state)
{
	if (state == VCU_STATE_CHARGE) {
		VCU_ChargerModeEnter();
	}
}

/**
 * @brief Hàm được gọi khi rời khỏi một trạng thái
 * 
 * Hàm này được gọi ngay trước khi chuyển sang trạng thái mới.
 * Dùng để dọn dẹp và kết thúc các hành động của trạng thái cũ:
 * - CHARGE: Ngắt giao tiếp với bộ sạc
 * - Các trạng thái khác: Có thể thêm logic dọn dẹp sau
 * 
 * @param state Trạng thái sắp rời khỏi
 */
static void VCU_OnExitState(vcu_state_t state)
{
	if (state == VCU_STATE_CHARGE) {
		VCU_ChargerModeExit();
	}
}

/**
 * @brief Thực hiện chuyển đổi trạng thái
 * 
 * Hàm này thực hiện quy trình chuyển đổi trạng thái theo thứ tự:
 * 1. Gọi OnExitState() để dọn dẹp trạng thái cũ
 * 2. Cập nhật biến trạng thái toàn cục và trong context
 * 3. Ghi nhận thời điểm chuyển đổi (để debug hoặc timeout)
 * 4. Gọi OnEnterState() để khởi tạo trạng thái mới
 * 
 * @param next_state Trạng thái mới cần chuyển đến
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
 * - Kiểm tra lỗi nghiêm trọng (BMS, Slider, Charger, System)
 * - Kiểm tra trạng thái BMS và Slider
 * - Kiểm tra có sạc không
 * 
 * Quyết định chuyển trạng thái:
 * 1. Có lỗi nghiêm trọng → ERROR
 * 2. BMS OK + Slider OK + Contactor request + Không sạc → PHYSICAL (chạy ngay)
 * 3. Có sạc → CHARGE
 * 4. Mặc định → WAITING (chờ điều kiện)
 * 
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandleInit(void)
{

	/* Kiểm tra Critical Error - nếu có lỗi nghiêm trọng, chuyển ngay sang ERROR */
	if (VCU_HasCriticalFault()) {
		return VCU_STATE_ERROR;
	}

	/* Nếu BMS OK, Contactor ON + Slider OK, No Charger → chuyển thẳng sang Physical MODE
	 * Đây là đường đi nhanh để bắt đầu chạy ngay khi mọi thứ đã sẵn sàng
	 */
	if (vcu_ctx.inputs.bms_ok &&
	    !vcu_ctx.inputs.charger_plugged &&
		HAL_GetTick() - vcu_ctx.last_transition_tick > 1000) {
		vcu_ctx.inputs.contactor_request = true;
		if (vcu_ctx.inputs.slider_ok) {
			return VCU_STATE_PHYSICAL;
		}
	}

	/* Nếu có charger → chuyển sang CHARGER MODE để sạc pin */
	if (vcu_ctx.inputs.charger_plugged) {
		return VCU_STATE_CHARGE;
	}

	/* Mặc định chuyển sang Waiting F/R → N, Waiting RPM <100
	 * Chờ các điều kiện an toàn trước khi vào CAN mode
	 */
	return VCU_STATE_INIT;
}

/**
 * @brief Xử lý logic của trạng thái WAITING
 * 
 * Trạng thái chờ các điều kiện an toàn trước khi vào CAN mode:
 * - F/R (Forward/Reverse) phải về vị trí Neutral
 * - RPM (tốc độ động cơ) phải < 100
 * 
 * Đây là trạng thái an toàn để đảm bảo xe đã dừng hoàn toàn
 * trước khi chuyển sang điều khiển qua CAN.
 * 
 * Quyết định chuyển trạng thái:
 * 1. Có lỗi nghiêm trọng → ERROR
 * 2. Có sạc → CHARGE
 * 3. F/R = Neutral VÀ RPM < 100 → CAN (điều kiện an toàn đã thỏa mãn)
 * 4. Mặc định → tiếp tục WAITING
 * 
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandleWaiting(void)
{
	/* Khi F/R về Neutral và RPM < 100 → chuyển sang CAN MODE
	 * Điều kiện này đảm bảo xe đã dừng hoàn toàn và ở trạng thái an toàn
	 */
	if (can_slider.slider_1.vehicle_mode.forward == 0 && can_slider.slider_1.vehicle_mode.reverse == 0 && can_slider.slider_1.motor_rpm < 100
		&& can_slider_vcu.vehicle_mode.forward == 0 && can_slider_vcu.vehicle_mode.reverse == 0 
		&& (can_slider_vcu.speed_high << 8 | can_slider_vcu.speed_low) < 100) {
		return VCU_STATE_CAN;
	}

	return VCU_STATE_WAITING;
}

/**
 * @brief Xử lý logic của trạng thái CAN MODE
 * 
 * Trạng thái điều khiển xe qua giao thức CAN:
 * - Đọc tham số từ CAN (Parameter, ALM/ERROR, I/O Status)
 * - Điều khiển phanh, hướng, tốc độ động cơ qua CAN
 * 
 * Quyết định chuyển trạng thái:
 * 1. Có lỗi nghiêm trọng → ERROR
 * 2. Có sạc → CHARGE (ưu tiên sạc)
 * 3. Yêu cầu chuyển sang Physical mode → PHYSICAL
 * 4. Yêu cầu lại CAN mode nhưng điều kiện không an toàn (F/R chưa Neutral hoặc RPM >= 100)
 *    → WAITING (để đảm bảo an toàn trước khi vào lại CAN mode)
 * 5. Mặc định → tiếp tục CAN
 * 
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandleCan(void)
{
	/* CAN MODE: Parameter, ALM/ERROR; I/O Status; Control Brake, Direction; Control Motor Speed */
	if (VCU_HasCriticalFault()) {
		return VCU_STATE_ERROR;
	}

	if (vcu_ctx.inputs.charger_plugged) {
		return VCU_STATE_CHARGE;
	}

	/* Select Mode: Physical → chuyển sang Physical MODE
	 * Người dùng yêu cầu chuyển từ CAN sang điều khiển vật lý (chân ga)
	 */
	if (!vcu_ctx.outputs.select_can_mode) {
		/* Thêm đk tín hiệu cần số và chân ga về mo trước khi chuyển 
		-> yêu cầu lấy được tín hiệu từ cần số và chân ga vào VCU */
		return VCU_STATE_PHYSICAL;
	}

	if (!vcu_ctx.outputs.contactor_on) {
		return VCU_STATE_IDLE;
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
 * 3. Yêu cầu tắt contactor → IDLE (tắt nguồn)
 * 4. Yêu cầu chuyển sang CAN mode → WAITING (chờ điều kiện an toàn)
 * 5. Mặc định → tiếp tục PHYSICAL
 * 
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandlePhysical(void)
{
	if (VCU_HasCriticalFault()) {
		return VCU_STATE_ERROR;
	}

	if (vcu_ctx.inputs.charger_plugged) {
		return VCU_STATE_CHARGE;
	}

	/* Contactor OFF → chuyển sang IDLE
	 * Người dùng yêu cầu tắt nguồn (tắt contactor)
	 */
	if (!vcu_ctx.outputs.contactor_on) {
		return VCU_STATE_IDLE;
	}

	/* Select Mode: CAN → chuyển sang Waiting để chuyển sang CAN MODE
	 * Phải qua WAITING để đảm bảo điều kiện an toàn (F/R Neutral, RPM < 100)
	 */
	if (vcu_ctx.outputs.select_can_mode) {
		return VCU_STATE_WAITING;
	}

	return VCU_STATE_PHYSICAL;
}

/**
 * @brief Xử lý logic của trạng thái CHARGE (CHARGER MODE)
 * 
 * Trạng thái sạc pin:
 * - Contactor luôn OFF (ngắt nguồn động cơ để sạc)
 * - Xử lý giao tiếp với bộ sạc
 * - Giám sát quá trình sạc
 * 
 * Quyết định chuyển trạng thái:
 * 1. Có lỗi nghiêm trọng hoặc lỗi sạc → ERROR
 * 2. Sạc đã dừng/đầy → IDLE (chờ các hành động tiếp theo)
 * 3. Mặc định → tiếp tục CHARGE
 * 
 * Lưu ý: Khi ở trạng thái này, không thể chạy xe vì contactor đã tắt
 * 
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandleCharge(void)
{
	/* CHARGER MODE: Contactor OFF - ngắt nguồn động cơ để sạc */
	vcu_ctx.inputs.contactor_request = false;

	VCU_ChargerModeProcess(); /* Xử lý giao tiếp và giám sát bộ sạc */

	if (VCU_HasCriticalFault() || vcu_ctx.inputs.charger_error) {
		return VCU_STATE_ERROR;
	}

	/* Charger Stopped/Full → chuyển sang IDLE
	 * Pin đã đầy hoặc sạc đã dừng, chuyển sang trạng thái chờ
	 */
	if (vcu_ctx.inputs.charger_stopped_full) {
		return VCU_STATE_IDLE;
	}

	return VCU_STATE_CHARGE;
}

/**
 * @brief Xử lý logic của trạng thái IDLE
 * 
 * Trạng thái chờ/standby:
 * - Contactor OFF (ngắt nguồn)
 * - Có thể chọn mode Physical
 * - Giám sát BMS, I/O Status, ALM/ERROR
 * 
 * Đây là trạng thái an toàn khi xe không hoạt động nhưng hệ thống vẫn chạy.
 * 
 * Quyết định chuyển trạng thái:
 * 1. Có lỗi nghiêm trọng → ERROR
 * 2. Có sạc → CHARGE (bắt đầu sạc)
 * 3. Yêu cầu bật contactor → PHYSICAL (bắt đầu chạy)
 * 4. Mặc định → tiếp tục IDLE (chờ lệnh)
 * 
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandleIdle(void)
{
	vcu_ctx.inputs.select_can_mode_request = false;

	if (VCU_HasCriticalFault()) {
		return VCU_STATE_ERROR;
	}

	/* Charger Plugged → chuyển sang CHARGER MODE
	 * Phát hiện có sạc, chuyển sang chế độ sạc
	 */
	if (vcu_ctx.inputs.charger_plugged) {
		return VCU_STATE_CHARGE;
	}

	/* Contactor ON → chuyển sang Physical MODE
	 * Người dùng yêu cầu bật nguồn để chạy xe
	 */
	if (vcu_ctx.outputs.contactor_on) {
		return VCU_STATE_PHYSICAL;
	}

	return VCU_STATE_IDLE;
}

/**
 * @brief Xử lý logic của trạng thái ERROR
 * 
 * Trạng thái lỗi nghiêm trọng:
 * - Contactor luôn OFF (ngắt nguồn để an toàn)
 * - Hiển thị trạng thái lỗi và mô tả lỗi
 * - VCU thực hiện các hành động an toàn
 * 
 * Quyết định chuyển trạng thái:
 * 1. Không còn lỗi nghiêm trọng → IDLE (phục hồi, chờ lệnh tiếp theo)
 * 2. Mặc định → tiếp tục ERROR (vẫn còn lỗi)
 * 
 * Lưu ý: Khi ở trạng thái này, hệ thống không thể hoạt động bình thường.
 * Phải xử lý lỗi và đợi lỗi được giải quyết mới có thể tiếp tục.
 * 
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandleError(void)
{
	/* ERROR STATE: ERROR Status; ERROR Description; VCU Action */
	vcu_ctx.inputs.contactor_request = false; /* Luôn tắt contactor khi lỗi nghiêm trọng để an toàn */

	/* No Critical Error → chuyển sang IDLE
	 * Khi tất cả lỗi nghiêm trọng đã được giải quyết, hệ thống phục hồi
	 */
	if (!VCU_HasCriticalFault()) {
		return VCU_STATE_IDLE;
	}

	return VCU_STATE_ERROR;
}

/**
 * @brief Hàm được gọi khi vào trạng thái CHARGE
 * 
 * Thực hiện các bước khởi tạo khi bắt đầu sạc:
 * - Thiết lập giao tiếp với bộ sạc qua CAN
 * - Gửi yêu cầu sạc với tham số (điện áp, dòng điện)
 * - Chờ xác nhận từ bộ sạc
 * 
 * TODO: Thực hiện khi đã có bản đồ CAN message cho bộ sạc
 */
static void VCU_ChargerModeEnter(void)
{
	/* Thêm sau */
}

/**
 * @brief Hàm xử lý định kỳ khi ở trạng thái CHARGE
 * 
 * Được gọi mỗi lần VCU_StateTask() chạy khi đang ở CHARGE state.
 * Thực hiện:
 * - Giám sát quá trình sạc (điện áp, dòng điện, nhiệt độ)
 * - Điều chỉnh tham số sạc nếu cần
 * - Kiểm tra điều kiện dừng sạc (đầy, lỗi, timeout)
 * 
 * TODO: Thực hiện khi đã có bản đồ CAN message cho bộ sạc
 */
static void VCU_ChargerModeProcess(void)
{
	/* Thêm sau */
}

/**
 * @brief Hàm được gọi khi rời khỏi trạng thái CHARGE
 * 
 * Thực hiện các bước dọn dẹp khi kết thúc sạc:
 * - Gửi lệnh dừng sạc đến bộ sạc
 * - Đóng giao tiếp CAN với bộ sạc
 * - Reset các biến trạng thái sạc
 * 
 * TODO: Thực hiện khi đã có bản đồ CAN message cho bộ sạc
 */
static void VCU_ChargerModeExit(void)
{
	/* Thêm sau */
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
			   MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_CAN_INFO_2;
		break;
		
	case VCU_STATE_WAITING:
	case VCU_STATE_PHYSICAL:
		/* WAITING & PHYSICAL: Tất cả menu trừ THROTTLE_CONTROL */
		mask = MENU_DETAIL_CAN_INFO_1 | MENU_DETAIL_CAN_INFO_2 | MENU_DETAIL_BMS_INFO | 
		       MENU_DETAIL_ALM_BMS | MENU_DETAIL_IO_INFO | MENU_DETAIL_BMS_BATT_ST2 | 
		       MENU_DETAIL_BMS_ALL_TEMP | MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_BMS_INFO_SYS | 
		       MENU_DETAIL_BMS_SW_STA | MENU_DETAIL_BMS_CELLVOL | MENU_DETAIL_BMS_CELLVOL_2 | 
		       MENU_DETAIL_BMS_CHG_INFO;
		break;
		
	case VCU_STATE_CAN:
		/* CAN: Tất cả menu */
		mask = MENU_DETAIL_CAN_INFO_1 | MENU_DETAIL_CAN_INFO_2 | MENU_DETAIL_THROTTLE_CONTROL | 
		       MENU_DETAIL_BMS_INFO | MENU_DETAIL_ALM_BMS | MENU_DETAIL_IO_INFO | MENU_DETAIL_BMS_BATT_ST2 | 
		       MENU_DETAIL_BMS_ALL_TEMP | MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_BMS_INFO_SYS | 
		       MENU_DETAIL_BMS_SW_STA | MENU_DETAIL_BMS_CELLVOL | MENU_DETAIL_BMS_CELLVOL_2 | 
		       MENU_DETAIL_BMS_CHG_INFO;
		break;
		
	case VCU_STATE_CHARGE:
		/* CHARGE: Ưu tiên menu sạc và BMS */
		mask = MENU_DETAIL_BMS_CHG_INFO | MENU_DETAIL_BMS_INFO | MENU_DETAIL_BMS_BATT_ST2 | 
		       MENU_DETAIL_BMS_ALL_TEMP | MENU_DETAIL_BMS_SW_STA | MENU_DETAIL_BMS_CELLVOL | 
		       MENU_DETAIL_BMS_CELLVOL_2 | MENU_DETAIL_ALM_BMS | MENU_DETAIL_BMS_ERR_INFO | 
		       MENU_DETAIL_BMS_INFO_SYS | MENU_DETAIL_IO_INFO;
		break;
		
	case VCU_STATE_IDLE:
		/* IDLE: Chỉ menu BMS và sạc và IO*/
		mask = MENU_DETAIL_BMS_INFO | MENU_DETAIL_ALM_BMS | MENU_DETAIL_IO_INFO 
				| MENU_DETAIL_BMS_BATT_ST2 | MENU_DETAIL_BMS_ALL_TEMP | MENU_DETAIL_BMS_ERR_INFO;
		break;
		
	case VCU_STATE_ERROR:
		/* ERROR: Chỉ menu lỗi và thông tin cơ bản */
		mask = MENU_DETAIL_ALM_BMS | MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_CAN_INFO_2 | MENU_DETAIL_IO_INFO;
		break;
		
	default:
		/* Mặc định: Tất cả menu */
		mask = 0x3FFF; /* Tất cả 14 bit */
		break;
	}
	
	return mask;
}
