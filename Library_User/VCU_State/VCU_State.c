/*
 * VCU_State.c
 *
 *  Created on: Nov 19, 2025
 *      Author: quang
 */

#include "VCU_State.h"
#include "driver_fault_config.h"
#include "modbus_slave_define.h"
#include "modbus_msg_handle.h"
#include "modbus_slave_comp.h"
#include "imd.h"
#include <string.h>

vcu_state_context_t vcu_ctx;

boat_error_count_t boat_error_count = {0};

vcu_state_t vcu_state = VCU_STATE_INIT;

static bool VCU_HasCriticalFault(void);
static bool VCU_HasSystemFault(void);
static bool VCU_SliderCriticalError(void);
static bool VCU_BMSCriticalError(void);
static void VCU_RefreshFaultStatus(void);

static void VCU_ApplyDriverFaultOverride(void);

static void VCU_UpdateInputs(void);
static void VCU_ApplyOutputs(void);
static void VCU_UpdateOutput(void);

static void VCU_OnEnterState(vcu_state_t state);
static void VCU_OnExitState(vcu_state_t state);
static void VCU_TransitionTo(vcu_state_t next_state);

/* Ngưỡng nhận biết "đang đạp ga" theo raw */
#define ACCEL_RAW_ACTIVE_THRESHOLD   (1u)
#define DRIVER_CAN_LOST_TIMEOUT_MS   (2000u)
#define DRIVER_STARTUP_GRACE_MS      (6000u)
#define DRIVER_ERR_IGNORE_MS    (5000u)
#define HMI_LOST_TIMEOUT_MS          (5000u)
#define HMI_STARTUP_GRACE_MS         (8000u)

#ifdef VCU_MULTI_LEVEL_IGNITION

#define IGNITION_CONTACTOR_RELEASE_DELAY_MS  (3000u)

static ignition_level_t VCU_ReadIgnitionLevel(void)
{
	const bool level_1_active =
			(HAL_GPIO_ReadPin(IGN_LEVEL1_GPIO_Port, IGN_LEVEL1_Pin) == GPIO_PIN_SET);
	const bool level_2_active =
			(HAL_GPIO_ReadPin(IGN_LEVEL2_GPIO_Port, IGN_LEVEL2_Pin) == GPIO_PIN_SET);

	if (!level_1_active && !level_2_active) {
		return IGNITION_LEVEL_0;
	}
	if (level_1_active && !level_2_active) {
		return IGNITION_LEVEL_1;
	}
	if (!level_1_active && level_2_active) {
		return IGNITION_LEVEL_2;
	}

	return IGNITION_LEVEL_INVALID;
}
#endif

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
	vcu_ctx.inputs.contactor_request = false;
	vcu_ctx.inputs.disable_motor_request = true;
	vcu_ctx.outputs.disable_motor = true;
	vcu_ctx.last_KSI = false;
	vcu_ctx.inputs.init_completed = false;
	vcu_ctx.last_ksi_tick = HAL_GetTick();
	vcu_ctx.precharge_start_tick = 0u;
	vcu_ctx.accel_safety_interlock_active = false;
	vcu_ctx.accel_press_since_ksi_on = false;
#ifdef VCU_MULTI_LEVEL_IGNITION
	vcu_ctx.inputs.ignition_level = IGNITION_LEVEL_0;
	vcu_ctx.last_ignition_level = IGNITION_LEVEL_0;
	vcu_ctx.last_ignition_level_tick = HAL_GetTick();
#endif

	DriverFaultConfig_Init();
}

static void VCU_ApplyDriverFaultOverride(void)
{
	const driver_fault_thresholds_t *driver_thr = DriverFaultConfig_Get();

	/* KSI OFF hoặc chưa có dữ liệu điện áp driver hợp lệ */
	if ((!vcu_ctx.inputs.KSI) || (can_slider.slider_2.battery_voltage <= 0.0f)) {
		can_slider.effective_raw_err_code = can_slider.raw_err_code;
		can_slider.effective_error_code = can_slider.slider_1.error_code;
		can_slider.effective_warning_code = 0u;
		return;
	}

	// Các bit lỗi thô từ controller đang dùng trong can_slider.slider_1.error_code:
	// CONTROLLER_TEMP_HIGH = 0x0002
	// UNDER_VOLTAGE_BATTERY = 0x0010
	// OVER_VOLTAGE_BATTERY  = 0x0020
	// MOTOR_TEMP_HIGH       = 0x0040
	const uint32_t mask_controller_temp_high = 0x0002u;
	const uint32_t mask_under_voltage_batt = 0x0010u;
	const uint32_t mask_over_voltage_batt = 0x0020u;
	const uint32_t mask_motor_temp_high = 0x0040u;

	const float under_v = (float)driver_thr->under_voltage_V;
	const float over_v = (float)driver_thr->over_voltage_V;
	const float under_warn_v = (float)driver_thr->under_voltage_warn_V;
	const float over_warn_v = (float)driver_thr->over_voltage_warn_V;

	const bool controller_temp_high = (can_slider.slider_1.controller_temp >= driver_thr->controller_temp_high_C);
	const bool motor_temp_high = (can_slider.slider_1.motor_temp >= driver_thr->motor_temp_high_C);
	const bool under_voltage_batt = (can_slider.slider_2.battery_voltage <= under_v);
	const bool over_voltage_batt = (can_slider.slider_2.battery_voltage >= over_v);
	const bool controller_temp_warn =
			(can_slider.slider_1.controller_temp >= driver_thr->controller_temp_warn_C) &&
			!controller_temp_high;
	const bool motor_temp_warn =
			(can_slider.slider_1.motor_temp >= driver_thr->motor_temp_warn_C) &&
			!motor_temp_high;
	const bool under_voltage_warn =
			(can_slider.slider_2.battery_voltage <= under_warn_v) &&
			!under_voltage_batt;
	const bool over_voltage_warn =
			(can_slider.slider_2.battery_voltage >= over_warn_v) &&
			!over_voltage_batt;

	// OR: nếu 1 trong 2 bên (controller hoặc mạch trung tâm) có lỗi thì báo lỗi.
	const uint32_t controller_mask = can_slider.slider_1.error_code;

	uint32_t center_mask = 0;
	if (controller_temp_high) center_mask |= mask_controller_temp_high;
	if (under_voltage_batt) center_mask |= mask_under_voltage_batt;
	if (over_voltage_batt) center_mask |= mask_over_voltage_batt;
	if (motor_temp_high) center_mask |= mask_motor_temp_high;

	uint32_t warning_mask = 0;
	if (controller_temp_warn) warning_mask |= mask_controller_temp_high;
	if (under_voltage_warn) warning_mask |= mask_under_voltage_batt;
	if (over_voltage_warn) warning_mask |= mask_over_voltage_batt;
	if (motor_temp_warn) warning_mask |= mask_motor_temp_high;

	const uint32_t effective_mask = controller_mask | center_mask;

	// raw_err_code: ưu tiên mã lỗi lớn hơn.
	const uint8_t controller_raw = (uint8_t)can_slider.raw_err_code;
	uint8_t center_raw = 0;
	if (controller_temp_high && center_raw < CONTROLLER_TEMP_HIGH) center_raw = CONTROLLER_TEMP_HIGH;
	if (under_voltage_batt && center_raw < UNDER_VOLTAGE_BATTERY) center_raw = UNDER_VOLTAGE_BATTERY;
	if (over_voltage_batt && center_raw < OVER_VOLTAGE_BATTERY) center_raw = OVER_VOLTAGE_BATTERY;
	if (motor_temp_high && center_raw < MOTOR_TEMP_HIGH) center_raw = MOTOR_TEMP_HIGH;

	const uint8_t effective_raw = (controller_raw > center_raw) ? controller_raw : center_raw;

	can_slider.effective_raw_err_code = effective_raw;
	can_slider.effective_error_code = effective_mask;
	can_slider.effective_warning_code = warning_mask;
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
	//static uint32_t send_time = 0;
	vcu_state_t next_state;

	/* Cập nhật các input */
	VCU_UpdateInputs();
	VCU_RefreshFaultStatus();
	next_state = vcu_state;

	/*
	 * KSI OFF:
	 * - Disable motor
	 * - Sau 3s mới tắt contactor và trở về INIT
	 */
	const bool ksi_is_off = (!vcu_ctx.inputs.KSI);
	if (ksi_is_off) {
		const bool ksi_off_delay_done = ((HAL_GetTick() - vcu_ctx.last_ksi_tick) >= 3000U);
		if (ksi_off_delay_done) {
			vcu_ctx.inputs.contactor_request = false;
		}
	}
#ifdef VCU_MULTI_LEVEL_IGNITION
	else if (vcu_ctx.inputs.ignition_level != IGNITION_LEVEL_2) {
		const bool level_release_delay_done =
				((HAL_GetTick() - vcu_ctx.last_ignition_level_tick) >= IGNITION_CONTACTOR_RELEASE_DELAY_MS);
		if (level_release_delay_done) {
			vcu_ctx.inputs.contactor_request = false;
		}
	}
#endif

	//  if (HAL_GetTick() - send_time > 500) {
	// 	send_time = HAL_GetTick();
	// 	Can_Vcu_Send_Slider(&hcan1);
	//  }

	/* Xử lý logic của trạng thái hiện tại để quyết định trạng thái tiếp theo */
	if (!ksi_is_off) {
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
	}

	/* Nếu trạng thái thay đổi, thực hiện chuyển đổi */
	if (next_state != vcu_state) {
		VCU_TransitionTo(next_state);
	}

	/* Cập nhật và áp dụng output */
	VCU_UpdateOutput();
	VCU_ApplyOutputs();
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
	if (!VCU_IsManualMotorControlAllowed()) {
		return;
	}

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

bool VCU_HasCriticalFaultActive(void)
{
	return vcu_ctx.critical_fault_active;
}

bool VCU_HasSystemFaultActive(void)
{
	return vcu_ctx.system_fault_active;
}

bool VCU_IsManualMotorControlAllowed(void)
{
	if (vcu_ctx.critical_fault_active) {
		return false;
	}

	return (vcu_state == VCU_STATE_PHYSICAL) ||
		   ((vcu_state == VCU_STATE_ERROR) && vcu_ctx.system_fault_active);
}

static void VCU_UpdateInputs(void) {
	/* Đọc KSI */
#ifdef VCU_MULTI_LEVEL_IGNITION
	vcu_ctx.inputs.ignition_level = VCU_ReadIgnitionLevel();
	vcu_ctx.inputs.KSI = (vcu_ctx.inputs.ignition_level == IGNITION_LEVEL_1) ||
						 (vcu_ctx.inputs.ignition_level == IGNITION_LEVEL_2);
#else
	vcu_ctx.inputs.KSI = (HAL_GPIO_ReadPin(KSI_GPIO_Port, KSI_Pin) == GPIO_PIN_SET) ? false : true;
#endif

#ifdef VCU_MULTI_LEVEL_IGNITION
	if (vcu_ctx.inputs.ignition_level != vcu_ctx.last_ignition_level) {
		vcu_ctx.last_ignition_level = vcu_ctx.inputs.ignition_level;
		vcu_ctx.last_ignition_level_tick = HAL_GetTick();
	}
#endif

	/* Bắt cạnh KSI (ON/OFF) */
	if (vcu_ctx.inputs.KSI != vcu_ctx.last_KSI) {
		vcu_ctx.last_ksi_tick = HAL_GetTick();
		vcu_ctx.last_KSI = vcu_ctx.inputs.KSI;

		if (vcu_ctx.inputs.KSI) {
			vcu_ctx.accel_press_since_ksi_on = false;
			if (can_slider.slider_2.thr_Val >= ACCEL_RAW_ACTIVE_THRESHOLD) {
				vcu_ctx.inputs.disable_motor_request = true;
				vcu_ctx.accel_safety_interlock_active = true;
			}
		}

		if (!vcu_ctx.inputs.KSI) {
			vcu_ctx.accel_safety_interlock_active = false;
			memset(&can_slider, 0, sizeof(can_slider));
			vcu_ctx.inputs.disable_motor_request = true;
			vcu_ctx.inputs.init_completed = false;
			vcu_ctx.inputs.slider_critical_error = false;
			vcu_ctx.inputs.bms_critical_error = false;
			vcu_ctx.inputs.driver_can_lost = false;
			vcu_ctx.inputs.hmi_lost = false;
			vcu_ctx.imd.pos_low_riso = false;
			vcu_ctx.imd.neg_low_riso = false;
			vcu_ctx.imd.measure_fault = false;
			vcu_ctx.imd.pos_riso_ohm = 0u;
			vcu_ctx.imd.neg_riso_ohm = 0u;
			vcu_ctx.inputs.system_fault = false;
			vcu_ctx.critical_fault_active = false;
			vcu_ctx.system_fault_active = false;
			vcu_ctx.slider_critical_error = false;
			vcu_ctx.bms_critical_error = false;
			boat_error_count.count_error_slider = 0;
			boat_error_count.count_error_bms = 0;
			vcu_ctx.precharge_start_tick = 0u;
			VCU_TransitionTo(VCU_STATE_INIT);
			for (int i=0; i<5; i++) {
				inputReg[110+i] = 0;
			}
		}
	}

	if (!vcu_ctx.inputs.KSI) {
		return;
	}

	BMS_Jikong_Service();
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

	// Override 4 lỗi nhiệt/điện áp theo ngưỡng trung tâm (không lấy các bit error 4 loại này từ controller).
	VCU_ApplyDriverFaultOverride();

	/* Nếu lần đầu nhấn ga kể từ khi KSI ON mà xuất hiện ACCELERATOR_FAULT
	 * thì coi là interlock an toàn (không báo lỗi). */
	if (!vcu_ctx.accel_safety_interlock_active &&
		vcu_ctx.inputs.KSI &&
		!vcu_ctx.accel_press_since_ksi_on &&
		(can_slider.effective_raw_err_code == ACCELERATOR_FAULT) &&
		(can_slider.slider_2.thr_Val >= ACCEL_RAW_ACTIVE_THRESHOLD)) {
		vcu_ctx.inputs.disable_motor_request = true;
		vcu_ctx.accel_press_since_ksi_on = true;
		vcu_ctx.accel_safety_interlock_active = true;
	}
	/* Interlock: yêu cầu nhả ga */
	if (vcu_ctx.accel_safety_interlock_active) {
		if (can_slider.slider_2.thr_Val < ACCEL_RAW_ACTIVE_THRESHOLD)
			if (vcu_ctx.last_accel_release_tick == 0)
				vcu_ctx.last_accel_release_tick = HAL_GetTick();
			else if ((HAL_GetTick() - vcu_ctx.last_accel_release_tick) >= 200) {
				vcu_ctx.accel_safety_interlock_active = false;
				if (vcu_state == VCU_STATE_PHYSICAL)
					vcu_ctx.inputs.disable_motor_request = false;
				vcu_ctx.last_accel_release_tick = 0;
				vcu_ctx.slider_critical_error = false;
				boat_error_count.count_error_slider = 0;
			}
	}

	/* Nếu lỗi chân ga do interlock thì không coi là critical fault */
	const bool accel_fault_safety_mode =
			(vcu_ctx.accel_safety_interlock_active && (can_slider.effective_raw_err_code == ACCELERATOR_FAULT));
	const bool driver_fault_reporting_ready =
			(vcu_ctx.inputs.KSI && vcu_ctx.outputs.contactor_on);
	const bool ignore_driver_comm_error_startup =
			((HAL_GetTick() - vcu_ctx.last_ksi_tick) < DRIVER_ERR_IGNORE_MS);
	if (driver_fault_reporting_ready &&
		(can_slider.effective_error_code != 0u) &&
		!accel_fault_safety_mode &&
		!ignore_driver_comm_error_startup) {
		vcu_ctx.inputs.slider_critical_error = true;
		vcu_ctx.inputs.slider_ok = false;
	} else {
		vcu_ctx.inputs.slider_critical_error = false;
		if (vcu_ctx.accel_safety_interlock_active) {
			vcu_ctx.inputs.slider_ok = false;
		} else if (can_slider.slider_2.battery_voltage > 0 && can_slider.slider_1.vehicle_mode.forward == 0
				&& can_slider.slider_1.vehicle_mode.reverse == 0 && can_slider.slider_1.motor_rpm < 100) {
			vcu_ctx.inputs.slider_ok = true;
		} else {
			vcu_ctx.inputs.slider_ok = false;
		}
	}

	if ((vcu_ctx.precharge_start_tick == 0u) && (can_slider.last_rx_tick != 0u)) {
		vcu_ctx.precharge_start_tick = can_slider.last_rx_tick;
	}

	vcu_ctx.inputs.driver_can_lost = false;
	const bool driver_grace_period_done =
			((HAL_GetTick() - vcu_ctx.last_ksi_tick) >= DRIVER_STARTUP_GRACE_MS);
	if (vcu_ctx.inputs.KSI &&
		driver_grace_period_done &&
		!Can_Slider_IsAlive(DRIVER_CAN_LOST_TIMEOUT_MS)) {
		vcu_ctx.inputs.driver_can_lost = true;
	}

	vcu_ctx.inputs.hmi_lost = false;
	const bool hmi_grace_period_done =
			((HAL_GetTick() - vcu_ctx.last_ksi_tick) >= HMI_STARTUP_GRACE_MS);
	if (vcu_ctx.inputs.KSI &&
		hmi_grace_period_done &&
		!ModbusSlaveComp_IsOnline(HMI_LOST_TIMEOUT_MS)) {
		vcu_ctx.inputs.hmi_lost = true;
	}

	IMD_GetStatus(&vcu_ctx.imd);

	vcu_ctx.inputs.system_fault = vcu_ctx.inputs.driver_can_lost || vcu_ctx.inputs.hmi_lost;
	if (vcu_ctx.imd.pos_low_riso || vcu_ctx.imd.neg_low_riso) {
		vcu_ctx.inputs.system_fault = true;
	}
	
	/* Đọc thông tin sạc từ BMS (BMS quản lý bộ sạc) */
	/* chgPlug = 1 : đã cắm sạc */
	vcu_ctx.inputs.charger_plugged = (bms.swSta.chgPlug == 1);
	
	/* Sạc dừng khi: chg_dev_sw = 1 (tắt) hoặc SOC >= 100% */
	vcu_ctx.inputs.charger_stopped_full = (bms.chgInfo.chg_dev_sw == 1) || 
	                                      (bms.batt1.soc_percent >= 100);

}

static void VCU_RefreshFaultStatus(void)
{
	vcu_ctx.critical_fault_active = VCU_HasCriticalFault();
	vcu_ctx.system_fault_active = VCU_HasSystemFault();
}

/**
 * @brief Kiểm tra xem có lỗi nghiêm trọng (Critical Fault) không
 */
static bool VCU_HasCriticalFault(void)
{
	bool slider_critical_error = VCU_SliderCriticalError();
	bool bms_critical_error = VCU_BMSCriticalError();
	vcu_ctx.slider_critical_error = slider_critical_error;
	vcu_ctx.bms_critical_error = bms_critical_error;
	const bool imd_critical_error = IMD_IsBothFault();
	return (slider_critical_error || bms_critical_error || imd_critical_error);
}

static bool VCU_HasSystemFault(void)
{
	return vcu_ctx.inputs.system_fault;
}

static bool VCU_SliderCriticalError(void)
{
	const uint8_t CRITICAL_FAULT_COUNT_THRESHOLD = 10;

	if (!vcu_ctx.inputs.KSI || !vcu_ctx.outputs.contactor_on) {
		boat_error_count.count_error_slider = 0;
		return false;
	}

	if (vcu_ctx.inputs.slider_critical_error) {
		if (boat_error_count.count_error_slider < CRITICAL_FAULT_COUNT_THRESHOLD) {
			boat_error_count.count_error_slider++;
		}
	} else {
		if (boat_error_count.count_error_slider > 0) {
			boat_error_count.count_error_slider--;
		}
	}

	if (boat_error_count.count_error_slider >= CRITICAL_FAULT_COUNT_THRESHOLD) {
		return true;
	} else if (boat_error_count.count_error_slider == 0) {
		return false;
	}
	return vcu_ctx.inputs.slider_critical_error;
}

static bool VCU_BMSCriticalError(void)
{
	const uint8_t CRITICAL_FAULT_COUNT_THRESHOLD = 10;

	if (vcu_ctx.inputs.bms_critical_error) {
		if (boat_error_count.count_error_bms < CRITICAL_FAULT_COUNT_THRESHOLD) {
			boat_error_count.count_error_bms++;
		}
	} else {
		if (boat_error_count.count_error_bms > 0) {
			boat_error_count.count_error_bms--;
		}
	}

	if (boat_error_count.count_error_bms >= CRITICAL_FAULT_COUNT_THRESHOLD) {
		return true;
	} else if (boat_error_count.count_error_bms == 0) {
		return false;
	}
	return vcu_ctx.inputs.bms_critical_error;
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
	//VCU_OnExitState(vcu_state);
	vcu_state = next_state;
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
	if (vcu_ctx.critical_fault_active || vcu_ctx.system_fault_active) {
		vcu_ctx.accel_init = false;
		return VCU_STATE_ERROR;
	}
	/* Nếu có charger → chuyển sang CHARGER MODE để sạc pin */
	if (vcu_ctx.inputs.charger_plugged) {
		vcu_ctx.accel_init = false;
		return VCU_STATE_CHARGE;
	}
	/* Nếu BMS OK, Contactor ON + Slider OK, No Charger → chuyển thẳng sang Physical MODE */
//	if (vcu_ctx.inputs.bms_ok &&
#ifdef VCU_MULTI_LEVEL_IGNITION
	if (vcu_ctx.inputs.ignition_level == IGNITION_LEVEL_INVALID) {
		vcu_ctx.inputs.contactor_request = false;
		vcu_ctx.inputs.disable_motor_request = true;
		vcu_ctx.inputs.init_completed = false;
		return VCU_STATE_INIT;
	}

	if (vcu_ctx.inputs.ignition_level == IGNITION_LEVEL_1) {
		vcu_ctx.inputs.contactor_request = false;
		vcu_ctx.inputs.disable_motor_request = true;
		vcu_ctx.inputs.init_completed = false;
		return VCU_STATE_INIT;
	}

	if (vcu_ctx.inputs.ignition_level == IGNITION_LEVEL_2) {
		if ((vcu_ctx.precharge_start_tick != 0u) &&
			((HAL_GetTick() - vcu_ctx.precharge_start_tick) >= 1000U)) {
			if (vcu_ctx.inputs.slider_ok) {
				vcu_ctx.inputs.contactor_request = true;
				vcu_ctx.inputs.init_completed = true;
				vcu_ctx.inputs.disable_motor_request = false;
				return VCU_STATE_PHYSICAL;
			}
		}
	}
#else
	if (vcu_ctx.inputs.KSI) {
		if ((vcu_ctx.precharge_start_tick != 0u) &&
			((HAL_GetTick() - vcu_ctx.precharge_start_tick) >= 1000U)) {
			/* Driver đã trả CAN và precharge đủ 3s -> bật contactor */
			vcu_ctx.inputs.contactor_request = true;
			if (vcu_ctx.inputs.slider_ok) {
				vcu_ctx.inputs.init_completed = true;
				vcu_ctx.inputs.disable_motor_request = false;
				return VCU_STATE_PHYSICAL;
			}
		}
	}
#endif

	if (can_slider.slider_2.thr_Val >= ACCEL_RAW_ACTIVE_THRESHOLD
		&& can_slider.motor_direc != 0) 
		vcu_ctx.accel_init = true;
	else
		vcu_ctx.accel_init = false;

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
#ifdef VCU_MULTI_LEVEL_IGNITION
	if (vcu_ctx.inputs.ignition_level != IGNITION_LEVEL_2) {
		return VCU_STATE_INIT;
	}
#endif

	if (vcu_ctx.inputs.charger_plugged) {
		return VCU_STATE_CHARGE;
	}

	if (can_slider.slider_2.control_mode == 0) {
		return VCU_STATE_PHYSICAL;
	}

	/* CAN MODE: Parameter, ALM/ERROR; I/O Status; Control Brake, Direction; Control Motor Speed */
	if (vcu_ctx.critical_fault_active || vcu_ctx.system_fault_active) {
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
#ifdef VCU_MULTI_LEVEL_IGNITION
	if (vcu_ctx.inputs.ignition_level != IGNITION_LEVEL_2) {
		return VCU_STATE_INIT;
	}
#endif

	if (vcu_ctx.inputs.charger_plugged) {
		return VCU_STATE_CHARGE;
	}

	if (can_slider.slider_2.control_mode == 1) {
		return VCU_STATE_CAN;
	}

	if (vcu_ctx.critical_fault_active || vcu_ctx.system_fault_active) {
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
#ifdef VCU_MULTI_LEVEL_IGNITION
	if (vcu_ctx.inputs.ignition_level != IGNITION_LEVEL_2) {
		return VCU_STATE_INIT;
	}
#endif

	if (can_slider.slider_2.thr_Val >= ACCEL_RAW_ACTIVE_THRESHOLD
		&& can_slider.motor_direc != 0) 
		vcu_ctx.accel_charge = true;
	else
		

	if (vcu_ctx.critical_fault_active || vcu_ctx.system_fault_active) {
		vcu_ctx.accel_charge = false;
		return VCU_STATE_ERROR;
	}

	if (!vcu_ctx.inputs.charger_plugged) {
		vcu_ctx.accel_charge = false;
		return VCU_STATE_INIT;
	}

	return VCU_STATE_CHARGE;
}

/**
 * @brief Xử lý logic của trạng thái ERROR
 * 
 * Trạng thái lỗi:
 * - Lỗi nghiêm trọng: Disable Motor
 * - Lỗi hệ thống: vẫn cho phép HMI chủ động bật/tắt motor
 * - Hiển thị trạng thái lỗi và mô tả lỗi
 * - VCU thực hiện các hành động an toàn
 * @return Trạng thái tiếp theo
 */
static vcu_state_t VCU_StateHandleError(void)
{
#ifdef VCU_MULTI_LEVEL_IGNITION
	if (vcu_ctx.inputs.ignition_level != IGNITION_LEVEL_2) {
		return VCU_STATE_INIT;
	}
#endif

	if (can_slider.slider_2.thr_Val >= ACCEL_RAW_ACTIVE_THRESHOLD
		&& can_slider.motor_direc != 0) 
		vcu_ctx.accel_error = true;
	else
		vcu_ctx.accel_error = false;

	if (vcu_ctx.critical_fault_active) {
		vcu_ctx.inputs.disable_motor_request = true;
	}

	if (!(vcu_ctx.critical_fault_active || vcu_ctx.system_fault_active)) {
		vcu_ctx.accel_error = false;
		return VCU_STATE_INIT;
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
	/* Tắt động cơ khi vào chế độ sạc */
	vcu_ctx.inputs.disable_motor_request = true;
}

static void VCU_ErrorModeEnter(void)
{
	if (vcu_ctx.critical_fault_active) {
		vcu_ctx.inputs.disable_motor_request = true;
	}
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
	vcu_ctx.inputs.disable_motor_request = false;
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
			   MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_CAN_INFO_2 | MENU_DETAIL_NETWORK | MENU_DETAIL_MODBUS;
		break;
		
	case VCU_STATE_PHYSICAL:
		/* WAITING & PHYSICAL: Tất cả menu trừ THROTTLE_CONTROL */
		mask = MENU_DETAIL_CAN_INFO_1 | MENU_DETAIL_CAN_INFO_2 | MENU_DETAIL_BMS_INFO | 
		       MENU_DETAIL_ALM_BMS | MENU_DETAIL_IO_INFO | MENU_DETAIL_BMS_BATT_ST2 | 
		       MENU_DETAIL_BMS_ALL_TEMP | MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_BMS_INFO_SYS | 
		       MENU_DETAIL_BMS_SW_STA | MENU_DETAIL_BMS_CELLVOL | MENU_DETAIL_BMS_CELLVOL_2 | 
		       MENU_DETAIL_BMS_CHG_INFO | MENU_DETAIL_NETWORK | MENU_DETAIL_MODBUS;
		break;
		
	case VCU_STATE_CAN:
		/* CAN: Tất cả menu */
		mask = MENU_DETAIL_CAN_INFO_1 | MENU_DETAIL_CAN_INFO_2 | MENU_DETAIL_THROTTLE_CONTROL | 
		       MENU_DETAIL_BMS_INFO | MENU_DETAIL_ALM_BMS | MENU_DETAIL_IO_INFO | MENU_DETAIL_BMS_BATT_ST2 | 
		       MENU_DETAIL_BMS_ALL_TEMP | MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_BMS_INFO_SYS | 
		       MENU_DETAIL_BMS_SW_STA | MENU_DETAIL_BMS_CELLVOL | MENU_DETAIL_BMS_CELLVOL_2 | 
		       MENU_DETAIL_BMS_CHG_INFO | MENU_DETAIL_NETWORK | MENU_DETAIL_MODBUS;
		break;
		
	case VCU_STATE_CHARGE:
		/* CHARGE: Ưu tiên menu sạc và BMS */
		mask = MENU_DETAIL_BMS_CHG_INFO | MENU_DETAIL_BMS_INFO | MENU_DETAIL_BMS_BATT_ST2 | 
		       MENU_DETAIL_BMS_ALL_TEMP | MENU_DETAIL_BMS_SW_STA | MENU_DETAIL_BMS_CELLVOL | 
		       MENU_DETAIL_BMS_CELLVOL_2 | MENU_DETAIL_ALM_BMS | MENU_DETAIL_BMS_ERR_INFO | 
		       MENU_DETAIL_BMS_INFO_SYS | MENU_DETAIL_IO_INFO | MENU_DETAIL_NETWORK | MENU_DETAIL_MODBUS;
		break;
		
	case VCU_STATE_ERROR:
		/* ERROR: Chỉ menu lỗi và thông tin cơ bản */
		mask = MENU_DETAIL_ALM_BMS | MENU_DETAIL_BMS_ERR_INFO | MENU_DETAIL_CAN_INFO_2 | MENU_DETAIL_IO_INFO | MENU_DETAIL_NETWORK | MENU_DETAIL_MODBUS;
		break;
		
	default:
		/* Mặc định: Tất cả menu */
		mask = 0xFFFF; /* Tất cả 16 bit */
		break;
	}
	
	return mask;
}
