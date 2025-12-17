#ifndef _BOAT_FRAME_H_
#define _BOAT_FRAME_H_

#include "cytypes.h"

typedef enum {
	CMD1_DRIVESTATUS = 0x01,
	CMD2_PACKBASIC = 0x02,
	CMD3_PACKCAPACITY = 0x03,
	CMD4_CELLEXTREME = 0x04,
	CMD5_TEMP = 0x05,
	CMD6_RUNTIME = 0x06,
	CMD7_MOSSTATE = 0x07,
	CMD8_CELLLIST = 0x08,
	CMD9_CHARGEREQ = 0x09,
	CMD10_GPSSPEED = 0x0A,
	CMD11_WARNERR = 0x0B,
} e_boat_frame_cmd;
extern e_boat_frame_cmd boat_frame_cmd;

uint32_t Boat_Frame_Cmd1_DriveStatus(uint8_t * buf,uint32_t size);
uint32_t Boat_Frame_Cmd2_PackBasic(uint8_t * buf,uint32_t size);
uint32_t Boat_Frame_Cmd3_PackCapacity(uint8_t * buf,uint32_t size);
uint32_t Boat_Frame_Cmd4_CellExtreme(uint8_t * buf,uint32_t size);
uint32_t Boat_Frame_Cmd5_Temp(uint8_t * buf,uint32_t size);
uint32_t Boat_Frame_Cmd6_Runtime(uint8_t * buf,uint32_t size);
uint32_t Boat_Frame_Cmd7_MosState(uint8_t * buf,uint32_t size);
uint32_t Boat_Frame_Cmd8_CellList(uint8_t * buf,uint32_t size);
uint32_t Boat_Frame_Cmd9_ChargeReq(uint8_t * buf,uint32_t size);
uint32_t Boat_Frame_Cmd10_GpsSpeed(uint8_t * buf,uint32_t size);
uint32_t Boat_Frame_Cmd11_WarnErr(uint8_t * buf,uint32_t size);
uint32_t Boat_Frame_Build(uint8_t cmd, uint8_t *buf, uint32_t size);

/* Giữ hàm cũ để tương thích: */
uint32_t Boat_Frame_Ping_Complete(uint8_t * buf,uint32_t size);   /* map CMD1 */
uint32_t Boat_Frame_Mains_Complete(e_boat_frame_cmd cmd, uint8_t * buf,uint32_t size);  /* map CMD2 */

#endif
