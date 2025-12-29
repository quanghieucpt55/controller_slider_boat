/*
 * dwt_delay.h
 *
 *  Created on: Feb 25, 2025
 *      Author: trank
 */

#ifndef INC_DWT_DELAY_H_
#define INC_DWT_DELAY_H_

#include "cytypes.h"

void CyDelay_Init(void);
void CyDelay(uint32_t ms);
void CyDelayUs(uint32_t us);

#endif /* INC_DWT_DELAY_H_ */
