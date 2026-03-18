#include "stm32f4xx_hal.h"
#include "clock.h"
#include "swconfig.h"
#include "bootloader.h"
//#include "usbd_cdc_if.h"
//#include "usb_device.h"
#include "system_update.h"
//extern USBD_HandleTypeDef hUsbDeviceFS;

uint8_t isSytemUpdate=0;
int32_t lastTimeSystemUpdate=0;
#define DFU_BOOT_FLAG 0xDEADBEEF
#define FLASH_BOOT_FLAG 0x1EC8BEFF

#define ENABLE_UPDATE_BOOTLOADER

extern int _bflag;
uint32_t *dfu_boot_flag;


void SystemUpdate_Init(void)
{
	dfu_boot_flag = (uint32_t*) (&_bflag); // set in linker script
}

void SystemUpdate_Require(uint8_t value)
{
    isSytemUpdate=value;
    lastTimeSystemUpdate=millis();
}

void CheckBootloaderJump()
{
	//Check if we did a system reset and want to jump into bootloader
	if(*dfu_boot_flag == DFU_BOOT_FLAG)
	{
		JumpToBootloader();
		while(1);
	}

}

void SystemUpdate_Run(void)
{
    if(isSytemUpdate!=0)
    {
    	int32_t delay=1000;
    	if(isSytemUpdate==UPDATE_FLASH_BOOTLOADER)
    		delay=2000;
        if(millis()-lastTimeSystemUpdate>=delay)
        {
        	if(isSytemUpdate==UPDATE_USB_BOOTLOADER)
        	{
        		*dfu_boot_flag = DFU_BOOT_FLAG;
        	}
        	else
        	{
        		*dfu_boot_flag = FLASH_BOOT_FLAG;
        	}
			HAL_NVIC_SystemReset();
        }
    }
    else
    {
        lastTimeSystemUpdate=millis();
    }

}
