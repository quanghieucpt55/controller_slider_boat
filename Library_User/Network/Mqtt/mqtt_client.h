#ifndef __MQTTUSER_H_
#define __MQTTUSER_H_

#include "mqtt_connect.h"
#include "mqtt_publish.h"
#include "mqtt_subscrice.h"
#include "cytypes.h"
#include "mqtt_unsubscrice.h"
#include "mqtt_interface.h"
#include "boat_frame.h"

typedef struct Client Client;
struct Client {
    unsigned int next_packetid;
    unsigned char *buf_recv;
    unsigned char *buf_resp;
    uint32_t * len_buf_recv;
    uint32_t size_buf_recv;
    uint32_t * len_buf_resp;
    uint32_t size_buf_resp;

    unsigned int keepAliveInterval;
    char ping_outstanding;
    int isconnected;

    void (*MessageHandle) (lwmqtt_message_t *);
    NetworkMqtt* ipstack;
};

/* USER */
#define MQTT_ALIVE 180
#define MQTT_USER "mqttboat"
#define MQTT_PASSWORD "boat12345"
#define MQTT_CLIENT_ID_BASE "boat"

#define TOPIC_BOAT_SUB  "boat/sub/"
#define TOPIC_BOAT_FIRM_SUB "boat/firm/sub/"

uint8_t IsFindNewID(void);

// Hàm subcribe vào topic (topic đk chính và topic nhận yc OTA)
void Client_SubTopic_Boat(Client * c);
void Client_SubTopic_FirmwareUpdate(Client * c);

// Hàm connect tới Broker
void Client_ConnectBroker(Client * c);

// Hàm publish thông tin sự kiện lỗi, cảnh báo, thay đổi I/O
//void Client_PublishBoat_Event(Client * c);

/* Hàm publish heartbeat (ping) gồm các dữ liệu tổng quan
 * Hàm publish dữ liệu chi tiết gồm nhiều frame
*/
void Client_Ping_Boat(Client * c);// ping connect
void Client_PublishBoat_Mains(Client * c, e_boat_frame_cmd cmd);

// Hàm giải mã dữ liệu nhận được
lwmqtt_err_t Mqtt_DecodePacket(Client * c);

// Hàm phản hồi lệnh đk và yc OTA cho server
void Client_PublishBoat_ResponseRemote(Client * c);;
void Client_PublishFirm_ResponseRemote(Client * c);
#endif

