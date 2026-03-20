#include "stm32f4xx_hal.h"
#include "cytypes.h"
#include "main.h"

unsigned int giay=0,ticktac=0;
unsigned char timeChange=0;
unsigned char secondChange=0;

void (*TimeCallBackHandle) (void )=NULL;

volatile int32_t milisec=0;
uint8_t old_tick=0,new_tick=0;
uint8_t old_sec=0,new_sec=0;

int32_t millis(void)
{
    return milisec;
}
void MiliSec_Run(void)
{
    milisec++;
}


void ClockTimer_Init(void)
{
	HAL_TIM_Base_Start_IT(htimer);
}

void ClockTimer(void)
{
	static unsigned int cnt=0;
	if(++cnt>=100)//
	{
		cnt=0;
		if(++ticktac>=10)
		{
			ticktac=0;
			if(++giay>=60)
				giay=0;
		}

	}
}

void Timer_SetCallBackHandle(void (*func)(void))
{
    TimeCallBackHandle=func;
}

uint8_t IsTickChange(void)
{
    return timeChange;
}

uint8_t IsSecondChange(void)
{
    return secondChange;
}
void ClockTimer_Run(void)
{

	old_tick=new_tick;
	new_tick=ticktac;
	if(old_tick!=new_tick)
	{
		timeChange=1;
	}
	else
		timeChange=0;
    // clock 1s
    old_sec=new_sec;
    new_sec=giay;
    if(old_sec!=new_sec)
    {
        secondChange=1;
    }
    else
        secondChange=0;

}


void ClockTimerIsr_Run(void)
{

    if(NULL!=TimeCallBackHandle)
    {
        TimeCallBackHandle();
    }
	ModbusComp_checkIfEndOfMessage();
    ClockTimer();
    MiliSec_Run();
}
