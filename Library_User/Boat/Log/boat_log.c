#include "boat_log.h"
#include "extern_rom.h"
#include "realtime.h"
#include "global.h"
#include "string.h"
#include "queue.h"
#include "stdlib.h"
#include "Can_Slider.h"
#include "jikong_can.h"
#include "VCU_State.h"
#include "main.h"

gps_data_t gpsData = {.latitude = 21.019627694455533*10000000, .longitude = 105.79244619716383*10000000, .sog = 1456, .cog = 180, .state = 3};  

#define SIZE_PACKED_EVENT_LOG (sizeof(boat_package_event_log_t)+2)
#define MAX_ADR_EVENT_LOG (SIZE_PACKED_EVENT_LOG*TOTAL_EVENT_BOAT_LOG+ADR_EXROM_EVENT_LOG)
#define ADR_EVENT_LOG_BASE ADR_EXROM_EVENT_LOG
uint32_t id_rom_event_log=0; // id lớn nhất hiện tại của ex rom
uint16_t adrCurrentRom_EventLog=ADR_EXROM_EVENT_LOG; // địa chỉ adr rom hiện tại (để ghi)
#define SIZE_QUEUE 10
Queue_t queueEventLog;
Queue_t queueRomAddress; // Queue lưu địa chỉ ROM tương ứng với mỗi item trong queueEventLog
current_event_data_t current_event_data = {0};
uint16_t countRomEventLog = 0; // Số lượng item còn lại trong ROM

void BoatEventLog_Init(void)
{
    // Khởi tạo queue và kiểm tra kết quả
    q_init(&queueEventLog,sizeof(boat_event_log_t),SIZE_QUEUE,FIFO,1);
    q_init(&queueRomAddress,sizeof(uint16_t),SIZE_QUEUE,FIFO,1); // Queue lưu địa chỉ ROM
    
    countRomEventLog = 0; // Khởi tạo biến đếm
    // tìm địa chỉ rom hiện tại và đếm số item hợp lệ trong ROM
    uint16_t currentAdr=ADR_EVENT_LOG_BASE;
    for(uint8_t i=0;i<TOTAL_EVENT_BOAT_LOG;i++)
    {
        boat_package_event_log_t item;
        if(ExRom_ReadParam_WithCRC16(currentAdr,(uint8_t *)&item,sizeof(item)))
        {
            if(item.id>id_rom_event_log)// id lớn hơn hiện tại
            {
                id_rom_event_log=item.id;
                adrCurrentRom_EventLog=currentAdr;
            }
            // Đếm số item hợp lệ (id > 0)
            if(item.id > 0)
            {
                countRomEventLog++;
            }
        }
        currentAdr+=SIZE_PACKED_EVENT_LOG;// cộng 2 do cuối mỗi dữ liệu có thêm byte cs
    }
    
    // Load các item còn lại trong ROM vào queue khi khởi động
    BoatEventLog_LoadFromRomToQueue();
}

uint32_t BoatEventLog_GetCurrentPos(void)
{
    return (adrCurrentRom_EventLog-ADR_EVENT_LOG_BASE)/SIZE_PACKED_EVENT_LOG;
}

void BoatEventLog_Write(e_event_log_t event,uint32_t event_data)
{
    if(!ValidTime(&currentTime))
    {
        //đọc thời gian có lỗi thì thoát luôn
        return;
    }
    id_rom_event_log++;
    // Xử lý overflow: nếu id đạt 0xFFFFFFFF, reset về 1
    if(id_rom_event_log == 0)
    {
        id_rom_event_log = 1;
    }
    // khởi tạo dữ liệu
    boat_package_event_log_t item;
    item.id=id_rom_event_log;
    memcpy(item.log.date_time,&currentTime,6);
    item.log.event=event;
    item.log.event_data=event_data;
    
    // Ưu tiên lưu vào queue trước
    // Kiểm tra queue đầy trước khi push
    if(!q_isFull(&queueEventLog))
    {
        q_push(&queueEventLog,&item.log);
        /* Push vào queue thành công, không cần lưu vào ROM
        Item này không có trong ROM nên không cần lưu địa chỉ ROM
        Push một giá trị đặc biệt (0xFFFF) để đánh dấu item này không có trong ROM */
        uint16_t noRomAddress = 0xFFFF;
        q_push(&queueRomAddress, &noRomAddress);
        return;
    }
    
    // Queue đầy, lưu vào ExROM
    // tịnh tiến địa chỉ
    adrCurrentRom_EventLog+=SIZE_PACKED_EVENT_LOG;
    if(adrCurrentRom_EventLog>=MAX_ADR_EVENT_LOG)// quá đia chỉ ->
    {
        adrCurrentRom_EventLog=ADR_EVENT_LOG_BASE;
    }
    // Kiểm tra xem địa chỉ này có item hợp lệ không (để tránh tăng count khi ghi đè)
    boat_package_event_log_t old_item;
    uint8_t had_valid_item = 0;
    if(ExRom_ReadParam_WithCRC16(adrCurrentRom_EventLog, (uint8_t *)&old_item, sizeof(old_item)))
    {
        if(old_item.id > 0)
        {
            had_valid_item = 1; // Địa chỉ này đã có item hợp lệ
        }
    }
    ExRom_SaveParam_WithCRC16(adrCurrentRom_EventLog,(uint8_t *)&item,sizeof(item));
    // Tăng count nếu ghi vào vị trí trống hoặc ghi đè item đã xóa
    if(!had_valid_item)
    {
        countRomEventLog++;
    }
}

// kiểm tra dữ liệu bộ nhớ queue
uint8_t IsHaveMsgQueueBoatEventLog(void)
{
    return q_getCount(&queueEventLog)>0?true:false;
}

// kiểm tra xem ROM còn item nào không
uint8_t IsHaveMsgInRomBoatEventLog(void)
{
    return countRomEventLog > 0 ? true : false;
}

// Hàm xóa item trong ROM (chỉ cần ghi id = 0x00000000 để đánh dấu đã xóa)
static void BoatEventLog_DeleteFromRom(uint16_t adr)
{
    // Kiểm tra xem item này có hợp lệ không trước khi xóa
    boat_package_event_log_t item;
    uint8_t had_valid_item = 0;
    if(ExRom_ReadParam_WithCRC16(adr, (uint8_t *)&item, sizeof(item)))
    {
        if(item.id > 0)
        {
            had_valid_item = 1; // Item này hợp lệ
        }
    }
    // Chỉ cần ghi id = 0x00000000 (4 byte đầu) để đánh dấu đã xóa
    // id hợp lệ phải > 0
    uint32_t deleted_id = 0x00000000;
    uint32_t error = 0;
    EEPRom24C_WriteBytes(adr, (uint8_t *)&deleted_id, sizeof(uint32_t), &error);
    // Giảm count nếu đã xóa item hợp lệ
    if(had_valid_item && countRomEventLog > 0)
    {
        countRomEventLog--;
    }
}

// đọc dữ liệu từ queue và xóa khỏi ROM sau khi lấy ra từ queue thành công
uint8_t BoatEventLog_Dequeue(uint8_t * buf)
{
    // check queue not empty
    if(q_getCount(&queueEventLog)>0)
    {
        if (q_pop(&queueEventLog,buf))
        {
            // Lấy địa chỉ ROM tương ứng và xóa item đó khỏi ROM
            uint16_t romAdr;
            if(q_pop(&queueRomAddress, &romAdr))
            {
                // Chỉ xóa khỏi ROM nếu item này có trong ROM (romAdr != 0xFFFF)
                if(romAdr != 0xFFFF)
                {
                    // Xóa item khỏi ROM sau khi đã lấy ra khỏi queue
                    BoatEventLog_DeleteFromRom(romAdr);
                }
            }
        }
        
        return sizeof(boat_event_log_t);
    }
    return 0;
}

// Load dữ liệu từ ROM vào queue khi queue rỗng
void BoatEventLog_LoadFromRomToQueue(void)
{
    // Scan toàn bộ ROM để tìm các item còn lại (chưa bị xóa)
    uint16_t currentAdr = ADR_EVENT_LOG_BASE;
    uint16_t loaded_count = 0; // Đếm số item đã load vào queue
    for(uint8_t i = 0; i < TOTAL_EVENT_BOAT_LOG && !q_isFull(&queueEventLog); i++)
    {
        boat_package_event_log_t item;
        if(ExRom_ReadParam_WithCRC16(currentAdr, (uint8_t *)&item, sizeof(item)))
        {
            // Item hợp lệ phải có id > 0 (id = 0 là đánh dấu đã xóa)
            if(item.id > 0)
            {
                // Tìm thấy item hợp lệ, load vào queue và lưu địa chỉ ROM
                if(q_push(&queueEventLog, &item.log))
                {
                    // Lưu địa chỉ ROM tương ứng vào queue song song
                    q_push(&queueRomAddress, &currentAdr);
                    loaded_count++;
                }
                else
                {
                    // Queue đã đầy, dừng lại
                    break;
                }
            }
        }
        currentAdr += SIZE_PACKED_EVENT_LOG;
    }
    // Giảm count sau khi load vào queue
    if(loaded_count > 0 && countRomEventLog >= loaded_count)
    {
        countRomEventLog -= loaded_count;
    }
    else if(loaded_count > 0)
    {
        // Trường hợp count không khớp, reset về 0
        countRomEventLog = 0;
    }
}


cystatus BoatEventLog_ReadPacket(boat_package_event_log_t * buf,uint8_t pos)
{
    uint16_t adr_start=pos*SIZE_PACKED_EVENT_LOG+ADR_EVENT_LOG_BASE;
    if(ExRom_ReadParam_WithCRC16(adr_start,(uint8_t *)buf,sizeof(boat_package_event_log_t)))
    {
        return CYRET_SUCCESS;
    }
    return CYRET_EMPTY;
}


void BoatEventLog_Read(uint8_t * buf,uint32_t buf_size,
                        uint8_t pack_start,uint8_t pack_end)
{
    uint8_t totalEvent=0;
    uint8_t * ptr_start=buf+1;// byte đầu dùng lưu tổng số sự kiện
    uint8_t * ptr_end=buf+buf_size;
    uint8_t noOfPacked=pack_end-pack_start;
    uint16_t adr_start=pack_start*SIZE_PACKED_EVENT_LOG+ADR_EVENT_LOG_BASE;
    for(uint8_t i=0;i<noOfPacked;i++)
    {
        boat_package_event_log_t item;
        if(ExRom_ReadParam_WithCRC16(adr_start,(uint8_t *)&item,sizeof(item)))
        {
            WriteBuffer(&ptr_start,item.log.date_time,sizeof(item.log.date_time),ptr_end);
            WriteChar_Buffer(&ptr_start,item.log.event,ptr_end);
            WriteInt32_Buffer(&ptr_start,item.log.event_data,ptr_end);
            totalEvent++;
        }
        adr_start+=SIZE_PACKED_EVENT_LOG;
    }
    buf[0]=totalEvent;
}

void BoatEventUpdate(void) 
{
    if (current_event_data.pow_status != vcu_ctx.outputs.contactor_on) {
        current_event_data.pow_status = vcu_ctx.outputs.contactor_on;
        BoatEventLog_Write(Pow_Status_Driver, current_event_data.pow_status);
    }
    if (current_event_data.err_bms != bms.bmsErrInfo.raw) {
        current_event_data.err_bms = bms.bmsErrInfo.raw;
        BoatEventLog_Write(Error_BMS, current_event_data.err_bms);
    }
    if (current_event_data.err_driver != can_slider.slider_1.error_code) {
        current_event_data.err_driver = can_slider.slider_1.error_code;
        BoatEventLog_Write(Error_Driver, current_event_data.err_driver);
    }
    if (current_event_data.motor_direc != can_slider.motor_direc) {
        current_event_data.motor_direc = can_slider.motor_direc;
        BoatEventLog_Write(ChDirection_Motor, current_event_data.motor_direc);
    } 
    if (can_slider.rpm_accel > 500 && current_event_data.rpm_accel != can_slider.rpm_accel) {
        current_event_data.rpm_accel = can_slider.rpm_accel;
        BoatEventLog_Write(Sudden_Acceleration, current_event_data.rpm_accel);
    }
    if (current_event_data.motor_status != vcu_ctx.outputs.disable_motor) {
        current_event_data.motor_status = vcu_ctx.outputs.disable_motor;
        BoatEventLog_Write(Motor_Status, !current_event_data.motor_status);
    }
    if (current_event_data.gps_status != gpsData.state) {
        current_event_data.gps_status = gpsData.state;
        BoatEventLog_Write(ChStatus_GPS, current_event_data.gps_status);
    }
    if (current_event_data.vcu_state != vcu_ctx.current_state) {
        current_event_data.vcu_state = vcu_ctx.current_state;
        BoatEventLog_Write(ChState_VCU, current_event_data.vcu_state);
    }
}

