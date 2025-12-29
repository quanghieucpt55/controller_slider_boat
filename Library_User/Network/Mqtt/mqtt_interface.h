#ifndef _MQTT_INTERFACE_H
#define _MQTT_INTERFACE_H
#include "cytypes.h"
#define TYPE_ETHERNET 0
#define TYPE_GPRS 1
typedef struct NetworkMqtt NetworkMqtt;

struct NetworkMqtt
{
	int type;
	uint16_t (*mqttwrite) ( uint8_t*, uint32_t);
};

#endif
