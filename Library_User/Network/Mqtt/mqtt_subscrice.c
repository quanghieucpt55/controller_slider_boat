#include "mqtt_subscrice.h"
#include "string.h"

lwmqtt_err_t lwmqtt_encode_subscribe(uint8_t *buf, uint32_t buf_len, uint32_t *len, uint16_t packet_id, int count,
                                     lwmqtt_string_t *topic_filters, lwmqtt_qos_t *qos_levels)
{
  // prepare pointer
  uint8_t *buf_ptr = buf;
  uint8_t *buf_end = buf + buf_len;

  // calculate remaining length
  uint32_t rem_len = 2;
  for (int i = 0; i < count; i++) {
    rem_len += 2 + topic_filters[i].len + 1;
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
  lwmqtt_write_bits(&header, LWMQTT_SUBSCRIBE_PACKET, 4, 4);

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
    if (err != LWMQTT_SUCCESS)
    {
        return err;
    }

    // write all subscriptions
    for (int i = 0; i < count; i++) {
    // write topic
    err = lwmqtt_write_string(&buf_ptr, buf_end, topic_filters[i]);
    if (err != LWMQTT_SUCCESS)
    {
      return err;
    }

    // write qos level
    err = lwmqtt_write_byte(&buf_ptr, buf_end, (uint8_t)qos_levels[i]);
    if (err != LWMQTT_SUCCESS) {
      return err;
    }
  }

  // set length
  *len = buf_ptr - buf;

  return LWMQTT_SUCCESS;
}

lwmqtt_err_t lwmqtt_decode_suback(uint8_t **buf, uint32_t buf_len, lwmqtt_message_t * msg)
{
    // prepare pointer
    uint8_t *buf_ptr = *buf;
    uint8_t *buf_end = *buf + buf_len;

    // read header
    uint8_t header;
    lwmqtt_err_t err = lwmqtt_read_byte(&buf_ptr, buf_end, &header);
    if (err != LWMQTT_SUCCESS)
    {
        return err;
    }

    // check packet type
    if (lwmqtt_read_bits(header, 4, 4) != LWMQTT_SUBACK_PACKET)
    {
        return LWMQTT_MISSING_OR_WRONG_PACKET;
    }

    // read remaining length
    uint32_t rem_len;
    err = lwmqtt_read_varnum(&buf_ptr, buf_end, &rem_len);
    if (err != LWMQTT_SUCCESS)
    {
        return err;
    }

    // check remaining length (packet id + min. one suback code)
    if (rem_len < 3)
    {
        return LWMQTT_REMAINING_LENGTH_MISMATCH;
    }
    buf_end=buf_ptr+rem_len;
    // read packet id
    err = lwmqtt_read_num(&buf_ptr, buf_end,&(msg->id));
    if (err != LWMQTT_SUCCESS)
    {
        return err;
    }
    int max_count=1;
    // read all suback codes
    for (int count = 0; count < (int)rem_len - 2; (count)++)
    {
        // check max count
        if (count > max_count)
        {
            return LWMQTT_SUBACK_ARRAY_OVERFLOW;
        }

        // read qos level
        uint8_t raw_qos_level;
        err = lwmqtt_read_byte(&buf_ptr, buf_end, &raw_qos_level);
        if (err != LWMQTT_SUCCESS)
        {
            return err;
        }

        // set qos level
        switch (raw_qos_level) {
        case 0:
            msg->qos = LWMQTT_QOS0;
            break;
        case 1:
            msg->qos = LWMQTT_QOS1;
            break;
        case 2:
            msg->qos = LWMQTT_QOS2;
            break;
        default:
            msg->qos = LWMQTT_QOS_FAILURE;
            break;
        }
    }
    *buf+=buf_end-*buf; // tự động next địa chỉ gói tin tiếp theo
  return LWMQTT_SUCCESS;
}
