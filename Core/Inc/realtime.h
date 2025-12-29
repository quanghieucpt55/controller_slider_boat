#ifndef _REAL_TIME_H
#define _REAL_TIME_H

#include "cytypes.h"

typedef enum e_day_of_week_t e_day_of_week_t;
enum e_day_of_week_t
{
    // Summary:
    //     Indicates Sunday.
    Sunday = 0,
    //
    // Summary:
    //     Indicates Monday.
    Monday = 1,
    //
    // Summary:
    //     Indicates Tuesday.
    Tuesday = 2,
    //
    // Summary:
    //     Indicates Wednesday.
    Wednesday = 3,
    //
    // Summary:
    //     Indicates Thursday.
    Thursday = 4,
    //
    // Summary:
    //     Indicates Friday.
    Friday = 5,
    //
    // Summary:
    //     Indicates Saturday.
    Saturday = 6
};

typedef struct realtime_t realtime_t;
struct realtime_t
{
    unsigned char sec;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned char year;
}__attribute__((packed));

extern realtime_t  currentTime;

extern uint8_t currentTime_date;// thứ

void RealTime_Init(void);
void Realtime_Run(void);
void Realtime_Update(realtime_t * timeUpdate);
void Realtime_UpdateNum(uint8_t year,uint8_t month,uint8_t day,
                            uint8_t hour,uint8_t minute,uint8_t sec);
void Realtime_UpdateWithDayOfWeek(uint8_t year,uint8_t month,uint8_t day,
                            uint8_t hour,uint8_t minute,uint8_t sec,
                            uint8_t dayOfWeek);
uint8_t ValidTime(realtime_t * time);
uint32_t RealtimeConvertValueToInterger(realtime_t * timeUpdate);

#endif
