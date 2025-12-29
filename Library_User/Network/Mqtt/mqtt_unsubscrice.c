#include "mqtt_funtion.h"
#include "mqtt_unsubscrice.h"
#include "string.h"

lwmqtt_err_t lwmqtt_encode_unsubscribe(uint8_t *buf, uint32_t buf_len, uint32_t *len, uint16_t packet_id, int count,
                                       lwmqtt_string_t *topic_filters)
{
  // prepare pointer
  uint8_t *buf_ptr = buf;
  uint8_t *buf_end = buf + buf_len;

  // calculate remaining length
  uint32_t rem_len = 2;
  for (int i = 0; i < count; i++) {
    rem_len += 2 + topic_filters[i].len;
  }

  // check remaining length length
  int rem_len_len;
  lwmqtt_err_t err = lwmqtt_varnum_length(rem_len, &rem_len_len);
  if (err == LWMQTT_VARNUM_OVERFLOW) {
    return LWMQTT_REMAINING_LENGTH_OVERFLOW;
  }

  // prepare header
  uint8_t header = 0;

  // set packet type
  lwmqtt_write_bits(&header, LWMQTT_UNSUBSCRIBE_PACKET, 4, 4);

  // set qos
  lwmqtt_write_bits(&header, LWMQTT_QOS1, 1, 2);

  // write header
  err = lwmqtt_write_byte(&buf_ptr, buf_end, header);
  if (err != LWMQTT_SUCCESS) {
    return err;
  }

  // write remaining length
  err = lwmqtt_write_varnum(&buf_ptr, buf_end, rem_len);
  if (err != LWMQTT_SUCCESS) {
    return err;
  }

  // write packet id
  err = lwmqtt_write_num(&buf_ptr, buf_end, packet_id);
  if (err != LWMQTT_SUCCESS) {
    return err;
  }

  // write topics
  for (int i = 0; i < count; i++) {
    err = lwmqtt_write_string(&buf_ptr, buf_end, topic_filters[i]);
    if (err != LWMQTT_SUCCESS) {
      return err;
    }
  }

  // set length
  *len = buf_ptr - buf;

  return LWMQTT_SUCCESS;
}
