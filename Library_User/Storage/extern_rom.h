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
 
 #define ADR_ROM_BOAT_CONFIG 0          // size boat config 70
 #define ADR_ROM_BATTERY_CONFIG 200          // size battery_config 32
 #define ADR_ROM_TIMERS_CONFIG 300           // size timers config 48
 #define ADR_ROM_NETWORK_CONFIG 400          // vị trí lưu cấu hình network
#define ADR_ROM_DRIVER_FAULT_THRESHOLDS 450 // ngưỡng lỗi driver: temp(C) + volt(0.1V)
 #define ADR_EXROM_EVENT_LOG 1024
 
 #endif
 
 #endif /* EXTERN_ROM_H_ */
 