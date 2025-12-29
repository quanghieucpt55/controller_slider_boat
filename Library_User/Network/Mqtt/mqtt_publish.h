
#ifndef __MQTTCUS_PUBLISH_H
#define __MQTTCUS_PUBLISH_H

#include "mqtt_funtion.h"
#include "mqtt_client.h"

lwmqtt_err_t MQTT_decodePublish(uint8_t ** buf,uint32_t lenBuf,lwmqtt_message_t *msg);
lwmqtt_err_t MQTT_decodePuback(uint8_t ** buf,uint32_t buf_len,lwmqtt_message_t * msg);
lwmqtt_err_t lwmqtt_encode_publish(uint8_t *buf, uint32_t buf_len, uint32_t *len, uint8_t dup, uint16_t packet_id,
                                   lwmqtt_string_t topic, lwmqtt_message_t msg) ;
unsigned char CheckPubcomp(char * Answer,unsigned char len);

#endif
