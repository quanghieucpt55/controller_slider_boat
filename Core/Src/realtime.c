/*
 * realtime.c
 *
 *  Created on: Feb 25, 2025
 *      Author: trank
 */

#include "realtime.h"
#include "clock.h"
#include "ds1302.h"

realtime_t currentTime;
uint8_t currentTime_date=0;// thứ
int32_t _lastTimeGetTime=0;

void RealTime_Init(void)
{
    DS1302_Init();
//    Realtime_UpdateNum(25, 9, 23, 11, 4,30);
}

uint8_t ValidTime(realtime_t * time)
{
	unsigned char result;
    // Khong dong thoi bang 0
	result =  time->year || time->month || time->day
            || time->hour || time->minute || time->sec;				

	result = result && (time->sec<60);		// giay <60
	result = result && (time->minute<60);		// phut <60
	result = result && (time->hour<24);		// gio 
	result = result && (time->day<=31);		// ngay
	result = result && (time->month>0);		// ngay
	result = result && (time->month<=12);		// thang
	result = result && (time->year!=0);		// thang
	return result;
}

uint32_t RealtimeConvertValueToInterger(realtime_t * timeUpdate)
{
    uint32_t totalSec=0;
    totalSec+=timeUpdate->sec;
    totalSec+=timeUpdate->minute*60;
    totalSec+=timeUpdate->hour*3600;
    return totalSec;
}



void Realtime_Run(void)
{

    if(millis()-_lastTimeGetTime>=300)
    {
    	_lastTimeGetTime=millis();
        // read from ds1307
        Time_s time;
        DS1302_GetTime(&time);

        // convert time bcd to in
        currentTime.year=time.year;
        currentTime.month=time.month;
        currentTime.day=time.date;
        currentTime.hour=time.hour;
        currentTime.minute=time.min;
        currentTime.sec=time.sec;
        currentTime_date=time.day;// thứ

    }
}

void Realtime_Update(realtime_t * timeUpdate)
{
	Time_s dstime;

    dstime.day=0;

    dstime.hour=timeUpdate->hour;
    dstime.min=timeUpdate->minute;
    dstime.sec=timeUpdate->sec;
    dstime.date=timeUpdate->day;
    dstime.month=timeUpdate->month;
    dstime.year=timeUpdate->year;

    dstime.clockPeriod=DS1302_CLK_AM_PERIOD;
    dstime.clockSystem=DS1302_CLK_SYSTEM_24;
    DS1302_SetTime(&dstime);
}
void Realtime_UpdateNum(uint8_t year,uint8_t month,uint8_t day,
                            uint8_t hour,uint8_t minute,uint8_t sec)
{
	Time_s dstime;

	dstime.day=0;

	dstime.hour=hour;
	dstime.min=minute;
	dstime.sec=sec;
	dstime.date=day;
	dstime.month=month;
	dstime.year=year;

	dstime.clockPeriod=DS1302_CLK_AM_PERIOD;
	dstime.clockSystem=DS1302_CLK_SYSTEM_24;
	DS1302_SetTime(&dstime);
}
void Realtime_UpdateWithDayOfWeek(uint8_t year,uint8_t month,uint8_t day,
                            uint8_t hour,uint8_t minute,uint8_t sec,uint8_t dayOfWeek)
{
	Time_s dstime;

	dstime.day=dayOfWeek;

	dstime.hour=hour;
	dstime.min=minute;
	dstime.sec=sec;
	dstime.date=day;
	dstime.month=month;
	dstime.year=year;

	dstime.clockPeriod=DS1302_CLK_AM_PERIOD;
	dstime.clockSystem=DS1302_CLK_SYSTEM_24;
	DS1302_SetTime(&dstime);
}



