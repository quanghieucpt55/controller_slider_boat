/*
 * modbus_slave_base.h
 *
 *  Created on: Jan 13, 2025
 *      Author: trank
 */

#ifndef MODBUS_SLAVE_BASE_H_
#define MODBUS_SLAVE_BASE_H_

#include <stdint.h>

// trang thai truyen thong
#define SUCCESSFUL  0
#define VALIDCRC  1
#define INVALIDCRC    2
#define INVALIDFNC  3
#define INVALIDADR  4
#define INVALIDRXLEN    5
#define NOTDONE  6
#define ERROR_SLAVER_ADD 7


#define READ_COIL_STATUS 1 // Reads the ON/OFF status of discrete outputs (0X references, coils) in the slave.
#define READ_INPUT_STATUS 2 // Reads the ON/OFF status of discrete inputs (1X references) in the slave.
#define READ_HOLDING_REGISTERS 3 // Reads the binary contents of holding registers (4X references) in the slave.
#define READ_INPUT_REGISTERS 4 // Reads the binary contents of input registers (3X references) in the slave. Not writable.
#define FORCE_SINGLE_COIL 5 // Forces a single coil (0X reference) to either ON (0xFF00) or OFF (0x0000).
#define PRESET_SINGLE_REGISTER 6 // Presets a value into a single holding register (4X reference).
#define FORCE_MULTIPLE_COILS 15 // Forces each coil (0X reference) in a sequence of coils to either ON or OFF.
#define PRESET_MULTIPLE_REGISTERS 16 // Presets values into a sequence of holding registers (4X references).
#define READ_GENERAL_REFEREN 20
#define WRITE_GENERAL_REFEREN 21

#define HOLDING_REG_SIZE 200
#define SIZE_INPUT_REG 400
#define COIL_REG_SIZE 100

#define SLAVER_ADDRESS 1


typedef struct Packet_Modbus_Slaver  Packet_Modbus_Slaver;
struct Packet_Modbus_Slaver
{
    uint8_t id_slaver;
    uint8_t funtion;
    uint16_t address;
    uint16_t data;
    uint16_t len_buf_recv; // chiều dài dữ liệu nhận được
    uint16_t size_buf_recv;
    uint16_t len_buf_resp; // chiều dài dữ liệu trả lời
    uint16_t size_buf_resp;


    uint8_t * buf_recv; // địa chỉ buffer nhận dữ liệu
    uint8_t * buf_resp; // địa chỉ buffer gửi số liệu
    void ( * OnMessageRecvHandle)(Packet_Modbus_Slaver *);
    void (* systemInforRefresh) (Packet_Modbus_Slaver *);
};


extern uint16_t  holdingReg[];
extern uint16_t inputReg[];
extern unsigned char coils[COIL_REG_SIZE];
uint16_t ModbusSlaver_CalculateCRC(uint8_t * frame, uint16_t frame_size) ;
uint8_t ModbusSlaver_CheckCRC(uint8_t * frame,uint8_t frameLen);
uint8_t ModbusSlaver_GetFuntion(Packet_Modbus_Slaver * packet);
uint16_t ModbusSlaver_GetAddress(Packet_Modbus_Slaver * packet);
uint16_t ModbusSlaver_GetData(Packet_Modbus_Slaver * packet);
uint8_t ModbusSlaver_DecodeMsg(Packet_Modbus_Slaver * packet);
void Slaver_Do(Packet_Modbus_Slaver * packet);
void Slaver_PrepareRespond(Packet_Modbus_Slaver * packet);


#endif /* MODBUS_SLAVE_BASE_H_ */
