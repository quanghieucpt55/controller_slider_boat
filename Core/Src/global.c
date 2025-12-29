/*
 * global.c
 *
 *  Created on: Jan 13, 2025
 *      Author: trank
 */

#include "cytypes.h"
#include "string.h"
#define PI 3.14159

void NumberFloatLimit(float min,float max,float * value)
{
    if(*value<min)
        *value=min;
    if(*value>max)
        *value=max;
}


int BCDToInt(int Hex)
{
	return (Hex>>4)*10+(Hex&0x0F);
}

int IntToBCD(int Int)
{
	return ((Int/10)<<4)|(Int%10);
}
void reboot(void)
{
	while (1);
}
unsigned char validBCD(unsigned char BCD)
{
	return (
		((BCD & 0x0F)<=9)&&
		((BCD & 0xF0)<=0x90)
		);
}
void farcpy(void * dest, void * source, unsigned int len)
{
	unsigned int i;
	for (i=0;i<len;i++)
		((char*)dest)[i]=((char*)source)[i];
}

void farset(void * dest, unsigned char value, unsigned int len)
{
	unsigned int i;
	for (i=0;i<len;i++)
		((unsigned char*)dest)[i]=value;
}

unsigned char cstrpos(const char answer[],char Answer[],unsigned char len)
{
    unsigned char i,j=0;
    for (i=0;(Answer[i]!=0)&&(i<len);i++)
    {
        if (Answer[i]==answer[j])               // Compare each character
        {
            j++;
            if (answer[j]==0)               // End of string
                return i+1;                   // found
        }
        else
            j=0;                            // RESET the string
    }
    return 0;
}

int32_t ComparePercent(int32_t value1,int32_t value2)
{
    int32_t result=0;
    if(value1>value2)
        result=value2*100/value1;
    else
        result=value1*100/value2;
    return result;
}

uint8_t BitsToBytes(uint8_t * bits,uint8_t lenBit)
{
    unsigned char sum = 0;
    for(uint8_t i=0; (i<lenBit) && (i<8); i++)
    {
        if(bits[i])
            sum |=(1<<i);
    }
    return sum;
}
void Swap(int32_t * xp,int32_t * yp)
{
     int32_t temp = *xp;
    *xp = *yp;
    *yp = temp;
}
// Function to perform Selection Sort
void SortAsc(int32_t arr[], int32_t n)
{
    int32_t i, j, min_idx;

    // One by one move boundary of unsorted subarray
    for (i = 0; i < n - 1; i++)
    {
        // Find the minimum element in unsorted array
        min_idx = i;
        for (j = i + 1; j < n; j++)
        {
            if (arr[j] < arr[min_idx])
                min_idx = j;
        }
        // Swap the found minimum element
        // with the first element
        Swap(&arr[min_idx], &arr[i]);
    }
}

float StringToFloat(uint8_t * str,uint8_t len)
{
    if(len>0)
    {
        int phanThuc=0;
        int phanAo=0;
        int heSoChia=1;
        int dotPos=0;
        float result=0;
        for(uint8_t i=0;i<len;i++)
        {
            if ((str[i]<='9')&&(str[i]>='0'))
            {
                phanThuc=phanThuc*10+str[i]-'0';
            }
            else if(str[i]=='.')
            {
                dotPos=i;
                break;
            }
        }
        if(dotPos>0)// có dấu '.'
        {
            for(uint8_t i=dotPos+1;i<len;i++)
            {
                if ((str[i]<='9')&&(str[i]>='0'))
                {
                    phanAo=phanAo*10+str[i]-'0';
                    heSoChia=heSoChia*10;
                }
            }
        }
        result=(float)phanThuc+((float)phanAo)/heSoChia;
        if(str[0]=='-')
            result=-result;
        return result;
    }
    else
    {
        return 0;
    }
}

float Radian_Convert(int32_t corner,int32_t cycle,int32_t trim_min,int32_t trim_max)
{
    if(corner>=cycle)
    {
        corner=corner-cycle;
    }
    float result=corner*360/cycle;
    // làm tròn số liệu
    if(result<=10)
        result=0;
    else if(result>=100 && result<=140)
        result=120;
    else if(result>=220 && result<=260)
        result=240;

    if(result<=trim_min ||  result>=trim_max)
        return 0;
    else
        return result*PI/180;
}

float Corn_Convert(float radian)
{
    return radian*180/PI;
}

int32_t LABS(int32_t value)
{
    return (value>=0)?value:(-value);
}
int32_t DeltaAbs(int32_t value1,int32_t value2)
{
    if(value1>=value2)
        return value1-value2;
    else
        return value2-value1;
}
int32_t Delta(int32_t value1,int32_t value2)
{
    return value1-value2;
}

uint16_t Frame_CalculateCRC(uint8_t * frame, uint16_t frame_size)
{
  uint16_t temp, temp2, flag;
  temp = 0xFFFF;
  for (uint16_t i = 0; i < frame_size; i++)
  {
    temp = temp ^ frame[i];
    for (uint8_t j = 1; j <= 8; j++)
    {
      flag = temp & 0x0001;
      temp >>= 1;
      if (flag)
        temp ^= 0xA001;
    }
  }
  // Reverse byte order.
  temp2 = temp >> 8;
  temp = (temp << 8) | temp2;
  temp &= 0xFFFF;
  // the returned value is already swapped
  // crcLo byte is first & crcHi byte is last
  return temp;
}

uint8_t Frame_GeneralCheckSum(uint8_t * frame,uint32_t len)
{
    uint8_t cs=0;
    for(uint32_t i=0;i<len;i++)
    {
        cs+=frame[i];
    }
    return 0xFF-cs;
}
uint8_t ArrayUin16ToByte(uint16_t * array,uint8_t len)
{
    uint8_t result=0;
    if(len>=8)
        len=8;
    for(uint8_t i=0;i<len;i++)
    {
        if(array[i])
        {
            result|=(1<<i);
        }
    }
    return result;
}
uint8_t BufferCompare(void * buf,void * buf_compare,uint32_t len)
{
    for (uint32_t i=0;i<len;i++)
    {
        if(((char*)buf)[i]!=((char*)buf_compare)[i])
        {
            return 1;
        }
    }
    return 0;
}

uint8_t Frame_CalculationChecksum(uint8_t * frame,uint32_t frame_size,
                                                            uint8_t checkSum)
{
    uint8_t cs=0;

    for(uint32_t i=0;i<frame_size;i++)
    {
        cs+=frame[i];
    }
    return (checkSum-cs);
}


int32_t ConvertTime_To_Sec(uint8_t hour,uint8_t minute,uint8_t second)
{
    int32_t total_Sec=(hour*60+minute)*60+second;
    return total_Sec;
}

uint32_t NumberMax(uint32_t a,uint32_t b)
{
    if(a>b)
        return a;
    else
        return b;
}

int32_t DeltaPercent(int32_t valueSet,int32_t valueCompare)
{
    int32_t delta=0,precent=0;
    delta=valueSet-valueCompare;
    precent=delta*100/valueSet;
    return precent;
}

cystatus WriteBuffer(uint8_t ** ptr,uint8_t * str,uint32_t len,uint8_t * ptr_end)
{
	if((ptr_end-(*ptr))<(int)len)
        return CYRET_EMPTY;
	farcpy(*ptr,str, len);
	*ptr += len;
    return CYRET_SUCCESS;
}

cystatus WriteChar_Buffer(uint8_t **ptr,uint8_t data,uint8_t * ptr_end)
{
    if((ptr_end-(*ptr))<1)
        return CYRET_EMPTY;
    farcpy(*ptr,&data, 1);
    (*ptr)+=1;
    return CYRET_SUCCESS;
}
cystatus WriteInt16_Buffer(uint8_t **ptr,uint16_t data,uint8_t * ptr_end)
{
    if((ptr_end-(*ptr))<2)
        return CYRET_EMPTY;
    *(*ptr)=LO8(data);
    (*ptr)++;
    *(*ptr)=HI8(data);
    (*ptr)++;
    return CYRET_SUCCESS;
}
cystatus WriteInt32_Buffer(uint8_t **ptr,uint32_t data,uint8_t * ptr_end)
{
    if((ptr_end-(*ptr))<4)
        return CYRET_EMPTY;
    farcpy(*ptr,(uint8_t *)&data,4);
    (*ptr)+=4;
    return CYRET_SUCCESS;
}

cystatus ReadChar_Buffer(uint8_t **ptr,uint8_t * data,uint8_t * ptr_end)
{
    if((ptr_end-(*ptr))<1)
        return CYRET_EMPTY;
    *data=*(*ptr);
    (*ptr)++;
    return CYRET_SUCCESS;
}
cystatus ReadInt16_Buffer(uint8_t **ptr,uint16_t * data,uint8_t * ptr_end)
{
    if((ptr_end-(*ptr))<2)
        return CYRET_EMPTY;
    farcpy(data,*ptr,2);
    (*ptr)+=2;
    return CYRET_SUCCESS;
}
cystatus ReadInt32_Buffer(uint8_t **ptr,uint32_t * data,uint8_t * ptr_end)
{
    if((ptr_end-(*ptr))<4)
        return CYRET_EMPTY;
    farcpy(data,*ptr,4);
    (*ptr)+=4;
    return CYRET_SUCCESS;
}
cystatus ReadArray_Buffer(uint8_t **ptr,uint8_t * data,uint32_t len,uint8_t * ptr_end)
{
    if((ptr_end-(*ptr))<(int)len)
        return CYRET_EMPTY;
    farcpy(data,*ptr,len);
    (*ptr)+=len;
    return CYRET_SUCCESS;
}



