/*
 * flash.h
 *
 *  Created on: Apr 4, 2025
 *      Author: trank
 */

#ifndef EXTERN_FLASH_H_
#define EXTERN_FLASH_H_

#include "cytypes.h"
#include "system_update.h"

void Flash_Init(void);
void Flash_TestReadWrite(void);


#define PAGE_SYSTEM_CONFIG 12288

extern flash_firmware_info_t flashFirmInfo;

void Flash_EraseSector(uint32_t sector);
uint8_t Flash_WritePage(uint32_t page,uint8_t * buf,uint32_t len,uint32_t offset);
uint8_t Flash_WritePageAndCheck(uint32_t page,uint8_t * buf,uint32_t len,uint32_t offset);

uint32_t Flash_SaveParam_WithCRC16(uint32_t pageAdr, uint8_t * ptr_data,uint16_t len);
uint32_t Flash_ReadParam_WithCRC16(uint16_t pageAdr,uint8_t * ptr_data,uint16_t len);

#endif /* EXTERN_FLASH_H_ */
