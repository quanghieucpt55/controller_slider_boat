#pragma once
#include <stdint.h>
#include "realtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Trạng thái GPS:
 * - 0: lỗi/không có dữ liệu
 * - 1: có dữ liệu nhưng chưa có giờ
 * - 2: đã có giờ UTC hợp lệ
 * - 3: hợp lệ (RMC status='A')
 */
typedef enum
{
    GNSS_STATE_0_ERROR     = 0,
    GNSS_STATE_1_RECV_DATA = 1,
    GNSS_STATE_2_TIME      = 2,
    GNSS_STATE_3_ACTIVE    = 3
} gnss_state_t;

/**
 * Thời gian UTC tối thiểu.
 */
#ifndef CY_PACKED_ATTR
  #if defined(__GNUC__)
    #define CY_PACKED_ATTR __attribute__((packed))
  #else
    #define CY_PACKED_ATTR
  #endif
#endif

typedef struct CY_PACKED_ATTR gnss_data_t
{
    uint8_t   haveData;    // 1: status='A', 0: status='V'
    uint8_t   state;       // xem gnss_state_t
    uint8_t   northSouth;  // 'N'/'S'
    uint8_t   eastWest;    // 'E'/'W'
    realtime_t dateTime;   // thời gian UTC
    uint32_t  latitude;    // abs(lat)*1e7 (độ)
    uint32_t  longitude;   // abs(lon)*1e7 (độ)
    uint32_t  speed;       // km/h*100
    uint32_t  cog;         // độ*100
} gnss_data_t;

#ifdef __cplusplus
}
#endif
