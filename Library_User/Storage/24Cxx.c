/*
 * 24Cxx.c
 *
 *  Created on: Feb 24, 2025
 *      Author: trank
 */

#include "stm32f4xx_hal.h"
#include "24Cxx.h"
#include "wdt.h"
#include "main.h"
#include "string.h"
#include "soft_i2c_1.h"
#include "dwt_delay.h"

#define SIZE_MEMORY EEPROM_SIZE/128
#define TIME_OUT 2
#define ADR_7BIT_EEPROM 0x50
#define WRITE 0
#define READ 1
#define WRITE_CYCYLE_TIME 10 // thời gian ghi 5ms -> để 10ms cho chắc
#if (SIZE_MEMORY<=2)
    #define EEPROM_PAGE_SIZE 8
#elif(SIZE_MEMORY<=16)
    #define EEPROM_PAGE_SIZE 16

#else
    #define EEPROM_PAGE_SIZE 32
#endif

void EEPRom24C_Init(void)
{
    SoftWire1_Init();
}

uint8_t EEPROM24C_Polling(void)
{
    uint32_t errStatus= SoftWire1_SendStart(ADR_7BIT_EEPROM,READ,TIME_OUT);
    if(errStatus==I2C_SOFT_SUCCESSS)
    {
        return 1;
    }
    return 0;
}

uint8_t EEPRom24C_ReadByte(uint16_t address,uint32_t * errStatus)
{
    uint32_t err=0;
    if(address>=EEPROM_SIZE)
        return 0;
    uint8_t result=0;

    #if (SIZE_MEMORY<=2)
        // 24C01 24C02
        err=I2C_I2CMasterSendStart(ADR_7BIT_EEPROM,WRITE,1);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
        err=I2C_I2CMasterWriteByte((uint8_t)address,1);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
        err=I2C_I2CMasterSendRestart(ADR_7BIT_EEPROM,READ,1);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
    #elif(SIZE_MEMORY<=16)
        err=I2C_I2CMasterSendStart(ADR_7BIT_EEPROM|((address >> 8) & 0x07),WRITE,TIME_OUT);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
        err=I2C_I2CMasterWriteByte((uint8_t)(address&0xFF),1);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
        err=I2C_I2CMasterSendRestart(ADR_7BIT_EEPROM|((address >> 8) & 0x07),READ,TIME_OUT);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
    #else
        err=SoftWire1_SendStart(ADR_7BIT_EEPROM,WRITE,TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
        err=SoftWire1_WriteByte((uint8_t)(address>>8),TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
        err=SoftWire1_WriteByte((uint8_t)(address&0xFF),TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
        err=SoftWire1_SendStart(ADR_7BIT_EEPROM,READ,TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
    #endif
        result=SoftWire1_readByte(SOFT_I2C_NACK);
        SoftWire1_Stop();
//        err=I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA,&result,TIME_OUT);
//        if(err!=I2C_SOFT_SUCCESSS)
//            (*errStatus)++;
//        err=I2C_I2CMasterSendStop(TIME_OUT);
//        if(err!=I2C_SOFT_SUCCESSS)
//            (*errStatus)++;
    return result;
}
// AT24Cxxx cho phép đọc dữ liệu tuần tự không giới hạn
uint32_t EEPRom24C_ReadBuffer(uint16_t address,uint8_t * buf,uint32_t lenBuf,uint32_t * errStatus)
{
    uint32_t err=0;
    if(lenBuf==0)
        return 0;
    #if(SIZE_MEMORY<=2)
        err=I2C_I2CMasterSendStart(ADR_7BIT_EEPROM,WRITE,TIME_OUT);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
        err=I2C_I2CMasterWriteByte((uint8_t)address,TIME_OUT);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
        err=I2C_I2CMasterSendRestart(ADR_7BIT_EEPROM,READ,TIME_OUT);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
    #elif(SIZE_MEMORY<=16)
        err=I2C_I2CMasterSendStart(ADR_7BIT_EEPROM|((address >> 8) & 0x07),WRITE,TIME_OUT);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
        err=I2C_I2CMasterWriteByte((uint8_t)(address&0xFF),1);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
        err=I2C_I2CMasterSendRestart(ADR_7BIT_EEPROM|((address >> 8) & 0x07),READ,TIME_OUT);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
    #else
        err=SoftWire1_SendStart(ADR_7BIT_EEPROM,WRITE,TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
        err=SoftWire1_WriteByte((uint8_t)(address>>8),TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
        err=SoftWire1_WriteByte((uint8_t)(address&0xFF),TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
        err=SoftWire1_SendStart(ADR_7BIT_EEPROM,READ,TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
    #endif
    for(uint32_t i=0;i<(lenBuf-1);i++)
    {
        if((i+address+1)>=EEPROM_SIZE)// overload address eeprom
        {
            break;
        }
        else
        {
            buf[i]=SoftWire1_readByte(SOFT_I2C_ACK);
//            err=I2C_I2CMasterReadByte(I2C_I2C_ACK_DATA,buf+i,TIME_OUT);
//            if(err!=I2C_I2C_MSTR_NO_ERROR)
//                (*errStatus)++;
        }
    }
    buf[lenBuf-1]=SoftWire1_readByte(SOFT_I2C_NACK);
    SoftWire1_Stop();
//    err=I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA,(buf+lenBuf-1),TIME_OUT);
//    if(err!=I2C_I2C_MSTR_NO_ERROR)
//        (*errStatus)++;
//    err=I2C_I2CMasterSendStop(TIME_OUT);
//    if(err!=I2C_I2C_MSTR_NO_ERROR)
//        (*errStatus)++;
    return lenBuf;
}

uint16_t EEPRom24C_ReadBytes(uint16_t address,uint8_t * p_data, uint16_t length,uint32_t * errCode)
{
    uint8_t bufferCount = length / EEPROM_PAGE_SIZE;
    for (uint8_t i = 0; i < bufferCount; i++)
    {
        uint32_t err=0;
        uint16_t offset = i * EEPROM_PAGE_SIZE;
        EEPRom24C_ReadBuffer(address + offset, p_data + offset, EEPROM_PAGE_SIZE,&err);
        (*errCode)+=err;
    }
    {
        uint32_t err=0;
        uint8_t remainingBytes = length % EEPROM_PAGE_SIZE;
        uint16_t offset = length - remainingBytes;
        EEPRom24C_ReadBuffer(address + offset,p_data + offset, remainingBytes,&err);
        (*errCode)+=err;
    }
    return length;
}

void EEPRom24C_WriteByte(uint16_t address,uint8_t data,uint32_t * errStatus)
{
    uint32_t err=0;
    #if(SIZE_MEMORY<=2)
        err=I2C_I2CMasterSendStart(ADR_7BIT_EEPROM,WRITE,1);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
    #elif(SIZE_MEMORY<=16)
        err=I2C_I2CMasterSendStart(ADR_7BIT_EEPROM|((address >> 8) & 0x07),WRITE,1);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
    #else
        err=SoftWire1_SendStart(ADR_7BIT_EEPROM,WRITE,TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
        err=SoftWire1_WriteByte((uint8_t)(address>>8),TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
    #endif
    err=SoftWire1_WriteByte((uint8_t)(address&0xFF),TIME_OUT);
    if(err!=I2C_SOFT_SUCCESSS)
        (*errStatus)++;
    err=SoftWire1_WriteByte(data,TIME_OUT);
    if(err!=I2C_SOFT_SUCCESSS)
        (*errStatus)++;
    SoftWire1_Stop();
//    err=I2C_I2CMasterSendStop(TIME_OUT);
//    if(err!=I2C_SOFT_SUCCESSS)
//        (*errStatus)++;
    CyDelay(WRITE_CYCYLE_TIME);
}

uint16_t EEPRom24C_WriteBuffer(uint16_t address,uint8_t * buf,uint16_t lenBuf,uint32_t * errStatus)
{
    uint32_t err=0;
    #if(SIZE_MEMORY<=2)
        err=I2C_I2CMasterSendStart(ADR_7BIT_EEPROM,WRITE,1);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
    #elif(SIZE_MEMORY<=16)
        err=I2C_I2CMasterSendStart(ADR_7BIT_EEPROM|((address >> 8) & 0x07),WRITE,1);
        if(err!=I2C_I2C_MSTR_NO_ERROR)
            (*errStatus)++;
    #else
        err=SoftWire1_SendStart(ADR_7BIT_EEPROM,WRITE,TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
        err=SoftWire1_WriteByte((uint8_t)(address>>8),TIME_OUT);
         if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
    #endif
    err=SoftWire1_WriteByte((uint8_t)(address&0xFF),TIME_OUT);
    if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
    for(uint8_t i=0;i<lenBuf;i++)
    {
        err=SoftWire1_WriteByte(buf[i],TIME_OUT);
        if(err!=I2C_SOFT_SUCCESSS)
            (*errStatus)++;
    }
//    err=I2C_I2CMasterSendStop(TIME_OUT);
//    if(err!=I2C_SOFT_SUCCESSS)
//            (*errStatus)++;
    SoftWire1_Stop();
    CyDelay(WRITE_CYCYLE_TIME);
    return lenBuf;
}

uint32_t EEPRom24C_WriteBytes(uint16_t address,uint8_t * data,uint16_t length,uint32_t * errStatus)
{
    // Write first page if not aligned.
    uint8_t * p_data=data;
    uint8_t notAlignedLength = 0;
    uint8_t pageOffset = address % EEPROM_PAGE_SIZE;// dư
    if (pageOffset > 0)//dư
    {
        notAlignedLength = EEPROM_PAGE_SIZE - pageOffset;
        if (length < notAlignedLength)
        {
            notAlignedLength = length;
        }
        EEPRom24C_WriteBuffer(address,p_data,notAlignedLength,errStatus);
        length -= notAlignedLength;
    }

    if (length > 0)
    {
        address += notAlignedLength;
        p_data += notAlignedLength;
        uint32_t err=0;
        // Write complete and aligned pages.
        uint8_t pageCount = length / EEPROM_PAGE_SIZE;
        for (uint8_t i = 0; i < pageCount; i++)
        {
            EEPRom24C_WriteBuffer(address, p_data,EEPROM_PAGE_SIZE,&err);
            address += EEPROM_PAGE_SIZE;
            p_data += EEPROM_PAGE_SIZE;
            length -= EEPROM_PAGE_SIZE;
            (*errStatus)+=err;
        }
        if (length > 0)
        {
            uint32_t err=0;
            // Write remaining uncomplete page.
            EEPRom24C_WriteBuffer(address,p_data, length,&err);
            (*errStatus)+=err;
        }
    }
    return length;
}

void EEPRom24_Earse(void)
{
    uint16_t address=0;
    uint8_t buf[EEPROM_PAGE_SIZE];
    memset(buf,0,EEPROM_PAGE_SIZE);

    for(uint16_t i=0;i<EEPROM_SIZE/EEPROM_PAGE_SIZE;i++)
    {
        for(uint8_t idx=0;idx<2;idx++)
        {
            uint32_t err=0;
            address+=EEPRom24C_WriteBuffer(address,buf,EEPROM_PAGE_SIZE,&err);
            Wdt_Reset();// xóa watchdock tránh reset chip
            if(err==0)
                break;
        }
    }
}
