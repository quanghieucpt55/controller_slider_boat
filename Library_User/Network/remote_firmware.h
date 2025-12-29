/*
 * remote_firmware.h
 *
 *  Created on: Apr 5, 2025
 *      Author: trank
 */

#ifndef REMOTE_FIRMWARE_H_
#define REMOTE_FIRMWARE_H_

#include "cytypes.h"

typedef enum TYPE_REMOTE_FIRM_CMD TYPE_REMOTE_FIRM_CMD;
enum TYPE_REMOTE_FIRM_CMD
{
    // tập lệnh cấp cao -> dành cho
    RM_FIRM_EARSE_FLASH=0,   // lệnh xóa flash để cập nhật firmware
	RM_FIRM_WRITE_FLASH,     // lệnh ghi firmware vào flash
	RM_FIRM_UPDATE_INFO,	 // lệnh cập nhật thông tin firmware vào flash
	RM_FIRM_RUN,			 // lệnh yêu cầu chạy cập nhật firmware

};

typedef enum ERR_FIRM_REMOTE_CMD ERR_FIRM_REMOTE_CMD;
enum ERR_FIRM_REMOTE_CMD
{
    RM_FIRM_SUCCESS=0,
    ERR_RM_FIRM_NOT_VALID, // lệnh không hợp lệ
    ERR_RM_FIRM_CMD_LORA, // lỗi đặt lệnh lora
    ERR_RM_FIRM_UPDATE_REALTIME, // lỗi cài đặt thời gian
    ERR_FIRM_FRAME_WRITE_TOO_SHORT,
    ERR_FIRM_NOT_CONFIG

};


extern uint8_t firm_buf_response_remote[];
extern uint32_t firm_len_bufResponseRemote;

uint8_t IsOnMsg_FirmResponseRemote(void);
void WriteFirm_Response_Remote(TYPE_REMOTE_FIRM_CMD type ,uint8_t * ptr_data,uint32_t len_data );
ERR_FIRM_REMOTE_CMD RemoteFirm_ExcuteCommand(uint8_t * frame,uint32_t len_frame);




#endif /* REMOTE_FIRMWARE_H_ */
