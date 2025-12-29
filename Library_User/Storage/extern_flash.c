#include "spif.h"
#include "cytypes.h"
#include "main.h"
#include "string.h"
#include "system_update.h"
#include "global.h"

SPIF_HandleTypeDef spif;
flash_firmware_info_t flashFirmInfo;



#define BUFFER_READ 256
#define TEST_BUF_SIZE 256

uint8_t flash_buf_read[BUFFER_READ];
uint8_t flash_buf_sector[SPIF_SECTOR_SIZE];

void Flash_Init(void)
{
	SPIF_Init(&spif,&hspi1, SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin);
}

void Flash_EraseSector(uint32_t sector)
{
	SPIF_EraseSector(&spif, sector);
}

uint8_t Flash_WritePage(uint32_t page,uint8_t * buf,uint32_t len,uint32_t offset)
{
	return SPIF_WritePage(&spif,page,buf, len, offset);
}

uint8_t Flash_WritePageAndCheck(uint32_t page,uint8_t * buf,uint32_t len,uint32_t offset)
{
	if(len>BUFFER_READ)
		return 1;
	Flash_WritePage(page,buf,len,offset);
	memset(flash_buf_read,0,len);
	SPIF_ReadPage(&spif,page, flash_buf_read, len, offset);
	for(uint32_t i=0;i<len;i++)
	{
		if(buf[i]!=flash_buf_read[i])
		{
			return 1;// có sự khac biết cái dừng luôn
		}
		else
		{

		}
	}
	return 0;
}


void Flash_TestReadWrite(void)
{
	SPIF_ReadPage(&spif,0, (uint8_t *)&flashFirmInfo, sizeof(flashFirmInfo), 0);
}

// ======================= Utils ============================= //
uint16_t Flash_CalculatorCRC(uint8_t * frame,uint32_t frame_size)
{
    uint16_t temp, flag;
    temp = 0xFFFF;
    for (uint32_t i = 0; i < frame_size; i++)
    {
        temp = temp ^ frame[i];
        for (uint8_t j = 1; j <= 8; j++)
        {
            flag = temp & 0x0001;
            temp >>= 1;
            if (flag)
                temp ^= 0xA001;
        }
    }
    return (temp&0xFFFF);
}

uint32_t Flash_SaveParam_WithCRC16(uint32_t pageAdr, uint8_t * ptr_data,uint16_t len)
{
	// tính toán vị trí sector
	uint32_t numPageInSector=SPIF_SECTOR_SIZE/SPIF_PAGE_SIZE;
	uint32_t sector=pageAdr/numPageInSector;

	// ghi dữ liệu vào flash_buf_sector
	uint32_t adr_buf=(pageAdr-sector*numPageInSector)*SPIF_PAGE_SIZE;
	if(adr_buf+len+2>SPIF_SECTOR_SIZE)// vượt quá kích thước bộ nhớ
		return 0;
	// đọc lại toàn bộ dữ liệu của sector
	SPIF_ReadSector(&spif, sector, flash_buf_sector, SPIF_SECTOR_SIZE, 0);

	// xóa toàn bộ sector đi
	SPIF_EraseSector(&spif, sector);

	// tiến hành ghi dữ liệu
	uint16_t crc_generator=Flash_CalculatorCRC(ptr_data,len);
	uint8_t * ptr_start=flash_buf_sector+adr_buf;
	uint8_t * ptr_end=flash_buf_sector+SPIF_SECTOR_SIZE;
	WriteBuffer(&ptr_start,ptr_data, len, ptr_end);
	WriteBuffer(&ptr_start,(uint8_t *)&crc_generator, 2, ptr_end);

	// ghi lại dữ liệu sector
	SPIF_WriteSector(&spif, sector, flash_buf_sector, SPIF_PAGE_SIZE, 0);
    return len+2;

}

uint32_t Flash_ReadParam_WithCRC16(uint16_t pageAdr,uint8_t * ptr_data,uint16_t len)
{
	SPIF_ReadPage(&spif,pageAdr, ptr_data, len, 0);
	int16_t crc_calculator=Flash_CalculatorCRC(ptr_data,len);
    uint16_t crc_read=0;
    SPIF_ReadPage(&spif,pageAdr, (uint8_t *)&crc_read, 2, len);
    if(crc_read==crc_calculator)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}



