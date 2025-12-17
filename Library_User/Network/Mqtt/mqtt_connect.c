#include "cytypes.h"
#include "mqtt_connect.h"
#include "string.h"
#include "mqtt_funtion.h"

lwmqtt_err_t lwmqtt_encode_connect(uint8_t *buf, uint32_t buf_len, uint32_t *len, lwmqtt_options_t * options,
                                   lwmqtt_will_t *will)
{
    // prepare pointers
    uint8_t *buf_ptr = buf;
    uint8_t *buf_end = buf + buf_len;

    // fixed header is 10
    uint32_t rem_len = 10;//10;
    if(options->username.len>0)// sử dụng username password
        rem_len=12;

    // add client id to remaining length
    rem_len += options->client_id.len + 2; // 18

    // add will if present to remaining length
    if (will != NULL) {
        rem_len += will->topic.len + 2 + will->payload.len + 2;
    } // 18

    // add username if present to remaining length
    if (options->username.len > 0)
    {
        rem_len += options->username.len + 2; // 27

        // add password if present to remaining length
        if (options->password.len > 0)
        {
            rem_len += options->password.len + 2; //37
        }
    }

    // check remaining length length
    int rem_len_len;
    lwmqtt_err_t err = lwmqtt_varnum_length(rem_len, &rem_len_len);
    if (err == LWMQTT_VARNUM_OVERFLOW)
    {
        return LWMQTT_REMAINING_LENGTH_OVERFLOW;
    }

    // prepare header
    uint8_t header = 0;
    lwmqtt_write_bits(&header, LWMQTT_CONNECT_PACKET, 4, 4);

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

    // write version string
    if(options->username.len==0)
    {
        err = lwmqtt_write_string(&buf_ptr, buf_end, lwmqtt_string("MQTT"));
        if (err != LWMQTT_SUCCESS) {
            return err;
        }
        // version number
        err = lwmqtt_write_byte(&buf_ptr, buf_end, 4);// version
        if (err != LWMQTT_SUCCESS) {
            return err;
        }
    }
    else
    {
        err = lwmqtt_write_string(&buf_ptr, buf_end, lwmqtt_string("MQIsdp"));
        if (err != LWMQTT_SUCCESS) {
            return err;
        }
        err = lwmqtt_write_byte(&buf_ptr, buf_end, 3);// version number
        if (err != LWMQTT_SUCCESS) {
            return err;
        }
    }

    // prepare flags
    uint8_t flags = 0;

    // set clean session
    lwmqtt_write_bits(&flags, (uint8_t)(options->clean_session), 1, 1);

    // set will flags if present
    if (will != NULL)
    {
        lwmqtt_write_bits(&flags, 1, 2, 1);
        lwmqtt_write_bits(&flags, will->qos, 3, 2);
        lwmqtt_write_bits(&flags, (uint8_t)(will->retained), 5, 1);
    }

    // set username flag if present
    if (options->username.len > 0)
    {
        lwmqtt_write_bits(&flags, 1, 7, 1);
        // set password flag if present
        if (options->password.len > 0) {
          lwmqtt_write_bits(&flags, 1, 6, 1);
        }
    }

    // write flags
    err = lwmqtt_write_byte(&buf_ptr, buf_end, flags);
    if (err != LWMQTT_SUCCESS) {
        return err;
    }

    // write keep alive
    err = lwmqtt_write_num(&buf_ptr, buf_end, options->keep_alive);
    if (err != LWMQTT_SUCCESS) {
        return err;
    }

    err = lwmqtt_write_num(&buf_ptr, buf_end, options->client_id.len);
    if (err != LWMQTT_SUCCESS)
    {
        return err;
    }

    // write client id
    err = lwmqtt_write_data(&buf_ptr, buf_end,(uint8_t *) options->client_id.data,options->client_id.len);
    if (err != LWMQTT_SUCCESS) {
        return err;
    }

    // write will if present
    if (will != NULL)
    {
        // write topic
        err = lwmqtt_write_string(&buf_ptr, buf_end, will->topic);
        if (err != LWMQTT_SUCCESS) {
          return err;
        }

        // write payload length
        err = lwmqtt_write_num(&buf_ptr, buf_end, (uint16_t)will->payload.len);
        if (err != LWMQTT_SUCCESS) {
          return err;
        }

        // write payload
        err = lwmqtt_write_data(&buf_ptr, buf_end, (uint8_t *)will->payload.data, will->payload.len);
        if (err != LWMQTT_SUCCESS) {
          return err;
        }
    }

    // write username if present
    if (options->username.len > 0)
    {
        err = lwmqtt_write_string(&buf_ptr, buf_end, options->username);
        if (err != LWMQTT_SUCCESS)
        {
            return err;
        }
    }

    // write password if present
    if (options->username.len > 0 && options->password.len > 0)
    {
        err = lwmqtt_write_string(&buf_ptr, buf_end, options->password);
        if (err != LWMQTT_SUCCESS)
        {
            return err;
        }
    }

    // set written length
    *len = buf_ptr - buf;

  return LWMQTT_SUCCESS;
}
lwmqtt_err_t lwmqtt_decode_connack(uint8_t **buf, uint32_t buf_len, lwmqtt_message_t * msg)
{
  // prepare pointers
  uint8_t *buf_ptr = *buf;
  uint8_t *buf_end = *buf + buf_len;

  // read header
  uint8_t header;
  lwmqtt_err_t err = lwmqtt_read_byte(&buf_ptr, buf_end, &header);
  if (err != LWMQTT_SUCCESS) {
    return err;
  }

  // check packet type
  if (lwmqtt_read_bits(header, 4, 4) != LWMQTT_CONNACK_PACKET) {
    return LWMQTT_MISSING_OR_WRONG_PACKET;
  }

  // read remaining length
  uint32_t rem_len;
  err = lwmqtt_read_varnum(&buf_ptr, buf_end, &rem_len);
  if (err != LWMQTT_SUCCESS) {
    return err;
  }

  // check remaining length
  if (rem_len != 2) {
    return LWMQTT_REMAINING_LENGTH_MISMATCH;
  }
    buf_end=buf_ptr+rem_len;

  // read flags
  uint8_t flags;
  err = lwmqtt_read_byte(&buf_ptr, buf_end, &flags);
  if (err != LWMQTT_SUCCESS) {
    return err;
  }

  // read return code
  uint8_t raw_return_code;
  err = lwmqtt_read_byte(&buf_ptr, buf_end, &raw_return_code);
  if (err != LWMQTT_SUCCESS) {
    return err;
  }

//   không dùng
//  // get session present
//  *session_present = lwmqtt_read_bits(flags, 7, 1) == 1;

  // get return code
  switch (raw_return_code) {
    case 0:
      msg->returnCode = LWMQTT_CONNECTION_ACCEPTED;
      break;
    case 1:
      msg->returnCode = LWMQTT_UNACCEPTABLE_PROTOCOL;
      break;
    case 2:
      msg->returnCode = LWMQTT_IDENTIFIER_REJECTED;
      break;
    case 3:
      msg->returnCode = LWMQTT_SERVER_UNAVAILABLE;
      break;
    case 4:
      msg->returnCode = LWMQTT_BAD_USERNAME_OR_PASSWORD;
      break;
    case 5:
      msg->returnCode = LWMQTT_NOT_AUTHORIZED;
      break;
    default:
      msg->returnCode = LWMQTT_UNKNOWN_RETURN_CODE;
  }
    *buf+=buf_end-*buf; // tự động next địa chỉ gói tin tiếp theo
  return LWMQTT_SUCCESS;
}
