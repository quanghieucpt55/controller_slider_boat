/*
 * modbus_slave_comp.c
 *
 *  Created on: Jan 13, 2025
 *      Author: trank
 */

#include <stdint.h>
#include "modbus_slave_comp.h"
#include "modbus_msg_handle.h"
#include "modbus_slave_base.h"
#include "modbus_rtu_interface.h"

#define BUFFER_SIZE 255

volatile uint8_t newMsg_comp = 1;
volatile uint8_t middleOfMsgComp = 0;
volatile uint8_t pos_buf_modbus_comp = 0;
volatile uint8_t msgLengthNotChanged_Comp = 0;
volatile uint8_t modbusCompHavedata = 0;
volatile uint8_t _oldMessageLength = 0;

unsigned char buf_recv_m_comp[BUFFER_SIZE];
unsigned char buf_send_m_comp[BUFFER_SIZE];

Packet_Modbus_Slaver packet_comp; // gói dữ liệu comport

// Trạng thái Modbus RS485
static volatile e_modbus_status modbusStatus = Modbus_Init_Status;

e_modbus_status ModbusSlaveComp_GetStatus(void)
{
    return modbusStatus;
}


void PacketComp_Init(void)
{
    packet_comp.id_slaver=SLAVER_ADDRESS;
    packet_comp.buf_recv=buf_recv_m_comp;
    packet_comp.buf_resp=buf_send_m_comp;
    packet_comp.size_buf_recv=BUFFER_SIZE;
    packet_comp.size_buf_resp=BUFFER_SIZE;
    packet_comp.OnMessageRecvHandle=SlaverSerial_MessageHandle;
    packet_comp.systemInforRefresh=SlaverSerial_Update;
}

void ModbusSlaveComp_Init(void)
{
	PacketComp_Init();
	modbusStatus = Modbus_Ready_Status;
}

void ModbusSlaveComp_DecodeMessage(uint8_t byte)
{
	if(middleOfMsgComp)
	{
		buf_recv_m_comp[pos_buf_modbus_comp] = byte;
		if(pos_buf_modbus_comp<BUFFER_SIZE-1)
			pos_buf_modbus_comp++;
		msgLengthNotChanged_Comp = 0;
		packet_comp.len_buf_recv = pos_buf_modbus_comp;
	}
	if(newMsg_comp)
	{
		buf_recv_m_comp[pos_buf_modbus_comp] = byte;
		if(pos_buf_modbus_comp<BUFFER_SIZE-1)
			pos_buf_modbus_comp++;
		packet_comp.len_buf_recv = pos_buf_modbus_comp;
		middleOfMsgComp = 1;
		newMsg_comp = 0;
		msgLengthNotChanged_Comp = 0;
		modbusStatus = Modbus_Receiving_Status;
	}
}

void ModbusComp_checkIfEndOfMessage(void)
{
    if(middleOfMsgComp)
    {
        if(_oldMessageLength == packet_comp.len_buf_recv)
        {
            msgLengthNotChanged_Comp++;
        }
        if(msgLengthNotChanged_Comp >= 5)
        {
            //We are at the end of the message
            newMsg_comp = 1;
            modbusCompHavedata = 1; //for main loop
            middleOfMsgComp= 0;
            msgLengthNotChanged_Comp = 0;
            pos_buf_modbus_comp=0;
            modbusStatus = Modbus_Processing_Status;
        }
        _oldMessageLength = packet_comp.len_buf_recv;
    }
}

void ModbusSlaverComp_SendData(uint8_t * frame,uint32_t len)
{
	ModbusRTU_UartPutArray(frame, len);
}

void ModbusSlaveComp_Run(void)
{
    if(modbusCompHavedata)// có dữ liệu gửi đến
    {
        if(ModbusSlaver_DecodeMsg(&packet_comp)==SUCCESSFUL)
        {
            modbusStatus = Modbus_Processing_Status;
            Slaver_Do(&packet_comp);// thực thi lệnh
            Slaver_PrepareRespond(&packet_comp);// chuẩn bị dữ liệu phản hồi
            modbusStatus = Modbus_Responding_Status;
            ModbusSlaverComp_SendData(packet_comp.buf_resp,packet_comp.len_buf_resp);
            modbusStatus = Modbus_Ready_Status;
        }
        else
        {
            modbusStatus = Modbus_Error_Status;
        }
        modbusCompHavedata=0;//clear flag
    }
    else
    {
        // Khi không có dữ liệu, chuyển về Idle nếu đang ở Ready
        if(modbusStatus == Modbus_Ready_Status)
        {
            modbusStatus = Modbus_Idle_Status;
        }
        else if(modbusStatus == Modbus_Error_Status)
        {
            // Tự động chuyển về Ready sau lỗi
            modbusStatus = Modbus_Ready_Status;
        }
    }
}
