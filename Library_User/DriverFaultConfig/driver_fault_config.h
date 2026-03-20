#ifndef DRIVER_FAULT_CONFIG_H_
#define DRIVER_FAULT_CONFIG_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint16_t controller_temp_high_C; // 80 degC
	uint16_t motor_temp_high_C;      // 120 degC
	uint16_t under_voltage_V;        // 72V
	uint16_t over_voltage_V;         // 128V
	uint16_t controller_temp_warn_C; // 75 degC
	uint16_t motor_temp_warn_C;      // 110 degC
	uint16_t under_voltage_warn_V;   // 74V
	uint16_t over_voltage_warn_V;    // 126V
} driver_fault_thresholds_t;

typedef enum {
	DRIVER_THR_STATUS_OK = 0,
	DRIVER_THR_STATUS_INVALID_RANGE = 1,
	DRIVER_THR_STATUS_INVALID_RELATION = 2,
	DRIVER_THR_STATUS_SAVE_OK = 3,
	DRIVER_THR_STATUS_SAVE_FAILED = 4,
} driver_fault_threshold_status_t;

void DriverFaultConfig_Init(void);
const driver_fault_thresholds_t *DriverFaultConfig_Get(void);
bool DriverFaultConfig_Validate(const driver_fault_thresholds_t *cfg, uint16_t *status_code);
bool DriverFaultConfig_Set(const driver_fault_thresholds_t *cfg);
bool DriverFaultConfig_Save(void);

#endif /* DRIVER_FAULT_CONFIG_H_ */
