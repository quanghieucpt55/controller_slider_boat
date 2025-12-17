/*
 * extern_rom.h
 *
 *  Created on: Feb 24, 2025
 *      Author: trank
 */

#ifndef EXTERN_ROM_H_
#define EXTERN_ROM_H_

#include "cytypes.h"

//#define USE_EX_INTERNAL_ROM // sử dụng rom nội

void ExRom_Init(void);
void ExRom_Earse(void);

uint32_t ExRom_ReadWithCheckSum(uint16_t adr,uint8_t * buf,uint32_t len);
uint32_t ExRom_SaveWithCheckSum(uint16_t adr,uint8_t * buf,uint32_t len);

uint32_t ExRom_SaveParam_WithCRC16(uint16_t adr,uint8_t * ptr_data,uint16_t len);
uint8_t ExRom_ReadParam_WithCRC16(uint16_t adr,uint8_t * ptr_data,uint16_t len);


#ifdef USE_EX_INTERNAL_ROM

#define ADR_EXROM_ENGINE_LOG 1950
#define ADR_EXROM_EVENT_LOG 2000

#endif

#ifndef USE_EX_INTERNAL_ROM

#define ADR_EXROM_ETHERNET_MAC 0

#define ADR_ROM_GENERATOR_CONFIG 0          // size generator config 70
#define ADR_ROM_INPUT_DIGITAL_CONFIG 100    // size input digital analog config 30
#define ADR_ROM_OUT_DIGITAL_CONFIG 150      // size out digital config 12
#define ADR_ROM_BATTERY_CONFIG 200          // size battery_config 32
#define ADR_ROM_STATION_CONTROL_CONFIG 250  // size statin config 22
#define ADR_ROM_TEMPERATURE_NTC_CONFIG 300  // size temperature ntc confg 8

#define ADR_ROM_TIMERS_CONFIG 500           // size timers config 48
#define ADR_ROM_NETWORK_CONFIG 600          // vị trí lưu cấu hình network
#define ADR_ROM_SYSTEM_CALIBRATION 700      // cấu hình hệ thống
#define ADR_ROM_OUTPUT_EXTERN_CONFIG 800    // cấu hình mở rộng đầu ra số
#define ADR_ROM_STATION_DISCHAGE_CONFIG 900 // cấu hình thông số xả trạm
#define ADR_ROM_PUMP_WATER_CONFIG 1000      // cấu hình bơm nước
#define ADR_ROM_AIR_WARNING_CONFIG 1050 	// cấu hình cảnh báo điều hòa


#define ADR_EXROM_EVENT_LOG 2048

#endif

#endif /* EXTERN_ROM_H_ */
