#include "main.h"
#include "global.h"
#include "clock.h"
#include "stdio.h"
#include "sim.h"
#include "mqtt_client.h"
#include "stdlib.h"
#include "mqtt_interface.h"
#include "network.h"
#include "string.h"
#include "sim_utils.h"
#include "remote_boat.h"
#include "uart_ring_buffer.h"
#include "sim_io.h"
#include "remote_firmware.h"
#include "VCU_State.h"
#include "Boat_param.h"

#define CMNET "\"APN\""

#define ENABLE_SIM_SLEEP
#define MODE_NOT_SECURITY 0
#define MODE_SSL 1

#define BUFFER_RECV_GPRS 500
#define BUFFER_RESP_GPRS 600
#define TIME_ON_MSG_MQTT 5

#define CYCLE_SEND netConfig.cycle_send_slow // gửi dữ liệu chậm khi thuyền không di chuyển
#define CYCLE_SEND_FAST netConfig.cycle_send_fast // gửi dữ liệu nhanh khi thuyền chạy

e_sim_work simCount=COUNT_SIM_PWON;
e_sim_status simStatus; // trạng thái sim

uint8_t cmdStep=1,
        simStep=4,
        simTimeOut=0,
        haveDataGprs,
        simCycleSend=2;
uint8_t sim_signal_quanlity;// chất lượng sóng
uint8_t simRetryCount=0; // số lần thử kết nối
uint8_t simErrorRetry=0; // số lần thử mạng
uint8_t simCard, // lựa chon sim
        simRequireSendImmediate=0; // yêu cầu sim gửi dữ liệu ngay lập tức
uint8_t simModeSecurity=MODE_NOT_SECURITY;
uint8_t buffer_recv_gprs[BUFFER_RECV_GPRS];
uint8_t buffer_resp_gprs[BUFFER_RESP_GPRS];
int16_t  SIM_HaveCommand=0;
uint32_t len_buf_recv_gprs,len_buf_send_gprs;
int32_t lastTime2Sim=0,lastTime3Sim=0; // thời gian trễ thứ 2 của sim
int32_t gprs_timeNewMsgMqtt;
NetworkMqtt n_gprs;
Client c_gprs;
lwmqtt_message_t gprs_MsgMqtt;


#define BUFFER_UART_SIZE 800
uint8_t buf_uart_sim[BUFFER_UART_SIZE];
ring_buffer_t tx_ring_buffer_sim;

void Sim_SwitchModeSecurity(void);

//-------------------------------------------------------------------------------//
int Sim_PWOn(void);
int Sim_StartUp(void);
int Sim_SelectSim(void);// 1
int Sim_SetUp(void);
int Sim_ConnectServer(void);
int Sim_MqttEstablish(void);
int Sim_MqttComunication(void);
int Sim_Disconnect(void);
int Sim_Sleep(void);
void Sim_MqttDecodeMsg(void);
void Sim_WakeUp(void);
void Sim1_CheckConnectServer(void);
void Sim_MqttMsgHandle(lwmqtt_message_t * msg);
uint16_t SendData_ThowGprs(uint8_t * buf_send,uint32_t lenBufferSend);
void NextCmdStep(void);
//-------------------------------------------------------------------------------//

void Sim_Init(void)
{

    n_gprs.mqttwrite=SendData_ThowGprs;
    n_gprs.type=TYPE_GPRS;

    c_gprs.buf_recv=buffer_recv_gprs;
    c_gprs.len_buf_recv=&len_buf_recv_gprs;
    c_gprs.size_buf_recv=BUFFER_RECV_GPRS;

    c_gprs.buf_resp=buffer_resp_gprs;
    c_gprs.len_buf_resp=&len_buf_send_gprs;
    c_gprs.size_buf_resp=BUFFER_RESP_GPRS;
    c_gprs.ipstack=&n_gprs;

    c_gprs.MessageHandle=Sim_MqttMsgHandle;

    tx_ring_buffer_sim.buffer=buf_uart_sim;
	tx_ring_buffer_sim.bufferSize=BUFFER_UART_SIZE;
	tx_ring_buffer_sim.head=0;
	tx_ring_buffer_sim.tail=0;

	/* Enable the UART Data Register not empty Interrupt */
	__HAL_UART_ENABLE_IT(uart_sim, UART_IT_RXNE);
}
void Sim_Work(void)
{
	int result=0;
    Sim1_CheckConnectServer();
    Sim_MqttDecodeMsg();
    Sim_WakeUp();
	switch (simCount)
	{
		case COUNT_SIM_PWON:
			result=Sim_PWOn();
			break;
        case COUNT_SIM_SELECT:
            result=2;//Sim_SelectSim();
            break;
    	case COUNT_SIM_STARTUP:
        	result=Sim_StartUp(); // chọn nhà mạng
         	break;
		case COUNT_SIM_SETUP: // cài đặt mạng
			result=Sim_SetUp();
			break;
		case COUNT_SIM_CONNECT:// kết nối server
			result=Sim_ConnectServer();
			break;
        case COUNT_MQTT_ESTABLISH: // thiết lập kết nối mqtt connect, sub
            result=Sim_MqttEstablish();
            break;
        case COUNT_MQTT_COMUNICATION:
            result=Sim_MqttComunication();
            break;
        case COUNT_SIM_DISCONNECT:
        	 result=Sim_Disconnect();
            break;
        #ifdef ENABLE_SIM_SLEEP
        case COUNT_SIM_SLEEP:
            result=Sim_Sleep();
            break;
        #endif

		default:
			simCount=COUNT_SIM_PWON;
			result=0;
	}
	if (result>0)
	{
		simCount+=result-1;
		simStep=0;
        simTimeOut=0;
		simRetryCount=0;
	}
	if (result<0)
	{
    	simStep=0;
        simTimeOut=0;
	}
}


void Sim_MqttMsgHandle(lwmqtt_message_t * msg)
{
    gprs_MsgMqtt.type=msg->type;
    gprs_MsgMqtt.id=msg->id;
    switch(msg->type)
    {
        case LWMQTT_CONNACK_PACKET:
            break;
        case LWMQTT_CONNECT_PACKET:

            break;
        case LWMQTT_PUBLISH_PACKET:
        {
            if(cstrpos(TOPIC_BOAT_SUB,msg->topicName.data,msg->topicName.len))
            {
                RemoteBoat_ExcuteCommand(msg->payload,msg->payload_len);
            }
            else
            {
            	RemoteFirm_ExcuteCommand(msg->payload,msg->payload_len);
            }

        }
        break;
        case LWMQTT_PUBACK_PACKET:
            break;
        case LWMQTT_PUBREC_PACKET:
        case LWMQTT_PUBREL_PACKET:
            break;
        case LWMQTT_SUBACK_PACKET:

            break;
    }
}

void UART_SIM_WriteByte(uint8_t c)
{
	int i = (tx_ring_buffer_sim.head + 1) % tx_ring_buffer_sim.bufferSize;
	while (i == tx_ring_buffer_sim.tail);

	tx_ring_buffer_sim.buffer[tx_ring_buffer_sim.head] = (uint8_t)c;
	tx_ring_buffer_sim.head = i;

	__HAL_UART_ENABLE_IT(uart_sim, UART_IT_TXE); // Enable UART transmission interrupt
}

void UART_SIM_PutArray(uint8_t * frame,uint32_t len)
{
	for(uint32_t i=0;i<len;i++)
	{
		UART_SIM_WriteByte(frame[i]);
	}
}

void UART_SIM_PutString(const char *str)
{
	 while(*str)
	{
		 UART_SIM_WriteByte(*str++);
	}
}


uint16_t SendData_ThowGprs(uint8_t * buf_send,uint32_t lenBufferSend)
{
	UART_SIM_PutArray(buf_send,lenBufferSend);
	return lenBufferSend;
}

cystatus Sim_CheckMsgMqtt(uint8_t typeMsg)
{
    if(gprs_MsgMqtt.type!=LWMQTT_NO_PACKET)
        if(gprs_MsgMqtt.type==typeMsg)
        {
            gprs_MsgMqtt.type=LWMQTT_NO_PACKET;// clear aviod duplicate data
            return CYRET_SUCCESS;
        }
    return CYRET_BAD_DATA;
}
void Stepto(int NewStep)
{
	simStep=NewStep;
	simTimeOut=0;
}
void Sim1CountTo(int count)
{
    simCount=count;
}
void NextStep(void)
{
	simStep++;
	simTimeOut=0;
}
void NextCmdStep(void)
{
	cmdStep++;
    if (cmdStep>11)
        cmdStep=1;
}
void PreviousStep(void)
{
	simStep--;
	simTimeOut=0;
}
void Sim_ResetConversation(void)
{
	len_buf_recv_gprs=0;
    SIM_HaveCommand=0;
    haveDataGprs=0;
}
void Sim1_CountToStepTo(e_sim_work newCount,int newStep)
{
	simStep=newStep;
	simTimeOut=0;
	simCount=newCount;
    Sim_ResetConversation();
}


void Sim_UartIsrHandle(void)
{
	UART_HandleTypeDef *huart=uart_sim;
	uint32_t isrflags   = READ_REG(huart->Instance->SR);
	uint32_t cr1its     = READ_REG(huart->Instance->CR1);

    /* if DR is not empty and the Rx Int is enabled */
    if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
    {

		huart->Instance->SR;                       /* Read status register */
        uint8_t c = huart->Instance->DR;     /* Read data register */
        unsigned char getchar=c;
		gprs_timeNewMsgMqtt=millis();
		haveDataGprs=1;
		if (!SIM_HaveCommand)
		{
			buffer_recv_gprs[len_buf_recv_gprs++]=getchar;
			if (len_buf_recv_gprs>=BUFFER_RECV_GPRS)
				SIM_HaveCommand=1;
		}

    }

    /*If interrupt is caused due to Transmit Data Register Empty */
    if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
    {
    	if(tx_ring_buffer_sim.head == tx_ring_buffer_sim.tail)
		{
		  // Buffer empty, so disable interrupts
		  __HAL_UART_DISABLE_IT(huart, UART_IT_TXE);

		}

    	else
		{
		  // There is more data in the output buffer. Send the next byte
		  unsigned char c = tx_ring_buffer_sim.buffer[tx_ring_buffer_sim.tail];
		  tx_ring_buffer_sim.tail = (tx_ring_buffer_sim.tail + 1) % tx_ring_buffer_sim.bufferSize;

		  huart->Instance->SR;
		  huart->Instance->DR = c;

		}
    }
}


uint8_t IsOnMessageMqtt(void)
{
    if(haveDataGprs)
    {
        if((millis()-gprs_timeNewMsgMqtt)>TIME_ON_MSG_MQTT)
        {
            return 1;
        }
    }
    return 0;
}
unsigned char UartSimBufferSize_OverFlow(void)
{
	return SIM_HaveCommand;
}
uint16_t UartSimBufferSize(void )
{
	return len_buf_recv_gprs;
}
void GPRS_Ask(const char cmd[])
{
	Sim_ResetConversation();
	UART_SIM_PutString(cmd);
	NextStep();
}
char Wait(unsigned char t)
{
	if (IsSecondChange())
	{
		if (simTimeOut==0)
			simTimeOut=t-1;
		else
			simTimeOut--;
		if (simTimeOut==0)
			return 2;
	}
	return 0;
}
void SimWait2_Reset(void)
{
    lastTime2Sim=millis();
}
void SimWait3_Reset(void)
{
    lastTime3Sim=millis();
}
void SimCmdTimer_Reset(void)
{
    Boat_CmdTimer_Reset();
}

uint8_t SimWait2(int32_t delay)
{
    if(millis()-lastTime2Sim>=delay)
    {
        lastTime2Sim=millis();
        return 1;
    }
    return 0;
}

uint8_t SimWait3(int32_t delay)
{
    if(millis()-lastTime3Sim>=delay)
    {
        lastTime3Sim=millis();
        return 1;
    }
    return 0;
}

unsigned char WaitAnswer(const char answer[])
{
	return(cstrpos(answer,(char *)buffer_recv_gprs,UartSimBufferSize()));
}
void PWRKEY_Off()
{
    PWRKEY_Write(0);
}
void PWRKEY_On()
{
    PWRKEY_Write(1);
}


uint8_t Sim_SignalQuanlityPercent(void)
{
    return sim_signal_quanlity;
}

//return:
// 0: dang thuc hien
// <0(0xFF): Co loi, phai khoi dong lai
// >0: Thanh cong
// Step Inc = return +/(-) 1
int Sim_PWOn(void)			// 0
{
    simStatus=Sim_StartUp_Status;
	switch(simStep)
	{
		case 0x00:					// Check that module correctly startup
			simRetryCount=0;
			GPRS_Ask("AT\r\n");							// Test connection
			break;
		case 0x01:
			if (WaitAnswer("OK"))						// Module buffer_recv_gprs
            {
                return 2;
            }
			else if (Wait(1))							// Module don't buffer_recv_gprs
				NextStep();
			break;
		case 0x02:
			PWRKEY_Off();
		  	if (Wait(1))
		  	{
				NextStep();
				Sim_ResetConversation();
			}
			break;
		case 0x03:				// Turn off
		  	PWRKEY_On();
			if (Wait(5))
		  	{
		  		NextStep();
		  	}
			break;
		case 0x04:				// Wait for startup
			PWRKEY_Off();//5v
			if (WaitAnswer(" POWER DOWN"))
				Stepto(0x02);
			else if (Wait(5))
				NextStep();
			break;
		case 0x05:					// Check that module correctly startup
			GPRS_Ask("AT\r\n");								// Test connection
			break;
		case 0x06:
			if (WaitAnswer("OK"))						// Module buffer_recv_gprs
            {
				return 2;
            }
			else if (Wait(5))							// Module don't buffer_recv_gprs
			{
                if (simRetryCount++ < 4) 
                    Stepto(0x05);
                else 
                    NextStep();
            }
            break;
		case 0x07:
			return -1;
			break;
		default:
			NextStep();
	}
	return 0;
}

int Sim_SelectSim(void)// 1
{
    switch(simStep)
    {

        case 2:
            GPRS_Ask("AT+CPIN?\r\n");
            break;
        case 3:

            if(WaitAnswer("ERROR"))
            {
                NextStep();
            }
            else if (WaitAnswer("+CPIN: READY")||WaitAnswer("OK"))
                return 2;
            else if (Wait(3))
                return -1;
            break;
        case 4:
            GPRS_Ask("AT+QDSIM=?\r\n");
            break;
        case 5:
            if(WaitAnswer("+QDSIM: 0"))
            {
                simCard=1;//chuyển sang sim 2
                NextStep();
            }
            else
                if(WaitAnswer("+QDSIM: 1"))
                {
                    simCard=0;
                    NextStep();
                }
            else if(Wait(2))
            {
                simCard=!simCard;
                NextStep();
            }
            break;
        case 6:
            if(simCard)// đang ở sim 1 -> chuyển sim 2
            {
                GPRS_Ask("AT+QDSIM=1,1\r\n");// chuyển sim 2
            }
            else
            {
                GPRS_Ask("AT+QDSIM=0,1\r\n");
            }
            break;
        case 7:
            GPRS_Ask("AT+QPOWD=0\r\n");
            break;
        case 8:
            if(WaitAnswer("OK"))
                return -1;
            else
                if(Wait(2))
                    return -2;

            break;
    }
    return 0;
}

int Sim_StartUp(void)			// 2
{
	switch (simStep)
	{
//        case 0:
//            GPRS_Ask("AT+QSIMDET=1,0\r\n");
//        break;
//        case 1:
//            if (WaitAnswer("OK"))						// Module buffer_recv_gprs
//				NextStep();
//			else if (Wait(3))	//2								// Module don't buffer_recv_gprs or No SIM
//            {
//                Sim1_CountToStepTo(COUNT_SIM_PWON,2);
//            }
//        break;
		case 2:
			GPRS_Ask("AT+CPIN?\r\n");							// Test connection
			break;
		case 3:
			if (WaitAnswer("+CPIN: READY")||WaitAnswer("OK"))
            {
                simErrorRetry=0;						// Module buffer_recv_gprs
                NextStep();
            }
			else if (Wait(2))	//2								// Module don't buffer_recv_gprs or No SIM
            {
                if(WaitAnswer("NO SIM"))
                {
                    simStatus=Sim_NoSim_Status;
                    return -1;
                }
                if (++simErrorRetry>=10)
                {
                    simErrorRetry=0;
                    Sim1_CountToStepTo(COUNT_SIM_PWON,2);
                } else return -1;
            }
			break;
		case 4:												// Check NetworkMqtt SIM_PROVIDER
			GPRS_Ask("AT+COPS?\r\n");					// Test connection
            simStatus=Sim_CheckSignal_Status;
			break;
		case 5:
			if (WaitAnswer("+COPS: 0,"))
			{
                simErrorRetry=0;
                if(Wait(2)) NextStep();
			}
			else if (Wait(2)) {
                if (++simErrorRetry>=15)
                {
                    simErrorRetry=0;
                    Sim1_CountToStepTo(COUNT_SIM_PWON,2);
                } else Stepto(4);
            }
			break;
        case 6:
            GPRS_Ask("AT+CSQ\r\n");
            break;
        case 7:
            if(WaitAnswer("OK"))
            {
                // lấy chất lượng đường truyền
                simErrorRetry=0;
                uint8_t pos=WaitAnswer("+CSQ: ");
                if(pos>0)
                {
                    sim_signal_quanlity=(buffer_recv_gprs[pos]-'0')*10+(buffer_recv_gprs[pos+1]-'0');
                    // quy đổi sang phần trăm
                    sim_signal_quanlity=Sim_CalculatorSignal(sim_signal_quanlity);
                }
                NextStep();
            }
			else if(Wait(2))
            {
                if (++simErrorRetry>=10)
                {
                    simErrorRetry=0;
                    Sim1_CountToStepTo(COUNT_SIM_PWON,2);
                } else Stepto(6);
            }
            break;
        case 8:
			GPRS_Ask("ATE0\r\n"); // tat echo
			break;
		case 9:
			if(WaitAnswer("OK"))
            {
                simErrorRetry=0;
				NextStep();
            }
			else if(Wait(2)) 
            {
                if (++simErrorRetry>=10)
                {
                    simErrorRetry=0;
                    Sim1_CountToStepTo(COUNT_SIM_PWON,2);
                } else Stepto(8);
            }
			break;
        case 10: // end set up!
            debug_print("Done");
            return 2;
        break;
		default:
			NextStep();
	}
	return 0;
}


int Sim_SetUp(void)
{
    simStatus=Sim_CheckSignal_Status;
    switch (simStep)
	{
        case 0:
            GPRS_Ask("AT+CGATT=1\r\n");
        break;
        case 1:
            if(WaitAnswer("OK")) {
                simErrorRetry=0;
                NextStep();
            }
			else if(Wait(2))
            {
                if (++simErrorRetry>=10)
                {
                    simErrorRetry=0;
                    Sim1_CountToStepTo(COUNT_SIM_PWON,2);
                } else return -1;
            }
			break;
        break;
		case 2:
            GPRS_Ask("AT+CGATT?\r\n");
            break;
        case 3:
            if (WaitAnswer("+CGATT: 1"))
			{
				simErrorRetry=0;
				NextStep();
			}
			else if(WaitAnswer("ERROR"))
			{
                Sim1_CountToStepTo(COUNT_SIM_PWON,2);
				return 0;
			}
            else
		        if (Wait(3))
				{
					if (++simErrorRetry>=10)
					{
						simErrorRetry=0;
						Sim1_CountToStepTo(COUNT_SIM_PWON,2);
					} else Stepto(2);
				}
            break;
        case 4:
            GPRS_Ask("AT+QICSGP=1\r\n");    //
            break;
        case 5:
            if (WaitAnswer("OK")) {
                simErrorRetry=0;
                NextStep();
            }
			else if(WaitAnswer("ERROR"))
			{
				Sim1_CountToStepTo(COUNT_SIM_PWON,2);
				return 0;
			}
            else
		        if (Wait(3))
                {
                    if (++simErrorRetry>=10)
                    {
                        simErrorRetry=0;
                        Sim1_CountToStepTo(COUNT_SIM_PWON,2);
                    } else Stepto(4);
                }
            break;
        case 6:
            GPRS_Ask("AT+QIACT=1\r\n");    // Activate scene 1, affected by the network status, the maximum response time is 150 seconds
            break;
        case 7:
            if (WaitAnswer("OK"))
            {
                simErrorRetry=0;
				NextStep();
            }
			else if(WaitAnswer("ERROR"))
			{
				Sim1_CountToStepTo(COUNT_SIM_PWON,2);
				return 0;
			}
            else
		        if (Wait(3))
                {
                    if (++simErrorRetry>=10)
                    {
                        simErrorRetry=0;
                        Sim1_CountToStepTo(COUNT_SIM_PWON,2);
                    } else Stepto(6);
                }
            break;
//        case 8:
//        {
//        	GPRS_Ask("AT+IPR=115200;&W\r\n");
//        }
//		break;
//        case 9:
//        {
//        	if (WaitAnswer("OK"))
//				NextStep();
//			else if(WaitAnswer("ERROR"))
//			{
//				Sim1_CountToStepTo(COUNT_SIM_PWON,2);
//				return 0;
//			}
//			else
//				if (Wait(3))
//					return -1;
//        }
//		break;

//        case 8:
//            GPRS_Ask("AT+QIACT=?\r\n");    // Activate scene 1, affected by the network status, the maximum response time is 150 seconds
//            break;
//        case 9:
//            if (WaitAnswer("+QIACT:"))
//				NextStep();
//			else if(WaitAnswer("ERROR"))
//			{
//				Sim1_CountToStepTo(COUNT_SIM_PWON,2);
//				return 0;
//			}
//            else
//		        if (Wait(3))
//					return -1;
//            break;
        case 12:
            return 2;
            break;
		default:
			NextStep();

	}
	return 0;
}


int Sim_ConnectServer(void)				// 4
{
    simStatus=Sim_Connecting_Status;
	switch (simStep)
	{
		case 0x0:
        {
            char strConnection[100];
            memset(strConnection,0,100);
            sprintf(strConnection,"AT+QIOPEN=1,0,\"TCP\",\"%s\",%d,0,2\r\n",SERVER_NAME,(int)SERVER_PORT);
            GPRS_Ask(strConnection);

        }
        break;
        case 0x1:
        {
            if (WaitAnswer("CONNECT"))							// Connect OK, Already connect
			{
				return 2;
			}
			else if (WaitAnswer("OK"))
				NextStep();
			else if (WaitAnswer("NOT READY"))
				return -1;
			else if (WaitAnswer("CLOSED")
				||WaitAnswer("CONNECT FAIL"))
				return -3;//-3
			else if (WaitAnswer("ERROR"))
                Stepto(0x4);
			else if(Wait(10))
				return -2;//-4
        }
		break;
        case 0x2:
        {
			if (WaitAnswer("CONNECT"))							// Connect OK, Already connect
			{
				return 2;
			}
			else if (WaitAnswer("CLOSED")
				||WaitAnswer("CONNECT FAIL"))
				return -3;//-3
			else if(Wait(10))
				return -2;//-4						// Re Connect
        }
		break;
        case 0x4:
			GPRS_Ask("AT+CEER\r\n");				// Test Error
			break;
		case 0x5:
			if (Wait(3))
			{
				Sim1_CountToStepTo(COUNT_SIM_PWON,2);
				return 0;
			}
			break;
		default:
			NextStep();
	}
	return 0;
}


int Sim_Disconnect(void)
{
    simStatus=Sim_Disconnected_Status;
	switch (simStep)
	{
		case 0:				// 0x60
			if (Wait(2))
				NextStep();
			break;
		case 1:
			GPRS_Ask("+++");
			break;
		case 2:
			if (Wait(2))
				NextStep();
			break;
		case 3:
			GPRS_Ask("AT+QICLOSE\r\n");			// Normal Power off
			break;
		case 4:
			if (WaitAnswer("OK"))				// Neu co tra loi -> Sim van lam viec -> Dat lai thong so
			{
                NextStep();
			}
			else if (Wait(4))
            {
                NextStep();
            }
			break;
        case 5:
            GPRS_Ask("AT+QIDEACT\r\n");
            break;
        case 6:
            if(WaitAnswer("DEACT OK"))
            {
                NextStep();
            }
            else if(Wait(4))
            {
                NextStep();
            }
            break;
        case 7:
                Sim1_CountToStepTo(COUNT_SIM_SETUP,0);
                return 1;
            break;
		default:
			NextStep();
	}
	return 0;
}


int Sim_MqttEstablish(void)
{
    switch(simStep)
    {
        case 0:
            NextStep();
            break;
        case 1:
            if(Wait(2))
                NextStep();
            break;
        case 2:
            Sim_ResetConversation();
            Client_ConnectBroker(&c_gprs);
            NextStep();
            break;
        case 3:
            if(Sim_CheckMsgMqtt(LWMQTT_CONNACK_PACKET)==CYRET_SUCCESS)
            {
    			NextStep();
            }
            else if(Wait(4))
            {
                Sim1_CountToStepTo(COUNT_SIM_DISCONNECT,0);
                return 1;
            }
            break;
       case 4:// mqtt subsribe
            Sim_ResetConversation();
            Client_SubTopic_Boat(&c_gprs);
            NextStep();
            break;
        case 5:
            if(Sim_CheckMsgMqtt(LWMQTT_SUBACK_PACKET)==CYRET_SUCCESS)
            {
                NextStep();
            }
            else if(Wait(4))// no respond server
            {
                Sim1_CountToStepTo(COUNT_SIM_DISCONNECT,0);
                return 1;
            }
            break;
        case 6:
        	Sim_ResetConversation();
        	Client_SubTopic_FirmwareUpdate(&c_gprs);
			NextStep();
        	break;
        case 7:
        	if(Sim_CheckMsgMqtt(LWMQTT_SUBACK_PACKET)==CYRET_SUCCESS)
			{
				// SimWait2_Reset();// xóa thời gian timer 2 đi
				// SimWait3_Reset();// xóa thời gian timer 2 đi
				SimCmdTimer_Reset();// Reset timer cho tất cả các CMD
				Sim_ResetConversation();
				Sim_RequireSendImmediate();// yêu cầu sim gửi ngay lập tức lần đầu kết nối
				return 2;
			}
			else if(Wait(4))// no respond server
			{
				Sim1_CountToStepTo(COUNT_SIM_DISCONNECT,0);
				return 1;
			}
			break;
        	break;
        default:
            NextStep();
    }
    return 0;
}

void Sim_MqttDecodeMsg(void)
{
    if(simCount==COUNT_MQTT_ESTABLISH || simCount==COUNT_MQTT_COMUNICATION)
    {
        if(IsOnMessageMqtt())
        {
            Mqtt_DecodePacket(&c_gprs);
            Sim_ResetConversation();
        }
    }
}

int Sim_MqttComunication(void)
{
    if(IsFindNewID())// tìm thấy ID mới ->disconnect đế sub lại
    {
        Sim1_CountToStepTo(COUNT_SIM_DISCONNECT,0);
        return 1;
    }
    simStatus=Sim_Connected_Status;
    Client_PublishBoat_ResponseRemote(&c_gprs);
    //Client_Publish_Event(&c_gprs);
    Client_PublishFirm_ResponseRemote(&c_gprs);
    // Gửi dữ liệu chính của Boat
    Sim_MqttSendBoatCmdStep();

    // gửi dữ liệu hearbeat
    switch(simStep)
    {
        case 0:
            NextStep();
            break;
        case 1:
            if (vcu_state==VCU_STATE_CAN || vcu_state==VCU_STATE_PHYSICAL) {
                if (simCycleSend!=CYCLE_SEND_FAST) 
                    simCycleSend=CYCLE_SEND_FAST;
            } else {
                if (simCycleSend!=CYCLE_SEND)
                    simCycleSend=CYCLE_SEND;
            }
            if(Wait(simCycleSend)||simRequireSendImmediate)
            {
                if(simRequireSendImmediate)
                {
                    simRequireSendImmediate=0;
                    simCycleSend=2;
                }
                Client_Ping_Boat(&c_gprs);
                //NextStep();
            }
            break;
        // case 2:
        //     if(Sim_CheckMsgMqtt(LWMQTT_PUBACK_PACKET)==CYRET_SUCCESS)
        //     {
        //         return -1;
        //     }
        //     else if(Wait(3))// đợi 3s
        //     {
        //         // không có phản hồi từ phía server ,tiếp tục gửi thêm 1 lần nữa
        //         Client_Ping_Boat(&c_gprs);
        //         NextStep();
        //     }
        //     break;
        // case 3:
        //     if(Sim_CheckMsgMqtt(LWMQTT_PUBACK_PACKET)==CYRET_SUCCESS)
        //     {
        //         return -1;
        //     }
        //     else if(Wait(3))
        //     {
        //         Sim1_CountToStepTo(COUNT_SIM_DISCONNECT,0);
        //         return 1;
        //     }
        //     break;
        //     default:
        //         NextStep();
    }
    return 0;
}

void Sim_MqttSendBoatCmdStep(void)
{
    // Duyệt qua TẤT CẢ các CMD, kiểm tra cờ và gửi những CMD có cờ được set
    for (uint8_t idx = 0; idx < BOAT_CMD_COUNT; idx++) {
        if (boat_cycle_param.cmdReadyFlag[idx]) {
            // Tính CMD trực tiếp từ enum 
            uint8_t cmd = (uint8_t)(CMD1_DRIVESTATUS + idx);
            Client_PublishBoat_Mains(&c_gprs, cmd);
            boat_cycle_param.cmdReadyFlag[idx] = 0;  // Clear cờ sau khi gửi
        }
    }
}

int Sim_Sleep(void)
{
    simStatus=Sim_Sleep_Status;
    sim_signal_quanlity=200;// chạy chế độ mạng lan , dừng sim
    switch(simStep)
    {
        case 0:
            PWRKEY_Off();
            if(Wait(1))
                NextStep();
            break;
        case 1:
            PWRKEY_On();// Reset
            if(Wait(1))
                NextStep();
            break;
        case 2:
            PWRKEY_Off();
            if(Wait(5))
				NextStep();
            break;
        case 3:
            if(Network_CheckInterfaceStatus(INTERFACE_GPRS)==INTERFACE_NEED_RUN)
            {
                Sim1_CountToStepTo(COUNT_SIM_PWON,0);// restart modul sim
                return 1;
            }
            else
            {

            }
            break;
    }
    return 0;
}
void Sim1_CheckConnectServer(void)
{
    static uint32 timeOutConnectServer=0;
    if(IsSecondChange())
    {
        if(simCount!=COUNT_MQTT_COMUNICATION)
        {
            if(++timeOutConnectServer>=120)
            {
                timeOutConnectServer=0;
                Sim1_CountToStepTo(COUNT_SIM_PWON,2);
                return ;
            }
        }
        else
        {
            timeOutConnectServer=0;
        }
    }
}

void Sim_SwitchModeSecurity(void)
{
    simModeSecurity=!simModeSecurity;
}

void Sim_WakeUp(void)
{
    if(simCount!=COUNT_SIM_SLEEP)
    {
        if(Network_CheckInterfaceStatus(INTERFACE_GPRS)==INTERFACE_NEED_SLEEP)
        {
            Sim1_CountToStepTo(COUNT_SIM_SLEEP,0);

        }
    }
}

uint8_t Sim_GetModeSecurity(void)
{
    return simModeSecurity;
}
uint8_t IsSimConnectedServer(void)
{
    return (simCount==COUNT_MQTT_COMUNICATION);
}

void Sim_RequireSendImmediate(void)
{
    simRequireSendImmediate=1;
}


/* [] END OF FILE */
