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
 #define BMS_DEVICE_ADDR       0x00        // Địa chỉ node của BMS, mặc định = 0

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
 #define ID_BATT_ST2       0x18F128F4 + BMS_DEVICE_ADDR    // BATT_ST2: dung lượng, số chu kỳ
 #define ID_ALL_TEMP       0x18F228F4 + BMS_DEVICE_ADDR    // ALL_TEMP: 5 cảm biến nhiệt độ (cell + MOS)
 #define ID_BMSERR_INFO    0x18F328F4 + BMS_DEVICE_ADDR    // BMSERR_INFO: lỗi bên trong (MOS, sensor, v.v.)
 #define ID_BMS_INFO       0x18F428F4 + BMS_DEVICE_ADDR    // BMS_INFO: thời gian chạy, dòng sưởi, SOH
 #define ID_BMSSW_STA      0x18F528F4 + BMS_DEVICE_ADDR    // BMSSW_STA: trạng thái MOS (charge/discharge/balance)
 #define ID_CELLVOL_BASE   0x18E028F4 + BMS_DEVICE_ADDR    // CELLVOL_BASE: khung chứa điện áp từng cell (4 cell/frame)
 #define ID_BMSCHG_INFO    0x1806E5F4 + BMS_DEVICE_ADDR    // BMSCHG_INFO: yêu cầu sạc (BMS gửi tới charger)
 #define ID_CTRL_INFO      0x18F0F428 + BMS_DEVICE_ADDR    // CTRL_INFO: lệnh điều khiển (gửi từ MCU tới BMS)

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

 /* 6.5 BATT_ST2 - Dung lượng & chu kỳ (ID: 0x18F128F4) */
 typedef struct {
    float cap_remain_Ah;      // Dung lượng còn lại (Ah)
    float cap_full_Ah;        // Dung lượng thực tế khi đầy (Ah)
    float cycle_cap_Ah;       // Tổng dung lượng đã xả qua trong suốt vòng đời (Ah)
    uint16_t cycle_count;     // Số chu kỳ sạc/xả
} BMS_BattSt2_t;

/* 6.6 ALL_TEMP - 5 cảm biến nhiệt độ (ID: 0x18F228F4) */
typedef struct {
    uint8_t mask;             // Bitmask: bit0–4 = cảm biến nào được hỗ trợ (=1: bật, =0: tắt)
    int8_t temp[5];           // Nhiệt độ 5 cảm biến (-50°C offset, 0xFF = không tồn tại)
} BMS_AllTemp_t;

/* 6.7 BMSERR_INFO - Thông tin lỗi bên trong (ID: 0x18F328F4) */
typedef struct {
   uint8_t line_res_high;        // điện trở dây cao
   uint8_t mos_overtemp;         // MMOS quá nhiệt
   uint8_t cell_count_mismatch;  // số lượng cell không khớp
   uint8_t cur_sensor_fault;     // lỗi cảm biến dòng
   uint8_t cell_overvolt;        // cell quá áp
   uint8_t pack_overvolt;        // tổng áp pack cao
   uint8_t chg_overcurrent;      // dòng sạc quá mức
   uint8_t chg_short;            // ngắn mạch khi sạc
   uint8_t chg_temp_high;        // nhiệt sạc cao
   uint8_t chg_temp_low;         // nhiệt sạc thấp
   uint8_t comm_inner_fault;     // lỗi truyền thông nội bộ
   uint8_t cell_undervolt;       // cell thấp áp
   uint8_t pack_undervolt;       // pack thấp áp
   uint8_t dchg_overcurrent;     // dòng xả quá mức
   uint8_t dchg_short;           // ngắn mạch khi xả
   uint8_t dchg_temp_high;       // nhiệt xả cao
   uint8_t chg_mos_fault;        // lỗi MOS sạc
   uint8_t dchg_mos_fault;       // lỗi MOS xả
   uint32_t raw; // Thông tin lỗi bên trong raw
} BMS_BmsErrInfo_t;

/* 6.8 BMS_INFO - Thông tin chung hệ thống (ID: 0x18F428F4) */
typedef struct {
    uint32_t runtime_s;       // Thời gian chạy của BMS (giây)
    float heat_current_A;     // Dòng gia nhiệt (mA/1000 = A)
    uint8_t soh_percent;      // SOH – độ khỏe của pin (%)
} BMS_Info_t;

/* 6.9 BMSSW_STA - Trạng thái MOS và thiết bị (ID: 0x18F528F4) */
typedef struct {
    uint8_t chgMOS;           // MOS sạc: 1 = bật, 0 = tắt
    uint8_t dchgMOS;          // MOS xả: 1 = bật, 0 = tắt
    uint8_t balance;          // Trạng thái cân bằng: 1 = đang cân bằng
    uint8_t heat;             // MOS sưởi: 1 = bật
    uint8_t chgPlug;          // Trạng thái sạc: 1 = sạc đã cắm
    uint8_t acc;              // Trạng thái ACC: 1 = bật
} BMS_SwSta_t;

/* 7.0 CELLVOL - Điện áp từng cell (4 cell/frame, tối đa 25 cell) (ID: 0x18E028F4) */
typedef struct {
    uint16_t cell_mV[25];     // Điện áp từng cell (mV)
    uint8_t cell_count;       // Số cell thực tế trong pack
} BMS_CellVolArray_t;

/* 7.2 BMSCHG_INFO - Thông tin yêu cầu sạc (BMS → Charger) (ID: 0x1806E5F4) */
typedef struct {
    float chg_volt_V;      // Điện áp yêu cầu sạc (0.1V/bit)
    float chg_curr_A;      // Dòng yêu cầu sạc (0.1A/bit)
    uint8_t chg_dev_sw;           // Công tắc sạc: 0 = bật, 1 = tắt
    uint8_t chg_and_heat;             // 0 = chế độ sạc, 1 = chế độ sưởi
} BMS_ChgInfo_t;

 /* 7.1 CTRL_INFO - Lệnh điều khiển gửi tới BMS (ID: 0x18F0F428)*/
 typedef struct {
    uint8_t mask;   // bit0=charge, bit1=discharge, bit2=balance (Sạc, xả, cân bằng)
    uint8_t chg_sw; // MOS sạc (0=tắt, 1=bật)
    uint8_t dchg_sw;// MOS xả (0=tắt, 1=bật)
    uint8_t bal_sw; // Cân bằng (0=tắt, 1=bật)
} BMS_CtrlInfo_t;

 /* Gộp tất cả dữ liệu thành một cấu trúc chính */
 typedef struct {
    BMS_BattSt1_t batt1;      // Trạng thái điện tổng
    BMS_CellVolt_t cellVolt;  // Điện áp cell cao/thấp
    BMS_CellTemp_t cellTemp;  // Nhiệt độ cell
    BMS_AlmInfo_t almInfo;    // Cảnh báo mức độ
    BMS_BattSt2_t batt2;      // Thông tin dung lượng
    BMS_AllTemp_t allTemp;    // Dữ liệu 5 cảm biến nhiệt
    BMS_BmsErrInfo_t bmsErrInfo; // Thông tin lỗi bên trong
    BMS_Info_t info;          // Thông tin hệ thống
    BMS_SwSta_t swSta;        // Trạng thái MOS và thiết bị
    BMS_CellVolArray_t cellArray; // Danh sách điện áp từng cell
    BMS_ChgInfo_t chgInfo;    // Thông tin yêu cầu sạc (BMS → Charger)
    BMS_CtrlInfo_t ctrlInfo;  // Lệnh điều khiển (MCU → BMS)
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

 /**
  * @brief Gửi lệnh điều khiển tới BMS (ví dụ bật/tắt sạc, xả, cân bằng)
  * @param hcan  CAN handle (ví dụ &hcan1)
  * @param mask  Bitmask: bit0=cho phép sạc, bit1=cho phép xả, bit2=cho phép cân bằng
  * @param chg   0 = tắt sạc, 1 = bật sạc
  * @param dchg  0 = tắt xả, 1 = bật xả
  * @param bal   0 = tắt cân bằng, 1 = bật cân bằng
  */
 void BMS_Jikong_RequestControl(CAN_HandleTypeDef *hcan,
    uint8_t mask, uint8_t chg, uint8_t dchg, uint8_t bal);

 /* Biến global chứa dữ liệu BMS */
 extern BMS_Jikong_t bms;


 #ifdef __cplusplus
 }
 #endif

 #endif /* BMS_JIKONG_JIKONG_CAN_H_ */
