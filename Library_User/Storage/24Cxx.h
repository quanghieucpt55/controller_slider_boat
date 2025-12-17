/*
 * 24Cxx.h
 *
 *  Created on: Feb 24, 2025
 *      Author: trank
 */

#ifndef _24CXX_H_
#define _24CXX_H_

#include "cytypes.h"
#define EEPROM_SIZE 4096 // eeprom 24C08 : 8kb =1024B
#define PHYSICAL_ADR_EEPROM_MAC 0


void EEPRom24C_Init(void);
uint8_t EEPRom24C_ReadByte(uint16_t address,uint32_t * errStatus);
uint16_t EEPRom24C_ReadBytes(uint16_t address,uint8_t * p_data, uint16_t length,uint32_t * errCode);
void EEPRom24C_WriteByte(uint16_t address,uint8_t data,uint32_t * errStatus);
uint32_t EEPRom24C_WriteBytes(uint16_t address,uint8_t * data,uint16_t length,uint32_t * errStatus);
void EEPRom24_Earse(void);



#endif /* 24CXX_H_ */
