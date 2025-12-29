#ifndef _SOFT_I2C_1_H_
#define _SOFT_I2C_1_H_

#include "cytypes.h"
#include "soft_i2c_define.h"


void SoftWire1_Init();
void SoftWire1_Start();
void SoftWire1_Stop();
E_I2C_SOFT_CODE SoftWire1_WriteByte(uint8_t data_byte,int32_t timeOut);
uint8_t SoftWire1_readByte(uint8_t ack);
E_I2C_SOFT_CODE SoftWire1_SendStart(uint8_t address,uint8_t bitRnW,int32_t timeOut);

#endif
