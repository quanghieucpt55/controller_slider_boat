/*
 * imd.c
 *
 * IMD (isolation monitoring) helper using ADS1115.
 */

#include <math.h>
#include "imd.h"
#include "ads1115.h"
#include "Can_Slider.h"
#include <string.h>

#define IMD_ADS1115_ADDR_7BIT           (0x4Bu)
#define IMD_MEASURE_INTERVAL_MS         (200u)
#define IMD_RELAY_SETTLE_MS             (20u)
#define IMD_RELAY_SWITCH_PAUSE_MS       (10u)
#define IMD_AVG_WINDOW                  (4u)
#define IMD_RISO_FAULT_OHM              (48000u)

#define IMD_VREF_V                      (2.5f)
#define IMD_FEEDBACK_OHM                (5110.0f)
#define IMD_INPUT_OHM                   (10000.0f)
#define IMD_MEASURE_RES_CHAIN_OHM       (1200000.0f)
#define IMD_EFFECTIVE_INPUT_OHM         (IMD_MEASURE_RES_CHAIN_OHM + IMD_INPUT_OHM)
#define IMD_MIN_VPACK_V                 (10.0f)
#define IMD_RATIO_EPSILON               (0.000001f)
#define IMD_SIGN_TOLERANCE_V            (0.1f)
#define IMD_MEASURE_FAIL_THRESHOLD      (10u)

typedef enum {
	IMD_PHASE_IDLE = 0,
	IMD_PHASE_POS_SETTLE,
	IMD_PHASE_NEG_PREPARE,
	IMD_PHASE_NEG_SETTLE
} imd_phase_t;

typedef struct {
	bool valid;
	float last_iso_voltage;
	float pole_voltage;
	uint32_t riso_ohm;
	uint8_t sample_count;
	uint8_t sample_index;
	float samples[IMD_AVG_WINDOW];
} imd_channel_t;

typedef struct {
	bool init_ok;
	uint32_t cycle_tick;
	uint32_t phase_tick;
	imd_phase_t phase;
	bool cycle_pos_ok;
	bool cycle_neg_ok;
	uint8_t consecutive_fail_cycles;
	bool measure_fault;
	imd_channel_t pos;
	imd_channel_t neg;
} imd_state_t;

static imd_state_t imd;

static void IMD_SetRelayOutputs(bool pos_on, bool neg_on)
{
	HAL_GPIO_WritePin(RELAY_POS_GPIO_Port, RELAY_POS_Pin, pos_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(RELAY_NEG_GPIO_Port, RELAY_NEG_Pin, neg_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void IMD_ClearChannel(imd_channel_t *channel)
{
	channel->valid = false;
	channel->last_iso_voltage = 0.0f;
	channel->pole_voltage = 0.0f;
	channel->riso_ohm = 0u;
	channel->sample_count = 0u;
	channel->sample_index = 0u;
	memset(channel->samples, 0, sizeof(channel->samples));
}

static void IMD_ClearResults(void)
{
	IMD_ClearChannel(&imd.pos);
	IMD_ClearChannel(&imd.neg);
	imd.cycle_pos_ok = false;
	imd.cycle_neg_ok = false;
	imd.consecutive_fail_cycles = 0u;
	imd.measure_fault = false;
}

static float IMD_GetAverageIsoVoltage(const imd_channel_t *channel)
{
	float sum = 0.0f;
	uint8_t count = channel->sample_count;

	if (count == 0u) {
		return 0.0f;
	}

	for (uint8_t i = 0u; i < count; i++) {
		sum += channel->samples[i];
	}

	return sum / (float)count;
}

static float IMD_ReconstructSignedPoleVoltage(float v_iso)
{
	/* Vpole là điện áp thật của cực đang đo so với chassis, có thể dương hoặc âm */
	const float k = IMD_EFFECTIVE_INPUT_OHM / IMD_FEEDBACK_OHM;
	return IMD_VREF_V + ((IMD_VREF_V - v_iso) * k);
}

static uint32_t IMD_ClampResistance(float r_ohm)
{
	if (r_ohm <= 0.0f) {
		return 0u;
	}
	if (r_ohm > 4294967295.0f) {
		return 0xFFFFFFFFu;
	}
	return (uint32_t)(r_ohm + 0.5f);
}
static void IMD_RefreshRiso(void)
{
	const float vpack = can_slider.slider_2.battery_voltage;

	imd.pos.riso_ohm = 0u;
	imd.neg.riso_ohm = 0u;

	if (!imd.pos.valid || !imd.neg.valid || (vpack < IMD_MIN_VPACK_V)) {
		return;
	}

	/* pos đo ra điện áp cực dương so với chassis -> phải dương */
	const float v_pos = imd.pos.pole_voltage;

	/* neg đo ra điện áp cực âm so với chassis -> thường âm
	   độ lớn nhánh âm dùng cho công thức rút gọn là |Vneg| = -Vneg */
	const float v_neg_signed = imd.neg.pole_voltage;
	const float v_neg_mag = (v_neg_signed < 0.0f) ? (-v_neg_signed) : 0.0f;

	/* Chấp nhận lệch nhỏ quanh 0 V do offset/noise, vượt ngưỡng mới coi là sai vật lý */
	if ((v_pos < -IMD_SIGN_TOLERANCE_V) || (v_neg_signed > IMD_SIGN_TOLERANCE_V)) {
		imd.pos.valid = false;
		imd.neg.valid = false;
		return;
	}

	const float p = v_pos / vpack;
	const float n = v_neg_mag / vpack;
	float common_ratio = 1.0f - p - n;

	/* p hoặc n có thể rất nhỏ, không nên loại chỉ vì == 0 */
	if ((p < 0.0f) || (n < 0.0f)) {
		imd.pos.valid = false;
		imd.neg.valid = false;
		return;
	}

	/* do sai số đo, common_ratio có thể hơi âm chút xíu */
	if (common_ratio < 0.0f) {
		if (common_ratio > -0.01f) {
			common_ratio = 0.0f;
		} else {
			imd.pos.valid = false;
			imd.neg.valid = false;
			return;
		}
	}

	const float pos_div = (n < IMD_RATIO_EPSILON) ? IMD_RATIO_EPSILON : n;
	const float neg_div = (p < IMD_RATIO_EPSILON) ? IMD_RATIO_EPSILON : p;

	const float pos_riso = IMD_EFFECTIVE_INPUT_OHM * common_ratio / pos_div;
	const float neg_riso = IMD_EFFECTIVE_INPUT_OHM * common_ratio / neg_div;

	imd.pos.riso_ohm = IMD_ClampResistance(pos_riso);
	imd.neg.riso_ohm = IMD_ClampResistance(neg_riso);
}

static bool IMD_UpdateChannelSample(imd_channel_t *channel, uint16_t mux)
{
	float voltage = 0.0f;
	if (ADS1115_readSingleEnded(mux, &voltage) != HAL_OK) {
		channel->valid = false;
		channel->pole_voltage = 0.0f;
		channel->riso_ohm = 0u;
		return false;
	}

	channel->last_iso_voltage = voltage;
	channel->samples[channel->sample_index] = voltage;
	channel->sample_index = (uint8_t)((channel->sample_index + 1u) % IMD_AVG_WINDOW);
	if (channel->sample_count < IMD_AVG_WINDOW) {
		channel->sample_count++;
	}

	const float avg_iso_voltage = IMD_GetAverageIsoVoltage(channel);
	channel->pole_voltage = IMD_ReconstructSignedPoleVoltage(avg_iso_voltage);
	channel->valid = true;
	return true;
}

static void IMD_UpdateMeasureFaultByCycle(void)
{
	const bool cycle_ok = imd.cycle_pos_ok && imd.cycle_neg_ok;
	if (cycle_ok) {
		imd.consecutive_fail_cycles = 0u;
		imd.measure_fault = false;
		return;
	}

	if (imd.consecutive_fail_cycles < 0xFFu) {
		imd.consecutive_fail_cycles++;
	}
	imd.measure_fault = (imd.consecutive_fail_cycles >= IMD_MEASURE_FAIL_THRESHOLD);
}

void IMD_Init(I2C_HandleTypeDef *hi2c)
{
	memset(&imd, 0, sizeof(imd));
	IMD_SetRelayOutputs(false, false);
	imd.phase = IMD_PHASE_IDLE;

	ADS1115_SetAddress(IMD_ADS1115_ADDR_7BIT);
	if (ADS1115_Init(hi2c, ADS1115_DATA_RATE_128, ADS1115_PGA_TWOTHIRDS) == HAL_OK) {
		imd.init_ok = true;
		imd.cycle_tick = HAL_GetTick();
	}
}

void IMD_Task(void)
{
	if (!imd.init_ok) {
		return;
	}

	const uint32_t now = HAL_GetTick();
	if ((can_slider.last_rx_tick == 0u) || (can_slider.slider_2.battery_voltage < IMD_MIN_VPACK_V)) {
		IMD_SetRelayOutputs(false, false);
		IMD_ClearResults();
		imd.phase = IMD_PHASE_IDLE;
		imd.cycle_tick = now;
		imd.phase_tick = now;
		return;
	}

	switch (imd.phase) {
	case IMD_PHASE_IDLE:
		if ((now - imd.cycle_tick) >= IMD_MEASURE_INTERVAL_MS) {
			imd.cycle_pos_ok = false;
			imd.cycle_neg_ok = false;
			IMD_SetRelayOutputs(true, false);
			imd.phase = IMD_PHASE_POS_SETTLE;
			imd.phase_tick = now;
		}
		break;

	case IMD_PHASE_POS_SETTLE:
		if ((now - imd.phase_tick) >= IMD_RELAY_SETTLE_MS) {
			imd.cycle_pos_ok = IMD_UpdateChannelSample(&imd.pos, ADS1115_MUX_AIN0);
			IMD_SetRelayOutputs(false, false);
			imd.phase = IMD_PHASE_NEG_PREPARE;
			imd.phase_tick = now;
		}
		break;

	case IMD_PHASE_NEG_PREPARE:
		if ((now - imd.phase_tick) >= IMD_RELAY_SWITCH_PAUSE_MS) {
			IMD_SetRelayOutputs(false, true);
			imd.phase = IMD_PHASE_NEG_SETTLE;
			imd.phase_tick = now;
		}
		break;

	case IMD_PHASE_NEG_SETTLE:
		if ((now - imd.phase_tick) >= IMD_RELAY_SETTLE_MS) {
			imd.cycle_neg_ok = IMD_UpdateChannelSample(&imd.neg, ADS1115_MUX_AIN1);
			IMD_SetRelayOutputs(false, false);
			IMD_RefreshRiso();
			IMD_UpdateMeasureFaultByCycle();
			imd.phase = IMD_PHASE_IDLE;
			imd.cycle_tick = now;
			imd.phase_tick = now;
		}
		break;

	default:
		IMD_SetRelayOutputs(false, false);
		imd.phase = IMD_PHASE_IDLE;
		imd.cycle_tick = now;
		imd.phase_tick = now;
		break;
	}
}

uint32_t IMD_GetRisoPosOhm(void)
{
	return imd.pos.riso_ohm;
}

uint32_t IMD_GetRisoNegOhm(void)
{
	return imd.neg.riso_ohm;
}

bool IMD_IsPosFault(void)
{
	return imd.pos.valid && (imd.pos.riso_ohm < IMD_RISO_FAULT_OHM);
}

bool IMD_IsNegFault(void)
{
	return imd.neg.valid && (imd.neg.riso_ohm < IMD_RISO_FAULT_OHM);
}

bool IMD_IsBothFault(void)
{
	return IMD_IsPosFault() && IMD_IsNegFault();
}

bool IMD_IsMeasureFault(void)
{
	return imd.measure_fault;
}

void IMD_GetStatus(imd_status_t *status)
{
	if (status == NULL) {
		return;
	}

	status->pos_low_riso = IMD_IsPosFault();
	status->neg_low_riso = IMD_IsNegFault();
	status->measure_fault = IMD_IsMeasureFault();
	status->pos_riso_ohm = IMD_GetRisoPosOhm();
	status->neg_riso_ohm = IMD_GetRisoNegOhm();
}
