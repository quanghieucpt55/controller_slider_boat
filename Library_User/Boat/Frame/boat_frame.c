#include "global.h"
#include "sim.h"
#include "string.h"
#include "jikong_can.h"
#include "Can_Slider.h"
#include "swconfig.h"
#include "realtime.h"
#include "boat_frame.h"
#include "boat_log.h"

#ifndef BOAT_DEVICE_ID
#define BOAT_DEVICE_ID 0x01u
#endif

#define HEADER_FRAME 0xF3
#define HEADER_EVENT 0xF1

/*
 * GPS giả lập để test bản đồ web:
 */
#ifndef BOAT_GPS_TEST_RAMP
#define BOAT_GPS_TEST_RAMP 1
#endif

/* 3 hàm build payload chính cho driver, sim/gps, bms*/
static uint8_t *build_driver_data(uint8_t **p, uint8_t *end);
static uint8_t *build_sim_gps_data(uint8_t **p, uint8_t *end);
static uint8_t *build_bms_data(uint8_t **p, uint8_t *end);

uint32_t Boat_Frame_Event(uint8_t *buf, uint32_t size) {
    boat_event_log_t item_log;
    if (BoatEventLog_Dequeue((uint8_t *)&item_log)==0) {
        return 0;
    }
    uint8_t * ptr_start = buf;
    uint8_t * ptr_end = buf + size;
    
    /* 1. Header: 3 byte HEADER_FRAME */
    WriteChar_Buffer(&ptr_start, HEADER_EVENT, ptr_end);
    WriteChar_Buffer(&ptr_start, HEADER_EVENT, ptr_end);
    WriteChar_Buffer(&ptr_start, HEADER_EVENT, ptr_end);
    
    /* 2. Dữ liệu event log: date_time (6 bytes), event (1 byte), event_data (4 bytes) */
    if (WriteBuffer(&ptr_start, item_log.date_time, sizeof(item_log.date_time), ptr_end) != CYRET_SUCCESS) return 0;
    if (WriteChar_Buffer(&ptr_start, (uint8_t)item_log.event, ptr_end) != CYRET_SUCCESS) return 0;
    if (WriteInt32_Buffer(&ptr_start, item_log.event_data, ptr_end) != CYRET_SUCCESS) return 0;
    
    /* 3. Checksum */
    buf[size - 1] = Frame_CalculationChecksum(buf, size-1, 0xFF);
    return size;
}

uint32_t Boat_Frame_Ping_Complete(uint8_t * buf,uint32_t size)
{
    static uint8_t counter = 0; 
    uint8_t * ptr_start = buf;
    uint8_t * ptr_end = buf + size;

    /* 1. Header: 3 byte HEADER_FRAME */
    WriteChar_Buffer(&ptr_start, HEADER_FRAME, ptr_end);
    WriteChar_Buffer(&ptr_start, HEADER_FRAME, ptr_end);
    WriteChar_Buffer(&ptr_start, HEADER_FRAME, ptr_end);
    
    /* 2. Thời gian hiện tại: 6 byte */
    if (ValidTime(&currentTime)) {
        WriteBuffer(&ptr_start, (uint8_t *)&currentTime, sizeof(currentTime), ptr_end);
    } else return 0;
    
    /* 3. Phiên bản phần mềm: 3 byte */
    WriteChar_Buffer(&ptr_start, VERSION_NUMBER_0, ptr_end);
    WriteChar_Buffer(&ptr_start, VERSION_NUMBER_1, ptr_end);
    WriteChar_Buffer(&ptr_start, VERSION_NUMBER_2, ptr_end);
    
    /* 4. Dữ liệu driver */
    if (build_driver_data(&ptr_start, ptr_end) == NULL) return 0;

    /* 5. Dữ liệu SIM/GPS */
    if (build_sim_gps_data(&ptr_start, ptr_end) == NULL) return 0;

    /* 6. Dữ liệu BMS */
    if (build_bms_data(&ptr_start, ptr_end) == NULL) return 0;

    // uint8_t * ptr_data = buf + 9;
    // uint8_t * ptr_data_end = buf + 299;
    
    // uint8_t value = 1;
    // for (uint8_t *p = ptr_data; p < ptr_data_end; p++) {
    //     *p = value++;
    // }
    
    
    buf[size-1]=Frame_CalculationChecksum(buf,size-1,0xFF);

    return size;
}

static inline uint8_t encode_temp_u8(int8_t t_c)
{
    /* Nếu không khả dụng, trả 0xFF */
    return (t_c <= -120) ? 0xFFu : (uint8_t)t_c;
}

/* Hàm 1: Dữ liệu driver - Trạng thái hướng/phanh, RPM, nhiệt, điện áp/dòng, mode */
static uint8_t *build_driver_data(uint8_t **p, uint8_t *end)
{
    /* Hướng: 0=mo, 1=thuận, 2=nghịch */
    uint8_t direction = 0;
    if (can_slider.slider_1.vehicle_mode.forward) direction = 1;
    else if (can_slider.slider_1.vehicle_mode.reverse) direction = 2;
    if (WriteChar_Buffer(p, direction, end) != CYRET_SUCCESS) return NULL;

    /* Phanh: 1=Kích hoạt, 0=Cancel */
    if (WriteChar_Buffer(p, (uint8_t)(can_slider.slider_1.vehicle_mode.brake ? 1 : 0), end) != CYRET_SUCCESS) return NULL;

    /* Tốc độ động cơ (RPM) */
    if (WriteInt16_Buffer(p, can_slider.slider_1.motor_rpm, end) != CYRET_SUCCESS) return NULL;

    /* Nhiệt độ động cơ (°C) và controller (°C) */
    if (WriteChar_Buffer(p, (uint8_t)can_slider.slider_1.motor_temp, end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, (uint8_t)can_slider.slider_1.controller_temp, end) != CYRET_SUCCESS) return NULL;

    /* Điện áp (0.1V), Dòng (0.1A), Chế độ điều khiển */
    if (WriteInt16_Buffer(p, (uint16_t)(can_slider.slider_2.battery_voltage * 10.0f), end) != CYRET_SUCCESS) return NULL;
    if (WriteInt16_Buffer(p, (uint16_t)(int16_t)(can_slider.slider_2.dc_current * 10.0f), end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, can_slider.slider_2.control_mode, end) != CYRET_SUCCESS) return NULL;
    return *p;
}

/* Hàm 2: Dữ liệu SIM/GPS - Chất lượng sóng SIM + GPS data */
static uint8_t *build_sim_gps_data(uint8_t **p, uint8_t *end)
{
    /* Chất lượng sóng SIM (%) */
    uint8_t sim_signal = Sim_SignalQuanlityPercent();

    if (WriteChar_Buffer(p, sim_signal, end) != CYRET_SUCCESS) return NULL;

    int32_t lat_out = gpsData.latitude;
    int32_t lon_out = gpsData.longitude;

#if (BOAT_GPS_TEST_RAMP)
    static int32_t sim_lat = 0;
    static int32_t sim_lon = 0;
    static uint8_t sim_inited = 0;
    const int32_t step = 5000; /* 5000 / 1e7 = 0.0005 độ */

    if (!sim_inited) {
        sim_lat = gpsData.latitude;
        sim_lon = gpsData.longitude;
        sim_inited = 1;
    }

    sim_lat += step;
    sim_lon += step;
    lat_out = sim_lat;
    lon_out = sim_lon;
#endif

    /* Vĩ độ (x10^7, int32) */
    if (WriteInt32_Buffer(p, (uint32_t)lat_out, end) != CYRET_SUCCESS) return NULL;
    
    /* Kinh độ (x10^7, int32) */
    if (WriteInt32_Buffer(p, (uint32_t)lon_out, end) != CYRET_SUCCESS) return NULL;
    
    /* Tốc độ (x10^2 km/h, u16) */
    if (WriteInt16_Buffer(p, gpsData.sog, end) != CYRET_SUCCESS) return NULL;
    
    /* Độ lệch hướng (x10^2 độ, u16) */
    if (WriteInt16_Buffer(p, gpsData.cog, end) != CYRET_SUCCESS) return NULL;
    
    /* Trạng thái (1 bytes) */
    // uint8_t status_bytes = gpsData.state;
    // if (WriteChar_Buffer(p, status_bytes, end) != CYRET_SUCCESS) return NULL;
    
    return *p;
}

/* Hàm 3: Dữ liệu BMS - Tất cả thông tin về pin và BMS */
static uint8_t *build_bms_data(uint8_t **p, uint8_t *end)
{
    /* SoC, tổng điện áp, tổng dòng điện (0.1A) */
    if (WriteChar_Buffer(p, bms.batt1.soc_percent, end) != CYRET_SUCCESS) return NULL;
    if (WriteInt16_Buffer(p, (uint16_t)(bms.batt1.voltage_V * 10.0f), end) != CYRET_SUCCESS) return NULL;
    if (WriteInt16_Buffer(p, (uint16_t)(int16_t)(bms.batt1.current_A * 10.0f), end) != CYRET_SUCCESS) return NULL;

    /* Dung lượng còn lại, đầy, đã xả, số chu kỳ (0.1Ah) */
    if (WriteInt16_Buffer(p, (uint16_t)(bms.batt2.cap_remain_Ah * 10.0f), end) != CYRET_SUCCESS) return NULL;
    if (WriteInt16_Buffer(p, (uint16_t)(bms.batt2.cap_full_Ah * 10.0f), end) != CYRET_SUCCESS) return NULL;
    if (WriteInt16_Buffer(p, (uint16_t)(bms.batt2.cycle_cap_Ah * 10.0f), end) != CYRET_SUCCESS) return NULL;
    if (WriteInt16_Buffer(p, bms.batt2.cycle_count, end) != CYRET_SUCCESS) return NULL;

    /* Vmax/Vmin và nhiệt độ cell */
    if (WriteInt16_Buffer(p, bms.cellVolt.max_cell_mV, end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, bms.cellVolt.max_cell_no, end) != CYRET_SUCCESS) return NULL;
    if (WriteInt16_Buffer(p, bms.cellVolt.min_cell_mV, end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, bms.cellVolt.min_cell_no, end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, (uint8_t)bms.cellTemp.max_temp_C, end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, bms.cellTemp.max_no, end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, (uint8_t)bms.cellTemp.min_temp_C, end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, bms.cellTemp.min_no, end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, (uint8_t)bms.cellTemp.avg_temp_C, end) != CYRET_SUCCESS) return NULL;

    /* Nhiệt độ cảm biến 1/2/3 (u8), bỏ qua nếu thiếu -> 0xFF */
    for (uint8_t i = 0; i < 5; i++) {
        if (WriteChar_Buffer(p, encode_temp_u8(bms.allTemp.temp[i]), end) != CYRET_SUCCESS) return NULL;
    }

    /* Thời gian chạy BMS (u32 giây), Dòng gia nhiệt (0.1A, u16), SOH (u8) */
    if (WriteInt32_Buffer(p, bms.info.runtime_s, end) != CYRET_SUCCESS) return NULL;
    if (WriteInt16_Buffer(p, (uint16_t)(bms.info.heat_current_A * 10.0f), end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, bms.info.soh_percent, end) != CYRET_SUCCESS) return NULL;

    /* MOS trạng thái sạc/xả/cân bằng/sưởi + TT sạc + TT ACC (u8) */
    if (WriteChar_Buffer(p, (uint8_t)(bms.swSta.chgMOS ? 1 : 0), end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, (uint8_t)(bms.swSta.dchgMOS ? 1 : 0), end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, (uint8_t)(bms.swSta.balance ? 1 : 0), end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, (uint8_t)(bms.swSta.heat ? 1 : 0), end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, (uint8_t)(bms.swSta.chgPlug ? 1 : 0), end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, (uint8_t)(bms.swSta.acc ? 1 : 0), end) != CYRET_SUCCESS) return NULL;

    /* Danh sách cell voltage: N (u8), Vcell[i] mV u16 */
    uint8_t n = bms.cellArray.cell_count;
    if (WriteChar_Buffer(p, n, end) != CYRET_SUCCESS) return NULL;
    uint8_t max_cells = (uint8_t)((end - *p) / 2);
    uint8_t count = (n < max_cells) ? n : max_cells;
    for (uint8_t i = 0; i < count; i++) {
        if (WriteInt16_Buffer(p, bms.cellArray.cell_mV[i], end) != CYRET_SUCCESS) return NULL;
    }

    /* Thông số sạc yêu cầu: Ureq 0.1V u16, Ireq 0.1A u16, switch sạc (u8), mode (u8) */
    if (WriteInt16_Buffer(p, (uint16_t)(bms.chgInfo.chg_volt_V * 10.0f), end) != CYRET_SUCCESS) return NULL;
    if (WriteInt16_Buffer(p, (uint16_t)(bms.chgInfo.chg_curr_A * 10.0f), end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, bms.chgInfo.chg_dev_sw, end) != CYRET_SUCCESS) return NULL;
    if (WriteChar_Buffer(p, bms.chgInfo.chg_and_heat, end) != CYRET_SUCCESS) return NULL;

    /* Warning code (u16) - Ánh xạ các cảnh báo BMS vào bit 1-9 */
    uint16_t warning_code_bms = 0;
    if (bms.almInfo.cell_overvolt > 0)  warning_code_bms |= (1U << 1);  // Bit 1: Cell quá áp
    if (bms.almInfo.cell_undervolt > 0) warning_code_bms |= (1U << 2);  // Bit 2: Cell thấp áp
    if (bms.almInfo.delta_over > 0)     warning_code_bms |= (1U << 3);  // Bit 3: Chênh lệch điện áp quá lớn
    if (bms.almInfo.dchg_oc > 0)        warning_code_bms |= (1U << 4);  // Bit 4: Dòng xả quá mức
    if (bms.almInfo.chg_oc > 0)         warning_code_bms |= (1U << 5);  // Bit 5: Dòng sạc quá mức
    if (bms.almInfo.temp_high > 0)      warning_code_bms |= (1U << 6);  // Bit 6: Nhiệt độ cao
    if (bms.almInfo.temp_low > 0)       warning_code_bms |= (1U << 7);  // Bit 7: Nhiệt độ thấp
    if (bms.almInfo.soc_low > 0)        warning_code_bms |= (1U << 8);  // Bit 8: Dung lượng thấp
    if (bms.almInfo.comm_fault > 0)     warning_code_bms |= (1U << 9);  // Bit 9: Lỗi truyền thông
    if (WriteInt16_Buffer(p, warning_code_bms, end) != CYRET_SUCCESS) return NULL;
    
    return *p;
}


