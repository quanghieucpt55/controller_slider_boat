#include "extern_rom.h"
#include "24Cxx.h"
#include "erom_interface.h"
#define CHECK_SUM 0xF3

#define NO_REPEAT_WHEN_ERROR 2



void ExRom_Init(void)
{
    EEPRom24C_Init();
}

void ExRom_Earse(void)
{
    EEPRom24_Earse();
}

uint8_t ExRom_CalculationChecksum(uint8_t * frame,uint32_t frame_size)
{
    uint8_t cs=0;

    for(uint32_t i=0;i<frame_size;i++)
    {
        cs+=frame[i];
    }
    return (CHECK_SUM-cs);
}
uint32_t ExRom_ReadWithCheckSum(uint16_t adr,uint8_t * buf,uint32_t len)
{
    uint8_t cs_read=0;
    for(uint8_t i=0;i<2;i++)
    {
        uint32_t errStatus=0;
        EEPRom24C_ReadBytes(adr,buf,len,&errStatus);
        EEPRom24C_ReadBytes(adr+len,&cs_read,1,&errStatus);
        if(errStatus==0)// đọc không có lỗi thì thoát luôn
            break;
    }
    uint8_t cs=0;
    for(uint32_t i=0;i<len;i++)
    {
        cs+=buf[i];
    }
    cs+=cs_read;
    if(cs==CHECK_SUM)
        return 1;
    else
        return 0;
}

uint32_t ExRom_SaveWithCheckSum(uint16_t adr,uint8_t * buf,uint32_t len)
{
    uint8_t cs_calc=ExRom_CalculationChecksum(buf,len);
    for(uint8_t i=0;i<NO_REPEAT_WHEN_ERROR;i++)
    {
        uint32_t errStatus=0;
        EEPRom24C_WriteBytes(adr,buf,len,&errStatus);
        EEPRom24C_WriteBytes(adr+len,(uint8_t *)&cs_calc,1,&errStatus);
        if(errStatus==0)
            break;
    }
    return len+1;
}


uint16_t ExRom_CalculatorCRC(uint8_t * frame,uint32_t frame_size)
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
uint32_t ExRom_SaveParam_WithCRC16(uint16_t adr,uint8_t * ptr_data,uint16_t len)
{
    #ifdef USE_EX_INTERNAL_ROM
        return I_Erom_SaveParam_WithCRC16(adr,ptr_data,len);
    #endif

    #ifndef USE_EX_INTERNAL_ROM
    for(uint8_t i=0;i<NO_REPEAT_WHEN_ERROR;i++)
    {
        uint32_t error=0;
        uint16_t crc_generator=ExRom_CalculatorCRC(ptr_data,len);
        EEPRom24C_WriteBytes(adr,ptr_data,len,&error);
        EEPRom24C_WriteBytes(adr+len,(uint8_t *)&crc_generator,2,&error);
        if(error==0)
            break;
    }

    return len+2;
    #endif

}



uint8_t ExRom_ReadParam_WithCRC16(uint16_t adr,uint8_t * ptr_data,uint16_t len)
{
    #ifdef USE_EX_INTERNAL_ROM
        return I_Erom_ReadParam_WithCRC16(adr,ptr_data,len);
    #endif
    #ifndef USE_EX_INTERNAL_ROM
    uint16_t crc_read=0;
    for(uint8_t i=0;i<NO_REPEAT_WHEN_ERROR;i++)
    {
        uint32_t errStatus=0;
        EEPRom24C_ReadBytes(adr,ptr_data,len,&errStatus);
        EEPRom24C_ReadBytes(adr+len,(uint8_t *)&crc_read,2,&errStatus);
        if(errStatus==0)
            break;
    }
    uint16_t crc_calculator=ExRom_CalculatorCRC(ptr_data,len);
    if(crc_read==crc_calculator)
    {
        return 1;
    }
    else
    {
        return 0;
    }
    #endif
}


