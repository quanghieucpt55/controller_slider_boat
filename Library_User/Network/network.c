#include "network.h"
#include "extern_rom.h"
#include "global.h"
#include "string.h"
#include "sim.h"

#define CYCLE_SEND_FAST_DEFAULT 2
#define CYCLE_SEND_SLOW_DEFAULT 10
#define CYCLE_SEND_MAINS_DEFAULT 30 // 30s gửi dữ liệu 1 lần

network_config_t netConfig;
network_config_t netConfigHistory;

#define COUNT_GPRS_READY 6
#define COUNT_ETHERNET_READY 6


extern uint8_t simCount,simStep;
extern uint8_t ethernetCount;
uint8_t interface_current;
uint8_t isNetworkRequireSendImmediate;

void Network_Init(void)
{
    if(!ExRom_ReadParam_WithCRC16(ADR_ROM_NETWORK_CONFIG,
        (uint8_t *)&netConfig,sizeof(network_config_t)))
    {
        netConfig.cycle_send_fast=CYCLE_SEND_FAST_DEFAULT;
        netConfig.cycle_send_slow=CYCLE_SEND_SLOW_DEFAULT;
    }
    farcpy(&netConfigHistory,&netConfig,sizeof(netConfigHistory));
}

uint8_t NetworkConfig_Save(void)
{
    if(BufferCompare(&netConfigHistory,&netConfig,sizeof(netConfig)))
    {
        ExRom_SaveParam_WithCRC16(ADR_ROM_NETWORK_CONFIG,(uint8_t *)&netConfig,sizeof(netConfig));
        farcpy(&netConfigHistory,&netConfig,sizeof(netConfigHistory));
        return 1;
    }
    return 0;
}


int8_t Network_CheckInterfaceStatus(uint8_t interface)
{
    if(interface>interface_current)
        return INTERFACE_NEED_RUN;
    else if(interface<interface_current)
        return INTERFACE_NEED_SLEEP;
    else
        return INTERFACE_RUNNING;
}

void Network_TaskSendImmediate(void)
{
    if(isNetworkRequireSendImmediate)
    {
        isNetworkRequireSendImmediate=0;
        Sim_RequireSendImmediate();
    }
}

void Network_Supervision(void)
{
    Network_TaskSendImmediate();
    if(simCount==COUNT_GPRS_READY)
    {
        interface_current=INTERFACE_GPRS;
    }
    else
        interface_current=INTERFACE_NULL;
}
uint8_t Network_GetCurrentInterface(void)
{
    return interface_current;
}

void Network_RequireSendImmediate(void)
{
    isNetworkRequireSendImmediate=1;
}

uint8_t IsNetwork_Ready(void)
{
    if(interface_current!=INTERFACE_NULL)
        return 1;
    else
        return 0;
}


