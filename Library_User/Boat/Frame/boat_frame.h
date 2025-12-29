#ifndef _BOAT_FRAME_H_
#define _BOAT_FRAME_H_

#include "cytypes.h"

uint32_t Boat_Frame_Ping_Complete(uint8_t * buf,uint32_t size);  
uint32_t Boat_Frame_Event(uint8_t *buf, uint32_t size);
uint8_t FrameCheck(uint8_t * buf,uint32_t size);

#endif
