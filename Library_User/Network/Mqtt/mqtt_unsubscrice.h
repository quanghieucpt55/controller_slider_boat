#ifndef __MQTT_UNSUBSCRICE_H
#define __MQTT_UNSUBSCRICE_H

#include "mqtt_funtion.h"

typedef struct MQTTcusUnSubscrice
{
    unsigned short messageID;
    MQTTcusString topicName;
}MQTTcusUnSubscrice;
lwmqtt_err_t lwmqtt_encode_unsubscribe(uint8_t *buf, uint32_t buf_len, uint32_t *len, uint16_t packet_id, int count,
                                       lwmqtt_string_t *topic_filters) ;
#endif

