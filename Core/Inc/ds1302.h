#ifndef _DS1302_H_
#define _DS1302_H_
#include "realtime.h"

#define DS1302_RAM_SIZE             31

/**
 * Enumeration for days of the week
 */
typedef enum
{
    MONDAY =  1, /* Valid values 1 - 7*/
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    SUNDAY,
}Day;

/**
 * Enumeration for clock system (12/24)
 */
typedef enum
{
    DS1302_CLK_SYSTEM_24,
    DS1302_CLK_SYSTEM_12,
}ClockSystem;

/**
 * Enumeration for clock period (AM or PM)
 */
typedef enum
{
    DS1302_CLK_AM_PERIOD,
    DS1302_CLK_PM_PERIOD,
}ClockPeriod;

/**
 * Structure for holding time data
 */
typedef struct
{
    Day day; /* Range: enum day values*/
    uint8_t sec; /* Range: 0-59 */
    uint8_t min; /* Range: 0-59 */
    uint8_t year; /* Range: 0 - 99 */
    uint8_t hour; /* Range: 1-12/0-23*/
    uint8_t date;  /* Range: 1 - 31 */
    uint8_t month; /* Range: 1 - 12 */
    ClockSystem clockSystem; /* 12 or 24 clock system */
    ClockPeriod clockPeriod; /* AM or PM*/
}Time_s;

void DS1302_Init(void);

void DS1302_GetTime(Time_s *time);
void DS1302_SetTime(const Time_s *time);

void DS1302_ClearRam(void);
uint8_t DS1302_ReadRam(const uint8_t addr);
void DS1302_WriteRam(const uint8_t addr, uint8_t data);

ClockSystem DS1302_GetClockSystem(void);
ClockPeriod DS1302_GetClockPeriod(void);

void DS1302_WriteRamBurst(uint8_t len, uint8_t *buff);
void DS1302_ReadRamBurst(uint8_t len, uint8_t *buff);

#endif

