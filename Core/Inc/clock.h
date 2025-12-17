/*
 * clock.h
 *
 *  Created on: Feb 24, 2025
 *      Author: trank
 */

#ifndef INC_CLOCK_H_
#define INC_CLOCK_H_

#include "cytypes.h"


void ClockTimer_Init(void);
void ClockTimer_Run(void);
void ClockTimerIsr_Run(void);
uint8_t IsTickChange(void);
uint8_t IsSecondChange(void);
int32_t millis(void);
void Timer_SetCallBackHandle(void (*func)(void));


#endif /* INC_CLOCK_H_ */
