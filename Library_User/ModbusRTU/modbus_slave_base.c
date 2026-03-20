/*
 * modbus_slave_base.c
 *
 *  Created on: Jan 13, 2025
 *      Author: trank
*/


#include <stdint.h>
#include <stddef.h>
#include "modbus_slave_base.h"

#define CHECK_BIT(var,pos) !!((var) & (1 << (pos)))


uint16_t  holdingReg[HOLDING_REG_SIZE];
uint16_t inputReg[SIZE_INPUT_REG];

unsigned char coils[COIL_REG_SIZE];

void ModbusSlaver_ReadCoils(Packet_Modbus_Slaver * packet);
void ModbusSlaver_ReadInpuState(Packet_Modbus_Slaver * packet);
void ModbusSlaver_ReadHoldingReg(Packet_Modbus_Slaver * packet); // ok
void ModbusSlaver_ReadInputReg(Packet_Modbus_Slaver * packet);
void ModbusSlaver_WriteSingleCoil(Packet_Modbus_Slaver * packet);
void ModbusSlaver_WriteMultipleCoils(Packet_Modbus_Slaver * packet);
void ModbusSlaver_PresetSingleRegister(Packet_Modbus_Slaver * packet);
void ModbusSlaver_WriteMultiRegister(Packet_Modbus_Slaver * packet);


// get funtion packet
uint8_t ModbusSlaver_GetFuntion(Packet_Modbus_Slaver * packet)
{
    return packet->buf_recv[1];
}
// get address include addresiger, address colis,address input ,.....
uint16_t ModbusSlaver_GetAddress(Packet_Modbus_Slaver * packet)
{
    uint16_t address=packet->buf_recv[2];
    address<<=8;
    address|=packet->buf_recv[3];
    return address;
}
uint16_t ModbusSlaver_GetData(Packet_Modbus_Slaver * packet)
{
    uint16_t data=packet->buf_recv[4];
    data<<=8;
    data|=packet->buf_recv[5];
    return data;
}
uint8_t ModbusSlaver_DecodeMsg(Packet_Modbus_Slaver * packet)
{
    uint8_t result=NOTDONE;
    uint8_t * buf_recv=packet->buf_recv;// get buffer recv
    uint8_t lenBufRecv=packet->len_buf_recv; // get lenbuffer recv
    if(buf_recv[0]==packet->id_slaver)// nếu đúng địa chỉ
    {
        if(ModbusSlaver_CheckCRC(buf_recv,lenBufRecv))// check CRC
        {
            // correct
            packet->funtion=ModbusSlaver_GetFuntion(packet);//
            packet->address=ModbusSlaver_GetAddress(packet);
            packet->data=ModbusSlaver_GetData(packet);
            result=SUCCESSFUL;// trả về thành công
        }
    }
    else
        result=ERROR_SLAVER_ADD;// sai địa chỉ slaver
    return result;
}
void Slaver_Do(Packet_Modbus_Slaver * packet)
{
    switch(packet->funtion)
    {
        case READ_COIL_STATUS:
        case READ_INPUT_STATUS:
        case READ_HOLDING_REGISTERS:
        case READ_INPUT_REGISTERS:
            break;
        case FORCE_SINGLE_COIL:
            ModbusSlaver_WriteSingleCoil(packet);// chưa dùng
            break;
        case FORCE_MULTIPLE_COILS:
            ModbusSlaver_WriteMultipleCoils(packet);// chưa dùng
            break;
        case PRESET_SINGLE_REGISTER:
            ModbusSlaver_PresetSingleRegister(packet);
            break;
        case PRESET_MULTIPLE_REGISTERS:
            ModbusSlaver_WriteMultiRegister(packet);
            break;
        case READ_GENERAL_REFEREN:
        case WRITE_GENERAL_REFEREN:
            break;
    }
    if(NULL!=packet->OnMessageRecvHandle)
        packet->OnMessageRecvHandle(packet);
}
void Slaver_PrepareRespond(Packet_Modbus_Slaver * packet)
{
    if(NULL!=packet->systemInforRefresh)
        packet->systemInforRefresh(packet);
    switch(packet->funtion)
    {
        case READ_COIL_STATUS:
            ModbusSlaver_ReadCoils(packet);
            break;
        case READ_INPUT_STATUS:
            ModbusSlaver_ReadInpuState(packet);
            break;
        case READ_HOLDING_REGISTERS:
            ModbusSlaver_ReadHoldingReg(packet);
            break;
        case READ_INPUT_REGISTERS:
            ModbusSlaver_ReadInputReg(packet);
            break;
        case FORCE_SINGLE_COIL:
        case PRESET_SINGLE_REGISTER:
        case FORCE_MULTIPLE_COILS:
        case PRESET_MULTIPLE_REGISTERS:
        {
            uint8_t * buf_resp=packet->buf_resp;
            buf_resp[0]=packet->id_slaver;
            buf_resp[1]=packet->funtion;
            buf_resp[2]=((packet->address)>>8);
            buf_resp[3]=(uint8_t)packet->address;
            buf_resp[4]=((packet->data)>>8);
            buf_resp[5]=(uint8_t)packet->data;
            uint16_t crc=ModbusSlaver_CalculateCRC(buf_resp,8);
            buf_resp[6]=crc>>8;
            buf_resp[7]=crc;
            packet->len_buf_resp=8;
        }
            break;
        case READ_GENERAL_REFEREN:
        case WRITE_GENERAL_REFEREN:
            break;
    }
}
void ModbusSlaver_WriteSingleCoil(Packet_Modbus_Slaver * packet)
{
    unsigned int wc_Address = 0;
    unsigned int wc_valToWrite = 0;

    wc_Address = packet->address;//Combine address bytes
    wc_valToWrite =packet->data;//Combine value to write regs
    // check buffer coil
    if(wc_Address<COIL_REG_SIZE)
    {
        if(wc_valToWrite)
        {
            coils[wc_Address] = 0xFF;
        }
        else
        {
            coils[wc_Address] = 0x00;
        }
    }
}
void ModbusSlaver_WriteMultipleCoils(Packet_Modbus_Slaver * packet)
{
    uint8_t * buf_recv=packet->buf_recv;
    //Combine address bytes
    uint16_t wmc_Address = packet->address;
    //Combine number of coils bytes
    uint16_t wmc_numCoils = packet->data;
    uint8_t howManyBytes = wmc_numCoils/8;
    uint8_t remainder = wmc_numCoils % 8;
    if(remainder)
        howManyBytes += 1;
    uint8_t adr_coil = wmc_Address;

    unsigned char byte_process = 7; //count through vals to write

    for(uint8_t i=howManyBytes; i!=0; i--)
    {
        uint8_t valToWrite = buf_recv[byte_process];
        byte_process++;
        if(i>1)
        {
            for(uint8_t j=0;j!=8;j++)
            {
                if(adr_coil<COIL_REG_SIZE)
                {
                    if(CHECK_BIT(valToWrite, j))
                    {
                        coils[adr_coil] = 1;
    	            }
    	            else
                    {
                        coils[adr_coil] = 0; //need to sort out remainder stuff
    	            }
    	            adr_coil++;
                }
                else
                    return;
            }
        }
        else
        {
            for(uint8_t j=0;j!=remainder;j++)
            {
                if(adr_coil<COIL_REG_SIZE)
                {
                    if(CHECK_BIT(valToWrite, j))
                    {
                        coils[adr_coil] = 1;
                    }
                    else
                    {
                        coils[adr_coil] = 0;
                    }
                    adr_coil++;
                }
                else
                    return;
            }
        }
    }
}
void ModbusSlaver_PresetSingleRegister(Packet_Modbus_Slaver * packet)
{
    unsigned int wr_Address = 0;
    unsigned int wr_valToWrite = 0;
    //Combine address bytes
    wr_Address = packet->address;
    //Combine value to write regs
    wr_valToWrite = packet->data;
    if(wr_Address<HOLDING_REG_SIZE)
        holdingReg[wr_Address] = wr_valToWrite;
}
void ModbusSlaver_WriteMultiRegister(Packet_Modbus_Slaver * packet)
{
    uint8_t * buf_recv=packet->buf_recv;
    //Combine address bytes
    unsigned int wmr_Address = packet->address;
    unsigned int wmr_numBytes = buf_recv[6];
    unsigned char index = 7;
    unsigned int wmr_numBytesTST = wmr_numBytes /2;

    for(unsigned int i=0;i<wmr_numBytesTST;i++)
    {
        unsigned int valToWrite = buf_recv[index];;
        valToWrite <<= 8;
        index++;
        valToWrite |= buf_recv[index];
        index++;
        if((wmr_Address+i)<HOLDING_REG_SIZE)// protected ram
            holdingReg[wmr_Address + i] = valToWrite;
        else
            break;
    }
}
void ModbusSlaver_ReadCoils(Packet_Modbus_Slaver * packet)
{
    uint8_t * buf_resp=packet->buf_resp;

    unsigned char howManyBytes = 0;
    unsigned char remainder = 0;
    unsigned char lsb = 0;
    unsigned char i,j,l = 0;

    //Combine address bytes
    unsigned int rc_Address = packet->address;
    //Combine number of coils bytes
    unsigned int rc_numCoils = packet->data;

    howManyBytes = rc_numCoils/8;
    remainder = rc_numCoils % 8;

    if(remainder){
        howManyBytes += 1;
    }
    l = rc_Address;
    uint8_t frameSize = 3; //start at buf_send_m_lora

    for(i=howManyBytes; i!=0; i--)
    {
        buf_resp[frameSize] = 0; // Initialize byte to zero
        if(i>1)
        {
            for(j=0;j!=8;j++)
            {
        	    if(coils[l])
                {
                  lsb = 1;
                }
    	        else
                {
                    lsb = 0;
    	        }
    	        buf_resp[frameSize] ^= (lsb << j);
    	        l++;
            }
            frameSize++;
        }
        else
        {
            for(j=0;j!=remainder;j++)
            {
                if(coils[l])
                {
                    lsb = 1;
                }
                else
                {
                    lsb = 0;
                }
                buf_resp[frameSize] ^= (lsb << j);
                l++;
            }
            frameSize++;
        }
    }
    frameSize+=2;
    buf_resp[0]=packet->id_slaver;
    buf_resp[1]=packet->funtion;
    buf_resp[2] = howManyBytes;

    packet->len_buf_resp=frameSize;
    // caculator crc
    uint16_t crc=ModbusSlaver_CalculateCRC(buf_resp,frameSize);
    buf_resp[frameSize-2]=crc>>8;
    buf_resp[frameSize-1]=crc;

}
void ModbusSlaver_ReadInpuState(Packet_Modbus_Slaver * packet)
{
    uint8_t * buf_resp=packet->buf_resp;
    unsigned char howManyBytes = 0;
    unsigned char remainder = 0;
    unsigned char lsb = 0;
    unsigned char i,j,l = 0;
    //Combine address bytes
    unsigned int rc_Address = packet->address;
    //Combine number of coils bytes
    unsigned int rc_numCoils = packet->data;

    howManyBytes = rc_numCoils/8;
    remainder = rc_numCoils % 8;
    if(remainder)
    {
        howManyBytes += 1;
    }
    l = rc_Address;
    uint8_t frameSize = 3; //start at buf_send_m_lora 3
    for(i=howManyBytes; i!=0; i--)
    {
        buf_resp[frameSize] = 0; // Initialize byte to zero
		if(i>1)
        {
            for(j=0;j!=8;j++)
            {
				if(coils[l])
                {
					lsb = 1;
				}
				else
                {
                    lsb = 0;
				}
				buf_resp[frameSize] ^= (lsb << j);
				l++;
	        }
			frameSize++;
	    }
		else
        {
			for(j=0;j!=remainder;j++)
            {
				if(coils[l])
                {
					lsb = 1;
				}
				else
                {
                    lsb = 0;
				}
                buf_resp[frameSize] ^= (lsb << j);
				l++;
			}
			frameSize++;
		}
    }
    frameSize+=2;

    packet->len_buf_resp=frameSize;// lấy chiều dài của frame
    buf_resp[0] = packet->id_slaver; // slaver address
    buf_resp[1] = packet->funtion; // Funtion
    buf_resp[2] = howManyBytes; // byte Count
    uint16_t crc = ModbusSlaver_CalculateCRC(buf_resp,frameSize);
    buf_resp[frameSize-2] = crc >> 8;
    buf_resp[frameSize-1] = crc;

}
void ModbusSlaver_ReadHoldingReg(Packet_Modbus_Slaver * packet) // ok
{
    uint8_t * buf_resp=packet->buf_resp;
    unsigned int rr_Address = 0;
    unsigned int rr_numRegs = 0;
    unsigned int crc = 0;

    //Combine address bytes
    rr_Address = packet->address;

    //Combine number of regs bytes
    rr_numRegs = packet->data;

    // Fill data Analog to holding address register
    unsigned char frameLen = 3;
    for(unsigned int i=rr_Address;(i<(rr_Address + rr_numRegs));i++)
    {
        if(i<HOLDING_REG_SIZE)
        {
            //Need to split it up into 2 bytes
            buf_resp[frameLen] = holdingReg[i] >> 8;
            frameLen++;
            buf_resp[frameLen] = holdingReg[i];
            frameLen++;
        }
        else
        {
            //Need to split it up into 2 bytes
            buf_resp[frameLen] = 0;
            frameLen++;
            buf_resp[frameLen] = 0;
            frameLen++;
        }

    }
    frameLen+=2;
    packet->len_buf_resp=frameLen;
    buf_resp[0] = packet->id_slaver;
    buf_resp[1] = packet->funtion;
    buf_resp[2] = rr_numRegs*2; //2 bytes per reg
    crc = ModbusSlaver_CalculateCRC(buf_resp,frameLen);
    buf_resp[frameLen-2] = crc >> 8;
    buf_resp[frameLen-1] = crc;
}
void ModbusSlaver_ReadInputReg(Packet_Modbus_Slaver * packet)
{
    uint8_t * buf_resp=packet->buf_resp;
    unsigned int rr_Address = 0;
    unsigned int rr_numRegs = 0;
    unsigned char frameSize = 3;

    //Combine address bytes
    rr_Address = packet->address;

    //Combine number of regs bytes
    rr_numRegs = packet->data;

    for(unsigned int i=rr_Address;i<(rr_Address + rr_numRegs);i++)
    {
        if(i<SIZE_INPUT_REG)
        {
            //Need to split it up into 2 bytes
            buf_resp[frameSize] = inputReg[i] >> 8;
            frameSize++;
            buf_resp[frameSize] = inputReg[i];
            frameSize++;
        }
        else
        {
            buf_resp[frameSize] = 0;
            frameSize++;
            buf_resp[frameSize] = 0;
            frameSize++;
        }
    }
    frameSize+=2;
    packet->len_buf_resp=frameSize;
    buf_resp[0] = packet->id_slaver;
    buf_resp[1] = packet->funtion;
    buf_resp[2] = rr_numRegs*2; //2 bytes per reg
    uint16_t crc = ModbusSlaver_CalculateCRC(buf_resp,frameSize);
    buf_resp[frameSize-2] = crc >> 8;
    buf_resp[frameSize-1] = crc;

}
uint16_t ModbusSlaver_CalculateCRC(uint8_t * frame, uint16_t frame_size)
{
  uint16_t temp, temp2, flag;
  temp = 0xFFFF;
  for (uint16_t i = 0; i < frame_size-2; i++)
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
  // Reverse byte order.
  temp2 = temp >> 8;
  temp = (temp << 8) | temp2;
  temp &= 0xFFFF;
  // the returned value is already swapped
  // crcLo byte is first & crcHi byte is last
  return temp;
}
uint8_t ModbusSlaver_CheckCRC(uint8_t * frame,uint8_t frameLen)
{
    uint16_t received_crc = ((frame[frameLen - 2] << 8) | frame[frameLen - 1]);
	uint16_t calculated_crc = ModbusSlaver_CalculateCRC(frame,frameLen);
    if(received_crc==calculated_crc)
        return 1;
    else
        return 0;
}
