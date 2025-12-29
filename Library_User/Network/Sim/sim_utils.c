#include "cytypes.h"
#include "global.h"

#define TABLE_SIGNAL_QUANLITY_SIZE 6
typedef struct quanlity_sim_table_t quanlity_sim_table_t;
struct quanlity_sim_table_t
{
    uint8_t percent;
    uint8_t signal;
}CY_PACKED_ATTR;


const quanlity_sim_table_t table_quanlity_sim[]=
{
    {
        .percent=0,
        .signal=0
    },
    {
        .percent=20,
        .signal=10
    },
    {
        .percent=40,
        .signal=16
    },
    {
        .percent=60,
        .signal=22
    },
    {
        .percent=80,
        .signal=28
    },
    {
        .percent=100,
        .signal=30
    }

};



int32_t Sim_CalculatorSignal(uint8_t signal)
{
    for(uint8_t i=0;i<TABLE_SIGNAL_QUANLITY_SIZE-1;i++)
    {
        quanlity_sim_table_t first=table_quanlity_sim[i];
        quanlity_sim_table_t after=table_quanlity_sim[i+1];
        if(signal>=first.signal && signal<after.signal)
        {
            int32_t percent=(signal-first.signal)*100/(after.signal-first.signal);
            int32_t result=first.percent+(after.percent-first.percent)*percent/100;
            return result;
        }
    }
    // không nằm trong phạm vi
    if(signal<table_quanlity_sim[0].signal)
        return table_quanlity_sim[0].percent;
    else if(signal>=table_quanlity_sim[TABLE_SIGNAL_QUANLITY_SIZE-1].signal)
        return table_quanlity_sim[TABLE_SIGNAL_QUANLITY_SIZE-1].percent;
    return 0;
}
