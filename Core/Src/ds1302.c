/*
 * ds1302.c
 *
 *  Created on: Feb 25, 2025
 *      Author: trank
 */

#include "dwt_delay.h"
#include "clock.h"
#include "realtime.h"
#include "global.h"
#include "main.h"
#include "string.h"
#include "ds1302.h"

/* Size definitions */
#define DS1302_DATA_SIZE            8
#define DS1302_CMD_SIZE             8
#define DS1302_RAM_ADDR_START       0xC0

/* Register definition according to the spec */
#define DS1302_REG_SEC              0x80
#define DS1302_REG_MIN              0x82
#define DS1302_REG_HOUR             0x84
#define DS1302_REG_DATE             0x86
#define DS1302_REG_MONTH            0x88
#define DS1302_REG_DAY              0x8A
#define DS1302_REG_YEAR             0x8C
#define DS1302_REG_CONTROL          0x8E
#define D1302_RAM_BURST_MODE        0xFE
#define D1302_CAL_BURST_MODE        0xBE /* Burst mode for calendar */

/* Macros for handling BCD format data stored in the RTC device */
#define BCD_TO_DEC(val)             ((val / 16 * 10) + (val % 16))
#define DEC_TO_BCD(val)             ((val / 10 * 16) + (val % 10))

/* Masks to get fields in the calendar*/
#define MASK_CLOCK_SYSTEM           0x80
#define MASK_CLOCK_PERIOD           0x20
#define MASK_HOURS_24               0x3F
#define MASK_HOURS_12               0x1F
#define MASK_SECONDS                0x7F


/**
 * @brief write a high value on RST line
 *
 * @param void
 * @return void
 */
void DS1302_SetRst(void)
{
    HAL_GPIO_WritePin(RST_RTC_GPIO_Port, RST_RTC_Pin, GPIO_PIN_SET);
}

/**
 * @brief write a low value on RST line
 *
 * @param void
 * @return void
 */
void DS1302_ResetRst(void)
{
    HAL_GPIO_WritePin(RST_RTC_GPIO_Port, RST_RTC_Pin, GPIO_PIN_RESET);
}

/**
 * @brief write a high value on CLK line
 *
 * @param void
 * @return void
 */
void Ds1302_SetClk(void)
{
    HAL_GPIO_WritePin(SCK_RTC_GPIO_Port, SCK_RTC_Pin, GPIO_PIN_SET);
}

/**
 * @brief write a low value on CLK line
 *
 * @param void
 * @return void
 */
void DS1302_ResetClk(void)
{
    HAL_GPIO_WritePin(SCK_RTC_GPIO_Port, SCK_RTC_Pin, GPIO_PIN_RESET);
}

/**
 * @brief Clock cycle on CLK line
 * @param void
 * @return void
 */
void DS1302_SetClkCycle(void)
{
	Ds1302_SetClk();
    CyDelayUs(1);
    DS1302_ResetClk();
    CyDelayUs(1);
}

/**
 * @brief Write a high value on DTA line
 * @param void
 * @return void
 */
void DS1302_SetData(void)
{
    HAL_GPIO_WritePin(SDA_RTC_GPIO_Port, SDA_RTC_Pin, GPIO_PIN_SET);
}


/**
 * @brief Write a low value on DTA line
 * @param void
 * @return void
 */
void DS1302_ResetData(void)
{
    HAL_GPIO_WritePin(SDA_RTC_GPIO_Port, SDA_RTC_Pin, GPIO_PIN_RESET);
}

/**
 * @brief Set idle state SDA, CLK and RST low value.
 * @param void
 * @return void
 */
void DS1302_SetIdleState(void)
{
	DS1302_ResetData();
    DS1302_ResetClk();
    DS1302_ResetRst();
}

/**
 * @brief Initialize the DS1302 device
 *
 * @param void
 * @return void
 */
void DS1302_Init(void)
{
	DS1302_SetIdleState();
}


void DS1302_SetReadModel(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.Pin = SDA_RTC_Pin;
    GPIO_InitStructure.Pull = GPIO_PULLDOWN;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Mode =  GPIO_MODE_INPUT;
    HAL_GPIO_Init(SDA_RTC_GPIO_Port, &GPIO_InitStructure);
}

void DS1302_SetWriteMode(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.Pin = SDA_RTC_Pin;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(SDA_RTC_GPIO_Port, &GPIO_InitStructure);
}

void DS1302_WriteCmd(uint8_t cmd)
{
    uint8_t i;
    /* RST high to start communication and write cmd/address */
    DS1302_SetRst();
    for (i = 0; i < DS1302_CMD_SIZE; i++)
    {
        (cmd & 1) ? DS1302_SetData() : DS1302_ResetData();
        DS1302_SetClkCycle();
        cmd >>= 1;
    }
}

void DS1302_WriteData(uint8_t addr, uint8_t data)
{
    uint8_t i;

    DS1302_WriteCmd(addr);
    /* Write data bit by bit */
    for (i = 0; i < DS1302_DATA_SIZE; i++)
    {
        (data & 1) ? DS1302_SetData() : DS1302_ResetData();
        DS1302_SetClkCycle();
        data >>= 1;
    }
    DS1302_SetIdleState();
}

GPIO_PinState DS1302_ReadDataPin(void)
{
   return HAL_GPIO_ReadPin(SDA_RTC_GPIO_Port, SDA_RTC_Pin);
}

uint8_t DS1302_ReadData(uint8_t addr)
{
    uint8_t i;
    uint8_t data = 0;

    /* Make sure LSB bit is high for reading an address */
    addr |= 0x1;
    DS1302_WriteCmd(addr);

    DS1302_SetReadModel();
    for (i = 0; i < DS1302_DATA_SIZE; i++)
    {
        /* Ones come from the MSB to LSB
         * by shifting data to the right
        */
        if (DS1302_ReadDataPin() == GPIO_PIN_SET)
        {
            data |= 0x80;
        }
        DS1302_SetClkCycle();
        if (i != (DS1302_DATA_SIZE - 1))
        {
            data >>= 1;
        }
    }
    DS1302_SetWriteMode();

    DS1302_SetIdleState();

    return data;
}

/**
 * @brief Get time from calendar registers
 *
 * @param time pointer to time structure
 * @return void
 */
void DS1302_GetTime(Time_s *time)
{
    uint8_t tmpMaskClockSystem = 0;

    if (time == NULL)
    {
        return;
    }
    tmpMaskClockSystem = (time->clockSystem == DS1302_CLK_SYSTEM_24) ?
                         MASK_HOURS_24: MASK_HOURS_12;
    time->min = BCD_TO_DEC(DS1302_ReadData(DS1302_REG_MIN));
    time->day = BCD_TO_DEC(DS1302_ReadData(DS1302_REG_DAY));
    time->year = BCD_TO_DEC(DS1302_ReadData(DS1302_REG_YEAR));
    time->date = BCD_TO_DEC(DS1302_ReadData(DS1302_REG_DATE));
    time->month = BCD_TO_DEC(DS1302_ReadData(DS1302_REG_MONTH));
    time->sec = BCD_TO_DEC((DS1302_ReadData(DS1302_REG_SEC) & MASK_SECONDS));
    time->hour = BCD_TO_DEC((DS1302_ReadData(DS1302_REG_HOUR) & tmpMaskClockSystem));
}

void DS1302_SetTime(const Time_s *time)
{
    uint8_t tmpMaskClockSystem = 0;
    if (time == NULL)
    {
        return;
    }

    /* When 12 clock system is set, bit 7 should be high
     * according to the spec, low for 24 clock system.
    */
    if (time->clockSystem != DS1302_CLK_SYSTEM_24)
    {
        /* Set 12 hour clock system */
        tmpMaskClockSystem |=  MASK_CLOCK_SYSTEM;
        if (time->clockPeriod == DS1302_CLK_PM_PERIOD)
        {
            /* Set PM clock period */
            tmpMaskClockSystem |= MASK_CLOCK_PERIOD;
        }
    }
    /* Enable write by driving protected bit to 0*/
    DS1302_WriteData(DS1302_REG_CONTROL, 0);

    /* Write time data to RTC calendar registers in BCD format */
    DS1302_WriteData(DS1302_REG_MIN, DEC_TO_BCD(time->min));
    DS1302_WriteData(DS1302_REG_SEC, DEC_TO_BCD(time->sec));
    DS1302_WriteData(DS1302_REG_DAY, DEC_TO_BCD(time->day));
    DS1302_WriteData(DS1302_REG_YEAR, DEC_TO_BCD(time->year));
    DS1302_WriteData(DS1302_REG_DATE, DEC_TO_BCD(time->date));
    DS1302_WriteData(DS1302_REG_MONTH, DEC_TO_BCD(time->month));
    DS1302_WriteData(DS1302_REG_HOUR, DEC_TO_BCD(time->hour) | tmpMaskClockSystem);

    /* Disable write by driving protected bit to 1 */
    DS1302_WriteData(DS1302_REG_CONTROL, 0x80);
}

/**
 * @brief Write to RAM, valid addresses 0 - 30
 * @param addr address you want to write to
 * @param data data to write into the address
 * @return void
 */
void DS1302_WriteRam(const uint8_t addr, uint8_t data)
{
    if (addr > (DS1302_RAM_SIZE - 1))
    {
        return;
    }
    /* Enable write by driving protected bit to 0*/
    DS1302_WriteData(DS1302_REG_CONTROL, 0);
    /* Write addresses for RAM are multiple of 2 */
    DS1302_WriteData(DS1302_RAM_ADDR_START + (2 * addr), data);
    /* Disable write by driving protected bit to 1 */
    DS1302_WriteData(DS1302_REG_CONTROL, 0x80);
}

/**
 * @brief Read from RAM, valid addresses 0 - 30
 * @param addr address you want to read from
 * @return data read
 */
uint8_t DS1302_ReadRam(const uint8_t addr)
{
    if (addr > (DS1302_RAM_SIZE - 1))
    {
        return 0;
    }
    return DS1302_ReadData(DS1302_RAM_ADDR_START + (2 * addr));
}

/**
 * @brief Clear the entire RAM addresses (0 - 30)
 * @param void
 * @return void
 */
void DS1302_ClearRam(void)
{
    int i;
    for (i = 0; i < DS1302_RAM_SIZE; i++)
    {
        DS1302_WriteRam(i, 0);
    }
}

/**
 * @brief get clock system 12 or 24 format
 * @param void
 * @return ClockSystem
 */
ClockSystem DS1302_GetClockSystem(void)
{
    return (DS1302_ReadData(DS1302_REG_HOUR) & MASK_CLOCK_SYSTEM) ?
            DS1302_CLK_SYSTEM_12 : DS1302_CLK_SYSTEM_24;
}

/**
 * @brief get clock period AM or PM
 * @param void
 * @return ClockPeriod
 */
ClockPeriod DS1302_GetClockPeriod(void)
{
    return (DS1302_ReadData(DS1302_REG_HOUR) & MASK_CLOCK_PERIOD) ?
           DS1302_CLK_PM_PERIOD : DS1302_CLK_AM_PERIOD;
}

/**
 * @brief read in burst mode
 *
 * @param addr address for ram or calendar
 * @param len  number of bytes to read
 * @param buff buffer where data will be stored
 * @return void
 */
void DS1302_ReadBurst(uint8_t addr, uint8_t len, uint8_t *buff)
{
    uint8_t i, j;

    /* Make sure LSB bit is high for reading an address */
    addr |= 0x1;
    DS1302_WriteCmd(addr);

    DS1302_SetReadModel();
    for (i = 0; i < len; i++)
    {
        buff[i] = 0;
        for (j = 0; j < DS1302_DATA_SIZE; j++)
        {
            /* Ones come from the MSB to LSB
            * by shifting data to the right
            */
            if (DS1302_ReadDataPin() == GPIO_PIN_SET)
            {
                buff[i] |= 0x80;
            }
            DS1302_SetClkCycle();
            if (j != (DS1302_DATA_SIZE - 1))
            {
                buff[i] >>= 1;
            }
        }
    }
    DS1302_SetWriteMode();
    DS1302_SetIdleState();
}

/**
 * @brief write in burst mode
 *
 * @param addr address for ram or calendar
 * @param len number of addresses to write to
 * @param buff buffer that will write to RTC
 * @return void
 */
static void DS1302_WriteBurst(uint8_t addr, uint8_t len, uint8_t *buff)
{
    uint8_t i, j;

    DS1302_WriteCmd(addr);
    /* Write data bit by bit */
    for (i = 0; i < len; i++)
    {
        for (j = 0; j < DS1302_DATA_SIZE; j++)
        {
            (buff[i] & 1) ? DS1302_SetData() : DS1302_ResetData();
            DS1302_SetClkCycle();
            buff[i] >>= 1;
        }
    }
    DS1302_SetIdleState();
}

/**
 * @brief Write to RAM in burst mode
 * @param len Number of addresses to write to
 * @param buff Buffer that will write to RTC
 * @return void
 */
void DS1302_WriteRamBurst(uint8_t len, uint8_t *buff)
{
    if(len > DS1302_RAM_SIZE)
    {
        return;
    }
    /* Enable write by driving protected bit to 0*/
    DS1302_WriteData(DS1302_REG_CONTROL, 0);
    /* Write addresses for RAM are multiple of 2 */
    DS1302_WriteBurst(D1302_RAM_BURST_MODE, len, buff);
    /* Disable write by driving protected bit to 1 */
    DS1302_WriteData(DS1302_REG_CONTROL, 0x80);
}

/**
 * @brief Read RAM in burst mode
 * @param len Number of addresses to read from RTC
 * @param buff Buffer where data will be stored
 */
void DS1302_ReadRamBurst(uint8_t len, uint8_t *buff)
{
    if (len > DS1302_RAM_SIZE)
    {
        return;
    }
    memset(buff, 0, 1);
    DS1302_ReadBurst(D1302_RAM_BURST_MODE, len, buff);
}

