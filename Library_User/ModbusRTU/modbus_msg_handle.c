#include <stdint.h>
#include <string.h>
#include "modbus_slave_base.h"
#include "modbus_slave_define.h"
#include "sim.h"
#include "Can_Slider.h"
#include "driver_fault_config.h"
#include "jikong_can.h"
#include "VCU_State.h"
#include "boat_log.h"
#include "imd.h"

static inline uint16_t u16_clamp_u32(uint32_t v)
{
    return (v > 0xFFFFu) ? 0xFFFFu : (uint16_t)v;
}

static inline uint16_t u16_from_float_scale(float v, float scale)
{
    float x = v * scale;
    if (x < 0.0f) x = 0.0f;
    if (x > 65535.0f) x = 65535.0f;
    return (uint16_t)(x + 0.5f);
}

static inline uint16_t u16_from_s16(int16_t v)
{
    return (uint16_t)v;
}

static inline uint16_t u16_from_float_s16_scale(float v, float scale)
{
    float x = v * scale;
    if (x > 32767.0f) x = 32767.0f;
    if (x < -32768.0f) x = -32768.0f;
    return u16_from_s16((int16_t)(x + (x >= 0 ? 0.5f : -0.5f)));
}

static driver_fault_thresholds_t s_driver_thr_pending;
static uint16_t s_driver_thr_status = DRIVER_THR_STATUS_OK;
static uint32_t s_driver_thr_status_tick = 0u;
static uint8_t s_driver_thr_pending_init = 0;

static void Modbus_InitDriverThresholdPending(void)
{
    if (s_driver_thr_pending_init) {
        return;
    }

    s_driver_thr_pending = *DriverFaultConfig_Get();
    s_driver_thr_pending_init = 1;
}

static void Modbus_MapHoldingRegs(void)
{
    Modbus_InitDriverThresholdPending();

    holdingReg[ADR_HOLD_DRIVER_TEMP_HIGH_C] = s_driver_thr_pending.controller_temp_high_C;
    holdingReg[ADR_HOLD_MOTOR_TEMP_HIGH_C] = s_driver_thr_pending.motor_temp_high_C;
    holdingReg[ADR_HOLD_DRIVER_UNDER_VOLT_V] = s_driver_thr_pending.under_voltage_V;
    holdingReg[ADR_HOLD_DRIVER_OVER_VOLT_V] = s_driver_thr_pending.over_voltage_V;
    holdingReg[ADR_HOLD_DRIVER_ALM_TEMP_HIGH_C] = s_driver_thr_pending.controller_temp_warn_C;
    holdingReg[ADR_HOLD_DRIVER_ALM_MOTOR_TEMP_C] = s_driver_thr_pending.motor_temp_warn_C;
    holdingReg[ADR_HOLD_DRIVER_ALM_UNDER_VOLT_V] = s_driver_thr_pending.under_voltage_warn_V;
    holdingReg[ADR_HOLD_DRIVER_ALM_OVER_VOLT_V] = s_driver_thr_pending.over_voltage_warn_V;
    holdingReg[ADR_HOLD_DRIVER_THR_SAVE] = 0;
}

static void Modbus_RefreshDriverThresholdStatus(void)
{
    if (((s_driver_thr_status == DRIVER_THR_STATUS_SAVE_OK) ||
         (s_driver_thr_status == DRIVER_THR_STATUS_SAVE_FAILED)) &&
        ((HAL_GetTick() - s_driver_thr_status_tick) >= 10000u)) {
        s_driver_thr_status = DRIVER_THR_STATUS_OK;
        s_driver_thr_status_tick = 0u;
    }

    inputReg[ADR_INPUT_DRIVER_THR_STATUS] = s_driver_thr_status;
}

static void Modbus_HandleDriverThresholdHoldingWrite(uint16_t start, uint16_t qty)
{
    uint8_t save_requested = 0;
    uint16_t status_code = DRIVER_THR_STATUS_OK;

    Modbus_InitDriverThresholdPending();

    for (uint16_t i = 0; i < qty; i++) {
        const uint16_t adr = (uint16_t)(start + i);

        switch (adr) {
        case ADR_HOLD_DRIVER_TEMP_HIGH_C:
            s_driver_thr_pending.controller_temp_high_C = holdingReg[adr];
            break;
        case ADR_HOLD_MOTOR_TEMP_HIGH_C:
            s_driver_thr_pending.motor_temp_high_C = holdingReg[adr];
            break;
        case ADR_HOLD_DRIVER_UNDER_VOLT_V:
            s_driver_thr_pending.under_voltage_V = holdingReg[adr];
            break;
        case ADR_HOLD_DRIVER_OVER_VOLT_V:
            s_driver_thr_pending.over_voltage_V = holdingReg[adr];
            break;
        case ADR_HOLD_DRIVER_ALM_TEMP_HIGH_C:
            s_driver_thr_pending.controller_temp_warn_C = holdingReg[adr];
            break;
        case ADR_HOLD_DRIVER_ALM_MOTOR_TEMP_C:
            s_driver_thr_pending.motor_temp_warn_C = holdingReg[adr];
            break;
        case ADR_HOLD_DRIVER_ALM_UNDER_VOLT_V:
            s_driver_thr_pending.under_voltage_warn_V = holdingReg[adr];
            break;
        case ADR_HOLD_DRIVER_ALM_OVER_VOLT_V:
            s_driver_thr_pending.over_voltage_warn_V = holdingReg[adr];
            break;
        case ADR_HOLD_DRIVER_THR_SAVE:
            save_requested = (holdingReg[adr] != 0u) ? 1u : 0u;
            break;
        default:
            break;
        }
    }

    if (!DriverFaultConfig_Validate(&s_driver_thr_pending, &status_code)) {
        s_driver_thr_status = status_code;
        s_driver_thr_status_tick = 0u;
        holdingReg[ADR_HOLD_DRIVER_THR_SAVE] = 0;
        return;
    }

    s_driver_thr_status = DRIVER_THR_STATUS_OK;
    s_driver_thr_status_tick = 0u;

    if (save_requested) {
        if (!DriverFaultConfig_Set(&s_driver_thr_pending)) {
            s_driver_thr_status = DRIVER_THR_STATUS_INVALID_RANGE;
            s_driver_thr_status_tick = 0u;
        } else if (DriverFaultConfig_Save()) {
            s_driver_thr_status = DRIVER_THR_STATUS_SAVE_OK;
            s_driver_thr_status_tick = HAL_GetTick();
        } else {
            s_driver_thr_status = DRIVER_THR_STATUS_SAVE_FAILED;
            s_driver_thr_status_tick = HAL_GetTick();
        }
        holdingReg[ADR_HOLD_DRIVER_THR_SAVE] = 0;
    }
}

static void Modbus_MapInputRegs(void)
{
    const uint32_t driver_error_mask = can_slider.effective_error_code;
    const uint32_t driver_warning_mask = can_slider.effective_warning_code;
    const uint32_t mask_controller_temp_high = 0x0002u;
    const uint32_t mask_under_voltage_batt = 0x0010u;
    const uint32_t mask_over_voltage_batt = 0x0020u;
    const uint32_t mask_motor_temp_high = 0x0040u;

    Modbus_RefreshDriverThresholdStatus();

    inputReg[ADR_INPUT_SIGNAL_QUALITY] = (uint16_t)Sim_SignalQuanlityPercent();

    uint16_t bms_status = 0;
    if (bms.almInfo.raw != 0) bms_status = 1;
    if (vcu_ctx.bms_critical_error && bms.bmsErrInfo.raw != 0) bms_status = 2;
    inputReg[ADR_INPUT_BMS_STATUS] = bms_status;

    inputReg[ADR_INPUT_SOLAR_FAULT] = 0;

    inputReg[ADR_INPUT_PACK_VOLTAGE] = u16_from_float_scale(bms.batt1.voltage_V, 10.0f);
    inputReg[ADR_INPUT_PACK_CURRENT] = u16_from_float_s16_scale(bms.batt1.current_A, 10.0f);
    inputReg[ADR_INPUT_PACK_CAPACITY] = (uint16_t)bms.batt1.soc_percent;
    inputReg[ADR_INPUT_CELL_TEMP] = (uint16_t)(int16_t)bms.cellTemp.avg_temp_C;

    int8_t mos_temp = bms.allTemp.temp[4];
    if (mos_temp == (int8_t)0xFF) {
        mos_temp = bms.cellTemp.max_temp_C;
    }
    inputReg[ADR_INPUT_MOS_TEMP] = (uint16_t)(int16_t)mos_temp;

    inputReg[ADR_INPUT_RUNTIME] = u16_clamp_u32(bms.info.runtime_s / 3600u);

    uint16_t ave = 0;
    for (uint8_t n = 1; n <= 30; n++)
    {
        uint16_t mv = 0;
        if (n <= bms.cellArray.cell_count)
        {
            mv = bms.cellArray.cell_mV[n - 1];
            ave += mv;
        }
        inputReg[ADR_INPUT_CELL_VOLT(n)] = mv;
    }

    // cell max/min
    inputReg[ADR_INPUT_CELL_MAX_INDEX] = (uint16_t)bms.cellVolt.max_cell_no;
    inputReg[ADR_INPUT_CELL_MIN_INDEX] = (uint16_t)bms.cellVolt.min_cell_no;
    inputReg[ADR_INPUT_CELL_MAX_VALUE] = bms.cellVolt.max_cell_mV;
    inputReg[ADR_INPUT_CELL_MIN_VALUE] = bms.cellVolt.min_cell_mV;
    // cell trung bình
    inputReg[ADR_INPUT_CELL_AVE_VALUE] = ave / 16;
    
    // Cảnh báo BMS - Từng trường riêng lẻ
    inputReg[ADR_INPUT_BMS_ALM_CELL_OVERVOLT] = (uint16_t)bms.almInfo.cell_overvolt;   // Cảnh báo Cell quá áp
    inputReg[ADR_INPUT_BMS_ALM_CELL_UNDERVOLT] = (uint16_t)bms.almInfo.cell_undervolt; // Cảnh báo Cell thấp áp
    inputReg[ADR_INPUT_BMS_ALM_DELTA_OVER] = (uint16_t)bms.almInfo.delta_over;         // Cảnh báo ΔV quá lớn
    inputReg[ADR_INPUT_BMS_ALM_DCHG_OC] = (uint16_t)bms.almInfo.dchg_oc;               // Cảnh báo quá dòng xả
    inputReg[ADR_INPUT_BMS_ALM_CHG_OC] = (uint16_t)bms.almInfo.chg_oc;                // Cảnh báo quá dòng sạc
    inputReg[ADR_INPUT_BMS_ALM_TEMP_HIGH] = (uint16_t)bms.almInfo.temp_high;          // Cảnh báo nhiệt độ cao
    inputReg[ADR_INPUT_BMS_ALM_TEMP_LOW] = (uint16_t)bms.almInfo.temp_low;            // Cảnh báo nhiệt độ thấp
    inputReg[ADR_INPUT_BMS_ALM_SOC_LOW] = (uint16_t)bms.almInfo.soc_low;              // Cảnh báo SOC thấp
    inputReg[ADR_INPUT_BMS_ALM_COMM_FAULT] = (uint16_t)bms.almInfo.comm_fault;         // Cảnh báo truyền thông thất bại
    
    inputReg[ADR_INPUT_GPS_ACTIVE] = (gpsData.state == 3u) ? 1u : 0u;
    const uint32_t imd_pos_ohm = IMD_GetRisoPosOhm();
    const uint32_t imd_neg_ohm = IMD_GetRisoNegOhm();
    inputReg[ADR_INPUT_IMD_POS_RISO_KOHM] = u16_clamp_u32((imd_pos_ohm + 500u) / 1000u);
    inputReg[ADR_INPUT_IMD_NEG_RISO_KOHM] = u16_clamp_u32((imd_neg_ohm + 500u) / 1000u);

    
    inputReg[ADR_INPUT_DRIVER_ALM_TEMP_HIGH] = ((driver_warning_mask & mask_controller_temp_high) != 0u) ? 1u : 0u;
    inputReg[ADR_INPUT_DRIVER_ALM_MOTOR_TEMP] = ((driver_warning_mask & mask_motor_temp_high) != 0u) ? 1u : 0u;
    inputReg[ADR_INPUT_DRIVER_ALM_UNDER_VOLT] = ((driver_warning_mask & mask_under_voltage_batt) != 0u) ? 1u : 0u;
    inputReg[ADR_INPUT_DRIVER_ALM_OVER_VOLT] = ((driver_warning_mask & mask_over_voltage_batt) != 0u) ? 1u : 0u;
    inputReg[ADR_INPUT_IMD_MEASURE_WARN] = IMD_IsMeasureFault() ? 1u : 0u;

    inputReg[ADR_INPUT_DRIVER_VOLT]    = u16_from_float_scale(can_slider.slider_2.battery_voltage, 10.0f);
    inputReg[ADR_INPUT_DRIVER_CURRENT] = u16_from_float_s16_scale(can_slider.slider_2.dc_current, 10.0f);
    inputReg[ADR_INPUT_BALANCE]        = (uint16_t)can_slider.motor_direc;
    inputReg[ADR_INPUT_SPEED]          = (uint16_t)can_slider.slider_1.motor_rpm;
    inputReg[ADR_INPUT_MOTOR_TEMP]     = (uint16_t)can_slider.slider_1.motor_temp;
    inputReg[ADR_INPUT_DRIVER_TEMP]    = (uint16_t)can_slider.slider_1.controller_temp;
    inputReg[ADR_INPUT_VELOCITY]       = (uint16_t)145;
    if (vcu_ctx.outputs.disable_motor) inputReg[ADR_INPUT_VELOCITY] = 0;
    if (vcu_ctx.accel_safety_interlock_active && (can_slider.effective_raw_err_code == ACCELERATOR_FAULT)) {
        inputReg[ADR_INPUT_DRIVER_FAULT] = 0u;
    }
    if (!vcu_ctx.accel_safety_interlock_active) {
        if (vcu_ctx.inputs.driver_can_lost)
            inputReg[ADR_INPUT_DRIVER_FAULT] = 2u;
        else if (driver_error_mask != 0u)
            inputReg[ADR_INPUT_DRIVER_FAULT] = 2u;
        else if (driver_warning_mask != 0u)
            inputReg[ADR_INPUT_DRIVER_FAULT] = 1u;
        else
            inputReg[ADR_INPUT_DRIVER_FAULT] = 0u;
    }
}

static void Modbus_MapCoils(void)
{
    const uint32_t driver_error_mask = can_slider.effective_error_code;
    const uint32_t mask_controller_temp_high = 0x0002u;
    const uint32_t mask_under_voltage_batt = 0x0010u;
    const uint32_t mask_over_voltage_batt = 0x0020u;
    const uint32_t mask_motor_temp_high = 0x0040u;

    coils[ADR_COIL_LOGO_BATTERY]  = (bms.batt1.voltage_V > 1.0f) ? 0x01 : 0x00;
    coils[ADR_COIL_LOGO_CHARGE]   = (bms.swSta.chgPlug) ? 0x01 : 0x00;
    coils[ADR_COIL_LOGO_MOTOR]    = (can_slider.slider_2.battery_voltage > 1.0f) ? 0x01 : 0x00;
    coils[ADR_COIL_MOS_CHARGE]    = (bms.swSta.chgMOS) ? 0x01 : 0x00;
    coils[ADR_COIL_MOS_DISCHARGE] = (bms.swSta.dchgMOS) ? 0x01 : 0x00;
    coils[ADR_COIL_MOS_BALANCE]   = (bms.swSta.balance) ? 0x01 : 0x00;
    coils[ADR_COIL_MOTOR_STATE] = (vcu_ctx.outputs.disable_motor) ? 0x00 : 0x01;
    
    // Lỗi BMS - Từng trường riêng lẻ (coil, mỗi giá trị 1 bit: 0=OK, 1=Lỗi)
    coils[ADR_COIL_BMS_ERR_LINE_RES_HIGH] = bms.bmsErrInfo.line_res_high ? 0x01 : 0x00;      // Điện trở dây cao
    coils[ADR_COIL_BMS_ERR_MOS_OVERTEMP] = bms.bmsErrInfo.mos_overtemp ? 0x01 : 0x00;       // MOS quá nhiệt
    coils[ADR_COIL_BMS_ERR_CELL_COUNT_MISMATCH] = bms.bmsErrInfo.cell_count_mismatch ? 0x01 : 0x00; // Số lượng cell không khớp
    coils[ADR_COIL_BMS_ERR_CUR_SENSOR_FAULT] = bms.bmsErrInfo.cur_sensor_fault ? 0x01 : 0x00; // Lỗi cảm biến dòng
    coils[ADR_COIL_BMS_ERR_CELL_OVERVOLT] = bms.bmsErrInfo.cell_overvolt ? 0x01 : 0x00;      // Cell quá áp
    coils[ADR_COIL_BMS_ERR_PACK_OVERVOLT] = bms.bmsErrInfo.pack_overvolt ? 0x01 : 0x00;      // Tổng áp pack cao
    coils[ADR_COIL_BMS_ERR_CHG_OVERCURRENT] = bms.bmsErrInfo.chg_overcurrent ? 0x01 : 0x00;   // Dòng sạc quá mức
    coils[ADR_COIL_BMS_ERR_CHG_SHORT] = bms.bmsErrInfo.chg_short ? 0x01 : 0x00;             // Ngắn mạch khi sạc
    coils[ADR_COIL_BMS_ERR_CHG_TEMP_HIGH] = bms.bmsErrInfo.chg_temp_high ? 0x01 : 0x00;      // Nhiệt sạc cao
    coils[ADR_COIL_BMS_ERR_CHG_TEMP_LOW] = bms.bmsErrInfo.chg_temp_low ? 0x01 : 0x00;       // Nhiệt sạc thấp
    coils[ADR_COIL_BMS_ERR_COMM_INNER_FAULT] = bms.bmsErrInfo.comm_inner_fault ? 0x01 : 0x00; // Lỗi truyền thông nội bộ
    coils[ADR_COIL_BMS_ERR_CELL_UNDERVOLT] = bms.bmsErrInfo.cell_undervolt ? 0x01 : 0x00;    // Cell thấp áp
    coils[ADR_COIL_BMS_ERR_PACK_UNDERVOLT] = bms.bmsErrInfo.pack_undervolt ? 0x01 : 0x00;    // Pack thấp áp
    coils[ADR_COIL_BMS_ERR_DCHG_OVERCURRENT] = bms.bmsErrInfo.dchg_overcurrent ? 0x01 : 0x00; // Dòng xả quá mức
    coils[ADR_COIL_BMS_ERR_DCHG_SHORT] = bms.bmsErrInfo.dchg_short ? 0x01 : 0x00;            // Ngắn mạch khi xả
    coils[ADR_COIL_BMS_ERR_DCHG_TEMP_HIGH] = bms.bmsErrInfo.dchg_temp_high ? 0x01 : 0x00;    // Nhiệt xả cao
    coils[ADR_COIL_BMS_ERR_CHG_MOS_FAULT] = bms.bmsErrInfo.chg_mos_fault ? 0x01 : 0x00;      // Lỗi MOS sạc
    coils[ADR_COIL_BMS_ERR_DCHG_MOS_FAULT] = bms.bmsErrInfo.dchg_mos_fault ? 0x01 : 0x00;    // Lỗi MOS xả

    // Lỗi Driver - Từng trường riêng lẻ (coil, mỗi giá trị 1 bit: 0=OK, 1=Lỗi)
    // effective_raw_err_code là mã lỗi hiệu dụng ưu tiên cao nhất; effective_error_code giữ đủ bit lỗi.
    coils[ADR_COIL_DRIVER_OVER_CURRENT]      = (can_slider.effective_raw_err_code == OVER_CURRENT) ? 0x01 : 0x00;       // Quá dòng
    coils[ADR_COIL_DRIVER_TEMP_HIGH]         = ((driver_error_mask & mask_controller_temp_high) != 0u) ? 0x01 : 0x00;  // Nhiệt độ bộ điều khiển cao
    coils[ADR_COIL_DRIVER_ENCODER_ERROR]     = (can_slider.effective_raw_err_code == MOTOR_ENCODER_ERROR) ? 0x01 : 0x00; // Lỗi bộ mã hóa động cơ
    coils[ADR_COIL_DRIVER_CAN_COMM_ERROR]    = (vcu_ctx.inputs.driver_can_lost ||
                                                (can_slider.effective_raw_err_code == COMMUNICATION_ERROR)) ? 0x01 : 0x00; // Lỗi truyền thông CAN
    coils[ADR_COIL_DRIVER_UNDER_VOLT]        = ((driver_error_mask & mask_under_voltage_batt) != 0u) ? 0x01 : 0x00;    // Điện áp quá thấp
    coils[ADR_COIL_DRIVER_OVER_VOLT]         = ((driver_error_mask & mask_over_voltage_batt) != 0u) ? 0x01 : 0x00;     // Điện áp quá cao
    coils[ADR_COIL_DRIVER_MOTOR_TEMP_HIGH]   = ((driver_error_mask & mask_motor_temp_high) != 0u) ? 0x01 : 0x00;       // Nhiệt độ động cơ cao
    coils[ADR_COIL_DRIVER_MOTOR_TEMP_SENSOR] = (can_slider.effective_raw_err_code == MOTOR_TEMP_SENSOR_ERROR) ? 0x01 : 0x00; // Lỗi cảm biến nhiệt độ động cơ
    coils[ADR_COIL_DRIVER_ACCEL_FAULT]       = ((can_slider.effective_raw_err_code == ACCELERATOR_FAULT) 
    && !vcu_ctx.accel_safety_interlock_active) ? 0x01 : 0x00; // Lỗi chân ga
    /* Interlock nhả ga */
    coils[ADR_COIL_DRIVER_ACCEL_SAFETY_INTERLOCK] = (vcu_ctx.inputs.init_completed && vcu_ctx.accel_safety_interlock_active) ? 0x01 : 0x00;
    coils[ADR_COIL_DRIVER_ACCELERATE_CHARGE] = vcu_ctx.accel_charge ? 0x01 : 0x00;
    coils[ADR_COIL_DRIVER_ACCELERATE_ERROR] = vcu_ctx.accel_error ? 0x01 : 0x00;
    coils[ADR_COIL_DRIVER_ACCELERATE_INIT] = vcu_ctx.accel_init ? 0x01 : 0x00;

    /*IMD*/
    coils[ADR_COIL_IMD_FAULT_BOTH] = IMD_IsBothFault() ? 1u : 0u;
    coils[ADR_COIL_IMD_FAULT_ANY] = (IMD_IsPosFault() || IMD_IsNegFault()) ? 1u : 0u;
    
}


static void SlaverSerial_MessageReadHoldingHandle(Packet_Modbus_Slaver * msg)
{
    uint16_t cmd=msg->address;
    uint8_t * ptr=(uint8_t *)&holdingReg[msg->address];
    
}

static void SlaverSerial_MessageReadInputHandle(Packet_Modbus_Slaver * msg)
{
    (void)msg;
}

static void SlaverSerial_MessagePresetMutilHandle(Packet_Modbus_Slaver * msg)
{
    uint16_t cmd=msg->address;
    uint8_t * ptr=(uint8_t *)&holdingReg[msg->address];
    
}

void SlaverSerial_MessageHandle(Packet_Modbus_Slaver * msg)
{
    switch(msg->funtion)
    {
        case PRESET_SINGLE_REGISTER:
        {
            Modbus_HandleDriverThresholdHoldingWrite(msg->address, 1);
        }
        break;
        case FORCE_SINGLE_COIL:
        {
            // Master ghi coil -> cập nhật vào hệ thống
            if (msg->address == ADR_COIL_MOTOR_STATE)
            {
                const bool disable = (coils[ADR_COIL_MOTOR_STATE] != 0);
                if (!VCU_IsManualMotorControlAllowed()) return;
                VCU_StateSetMotorStatus(disable ? ENABLE_MOTOR : DISABLE_MOTOR);
            }
        }
        break;
        case FORCE_MULTIPLE_COILS:
        {
            // Với function 15: address=start, data=quantity of coils
            const uint16_t start = msg->address;
            const uint16_t qty = msg->data;
            if ((start <= ADR_COIL_MOTOR_STATE) &&
                ((uint32_t)start + (uint32_t)qty > (uint32_t)ADR_COIL_MOTOR_STATE))
            {
                const bool disable = (coils[ADR_COIL_MOTOR_STATE] != 0);
                if (!VCU_IsManualMotorControlAllowed()) return;
                VCU_StateSetMotorStatus(disable ? ENABLE_MOTOR : DISABLE_MOTOR);
            }
        }
        break;
        case READ_HOLDING_REGISTERS:
        {
            SlaverSerial_MessageReadHoldingHandle(msg);
        }
        break;
        case READ_INPUT_REGISTERS:
        {
            SlaverSerial_MessageReadInputHandle(msg);
        }
        break;
        case PRESET_MULTIPLE_REGISTERS:
        {
            Modbus_HandleDriverThresholdHoldingWrite(msg->address, msg->data);
            SlaverSerial_MessagePresetMutilHandle(msg);
        }
        break;
        default:
            break;
    }
}

void SlaverSerial_Update(Packet_Modbus_Slaver * msg)
{
    switch(msg->funtion)
    {
        case READ_COIL_STATUS:
        {
            Modbus_MapCoils();
        }
        break;
        case READ_INPUT_REGISTERS:
        {
            Modbus_MapInputRegs();
        }
        break;
        case READ_HOLDING_REGISTERS:
        {
            Modbus_MapHoldingRegs();
        }
        break;
        default:
            break;
    }
}
