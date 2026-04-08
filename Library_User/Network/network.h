#ifndef _NETWORK_H_
#define _NETWORK_H_


#define SERVER_24

#include "cytypes.h"
#define INTERFACE_NEED_RUN 1
#define INTERFACE_RUNNING 0
#define INTERFACE_NEED_SLEEP -1

#define INTERFACE_ETHERNET 3// đang chạy kết nối ethernet
#define INTERFACE_WIFI 2 // đang chạy kết nối wifi
#define INTERFACE_GPRS 1 // đang chạy kết nối GPS
#define INTERFACE_NULL 0 // đang không kết nối

#define SERVER_NAME "boat.dgt-iot.com.vn"

#ifdef SERVER_24
    #define SERVER_PORT 24127
    #define SERVER_PORT_SERCURITY 24128
#endif

typedef struct network_config_t network_config_t;
struct network_config_t
{
    int32_t cycle_send_fast;// chu kì gửi dữ liệu nhanh
    int32_t cycle_send_slow;// chu kì gửi dữ liệu chậm
}CY_PACKED_ATTR;

extern network_config_t netConfig;
extern uint8_t interface_current;
void Network_Init(void);
uint8_t NetworkConfig_Save(void);
void Network_Supervision(void);
int8_t Network_CheckInterfaceStatus(uint8_t interface);
uint8_t Network_GetCurrentInterface(void);
void Network_RequireSendImmediate(void);
void Network_TaskSendImmediate(void);
uint8_t IsNetwork_Ready(void);
cystatus Network_WriteConfig(uint8_t * data,uint8_t len_data);
#endif
