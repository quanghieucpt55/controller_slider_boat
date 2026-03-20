#ifndef __SIM9_H_
#define __SIM9_H_
#include "cytypes.h"

typedef enum e_sim_status e_sim_status;
enum e_sim_status
{
    Sim_StartUp_Status=0,
    Sim_NoSim_Status,
    Sim_CheckSignal_Status_1,
	Sim_CheckSignal_Status_2,
    Sim_CheckNetwork_Status,
    Sim_Connecting_Status,
    Sim_Connected_Status,
    Sim_Disconnected_Status,
    Sim_Sleep_Status
};
extern e_sim_status simStatus; // trạng thái kết nối 4g của sim

// Trạng thái làm việc của sim
typedef enum e_sim_work e_sim_work;
enum e_sim_work
{
    COUNT_SIM_PWON=0,
    COUNT_SIM_GPS=1,
    COUNT_SIM_STARTUP=2,
    COUNT_SIM_SETUP=3,
    COUNT_SIM_CONNECT=4,
    COUNT_MQTT_ESTABLISH=5,
    COUNT_MQTT_COMUNICATION=6,
    COUNT_SIM_DISCONNECT=7,
    COUNT_SIM_SLEEP=10
};

typedef enum 
{
    SIM_MESSAGE_TYPE_PING=0,
    SIM_MESSAGE_TYPE_EVENT=1,
} e_sim_messageType_t;

void Sim_Work(void);
void Sim_Init(void);
uint8_t Sim_SignalQuanlityPercent(void);
void Sim_RequireSendImmediate(void);
uint8_t Sim_GetModeSecurity(void);
void Sim_UartIsrHandle(void);


extern uint8_t simStep,simCount;
#endif
