#ifndef _BOAT_LOG_H_
#define _BOAT_LOG_H_

#include "cytypes.h"

#define TOTAL_EVENT_BOAT_LOG 50

typedef enum e_event_log_t e_event_log_t;
enum e_event_log_t
{
    Pow_Status_Driver = 1,         // Contactor đóng/mở

    Error_BMS,                     // Lỗi BMS
    Error_Driver,                  // Lỗi Driver

    ChDirection_Motor,             // chuyển hướng motor
    Sudden_Acceleration,           // tăng tốc độ đột ngột
    Motor_Status,                  // Kích hoạt phanh

    ChStatus_GPS,                  // Thay đổi trạng thái GPS

    ChState_VCU,                  // Thay đổi trạng thái VCU
};

/* Lưu dữ liệu của các event */
typedef struct {
    uint32_t pow_status;
    uint32_t err_bms;
    uint32_t err_driver;
    uint32_t motor_direc;
    uint32_t rpm_accel;
    uint32_t motor_status;
    uint32_t gps_status;
    uint32_t vcu_state;
} current_event_data_t;
extern current_event_data_t current_event_data;

typedef struct boat_event_log_t boat_event_log_t;
struct boat_event_log_t
{
    uint8_t date_time[6]; // thời gian xảy ra
    e_event_log_t event; // sự kiện
    uint32_t event_data; // dữ liệu sự kiện
}CY_PACKED_ATTR;


typedef struct boat_package_event_log_t boat_package_event_log_t;
struct boat_package_event_log_t
{
    uint32_t id;
    boat_event_log_t log;
}CY_PACKED_ATTR;

/* Cấu trúc gpsData với các trường: latitude (int32), longitude (int32), sog (uint16), cog (uint16), state (uint8) */
typedef struct {
    uint32_t latitude;
    uint32_t longitude;
    uint16_t sog;
    uint16_t cog;
    uint8_t state;
} gps_data_t;
extern gps_data_t gpsData;

cystatus BoatEventLog_ReadPacket(boat_package_event_log_t * buf,uint8_t pos);

void BoatEventLog_Init(void);
void BoatEventLog_Write(e_event_log_t event,uint32_t event_data);
void BoatEventLog_Read(uint8_t * buf,uint32_t buf_size,
                        uint8_t pack_start,uint8_t pack_end);

uint32_t BoatEventLog_GetCurrentPos(void);
uint8_t IsHaveMsgQueueBoatEventLog(void);
uint8_t IsHaveMsgInRomBoatEventLog(void);
uint8_t BoatEventLog_Dequeue(uint8_t * buf);
void BoatEventLog_LoadFromRomToQueue(void);

void BoatEventUpdate(void);

#endif
