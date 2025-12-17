#include "global.h"
#include "remote_firmware.h"
#include "swconfig.h"
#include "extern_flash.h"
#include "system_update.h"

#define HEADER 0x60
#define POS_HEADER 0
#define POS_ADR 1
#define POS_CMD 3
#define POS_LEN_DATA 4
#define POS_DATA 6

/*
    cấu trúc frame remote đặt ở phía server
    header : 1 byte
    address: 1 byte
    cmd: 1 byte
    len: 1 byte
    data: n byte len
    cs: 1 byte
*/
uint8_t firm_flag_response_remote=0;
#define SIZE_BUF_RESPONSE_REMOTE 350
uint8_t firm_buf_response_remote[SIZE_BUF_RESPONSE_REMOTE];
uint32_t firm_len_bufResponseRemote=0;

uint8_t IsOnMsg_FirmResponseRemote(void)
{
    if(firm_flag_response_remote)
    {
        firm_flag_response_remote=0;
        return 1;
    }
    else
    {
        return 0;
    }
}


void FirmWrite_Response_Remote(TYPE_REMOTE_FIRM_CMD type ,uint8_t * ptr_data,uint32_t len_data )
{
    uint8_t * cusor=firm_buf_response_remote;
    uint8_t * cusor_end=cusor+SIZE_BUF_RESPONSE_REMOTE;
    WriteChar_Buffer(&cusor,HEADER,cusor_end);
    WriteInt16_Buffer(&cusor,ID_DEVICE,cusor_end);
    WriteChar_Buffer(&cusor,type,cusor_end);
    if(len_data>0)
    {
        WriteBuffer(&cusor,(uint8_t *)&len_data,2,cusor_end);
        WriteBuffer(&cusor,ptr_data,len_data,cusor_end);
    }
    else
    {
        WriteChar_Buffer(&cusor,len_data,cusor_end);
    }
    uint8_t cs=Frame_GeneralCheckSum(firm_buf_response_remote,(cusor-firm_buf_response_remote));
    WriteChar_Buffer(&cusor,cs,cusor_end);
    firm_len_bufResponseRemote=cusor-firm_buf_response_remote;
    firm_flag_response_remote=1;
}

void FrmRemote_SaveConfig(uint8_t cmd,void * ptr_destination,uint32_t sizedestination,void * ptr_source,uint32_t lenSource)
{
    if(lenSource>=sizedestination)// đảm bảo đủ dữ liệu
    {
        // copy dữ liệu vào ram rồi save;
        farcpy(ptr_destination,ptr_source,sizedestination);
        // báo ghi thành công
        uint8_t error=RM_FIRM_SUCCESS;
        FirmWrite_Response_Remote(cmd,&error,sizeof(error));
    }
    else
    {
        uint8_t error=ERR_FIRM_FRAME_WRITE_TOO_SHORT;
        FirmWrite_Response_Remote(cmd,&error,sizeof(error));
    }
}

ERR_FIRM_REMOTE_CMD RemoteFirm_ExcuteCommand(uint8_t * frame,uint32_t len_frame)
{
	ERR_FIRM_REMOTE_CMD errCode=RM_FIRM_SUCCESS;
    if(len_frame==0)
        errCode= ERR_RM_FIRM_NOT_VALID;
    else
    {
        uint8_t cs_calculator=Frame_GeneralCheckSum(frame,len_frame-1);
        uint8_t cs_read=frame[len_frame-1];

        if(frame[POS_HEADER]!=HEADER)
            errCode= ERR_RM_FIRM_NOT_VALID;
        else if(cs_calculator!=cs_read)
            errCode= ERR_RM_FIRM_NOT_VALID;
        else if(len_frame<5)
            errCode= ERR_RM_FIRM_NOT_VALID;
        else
        {
            uint8_t * ptr_data=&frame[POS_DATA];
            uint16_t len_data=0;
            farcpy(&len_data,frame+POS_LEN_DATA,2);
            uint8_t cmd=frame[POS_CMD];
            switch(cmd)
            {
                // lệnh điều khiển
                case RM_FIRM_EARSE_FLASH:
                {
                	if(len_data>=4)
                	{
                		uint32_t sectorAdr=0;
                		farcpy(&sectorAdr, ptr_data, len_data);
                		Flash_EraseSector(sectorAdr);
                		uint8_t error=RM_FIRM_SUCCESS;
						FirmWrite_Response_Remote(cmd,&error,sizeof(error));
                	}
                	else
                	{
                		uint8_t error=ERR_FIRM_FRAME_WRITE_TOO_SHORT;
                		FirmWrite_Response_Remote(cmd,&error,sizeof(error));
                	}
                }
				break;
                case RM_FIRM_WRITE_FLASH:
                {
                	if(len_data==256)// ghi chính xác 1 page
                	{
                		// 4 byte cuối cùng chứa :
                		// 2 byte địa chỉ page
                		// 2 byte mã kiểm tra crc
                		uint16_t pageAdr=ptr_data[252]+(uint16_t)ptr_data[253]*256;
                		uint16_t crcRead=ptr_data[254]+(uint16_t)ptr_data[255]*256;
                		uint16_t crcCal=Frame_CalculateCRC(ptr_data,254);
                		if(crcRead==crcCal)
                		{
                			uint8_t result=Flash_WritePageAndCheck(pageAdr, ptr_data, 256, 0);
							if(result==0)
							{
								uint8_t error=RM_FIRM_SUCCESS;
								FirmWrite_Response_Remote(cmd,&error,sizeof(error));
							}
							else
							{
								uint8_t error=ERR_RM_FIRM_NOT_VALID;
								FirmWrite_Response_Remote(cmd,&error,sizeof(error));
							}
                		}
                		else
                		{
                			uint8_t error=ERR_RM_FIRM_NOT_VALID;
							FirmWrite_Response_Remote(cmd,&error,sizeof(error));
                		}

                	}
                	else
                	{
                		uint8_t error=ERR_FIRM_FRAME_WRITE_TOO_SHORT;
						FirmWrite_Response_Remote(cmd,&error,sizeof(error));
                	}
                }
				break;
                case RM_FIRM_UPDATE_INFO:// lệnh cập nhật thông tin firmware
                {
                	if(len_data>=sizeof(flash_firmware_info_t))
                	{
                		flash_firmware_info_t info;
                		farcpy(&info,ptr_data, sizeof(flash_firmware_info_t));
                		uint8_t result=Flash_WritePageAndCheck(info.pageAdrInfo,(uint8_t *)&info, sizeof(info), 0);
                		if(result==0)
						{
							uint8_t error=RM_FIRM_SUCCESS;
							FirmWrite_Response_Remote(cmd,&error,sizeof(error));
						}
						else
						{
							uint8_t error=ERR_RM_FIRM_NOT_VALID;
							FirmWrite_Response_Remote(cmd,&error,sizeof(error));
						}
                	}
                	else
                	{
                		uint8_t error=ERR_FIRM_FRAME_WRITE_TOO_SHORT;
						FirmWrite_Response_Remote(cmd,&error,sizeof(error));
                	}
                }
				break;
                case RM_FIRM_RUN:
                {
                	if(len_data>=4)
                	{
                		uint32_t value=0;
                		farcpy(&value,ptr_data, sizeof(value));
                		if(value==0x1EC2)// lệnh đặc biệt để yêu cầu vào chế bộ bootloader
                		{
                			SystemUpdate_Require(UPDATE_FLASH_BOOTLOADER);
                			uint8_t error=RM_FIRM_SUCCESS;
							FirmWrite_Response_Remote(cmd,&error,sizeof(error));
                		}
                		else
                		{
                			uint8_t error=ERR_RM_FIRM_NOT_VALID;
                			FirmWrite_Response_Remote(cmd,&error,sizeof(error));
                		}
                	}
                	else
                	{
                		uint8_t error=ERR_FIRM_FRAME_WRITE_TOO_SHORT;
						FirmWrite_Response_Remote(cmd,&error,sizeof(error));
                	}
                }
				break;

            }
        }
    }

    return errCode;
}


