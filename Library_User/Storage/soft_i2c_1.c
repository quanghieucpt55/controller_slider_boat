#include "stm32f4xx_hal.h"
#include "soft_i2c_define.h"
#include "dwt_delay.h"
#include "main.h"

#define LOW 0
#define WRITE 0
#define READ 1

void SoftWire1_i2cDelay() {
    CyDelayUs(3);
}

void SoftWire1_sclHi()
{
	HAL_GPIO_WritePin(SOFT_I2C_SCL_GPIO_Port, SOFT_I2C_SCL_Pin, 1);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = SOFT_I2C_SCL_Pin;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull=GPIO_PULLUP;
	HAL_GPIO_Init(SOFT_I2C_SCL_GPIO_Port, &GPIO_InitStructure);

//    SoftI2C_Scl_1_Write(1);
//    SoftI2C_Scl_1_SetDriveMode(SoftI2C_Scl_1_DM_RES_UP);
}

void SoftWire1_sdaHi() {
	HAL_GPIO_WritePin(SOFT_I2C_SDA_GPIO_Port, SOFT_I2C_SDA_Pin, 1);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = SOFT_I2C_SDA_Pin;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull=GPIO_PULLUP;
	HAL_GPIO_Init(SOFT_I2C_SDA_GPIO_Port, &GPIO_InitStructure);

//    SoftI2C_Sda_1_Write(1);
//    SoftI2C_Sda_1_SetDriveMode(SoftI2C_Sda_1_DM_RES_UP);
}

void SoftWire1_sclLo() {

	HAL_GPIO_WritePin(SOFT_I2C_SCL_GPIO_Port, SOFT_I2C_SCL_Pin, 0);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = SOFT_I2C_SCL_Pin;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(SOFT_I2C_SCL_GPIO_Port, &GPIO_InitStructure);

//    SoftI2C_Scl_1_Write(LOW);
//    SoftI2C_Scl_1_SetDriveMode(SoftI2C_Scl_1_DM_STRONG);
}

void SoftWire1_sdaLo() {
	HAL_GPIO_WritePin(SOFT_I2C_SDA_GPIO_Port, SOFT_I2C_SDA_Pin, 0);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = SOFT_I2C_SDA_Pin;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(SOFT_I2C_SDA_GPIO_Port, &GPIO_InitStructure);

//    SoftI2C_Sda_1_Write(LOW);
//    SoftI2C_Sda_1_SetDriveMode(SoftI2C_Sda_1_DM_STRONG);
}

uint8_t SoftI2C_Sda_1_Read(void)
{
	return HAL_GPIO_ReadPin(SOFT_I2C_SDA_GPIO_Port, SOFT_I2C_SDA_Pin);
}

void SoftWire1_Init()
{
  SoftWire1_sdaHi();
  SoftWire1_sclHi();
}

void SoftWire1_Start()
{
  SoftWire1_sdaHi();
  SoftWire1_sclHi();
  SoftWire1_i2cDelay();
  SoftWire1_sdaLo();
  SoftWire1_i2cDelay();
  SoftWire1_sclLo();
  SoftWire1_i2cDelay();
}

void SoftWire1_Stop()
{
  SoftWire1_sdaLo();
  SoftWire1_sclLo();
  SoftWire1_i2cDelay();
  SoftWire1_sclHi();
  SoftWire1_i2cDelay();
  SoftWire1_sdaHi();
  SoftWire1_i2cDelay();
}

E_I2C_SOFT_CODE SoftWire1_ReadAck(int32_t timeOut)
{
    SoftWire1_sdaHi();
    SoftWire1_sclHi();
    SoftWire1_i2cDelay();
    int32_t timeOutUs=timeOut*1000;
    uint8_t count=1;
    E_I2C_SOFT_CODE error=I2C_SOFT_TIMEOUT;
    do
    {
        if( SoftI2C_Sda_1_Read()==0)
        {
            count--;
            error=I2C_SOFT_SUCCESSS;
        }
        CyDelayUs(1);
        timeOutUs--;
    }
    while(count!=0 && timeOutUs>0);
    SoftWire1_sclLo();
    SoftWire1_sdaLo();
    SoftWire1_i2cDelay();
    return error;
}

E_I2C_SOFT_CODE SoftWire1_WriteByte(uint8_t data_byte,int32_t timeOut)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if((data_byte&(0x80>>i))!=0x00)
    {
      SoftWire1_sdaHi(); // write 1
    } else
    {
      SoftWire1_sdaLo(); // write 0
    }
    SoftWire1_i2cDelay();
    SoftWire1_sclHi();
    SoftWire1_i2cDelay();
    SoftWire1_sclLo();
  }
  return SoftWire1_ReadAck(timeOut);
}

void SoftWire1_SendAck()
{
  SoftWire1_sdaLo();
  SoftWire1_i2cDelay();
  SoftWire1_sclHi();
  SoftWire1_i2cDelay();
  SoftWire1_sclLo();
  SoftWire1_i2cDelay();
}

void SoftWire1_sendNack()
{
  SoftWire1_sdaHi();
  SoftWire1_i2cDelay();
  SoftWire1_sclHi();
  SoftWire1_i2cDelay();
  SoftWire1_sclLo();
  SoftWire1_sdaLo();
  SoftWire1_i2cDelay();
}

uint8_t SoftWire1_readByte(uint8_t ack)
{
  uint8_t out_byte = 0;
  SoftWire1_sdaHi();
  SoftWire1_i2cDelay();
  for (uint8_t i = 0; i < 8; i++) {
    SoftWire1_sclHi();
    SoftWire1_i2cDelay();
    uint8_t out_bit = SoftI2C_Sda_1_Read();// digitalRead(data_pin);
    SoftWire1_sclLo();
    SoftWire1_i2cDelay();
    if(out_bit)
    {
        out_byte|=(0x80>>i);
    }
    //bitWrite(out_byte, 7 - i, readBit());
  }
  if(ack)
    SoftWire1_SendAck();
  else
    SoftWire1_sendNack();
  return out_byte;
}

E_I2C_SOFT_CODE SoftWire1_SendStart(uint8_t address,uint8_t bitRnW,int32_t timeOut)
{
    SoftWire1_Start();
    if(bitRnW==WRITE)
      return  SoftWire1_WriteByte(address<<1|0,timeOut);
    else
      return  SoftWire1_WriteByte(address<<1|1,timeOut);
}


