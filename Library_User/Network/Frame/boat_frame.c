#include "global.h"
#include "sim.h"
#include "string.h"
#include "jikong_can.h"
#include "Can_Slider.h"
#include "swconfig.h"
#include "realtime.h"
#include "boat_frame.h"

/* Định dạng frame:
 * Byte0: ID Boat
 * Byte1: CMD
 * Byte2: Len (Số byte của payload)
 * Byte3..: Payload (Len byte)
 * Byte(N-1): Checksum (0xFF - tổng các byte trước đó)
 */
#ifndef BOAT_DEVICE_ID
#define BOAT_DEVICE_ID 0x01u
#endif

e_boat_frame_cmd boat_frame_cmd;

enum {
    CMD_BOAT_1_STATUS_DRIVE = 0x01,
    CMD_BOAT_2_PACK_BASIC   = 0x02,
    CMD_BOAT_3_CAPACITY     = 0x03,
    CMD_BOAT_4_CELL_EXTREME = 0x04,
    CMD_BOAT_5_TEMP         = 0x05,
    CMD_BOAT_6_RUNTIME      = 0x06,
    CMD_BOAT_7_MOS_STATE    = 0x07,
    CMD_BOAT_8_CELL_LIST    = 0x08,
    CMD_BOAT_9_CHARGE_REQ   = 0x09,
    CMD_BOAT_10_GPS_SPEED   = 0x0A,
    CMD_BOAT_11_WARN_ERR    = 0x0B,
};

/* nạp dữ liệu kiểu little-endian vào payload với kiểm tra tràn */
static inline uint8_t *put_u8(uint8_t *p, uint8_t *end, uint8_t v) {
    if (p >= end) return NULL;
    *p++ = v;
    return p;
}

static inline uint8_t *put_u16(uint8_t *p, uint8_t *end, uint16_t v) {
    if ((end - p) < 2) return NULL;
    *p++ = (uint8_t)(v & 0xFF);
    *p++ = (uint8_t)(v >> 8);
    return p;
}

static inline uint8_t *put_i16(uint8_t *p, uint8_t *end, int16_t v) {
    return put_u16(p, end, (uint16_t)v);
}

static inline uint8_t encode_temp_u8(int8_t t_c)
{
    /* Nếu không khả dụng, trả 0xFF; ngược lại cast về u8 (đã offset sẵn) */
    return (t_c <= -120) ? 0xFFu : (uint8_t)t_c;
}

static uint32_t BoatFrame_Build(uint8_t cmd, uint8_t *payload, uint8_t payload_len,
                                uint8_t *out, uint32_t out_size) {
    // Cần: 3 byte header (ID + CMD + LEN) + payload + 1 byte checksum
    if (out_size < (uint32_t)payload_len + 4) return 0;
    out[0] = BOAT_DEVICE_ID;
    out[1] = cmd;
    out[2] = payload_len;
    memcpy(&out[3], payload, payload_len);
    
    // Tính checksum cho toàn bộ frame (không bao gồm byte checksum)
    uint32_t frame_size = payload_len + 3;
    out[frame_size] = Frame_CalculationChecksum(out, frame_size, 0xFF);
    
    return frame_size + 1; // Trả về độ dài bao gồm checksum
}

/* CMD1: Trạng thái hướng/phanh, RPM, nhiệt, điện áp/dòng, mode */
static uint8_t *build_cmd1_payload(uint8_t *p, uint8_t *end)
{
    /* Hướng: 0=neutral, 1=forward, 2=reverse */
    uint8_t direction = 0;
    if (can_slider.slider_1.vehicle_mode.forward) direction = 1;
    else if (can_slider.slider_1.vehicle_mode.reverse) direction = 2;
    p = put_u8(p, end, direction);
    if (!p) return 0;

    /* Phanh: 1=Effective, 0=Cancel */
    p = put_u8(p, end, (uint8_t)(can_slider.slider_1.vehicle_mode.brake ? 1 : 0));
    if (!p) return 0;

    /* RPM */
    p = put_u16(p, end, can_slider.slider_1.motor_rpm);
    if (!p) return 0;

    /* Nhiệt độ động cơ (°C) và controller (°C), i8 */
    p = put_u8(p, end, (uint8_t)can_slider.slider_1.motor_temp);
    if (!p) return 0;
    p = put_u8(p, end, (uint8_t)can_slider.slider_1.controller_temp);
    if (!p) return 0;

    /* Điện áp 0.1V, Dòng 0.1A (signed), Mode điều khiển */
    p = put_u16(p, end, (uint16_t)(can_slider.slider_2.battery_voltage * 10.0f));
    if (!p) return 0;
    p = put_i16(p, end, (int16_t)(can_slider.slider_2.dc_current * 10.0f));
    if (!p) return 0;
    p = put_u8(p, end, can_slider.slider_2.control_mode);
    return p;
}

/* CMD2: SoC, tổng điện áp, tổng dòng điện (0.1A) */
static uint8_t *build_cmd2_payload(uint8_t *p, uint8_t *end)
{
    p = put_u8(p, end, bms.batt1.soc_percent);
    if (!p) return 0;
    p = put_u16(p, end, (uint16_t)(bms.batt1.voltage_V * 10.0f));
    if (!p) return 0;
    p = put_i16(p, end, (int16_t)(bms.batt1.current_A * 10.0f)); /* 0.1A signed */
    return p;
}

/* CMD3: Dung lượng còn lại, đầy, đã xả, số chu kỳ (0.1Ah) */
static uint8_t *build_cmd3_payload(uint8_t *p, uint8_t *end)
{
    p = put_u16(p, end, (uint16_t)(bms.batt2.cap_remain_Ah * 10.0f));
    if (!p) return 0;
    p = put_u16(p, end, (uint16_t)(bms.batt2.cap_full_Ah * 10.0f));
    if (!p) return 0;
    p = put_u16(p, end, (uint16_t)(bms.batt2.cycle_cap_Ah * 10.0f));
    if (!p) return 0;
    p = put_u16(p, end, bms.batt2.cycle_count);
    return p;
}

/* CMD4: Vmax/Vmin và nhiệt độ cell */
static uint8_t *build_cmd4_payload(uint8_t *p, uint8_t *end)
{
    p = put_u16(p, end, bms.cellVolt.max_cell_mV);
    if (!p) return 0;
    p = put_u8(p, end, bms.cellVolt.max_cell_no);
    if (!p) return 0;
    p = put_u16(p, end, bms.cellVolt.min_cell_mV);
    if (!p) return 0;
    p = put_u8(p, end, bms.cellVolt.min_cell_no);
    if (!p) return 0;

    p = put_u8(p, end, (uint8_t)bms.cellTemp.max_temp_C);
    if (!p) return 0;
    p = put_u8(p, end, bms.cellTemp.max_no);
    if (!p) return 0;
    p = put_u8(p, end, (uint8_t)bms.cellTemp.min_temp_C);
    if (!p) return 0;
    p = put_u8(p, end, bms.cellTemp.min_no);
    if (!p) return 0;
    p = put_u8(p, end, (uint8_t)bms.cellTemp.avg_temp_C);
    return p;
}

/* CMD5: Nhiệt độ cảm biến 1/2/3 (u8), bỏ qua nếu thiếu -> 0xFF */
static uint8_t *build_cmd5_payload(uint8_t *p, uint8_t *end)
{
    /* Dùng allTemp.temp[0..2]; giá trị -127 coi như không khả dụng */
    for (uint8_t i = 0; i < 3; i++) {
        p = put_u8(p, end, encode_temp_u8(bms.allTemp.temp[i]));
        if (!p) return 0;
    }
    return p;
}

/* CMD6: Thời gian chạy BMS (u32 giây), Dòng gia nhiệt (0.1A, u16), SOH (u8) */
static uint8_t *build_cmd6_payload(uint8_t *p, uint8_t *end)
{
    if ((end - p) < 4) return 0;
    /* runtime_s là uint32 */
    memcpy(p, &bms.info.runtime_s, 4);
    p += 4;
    p = put_u16(p, end, (uint16_t)(bms.info.heat_current_A * 10.0f));
    if (!p) return 0;
    p = put_u8(p, end, bms.info.soh_percent);
    return p;
}

/* CMD7: MOS trạng thái sạc/xả/cân bằng/sưởi + TT sạc + TT ACC (u8) */
static uint8_t *build_cmd7_payload(uint8_t *p, uint8_t *end)
{
    p = put_u8(p, end, (uint8_t)(bms.swSta.chgMOS ? 1 : 0));
    if (!p) return 0;
    p = put_u8(p, end, (uint8_t)(bms.swSta.dchgMOS ? 1 : 0));
    if (!p) return 0;
    p = put_u8(p, end, (uint8_t)(bms.swSta.balance ? 1 : 0));
    if (!p) return 0;
    p = put_u8(p, end, (uint8_t)(bms.swSta.heat ? 1 : 0));
    if (!p) return 0;
    p = put_u8(p, end, (uint8_t)(bms.swSta.chgPlug ? 1 : 0));
    if (!p) return 0;
    p = put_u8(p, end, (uint8_t)(bms.swSta.acc ? 1 : 0));
    return p;
}

/* CMD8: Danh sách cell voltage: N (u8), Vcell[i] mV u16 */
static uint8_t *build_cmd8_payload(uint8_t *p, uint8_t *end)
{
    uint8_t n = bms.cellArray.cell_count;
    if (!n || n > 25) n = 0;
    p = put_u8(p, end, n);
    if (!p) return 0;
    uint8_t max_cells = (uint8_t)((end - p) / 2);
    uint8_t count = (n < max_cells) ? n : max_cells;
    for (uint8_t i = 0; i < count; i++) {
        p = put_u16(p, end, bms.cellArray.cell_mV[i]);
        if (!p) return 0;
    }
    return p;
}

/* CMD9: Thông số sạc yêu cầu: Ureq 0.1V u16, Ireq 0.1A u16, switch sạc (u8), mode (u8) */
static uint8_t *build_cmd9_payload(uint8_t *p, uint8_t *end)
{
    p = put_u16(p, end, (uint16_t)(bms.chgInfo.chg_volt_V * 10.0f));
    if (!p) return 0;
    p = put_u16(p, end, (uint16_t)(bms.chgInfo.chg_curr_A * 10.0f));
    if (!p) return 0;
    p = put_u8(p, end, bms.chgInfo.chg_dev_sw);
    if (!p) return 0;
    p = put_u8(p, end, bms.chgInfo.chg_and_heat);
    return p;
}

/* CMD10: GPS/Speed placeholder: Kinh độ, Vĩ độ (x10^7, int32), Tốc độ (km/h, u8) */
static uint8_t *build_cmd10_payload(uint8_t *p, uint8_t *end)
{
    /* Chưa có nguồn dữ liệu GPS, tạm thời gửi 0 */
    int32_t lon = 0;
    int32_t lat = 0;
    uint8_t speed = 0;
    if ((end - p) < 8) return 0;
    memcpy(p, &lon, 4); p += 4;
    memcpy(p, &lat, 4); p += 4;
    p = put_u8(p, end, speed);
    return p;
}

/* CMD11: Warning code, Error code (u8) */
static uint8_t *build_cmd11_payload(uint8_t *p, uint8_t *end)
{
    /* Gói gọn từ bms_fault: low byte = warn, high byte = err */
    uint16_t bms_fault = 0;
    bms_fault |= (bms.bmsErrInfo.cell_overvolt     & 0x01) << 0;
    bms_fault |= (bms.bmsErrInfo.cell_undervolt    & 0x01) << 1;
    bms_fault |= (bms.bmsErrInfo.pack_overvolt     & 0x01) << 2;
    bms_fault |= (bms.bmsErrInfo.pack_undervolt    & 0x01) << 3;
    bms_fault |= (bms.bmsErrInfo.chg_overcurrent   & 0x01) << 4;
    bms_fault |= (bms.bmsErrInfo.dchg_overcurrent  & 0x01) << 5;
    bms_fault |= (bms.bmsErrInfo.mos_overtemp      & 0x01) << 6;
    bms_fault |= (bms.bmsErrInfo.comm_inner_fault  & 0x01) << 7;
    bms_fault |= (bms.bmsErrInfo.chg_mos_fault     & 0x01) << 8;
    bms_fault |= (bms.bmsErrInfo.dchg_mos_fault    & 0x01) << 9;

    uint8_t warn = (uint8_t)(bms_fault & 0xFF);
    uint8_t err  = (uint8_t)((bms_fault >> 8) & 0xFF);
    p = put_u8(p, end, warn);
    if (!p) return 0;
    p = put_u8(p, end, err);
    return p;
}

uint32_t Boat_Frame_Build(uint8_t cmd, uint8_t *buf, uint32_t size)
{
    uint8_t payload[64];
    uint8_t *p = payload;
    uint8_t *end = payload + sizeof(payload);

    switch (cmd)
    {
    case CMD_BOAT_1_STATUS_DRIVE:
        p = build_cmd1_payload(p, end);
        break;
    case CMD_BOAT_2_PACK_BASIC:
        p = build_cmd2_payload(p, end);
        break;
    case CMD_BOAT_3_CAPACITY:
        p = build_cmd3_payload(p, end);
        break;
    case CMD_BOAT_4_CELL_EXTREME:
        p = build_cmd4_payload(p, end);
        break;
    case CMD_BOAT_5_TEMP:
        p = build_cmd5_payload(p, end);
        break;
    case CMD_BOAT_6_RUNTIME:
        p = build_cmd6_payload(p, end);
        break;
    case CMD_BOAT_7_MOS_STATE:
        p = build_cmd7_payload(p, end);
        break;
    case CMD_BOAT_8_CELL_LIST:
        p = build_cmd8_payload(p, end);
        break;
    case CMD_BOAT_9_CHARGE_REQ:
        p = build_cmd9_payload(p, end);
        break;
    case CMD_BOAT_10_GPS_SPEED:
        p = build_cmd10_payload(p, end);
        break;
    case CMD_BOAT_11_WARN_ERR:
        p = build_cmd11_payload(p, end);
        break;
    default:
        return 0;
    }

    if (!p) return 0;
    uint8_t payload_len = (uint8_t)(p - payload);
    return BoatFrame_Build(cmd, payload, payload_len, buf, size);
}

/* Wrapper giữ tên cũ */
uint32_t Boat_Frame_Cmd1_DriveStatus(uint8_t * buf,uint32_t size)
{
    return Boat_Frame_Build(CMD_BOAT_1_STATUS_DRIVE, buf, size);
}

uint32_t Boat_Frame_Cmd2_PackBasic(uint8_t * buf,uint32_t size)
{
    return Boat_Frame_Build(CMD_BOAT_2_PACK_BASIC, buf, size);
}

uint32_t Boat_Frame_Cmd3_PackCapacity(uint8_t * buf,uint32_t size)
{
    return Boat_Frame_Build(CMD_BOAT_3_CAPACITY, buf, size);
}

uint32_t Boat_Frame_Cmd4_CellExtreme(uint8_t * buf,uint32_t size)
{
    return Boat_Frame_Build(CMD_BOAT_4_CELL_EXTREME, buf, size);
}

uint32_t Boat_Frame_Cmd5_Temp(uint8_t * buf,uint32_t size)
{
    return Boat_Frame_Build(CMD_BOAT_5_TEMP, buf, size);
}

uint32_t Boat_Frame_Cmd6_Runtime(uint8_t * buf,uint32_t size)
{
    return Boat_Frame_Build(CMD_BOAT_6_RUNTIME, buf, size);
}

uint32_t Boat_Frame_Cmd7_MosState(uint8_t * buf,uint32_t size)
{
    return Boat_Frame_Build(CMD_BOAT_7_MOS_STATE, buf, size);
}

uint32_t Boat_Frame_Cmd8_CellList(uint8_t * buf,uint32_t size)
{
    return Boat_Frame_Build(CMD_BOAT_8_CELL_LIST, buf, size);
}

uint32_t Boat_Frame_Cmd9_ChargeReq(uint8_t * buf,uint32_t size)
{
    return Boat_Frame_Build(CMD_BOAT_9_CHARGE_REQ, buf, size);
}

uint32_t Boat_Frame_Cmd10_GpsSpeed(uint8_t * buf,uint32_t size)
{
    return Boat_Frame_Build(CMD_BOAT_10_GPS_SPEED, buf, size);
}

uint32_t Boat_Frame_Cmd11_WarnErr(uint8_t * buf,uint32_t size)
{
    return Boat_Frame_Build(CMD_BOAT_11_WARN_ERR, buf, size);
}

uint32_t Boat_Frame_Ping_Complete(uint8_t * buf,uint32_t size)
{
    uint8_t * ptr_start=buf;
    uint8_t * ptr_end=buf+size;

    // gửi thời gian hiện tại
    WriteBuffer(&ptr_start,(uint8_t *)&currentTime,sizeof(currentTime),ptr_end);//6
    // phiên bản phần mềm
    WriteChar_Buffer(&ptr_start,VERSION_NUMBER_0,ptr_end);
    WriteChar_Buffer(&ptr_start,VERSION_NUMBER_1,ptr_end);
    WriteChar_Buffer(&ptr_start,VERSION_NUMBER_2,ptr_end);
    
    buf[size-1]=Frame_CalculationChecksum(buf,size-1,0xFF);

    return size;
}

uint32_t Boat_Frame_Mains_Complete(e_boat_frame_cmd cmd, uint8_t * buf,uint32_t size)
{
    switch (cmd)
    {
    case CMD1_DRIVESTATUS:
        return Boat_Frame_Cmd1_DriveStatus(buf, size);
        break;
    case CMD2_PACKBASIC:
        return Boat_Frame_Cmd2_PackBasic(buf, size);
        break;
    case CMD3_PACKCAPACITY:
        return Boat_Frame_Cmd3_PackCapacity(buf, size);
        break;
    case CMD4_CELLEXTREME:
        return Boat_Frame_Cmd4_CellExtreme(buf, size);
        break;
    case CMD5_TEMP:
        return Boat_Frame_Cmd5_Temp(buf, size);
        break;
    case CMD6_RUNTIME:
        return Boat_Frame_Cmd6_Runtime(buf, size);
        break;
    case CMD7_MOSSTATE:
        return Boat_Frame_Cmd7_MosState(buf, size);
        break;
    case CMD8_CELLLIST:
        return Boat_Frame_Cmd8_CellList(buf, size);
        break;
    case CMD9_CHARGEREQ:
        return Boat_Frame_Cmd9_ChargeReq(buf, size);
        break;
    case CMD10_GPSSPEED:
        return Boat_Frame_Cmd10_GpsSpeed(buf, size);
        break;
    case CMD11_WARNERR:
        return Boat_Frame_Cmd11_WarnErr(buf, size);
        break;
    default:
        break;
    }
}

