#pragma once
#include <stdint.h>
#include <stddef.h>
#include "gps_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parser NMEA RMC dạng stream:
 * - IRQ/DMA: gom byte (gps_rmc_feed_byte)
 * - Main: parse (gps_rmc_process) và lấy dữ liệu (gps_rmc_pop)
 */

void gps_rmc_init(void);

/**
 * Feed 1 byte vào bộ gom câu.
 * Trả 1 nếu vừa kết thúc 1 dòng NMEA ('\n'), ngược lại 0.
 */
int  gps_rmc_feed_byte(uint8_t b);

/**
 * Parse các câu đã gom.
 * Trả 1 nếu parse được ít nhất 1 câu RMC hợp lệ.
 */
int  gps_rmc_process(void);

/** Lấy gói GPS mới nhất. Trả 1 nếu có dữ liệu mới. */
int  gps_rmc_pop(gnss_data_t *out);

/** Xoá cờ dữ liệu mới. */
void gps_rmc_clear_new(void);

#ifdef __cplusplus
}
#endif
