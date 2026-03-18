#ifndef _REMOTE_GENERATOR_H_
#define _REMOTE_GENERATOR_H_

#include "cytypes.h"

/* Danh sách lệnh REMOTE dành cho thuyền điện 
(chỉ cấu hình/giám sát, không điều khiển truyền động) */
typedef enum
{
    // SYSTEM
    RM_BOAT_READ_SYSTEM_STATUS, // Đọc trạng thái hệ thống VCU
    RM_BOAT_READ_VERSION,

    // TIME
    RM_BOAT_READ_TIME,
    RM_BOAT_UPDATE_REALTIME,   

    // NETWORK
    RM_BOAT_READ_NETWORK_CONFIG,
    RM_BOAT_WRITE_NETWORK_CONFIG,

    // GPS, SIM
    RM_BOAT_READ_GPS_STATUS, // Đọc trạng thái GPS
    RM_BOAT_READ_SIGNAL_SIM,

    // CONTROL
    RM_BOAT_CONTROL_MOTOR,
    RM_BOAT_CONTROL_FAN,
	RM_BOAT_CONTROL_AC

} TYPE_GEN_REMOTE_CMD;

typedef enum
{
    RM_BOAT_SUCCESS = 0,
    ERR_BOAT_RM_NOT_VALID,          // Lệnh không hợp lệ / checksum sai
    ERR_BOAT_RM_UPDATE_REALTIME,    // Lỗi cài đặt thời gian
    ERR_BOAT_RM_UPDATE_NETWORK_CONFIG, // Lỗi cài đặt cấu hình mạng
    ERR_BOAT_RM_FRAME_WRITE_TOO_SHORT,
} ERR_BOAT_REMOTE_CMD;

typedef enum 
{
    RP_BOAT_SUCCESS = 0x01,
    RP_BOAT_FAIL = 0x00
} RP_BOAT_RESPONSE_CMD;

extern uint8_t boat_buf_response_remote[];
extern uint32_t boat_len_bufResponseRemote;

uint8_t IsOnMsg_BoatResponseRemote(void);
void WriteBoat_Response_Remote(TYPE_GEN_REMOTE_CMD type ,uint8_t * ptr_data,uint32_t len_data );
ERR_BOAT_REMOTE_CMD RemoteBoat_ExcuteCommand(uint8_t * frame,uint8_t len_frame);

#endif
