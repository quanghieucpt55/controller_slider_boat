
#ifndef __MQTTCUS_FUNTION_H
#define __MQTTCUS_FUNTION_H

#include "stddef.h"
#include "cytypes.h"
typedef struct
{
	char * buf;
}MQTTcusString;


typedef struct
{
	unsigned char header;
}MQTTcusHeader;


typedef enum {
  LWMQTT_NO_PACKET = 0,
  LWMQTT_CONNECT_PACKET = 1,
  LWMQTT_CONNACK_PACKET,
  LWMQTT_PUBLISH_PACKET,
  LWMQTT_PUBACK_PACKET,
  LWMQTT_PUBREC_PACKET,
  LWMQTT_PUBREL_PACKET,
  LWMQTT_PUBCOMP_PACKET,
  LWMQTT_SUBSCRIBE_PACKET,
  LWMQTT_SUBACK_PACKET,
  LWMQTT_UNSUBSCRIBE_PACKET,
  LWMQTT_UNSUBACK_PACKET,
  LWMQTT_PINGREQ_PACKET,
  LWMQTT_PINGRESP_PACKET,
  LWMQTT_DISCONNECT_PACKET
} lwmqtt_packet_type_t;

typedef enum {
  LWMQTT_SUCCESS = 0,
  LWMQTT_BUFFER_TOO_SHORT = -1,
  LWMQTT_VARNUM_OVERFLOW = -2,
  LWMQTT_NETWORK_FAILED_CONNECT = -3,
  LWMQTT_NETWORK_TIMEOUT = -4,
  LWMQTT_NETWORK_FAILED_READ = -5,
  LWMQTT_NETWORK_FAILED_WRITE = -6,
  LWMQTT_REMAINING_LENGTH_OVERFLOW = -7,
  LWMQTT_REMAINING_LENGTH_MISMATCH = -8,
  LWMQTT_MISSING_OR_WRONG_PACKET = -9,
  LWMQTT_CONNECTION_DENIED = -10,
  LWMQTT_FAILED_SUBSCRIPTION = -11,
  LWMQTT_SUBACK_ARRAY_OVERFLOW = -12,
  LWMQTT_PONG_TIMEOUT = -13,
} lwmqtt_err_t;
typedef struct {
  uint16_t len;
  char *data;
} lwmqtt_string_t;
/**
 * The available QOS levels.
 */
typedef enum { LWMQTT_QOS0 = 0, LWMQTT_QOS1 = 1, LWMQTT_QOS2 = 2, LWMQTT_QOS_FAILURE = 128 } lwmqtt_qos_t;

typedef struct
{
  lwmqtt_qos_t qos;
  uint8_t type;
  uint8_t retained;
  uint8_t *payload;
  uint8_t returnCode;
  int32_t payload_len;
  uint16_t id;
  lwmqtt_string_t topicName;

} lwmqtt_message_t;

/**
 * The object defining the last will of a client.
 */
typedef struct {
  lwmqtt_string_t topic;
  lwmqtt_qos_t qos;
  uint8_t retained;
  lwmqtt_string_t payload;
} lwmqtt_will_t;

/**
 * The object containing the connection options for a client.
 */
typedef struct {
  lwmqtt_string_t client_id;
  uint16_t keep_alive;
  uint8_t clean_session;
  lwmqtt_string_t username;
  lwmqtt_string_t password;
} lwmqtt_options_t;

typedef struct lwmqtt_packet_subscribe
{
    uint16_t packet_id;
    uint32_t no_of_topic;
    lwmqtt_string_t * topic_filter;
    lwmqtt_qos_t * qos_level;
}lwmqtt_packet_subscribe;

uint8_t lwmqtt_read_bits(uint8_t byte, int pos, int num);
lwmqtt_err_t lwmqtt_read_byte(uint8_t ** buf,uint8_t * buf_end,uint8_t * byte);
lwmqtt_err_t lwmqtt_read_string(uint8_t **buf, uint8_t *buf_end, lwmqtt_string_t *str);
lwmqtt_err_t lwmqtt_read_data(uint8_t **buf,  uint8_t *buf_end, uint8_t **data, uint32_t len);
lwmqtt_err_t lwmqtt_read_num(uint8_t **buf,  uint8_t *buf_end, uint16_t *num);
lwmqtt_err_t lwmqtt_read_varnum(uint8_t **buf, uint8_t *buf_end, uint32_t *varnum);
void lwmqtt_write_bits(uint8_t *byte, uint8_t value, int pos, int num);
lwmqtt_err_t lwmqtt_write_byte(uint8_t **buf, uint8_t * buf_end, uint8_t byte) ;
lwmqtt_err_t lwmqtt_write_data(uint8_t **buf, uint8_t *buf_end, uint8_t *data, uint32_t len);
lwmqtt_err_t lwmqtt_write_num(uint8_t **buf, uint8_t *buf_end, uint16_t num);
lwmqtt_err_t lwmqtt_write_string(uint8_t **buf, uint8_t *buf_end, lwmqtt_string_t str);
lwmqtt_err_t lwmqtt_write_varnum(uint8_t **buf, uint8_t *buf_end, uint32_t varnum);

lwmqtt_string_t lwmqtt_string(char *str);
lwmqtt_err_t lwmqtt_varnum_length(uint32_t varnum, int *len);

#endif


