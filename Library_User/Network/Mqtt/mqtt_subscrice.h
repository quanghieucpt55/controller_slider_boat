#ifndef __MQTTCUS_SUBSCRICE_H
#define __MQTTCUS_SUBSCRICE_H

#include "mqtt_funtion.h"

lwmqtt_err_t lwmqtt_encode_subscribe(uint8_t *buf, uint32_t buf_len, uint32_t *len, uint16_t packet_id, int count,
                                     lwmqtt_string_t *topic_filters, lwmqtt_qos_t *qos_levels);
lwmqtt_err_t lwmqtt_decode_suback(uint8_t **buf, uint32_t buf_len, lwmqtt_message_t * msg);
#endif

