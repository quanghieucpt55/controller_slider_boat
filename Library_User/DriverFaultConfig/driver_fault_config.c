#include "driver_fault_config.h"

#include "extern_rom.h"
#include <string.h>

static driver_fault_thresholds_t g_driver_fault_thr;

typedef struct {
	uint16_t controller_temp_high_C;
	uint16_t motor_temp_high_C;
	uint16_t under_voltage_V;
	uint16_t over_voltage_V;
} driver_fault_thresholds_legacy_t;

static void DriverFaultConfig_LoadDefaults(driver_fault_thresholds_t *cfg)
{
	cfg->controller_temp_high_C = 80u;
	cfg->motor_temp_high_C = 120u;
	cfg->under_voltage_V = 72u;
	cfg->over_voltage_V = 128u;
	cfg->controller_temp_warn_C = 75u;
	cfg->motor_temp_warn_C = 110u;
	cfg->under_voltage_warn_V = 74u;
	cfg->over_voltage_warn_V = 126u;
}

void DriverFaultConfig_Init(void)
{
	memset(&g_driver_fault_thr, 0, sizeof(g_driver_fault_thr));
	DriverFaultConfig_LoadDefaults(&g_driver_fault_thr);

	if (!ExRom_ReadParam_WithCRC16(ADR_ROM_DRIVER_FAULT_THRESHOLDS,
			(uint8_t *)&g_driver_fault_thr, (uint16_t)sizeof(g_driver_fault_thr))) {
		driver_fault_thresholds_legacy_t legacy_cfg;

		memset(&legacy_cfg, 0, sizeof(legacy_cfg));
		if (ExRom_ReadParam_WithCRC16(ADR_ROM_DRIVER_FAULT_THRESHOLDS,
				(uint8_t *)&legacy_cfg, (uint16_t)sizeof(legacy_cfg))) {
			g_driver_fault_thr.controller_temp_high_C = legacy_cfg.controller_temp_high_C;
			g_driver_fault_thr.motor_temp_high_C = legacy_cfg.motor_temp_high_C;
			g_driver_fault_thr.under_voltage_V = legacy_cfg.under_voltage_V;
			g_driver_fault_thr.over_voltage_V = legacy_cfg.over_voltage_V;
			if (g_driver_fault_thr.controller_temp_warn_C > g_driver_fault_thr.controller_temp_high_C) {
				g_driver_fault_thr.controller_temp_warn_C = g_driver_fault_thr.controller_temp_high_C;
			}
			if (g_driver_fault_thr.motor_temp_warn_C > g_driver_fault_thr.motor_temp_high_C) {
				g_driver_fault_thr.motor_temp_warn_C = g_driver_fault_thr.motor_temp_high_C;
			}
			if (g_driver_fault_thr.under_voltage_warn_V < g_driver_fault_thr.under_voltage_V) {
				g_driver_fault_thr.under_voltage_warn_V = g_driver_fault_thr.under_voltage_V;
			}
			if (g_driver_fault_thr.over_voltage_warn_V > g_driver_fault_thr.over_voltage_V) {
				g_driver_fault_thr.over_voltage_warn_V = g_driver_fault_thr.over_voltage_V;
			}
		}

		if (!DriverFaultConfig_Validate(&g_driver_fault_thr, NULL)) {
			DriverFaultConfig_LoadDefaults(&g_driver_fault_thr);
		}

		(void)ExRom_SaveParam_WithCRC16(ADR_ROM_DRIVER_FAULT_THRESHOLDS,
			(uint8_t *)&g_driver_fault_thr, (uint16_t)sizeof(g_driver_fault_thr));
	} else if (!DriverFaultConfig_Validate(&g_driver_fault_thr, NULL)) {
		DriverFaultConfig_LoadDefaults(&g_driver_fault_thr);
		(void)ExRom_SaveParam_WithCRC16(ADR_ROM_DRIVER_FAULT_THRESHOLDS,
				(uint8_t *)&g_driver_fault_thr, (uint16_t)sizeof(g_driver_fault_thr));
	}
}

const driver_fault_thresholds_t *DriverFaultConfig_Get(void)
{
	return &g_driver_fault_thr;
}

bool DriverFaultConfig_Validate(const driver_fault_thresholds_t *cfg, uint16_t *status_code)
{
	if (status_code != NULL) {
		*status_code = DRIVER_THR_STATUS_OK;
	}

	if (cfg == NULL) {
		if (status_code != NULL) {
			*status_code = DRIVER_THR_STATUS_INVALID_RANGE;
		}
		return false;
	}

	if ((cfg->controller_temp_high_C == 0u) || (cfg->controller_temp_high_C > 80u) ||
		(cfg->motor_temp_high_C == 0u) || (cfg->motor_temp_high_C > 140u) ||
		(cfg->under_voltage_V == 0u) || (cfg->under_voltage_V < 70u) ||
		(cfg->over_voltage_V == 0u) || (cfg->over_voltage_V > 130u) ||
		(cfg->controller_temp_warn_C == 0u) || (cfg->controller_temp_warn_C > 80u) ||
		(cfg->motor_temp_warn_C == 0u) || (cfg->motor_temp_warn_C > 140u) ||
		(cfg->under_voltage_warn_V == 0u) || (cfg->under_voltage_warn_V < 70u) ||
		(cfg->over_voltage_warn_V == 0u) || (cfg->over_voltage_warn_V > 130u)) {
		if (status_code != NULL) {
			*status_code = DRIVER_THR_STATUS_INVALID_RANGE;
		}
		return false;
	}

	if ((cfg->under_voltage_V >= cfg->over_voltage_V) ||
		(cfg->under_voltage_warn_V >= cfg->over_voltage_warn_V) ||
		(cfg->controller_temp_warn_C > cfg->controller_temp_high_C) ||
		(cfg->motor_temp_warn_C > cfg->motor_temp_high_C) ||
		(cfg->under_voltage_warn_V < cfg->under_voltage_V) ||
		(cfg->over_voltage_warn_V > cfg->over_voltage_V)) {
		if (status_code != NULL) {
			*status_code = DRIVER_THR_STATUS_INVALID_RELATION;
		}
		return false;
	}

	return true;
}

bool DriverFaultConfig_Set(const driver_fault_thresholds_t *cfg)
{
	if (!DriverFaultConfig_Validate(cfg, NULL)) {
		return false;
	}

	g_driver_fault_thr = *cfg;
	return true;
}

bool DriverFaultConfig_Save(void)
{
	driver_fault_thresholds_t verify_cfg;

	(void)ExRom_SaveParam_WithCRC16(ADR_ROM_DRIVER_FAULT_THRESHOLDS,
			(uint8_t *)&g_driver_fault_thr, (uint16_t)sizeof(g_driver_fault_thr));

	memset(&verify_cfg, 0, sizeof(verify_cfg));
	if (!ExRom_ReadParam_WithCRC16(ADR_ROM_DRIVER_FAULT_THRESHOLDS,
			(uint8_t *)&verify_cfg, (uint16_t)sizeof(verify_cfg))) {
		return false;
	}

	return (memcmp(&verify_cfg, &g_driver_fault_thr, sizeof(g_driver_fault_thr)) == 0);
}
