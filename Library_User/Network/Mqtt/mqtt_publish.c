#include "cytypes.h"
#include "mqtt_publish.h"
#include "mqtt_client.h"
#include "string.h"


lwmqtt_err_t lwmqtt_encode_publish(uint8_t *buf, uint32_t buf_len, uint32_t *len, uint8_t dup, uint16_t packet_id,
                                   lwmqtt_string_t topic, lwmqtt_message_t msg)
{
  // prepare pointer
  uint8_t *buf_ptr = buf;
  uint8_t *buf_end = buf + buf_len;

  // calculate remaining length
  uint32_t rem_len = 2 + topic.len + (uint32_t)msg.payload_len;
  if (msg.qos > 0) {
    rem_len += 2;
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
  lwmqtt_write_bits(&header, LWMQTT_PUBLISH_PACKET, 4, 4);

  // set dup
  lwmqtt_write_bits(&header, (uint8_t)(dup), 3, 1);

  // set qos
  lwmqtt_write_bits(&header, msg.qos, 1, 2);

  // set retained
  lwmqtt_write_bits(&header, (uint8_t)(msg.retained), 0, 1);

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

  // write topic
  err = lwmqtt_write_string(&buf_ptr, buf_end, topic);
  if (err != LWMQTT_SUCCESS) {
    return err;
  }

  // write packet id if qos is at least 1
  if (msg.qos > 0) {
    err = lwmqtt_write_num(&buf_ptr, buf_end, packet_id);
    if (err != LWMQTT_SUCCESS) {
      return err;
    }
  }

  // write payload
  err = lwmqtt_write_data(&buf_ptr, buf_end, msg.payload, msg.payload_len);
  if (err != LWMQTT_SUCCESS) {
    return err;
  }

  // set length
  *len = buf_ptr - buf;

  return LWMQTT_SUCCESS;
}

lwmqtt_err_t MQTT_decodePublish(uint8_t ** buf,uint32_t lenBuf,lwmqtt_message_t *msg)
{
	// modifile MQTT library ,not use public source
	unsigned char header=0;
	unsigned char * buf_ptr=*buf;
    uint8_t * buf_end=buf_ptr+lenBuf;

   // read header
   lwmqtt_err_t err = lwmqtt_read_byte(&buf_ptr, buf_end, &header);
    if (err != LWMQTT_SUCCESS) {
        return err;
    }
    // check packet type
    if (lwmqtt_read_bits(header, 4, 4) != LWMQTT_PUBLISH_PACKET) {
        return LWMQTT_MISSING_OR_WRONG_PACKET;
     }
      // check packet type
    // get retained
    msg->retained = lwmqtt_read_bits(header, 0, 1) == 1;
    // get qos
    switch (lwmqtt_read_bits(header, 1, 2))
    {
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
          msg->qos = LWMQTT_QOS0;
          break;
    }

    // read remaining length
    uint32_t rem_len;
    err = lwmqtt_read_varnum(&buf_ptr, buf_end, &rem_len);
    if (err != LWMQTT_SUCCESS)
    {
        return err;
    }
    // check remaining length
    if (rem_len < 2)
    {
        return LWMQTT_REMAINING_LENGTH_MISMATCH;
    }
    // check buffer capacity
    if ((uint32_t)(buf_end - buf_ptr) < rem_len) {
        return LWMQTT_BUFFER_TOO_SHORT;
    }
    // reset buf end
    buf_end = buf_ptr + rem_len;
      // read topic
    err = lwmqtt_read_string(&buf_ptr, buf_end, &(msg->topicName));
    if (err != LWMQTT_SUCCESS )
    {
        return LWMQTT_FAILED_SUBSCRIPTION;
    }

    // read packet id if qos is at least 1
  if (msg->qos > 0) {
    err = lwmqtt_read_num(&buf_ptr, buf_end, &(msg->id));
    if (err != LWMQTT_SUCCESS) {
      return err;
    }
  } else
    {
        msg->id = 0;
    }

      // set payload length
      msg->payload_len = buf_end - buf_ptr;

    // read payload
    err = lwmqtt_read_data(&buf_ptr, buf_end, &msg->payload, buf_end - buf_ptr);
    if (err != LWMQTT_SUCCESS) {
    return err;
    }
    *buf+=buf_end-*buf; // tự động next địa chỉ gói tin tiếp theo
    return LWMQTT_SUCCESS;
}
lwmqtt_err_t MQTT_decodePuback(uint8_t ** buf,uint32_t buf_len,lwmqtt_message_t * msg)
{
      // prepare pointer
    uint8_t *buf_ptr = *buf;
    uint8_t *buf_end = *buf + buf_len;

    // read header
    uint8_t header = 0;
    lwmqtt_err_t err = lwmqtt_read_byte(&buf_ptr, buf_end, &header);
    if (err != LWMQTT_SUCCESS)
    {
        return err;
    }

    // check packet type
    if (lwmqtt_read_bits(header, 4, 4) != LWMQTT_PUBACK_PACKET)
    {
        return LWMQTT_MISSING_OR_WRONG_PACKET;
    }
    // read remaining length
    uint32_t rem_len;
    err = lwmqtt_read_varnum(&buf_ptr, *buf + buf_len, &rem_len);
    if (err != LWMQTT_SUCCESS)
    {
        return err;
    }

    // check remaining length
    if (rem_len != 2)
    {
        return LWMQTT_REMAINING_LENGTH_MISMATCH;
    }
    buf_end=buf_ptr+rem_len;
    // read packet id
    err = lwmqtt_read_num(&buf_ptr, buf_end, &(msg->id));
    if (err != LWMQTT_SUCCESS)
    {
        return err;
    }
    *buf+=buf_end-*buf; // tự động next địa chỉ gói tin tiếp theo
    return LWMQTT_SUCCESS;

}


