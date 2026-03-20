#include "gps_rmc.h"

// gọi 1 lần khi init
void GPS_Init(void)
{
    gps_rmc_init();
}

// gọi khi nhận 1 byte từ UART (IRQ/DMA)
void GPS_OnUartByte(uint8_t b)
{
    gps_rmc_feed_byte(b);
}

// gọi trong vòng lặp main
void GPS_Task(void)
{
    // parse các câu đã gom
    gps_rmc_process();

    // lấy gói GPS mới nhất
    gnss_data_t gps;
    if (gps_rmc_pop(&gps))
    {
        // haveData: 1 nếu RMC status='A'
        // lat/lon: abs(deg)*1e7, N/S và E/W ở northSouth/eastWest
        // speed: km/h*100, cog: deg*100

        // TODO: dùng gps cho hiển thị/điều khiển
    }
}
