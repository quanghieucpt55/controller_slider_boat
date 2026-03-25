/*
 * imd.h
 *
 * IMD (isolation monitoring) helper using ADS1115.
 */

#ifndef IMD_H
#define IMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
	bool pos_low_riso;
	bool neg_low_riso;
	bool measure_fault;
	uint32_t pos_riso_ohm;
	uint32_t neg_riso_ohm;
} imd_status_t;

void IMD_Init(I2C_HandleTypeDef *hi2c);
void IMD_Task(void);
uint32_t IMD_GetRisoPosOhm(void);
uint32_t IMD_GetRisoNegOhm(void);
bool IMD_IsPosFault(void);
bool IMD_IsNegFault(void);
bool IMD_IsBothFault(void);
bool IMD_IsMeasureFault(void);
void IMD_GetStatus(imd_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* IMD_H */
