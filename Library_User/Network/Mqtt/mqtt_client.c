#include "stdio.h"
#include "mqtt_client.h"
#include "global.h"
#include "stdlib.h"
#include "swconfig.h"
#include "boat_frame.h"
#include "string.h"
#include "remote_firmware.h"
#include "remote_boat.h"
#include "boat_log.h"
#include "network.h"

#define HEADER_TOPIC_BOAT_EVENT "boat/event/" // header topic gửi sự kiện lỗi, thông báo
#define HEADER_TOPIC_BOAT_LOG "boat/log/" // header topic gửi status Boat (ping)
#define HEADER_TOPIC_BOAT_REMOTE "boat/remote/" // header topic gửi phản hồi lại server
#define HEADER_TOPIC_FIRMWARE_REMOTE "boat/firm/resp/" // header topic gửi phản hồi OTA

// retain
#define RETAIN_TOPIC_BOAT_MAINS 0
#define RETAIN_TOPIC_BOAT_EVENT 0
#define RETAIN_TOPIC_BOAT_LOG 0
#define RETAIN_TOPIC_BOAT_REMOTE 0

#define QoS_TOPIC_BOAT_MAINS 0
#define QoS_TOPIC_BOAT_EVENT 1
#define Qos_TOPIC_BOAT_LOG 1
#define QoS_TOPIC_BOAT_REMOTE 0




uint32_t id_subscribed=0;// lưu trữ id đã sub
#define SIZE_BUFFER_TOPIC 100
char bufferTopic[SIZE_BUFFER_TOPIC];
#define MAX_BUF_GLOBAL_CACHE 500
uint8_t buf_Cache[MAX_BUF_GLOBAL_CACHE];
#define MAX_BUF_EVENT 15
uint8_t buf_Event[MAX_BUF_EVENT];

// Hàm gửi xác nhận ACK với Qos 1,2
void SendPuback(Client * c ,uint16_t  messageID)
{
    uint8 frameSuback[4]={0,0,0,0};
    frameSuback[0]=0x40;
    frameSuback[1]=0x02;
    frameSuback[2]=messageID>>8;
    frameSuback[3]=messageID;
    c->ipstack->mqttwrite(frameSuback,4);
}

// Hàm giải mã gói tin nhận được
lwmqtt_err_t Mqtt_DecodePacket(Client * c)
{
    uint8_t * ptr_buf=c->buf_recv;
    uint32_t lenBuf=*c->len_buf_recv;
    while(ptr_buf<(c->buf_recv+lenBuf))// giải mã toàn bộ frame cho đến khi hết dữ liệu
    {
        uint8_t packet_type= lwmqtt_read_bits(ptr_buf[0], 4, 4);
        lwmqtt_err_t err=LWMQTT_SUCCESS;
        switch(packet_type)
        {
            case LWMQTT_CONNACK_PACKET:
            {
                lwmqtt_message_t msg;
                err=lwmqtt_decode_connack(&ptr_buf,(lenBuf-(ptr_buf-c->buf_recv)),&msg);
                if(err==LWMQTT_SUCCESS)
                {
                    msg.type=LWMQTT_CONNACK_PACKET;
                    if(NULL!=c->MessageHandle)// raising event
                        c->MessageHandle(&msg);
                }
                else
                    return err;
            }
                break;
            case LWMQTT_PUBLISH_PACKET:
            {
                lwmqtt_message_t msg;
                err=MQTT_decodePublish(&ptr_buf,(lenBuf-(ptr_buf-c->buf_recv)),&msg);
                if(err==LWMQTT_SUCCESS)
                {
                    msg.type=LWMQTT_PUBLISH_PACKET;
                    if(msg.qos==LWMQTT_QOS1)
                        SendPuback(c,msg.id);
                    if(NULL!=c->MessageHandle)// raising event
                        c->MessageHandle(&msg);
                }
                else
                    return err;
            }
                break;
            case LWMQTT_PUBACK_PACKET:
            {
                lwmqtt_message_t msg;
                err=MQTT_decodePuback(&ptr_buf,(lenBuf-(ptr_buf-c->buf_recv)),&msg);
                if(err==LWMQTT_SUCCESS)
                {
                    msg.type=LWMQTT_PUBACK_PACKET;
                    if(NULL!=c->MessageHandle)
                        c->MessageHandle(&msg);
                }
                else
                   return err;
            }
                break;
            case LWMQTT_SUBACK_PACKET:
            {
                lwmqtt_message_t msg;
                err=lwmqtt_decode_suback(&ptr_buf,(lenBuf-(ptr_buf-c->buf_recv)),&msg);
                if(err==LWMQTT_SUCCESS)
                {
                    msg.type=LWMQTT_SUBACK_PACKET;
                    if(NULL!=c->MessageHandle)
                        c->MessageHandle(&msg);
                }
                else
                   return err;
            }
            break;
            default:
                return err;
        }
    }
    return LWMQTT_SUCCESS;
}

// Hàm chuẩn bị frame Publish trước khi gửi
uint16 Mqtt_Encode_Publish(uint8 * _buffer,uint32_t buf_size,uint8 * headerTopic,
                        uint8 *pMessage,uint32_t lengthMessage,uint8 retain,uint8 Qos,uint32_t id_topic)// ,cuoc,
{
    uint32_t lenBuf=0;
    char * bufTopic= bufferTopic;
    snprintf(bufTopic,SIZE_BUFFER_TOPIC,"%s%d",headerTopic,(int)id_topic);
    lwmqtt_string_t topic;
    topic.data=bufTopic;
    topic.len=strlen(bufTopic);
    lwmqtt_message_t msg;
    msg.qos=Qos;
    msg.retained=retain;
    msg.payload=pMessage;
    msg.payload_len=lengthMessage;
    msg.id=rand();
    lwmqtt_err_t err= lwmqtt_encode_publish(_buffer,buf_size,&lenBuf,0,rand(),topic,msg);
    if(err==LWMQTT_SUCCESS)
        return lenBuf;
    else
        return 0;
}

// Hàm kết nối Broker
void Client_ConnectBroker(Client * c)
{
	int rand_client_id=rand();
	if(rand_client_id<0)
		rand_client_id=-rand_client_id;
	char * bufClientId=(char *)buf_Cache;
	snprintf(bufClientId,36,"%s_%d",MQTT_CLIENT_ID_BASE,(int)ID_DEVICE,rand_client_id);
    lwmqtt_options_t option_connect;
    option_connect.clean_session=1;
    option_connect.client_id.data=bufClientId;
    option_connect.client_id.len=strlen(bufClientId);
    option_connect.keep_alive=MQTT_ALIVE;
    option_connect.password.data=MQTT_PASSWORD;
    option_connect.password.len=strlen(MQTT_PASSWORD);
    option_connect.username.data=MQTT_USER;
    option_connect.username.len=strlen(MQTT_USER);
    lwmqtt_err_t err=lwmqtt_encode_connect(c->buf_resp,c->size_buf_resp,c->len_buf_resp,&option_connect,NULL);
    if(err==LWMQTT_SUCCESS)// encoder no err
        c->ipstack->mqttwrite(c->buf_resp,*(c->len_buf_resp));
}

void Client_SubTopic_FirmwareUpdate(Client * c)
{
    char * bufTopic=(char *)buf_Cache;
    snprintf(bufTopic,100,"%s%d",TOPIC_BOAT_FIRM_SUB,(int)ID_DEVICE);
    id_subscribed=ID_DEVICE;// lưu trữ ID đã subscribe
    lwmqtt_string_t topic ;
    topic.data=bufTopic;
    topic.len=strlen(bufTopic);
    lwmqtt_qos_t qos=LWMQTT_QOS1;
    lwmqtt_err_t err= lwmqtt_encode_subscribe(c->buf_resp,c->size_buf_resp,c->len_buf_resp,rand(),1,&topic,&qos);
    if(err==LWMQTT_SUCCESS)
        c->ipstack->mqttwrite(c->buf_resp,*(c->len_buf_resp));
}


void Client_SubTopic_Boat(Client * c)
{
    char * bufTopic=(char *)buf_Cache;
    snprintf(bufTopic,100,"%s%d",TOPIC_BOAT_SUB,(int)ID_DEVICE);
    id_subscribed=ID_DEVICE;// lưu trữ ID đã subscribe
    lwmqtt_string_t topic ;
    topic.data=bufTopic;
    topic.len=strlen(bufTopic);
    lwmqtt_qos_t qos=LWMQTT_QOS1;
    lwmqtt_err_t err= lwmqtt_encode_subscribe(c->buf_resp,c->size_buf_resp,c->len_buf_resp,rand(),1,&topic,&qos);
    if(err==LWMQTT_SUCCESS)
        c->ipstack->mqttwrite(c->buf_resp,*(c->len_buf_resp));
}


// trạng thái kết nối của Boat để giữ nhịp truyền ,kiểm soát kết nối
void Client_Ping_Boat(Client * c)
{
    uint8_t * frameDataNetwork=buf_Cache;
    uint32_t lenFrame=Boat_Frame_Ping_Complete(frameDataNetwork,300);
    if (lenFrame==0)
        return;
    *(c->len_buf_resp)=Mqtt_Encode_Publish(c->buf_resp,c->size_buf_resp,
                        (uint8_t *)HEADER_TOPIC_BOAT_LOG,
                        frameDataNetwork,lenFrame,
                        RETAIN_TOPIC_BOAT_LOG,Qos_TOPIC_BOAT_LOG,ID_DEVICE);
    c->ipstack->mqttwrite(c->buf_resp,*(c->len_buf_resp));
}

// dữ liệu phản hồi từ boat khi cấu hình từ server
void Client_PublishBoat_ResponseRemote(Client * c)
{
    if(IsOnMsg_BoatResponseRemote())
    {

        *(c->len_buf_resp)=Mqtt_Encode_Publish(c->buf_resp,c->size_buf_resp,
                                (uint8_t *)HEADER_TOPIC_BOAT_REMOTE,
                                boat_buf_response_remote,
                                boat_len_bufResponseRemote,
                                RETAIN_TOPIC_BOAT_REMOTE,QoS_TOPIC_BOAT_REMOTE,ID_DEVICE);
        c->ipstack->mqttwrite(c->buf_resp,*(c->len_buf_resp));
    }
}

void Client_PublishFirm_ResponseRemote(Client * c)
{
    if(IsOnMsg_FirmResponseRemote())
    {

        *(c->len_buf_resp)=Mqtt_Encode_Publish(c->buf_resp,c->size_buf_resp,
                                (uint8_t *)HEADER_TOPIC_FIRMWARE_REMOTE,
                                firm_buf_response_remote,
                                firm_len_bufResponseRemote,
                                RETAIN_TOPIC_BOAT_REMOTE,0,ID_DEVICE);
        c->ipstack->mqttwrite(c->buf_resp,*(c->len_buf_resp));
    }
}

bool Client_PublishBoat_Event(Client * c)
{
    // sự kiện lỗi
    if(IsHaveMsgQueueBoatEventLog())
    {
        uint8_t * frameEvent=buf_Event;
        uint8_t len_frame_event=Boat_Frame_Event(frameEvent, MAX_BUF_EVENT);
        if (len_frame_event==0)
            return false;
        *(c->len_buf_resp)=Mqtt_Encode_Publish(c->buf_resp,c->size_buf_resp,
                        (uint8_t *)HEADER_TOPIC_BOAT_EVENT,
                        frameEvent,len_frame_event,
                        RETAIN_TOPIC_BOAT_EVENT,QoS_TOPIC_BOAT_EVENT,ID_DEVICE);
        c->ipstack->mqttwrite(c->buf_resp,*(c->len_buf_resp));
        return true;
    } else
    {
    //nếu queue rỗng và ROM còn item thì load từ ROM vào queue
        if(IsHaveMsgInRomBoatEventLog())
        {
            BoatEventLog_LoadFromRomToQueue();
        }
    }
    return false;
}

uint8_t IsFindNewID(void)
{
    if(id_subscribed!=ID_DEVICE)
        return 1;
    else
        return 0;
}
