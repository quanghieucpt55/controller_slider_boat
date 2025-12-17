//#include <modbus_cmd_gen.h>
#include "remote_boat.h"
//#include "generator_param.h"
#include "global.h"
#include "swconfig.h"


#define HEADER 0x60
#define POS_HEADER 0
#define POS_ADR 1
#define POS_CMD 3
#define POS_LEN_DATA 4
#define POS_DATA 5
/*
    cấu trúc frame remote đặt ở phía server
    header : 1 byte
    address: 1 byte
    cmd: 1 byte
    len: 1 byte
    data: n byte len
    cs: 1 byte
*/
uint8_t boat_flag_response_remote=0;
#define SIZE_BUF_RESPONSE_REMOTE 300
uint8_t boat_buf_response_remote[SIZE_BUF_RESPONSE_REMOTE];
uint32_t boat_len_bufResponseRemote=0;

uint8_t IsOnMsg_BoatResponseRemote(void)
{
    if(boat_flag_response_remote)
    {
        boat_flag_response_remote=0;
        return 1;
    }
    else
    {
        return 0;
    }
}

void WriteBoat_Response_Remote(TYPE_GEN_REMOTE_CMD type ,uint8_t * ptr_data,uint32_t len_data )
{
    uint8_t * cusor=boat_buf_response_remote;
    uint8_t * cusor_end=cusor+SIZE_BUF_RESPONSE_REMOTE;
    WriteChar_Buffer(&cusor,HEADER,cusor_end);
    WriteInt16_Buffer(&cusor,ID_DEVICE,cusor_end);
    WriteChar_Buffer(&cusor,type,cusor_end);
    if(len_data>0)
    {
        WriteChar_Buffer(&cusor,len_data,cusor_end);
        WriteBuffer(&cusor,ptr_data,len_data,cusor_end);
    }
    else
    {
        WriteChar_Buffer(&cusor,len_data,cusor_end);
    }
    uint8_t cs=Frame_GeneralCheckSum(boat_buf_response_remote,(cusor-boat_buf_response_remote));
    WriteChar_Buffer(&cusor,cs,cusor_end);
    boat_len_bufResponseRemote=cusor-boat_buf_response_remote;
    boat_flag_response_remote=1;
}

ERR_BOAT_REMOTE_CMD RemoteBoat_ExcuteCommand(uint8_t * frame,uint8_t len_frame)
{
	ERR_BOAT_REMOTE_CMD errCode=RM_BOAT_SUCCESS;
	if(len_frame==0)
		errCode= ERR_BOAT_RM_NOT_VALID;
	else
	{
		uint8_t cs_calculator=Frame_GeneralCheckSum(frame,len_frame-1);
		uint8_t cs_read=frame[len_frame-1];
		if(frame[POS_HEADER]!=HEADER)
			errCode= ERR_BOAT_RM_NOT_VALID;
		else if(cs_calculator!=cs_read)
			errCode= ERR_BOAT_RM_NOT_VALID;
		else if(len_frame<5)
			errCode= ERR_BOAT_RM_NOT_VALID;
		else
		{
			uint8_t * ptr_data=&frame[POS_DATA];
			uint8_t len_data=frame[POS_LEN_DATA];
			uint8_t cmd=frame[POS_CMD];
			switch(cmd)
			{
				// lệnh điều khiển

			}
		}
	}
	return errCode;

}









