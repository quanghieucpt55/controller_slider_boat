/*
 * jikong_can.h
 *
 *  Created on: Oct 28, 2025
 *      Author: quang
 */

 #ifndef BMS_JIKONG_JIKONG_CAN_H_
 #define BMS_JIKONG_JIKONG_CAN_H_

 #ifdef __cplusplus
 extern "C" {
 #endif

 #include "main.h"

 /* ==============================================================
  *                  JIKONG BMS CAN DEFINITIONS
  * ==============================================================
  * Theo tài liệu Jikong BMS-CAN V2.1:
  * - Chuẩn CAN2.0A/B
  * - Baudrate: 250 kbps
  * - Little-Endian cho tất cả dữ liệu nhiều byte
  * - ID gốc mặc định là 0xF4, có thể cộng thêm thiết bị ID nếu có nhiều BMS
  * ==============================================================
  */
 #define BMS_DEVICE_ADDR       0x01        // Địa chỉ node của BMS, mặc định = 0

 /* ==============================================================
  *                      FRAME ID ĐỊNH NGHĨA
  * ==============================================================
  * Mỗi ID tương ứng với một nhóm dữ liệu (Parameter Group) BMS gửi đi
  * theo chu kỳ khác nhau (20–1000ms)
  */
 #define ID_BATT_ST1       0x02F4 + BMS_DEVICE_ADDR        // BATT_ST1: trạng thái pin (điện áp, dòng, SOC)
 #define ID_CELL_VOLT      0x04F4 + BMS_DEVICE_ADDR        // CELL_VOLT: cell cao nhất / thấp nhất
 #define ID_CELL_TEMP      0x05F4 + BMS_DEVICE_ADDR        // CELL_TEMP: nhiệt độ cell cao/thấp/trung bình
 #define ID_ALM_INFO       0x07F4 + BMS_DEVICE_ADDR        // ALM_INFO: cảnh báo mức độ (OV, UV, OC, temp…)

 /* ==============================================================
  *                        CẤU TRÚC DỮ LIỆU
  * ==============================================================
  * Các struct dưới đây được xây dựng tương ứng từng nhóm dữ liệu
  * mô tả trong tài liệu mục 6.x – 7.x
  */

 /* 6.1 BATT_ST1 - Trạng thái điện tổng (ID: 0x02F4) */
 typedef struct {
	 float voltage_V;          // Tổng điện áp pack (0.1V/bit)
	 float current_A;          // Tổng dòng điện (0.1A/bit, offset -400A)
	 uint8_t soc_percent;      // Mức SOC hiện tại (%)
 } BMS_BattSt1_t;

 /* 6.2 CELL_VOLT - Cell điện áp cao/thấp (ID: 0x04F4) */
 typedef struct {
	uint16_t max_cell_mV;     // Điện áp cao nhất (mV)
	uint8_t max_cell_no;      // Số thứ tự cell cao nhất
	uint16_t min_cell_mV;     // Điện áp thấp nhất (mV)
	uint8_t min_cell_no;      // Số thứ tự cell thấp nhất
} BMS_CellVolt_t;

/* 6.3 CELL_TEMP - Nhiệt độ cell cao/thấp/trung bình (ID: 0x05F4) */
typedef struct {
	int8_t max_temp_C;        // Nhiệt độ cao nhất (°C, offset -50)
	uint8_t max_no;           // Số cell có nhiệt cao nhất
	int8_t min_temp_C;        // Nhiệt độ thấp nhất (°C, offset -50)
	uint8_t min_no;           // Số cell có nhiệt thấp nhất
	int8_t avg_temp_C;        // Nhiệt độ trung bình (°C, offset -50)
} BMS_CellTemp_t;

/* 6.4 ALM_INFO - Thông tin cảnh báo hệ thống (ID: 0x07F4) */
typedef struct {
    uint8_t cell_overvolt;    // Cell quá áp
    uint8_t cell_undervolt;   // Cell thấp áp
    uint8_t delta_over;       // Chênh lệch điện áp cell quá lớn
    uint8_t dchg_oc;          // Dòng xả quá mức
    uint8_t chg_oc;           // Dòng sạc quá mức
    uint8_t temp_high;        // Nhiệt độ cao
    uint8_t temp_low;         // Nhiệt độ thấp
    uint8_t soc_low;          // Dung lượng còn lại thấp
    uint8_t comm_fault;       // Lỗi truyền thông nội bộ
} BMS_AlmInfo_t;

 /* Gộp tất cả dữ liệu thành một cấu trúc chính */
 typedef struct {
	 BMS_BattSt1_t batt1;      // Trạng thái điện tổng
	 BMS_CellVolt_t cellVolt;  // Điện áp cell cao/thấp
	 BMS_CellTemp_t cellTemp;  // Nhiệt độ cell
	 BMS_AlmInfo_t almInfo;    // Cảnh báo mức độ
 } BMS_Jikong_t;


 /* ==============================================================
  *                        API FUNCTIONS
  * ==============================================================*/

  void BMS_Jikong_Init(CAN_HandleTypeDef *hcan);

 /**
  * @brief Giải mã frame CAN nhận được từ BMS Jikong.
  *        Hàm này đọc ID frame và cập nhật dữ liệu vào struct BMS_Jikong_t tương ứng.
  * @param bms  Con trỏ đến struct BMS_Jikong_t cần cập nhật
  * @param hdr  Header CAN (chứa ID)
  * @param data Mảng byte 8 byte dữ liệu CAN
  */
 void BMS_Jikong_Process(BMS_Jikong_t *bms, CAN_RxHeaderTypeDef *hdr, uint8_t *data);

 /* Biến global chứa dữ liệu BMS */
 extern BMS_Jikong_t bms;


 #ifdef __cplusplus
 }
 #endif

 #endif /* BMS_JIKONG_JIKONG_CAN_H_ */
