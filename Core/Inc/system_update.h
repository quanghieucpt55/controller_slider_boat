/*
 * system_update.h
 *
 *  Created on: Mar 14, 2025
 *      Author: trank
 */

#ifndef INC_SYSTEM_UPDATE_H_
#define INC_SYSTEM_UPDATE_H_

#include "cytypes.h"

#define UPDATE_USB_BOOTLOADER 1
#define UPDATE_FLASH_BOOTLOADER 2

typedef struct flash_firmware_info_t flash_firmware_info_t;
struct flash_firmware_info_t
{
	uint32_t pageAdrInfo;	// trang chứa thông tin bắt đầu
	uint32_t pageAdrStart;	// địa chỉ page bắt đầu lưu trữ chương trình
	uint32_t totalBytes;// tổng số byte chiều dài
	uint16_t crc;
}CY_PACKED_ATTR;

void SystemUpdate_Init(void);
void SystemUpdate_Require(uint8_t value);
void SystemUpdate_Run(void);

#endif /* INC_SYSTEM_UPDATE_H_ */
