/*
 * global.h
 *
 *  Created on: Feb 24, 2025
 *      Author: trank
 */

#ifndef INC_GLOBAL_H_
#define INC_GLOBAL_H_

#include "cytypes.h"



int BCDToInt(int Hex);
int IntToBCD(int Int);

void NumberFloatLimit(float min,float max,float * value);
unsigned char validBCD(unsigned char BCD);
char cstrpos(const char answer[],char Answer[],unsigned char len);
void farcpy(void * dest, void * source, unsigned int len);
void farset(void * dest, unsigned char value, unsigned int len);
void Swap(int32_t * xp,int32_t * yp);
void SortAsc(int32_t arr[], int32_t n);
uint8_t BitsToBytes(uint8_t * bits,uint8_t lenBit);
int32_t LABS(int32_t value);
int32_t DeltaAbs(int32_t value1,int32_t value2);
int32_t Delta(int32_t value1,int32_t value2);
float Radian_Convert(int32_t corner,int32_t cycle,int32_t trim_min,int32_t trim_max);
float Corn_Convert(float radian);
int32_t ConvertTime_To_Sec(uint8_t hour,uint8_t minute,uint8_t second);
uint8_t ArrayUin16ToByte(uint16_t * array,uint8_t len);
uint8_t BufferCompare(void * buf,void * buf_compare,uint32_t len);
uint8_t Frame_CalculationChecksum(uint8_t * frame,uint32_t frame_size,uint8_t checkSum);
uint16_t Frame_CalculateCRC(uint8_t * frame, uint16_t frame_size);
uint32_t NumberMax(uint32_t a,uint32_t b);
int32_t ComparePercent(int32_t value1,int32_t value2);
float StringToFloat(uint8_t * str,uint8_t len);
int32_t DeltaPercent(int32_t valueSet,int32_t valueCompare);

cystatus WriteBuffer(uint8_t ** ptr,uint8_t * str,uint32_t len,uint8_t * ptr_end);
cystatus WriteChar_Buffer(uint8_t **ptr,uint8_t data,uint8_t * ptr_end);
cystatus WriteInt16_Buffer(uint8_t **ptr,uint16_t data,uint8_t * ptr_end);
cystatus WriteInt32_Buffer(uint8_t **ptr,uint32_t data,uint8_t * ptr_end);

cystatus ReadChar_Buffer(uint8_t **ptr,uint8_t * data,uint8_t * ptr_end);
cystatus ReadInt16_Buffer(uint8_t **ptr,uint16_t * data,uint8_t * ptr_end);
cystatus ReadInt32_Buffer(uint8_t **ptr,uint32_t * data,uint8_t * ptr_end);
cystatus ReadArray_Buffer(uint8_t **ptr,uint8_t * data,uint32_t len,uint8_t * ptr_end);
uint8_t Frame_GeneralCheckSum(uint8_t * frame,uint32_t len);


#endif /* INC_GLOBAL_H_ */
