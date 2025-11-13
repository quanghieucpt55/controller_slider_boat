/*
 * jihong_can.c
 *
 *  Created on: Oct 28, 2025
 *      Author: quang
 */

 #include "jikong_can.h"

/* ==============================================================
*  Hàm macro đọc dữ liệu little-endian (theo chuẩn BMS Jikong)
*  - GET_U16_LE: ghép 2 byte thành uint16_t (LSB trước)
*  - GET_U32_LE: ghép 4 byte thành uint32_t (LSB trước)
* ==============================================================*/
#define GET_U16_LE(p)  ((uint16_t)((p)[0] | ((p)[1] << 8)))
#define GET_U32_LE(p)  ((uint32_t)((p)[0] | ((p)[1] << 8) | ((p)[2] << 16) | ((p)[3] << 24)))

 /* ==============================================================
  *                   FRAME DECODING FUNCTION
  * ==============================================================
  * Hàm này dùng để giải mã các frame CAN nhận được từ BMS.
  * Dựa vào ID frame, hàm xác định loại dữ liệu (BATT_ST1, CELL_VOLT, v.v.)
  * rồi cập nhật giá trị tương ứng vào cấu trúc BMS_Jikong_t.
  * ==============================================================
  */
 BMS_Jikong_t bms;

void BMS_Jikong_Init(CAN_HandleTypeDef *hcan) {
	CAN_FilterTypeDef FilterCan;
	FilterCan.FilterBank = 0;
    FilterCan.FilterMode = CAN_FILTERMODE_IDMASK;
    FilterCan.FilterScale = CAN_FILTERSCALE_32BIT;
    FilterCan.FilterIdHigh = 0x00f5 << 5;
    FilterCan.FilterIdLow = 0x0000;
    FilterCan.FilterMaskIdHigh = 0x00ff << 5;
    FilterCan.FilterMaskIdLow = 0x0000;
    FilterCan.FilterFIFOAssignment = CAN_RX_FIFO1;
    FilterCan.FilterActivation = ENABLE;
    FilterCan.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(hcan, &FilterCan);

	HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
	if (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1) > 0) {
		CAN_RxHeaderTypeDef hdr;
		uint8_t d[8];
		HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &hdr, d);
		BMS_Jikong_Process(&bms, &hdr, d);
	}
}

void BMS_Jikong_Process(BMS_Jikong_t *bms, CAN_RxHeaderTypeDef *hdr, uint8_t *d)
{
	// Lấy ID thực tế (chuẩn hoặc mở rộng)
	uint32_t id = (hdr->IDE == CAN_ID_EXT) ? hdr->ExtId : hdr->StdId;

	switch (id)
	{
		/* ----------------------------------------------------------
		* 6.1 BATT_ST1 - Trạng thái điện áp, dòng, SOC (ID: 0x02F4)
		* ---------------------------------------------------------- */
		case ID_BATT_ST1:
			// Điện áp tổng (0.1 V / bit)
			bms->batt1.voltage_V = GET_U16_LE(&d[0]) * 0.1f;
			// Dòng tổng (0.1 A / bit, offset -400)
			bms->batt1.current_A = GET_U16_LE(&d[2]) * 0.1f - 400.0f;
			// SOC
			bms->batt1.soc_percent = d[4];
			break;

		/* ----------------------------------------------------------
		* 6.2 CELL_VOLT - Cell cao nhất và thấp nhất (ID: 0x04F4)
		* ---------------------------------------------------------- */
		case ID_CELL_VOLT:
			bms->cellVolt.max_cell_mV = GET_U16_LE(&d[0]); // mV cell cao nhất
			//bms->cellVolt.max_cell_no = d[2];              // Số thứ tự cell cao nhất
			bms->cellVolt.min_cell_mV = GET_U16_LE(&d[2]); // mV cell thấp nhất
			//bms->cellVolt.min_cell_no = d[5];              // Số thứ tự cell thấp nhất
			break;

		/* ----------------------------------------------------------
		* 6.3 CELL_TEMP - Nhiệt độ cao/thấp/trung bình (ID: 0x05F4)
		*  Mỗi giá trị có offset -50°C.
		* ---------------------------------------------------------- */
		case ID_CELL_TEMP:
		bms->cellTemp.max_temp_C = (int8_t)d[0] - 50; // MaxTemp
		bms->cellTemp.max_no     = d[1];              // Cell số bao nhiêu
		bms->cellTemp.min_temp_C = (int8_t)d[2] - 50; // MinTemp
		bms->cellTemp.min_no     = d[3];
		bms->cellTemp.avg_temp_C = (int8_t)d[4] - 50; // AvgTemp
		break;

		/* ----------------------------------------------------------
		* 6.4 ALM_INFO - Thông tin cảnh báo hệ thống (ID: 0x07F4)
		* ----------------------------------------------------------
		* Mỗi lỗi chiếm 2 bit (0–3):
		*   0: không cảnh báo
		*   1: cảnh báo nghiêm trọng
		*   2: cảnh báo quan trọng
		*   3: cảnh báo nhẹ
		*
		* Dữ liệu chỉ dùng 4 byte đầu tiên (8 lỗi chính được mã hóa)
		* ---------------------------------------------------------- */
		case ID_ALM_INFO: {
		uint32_t raw = GET_U32_LE(&d[0]); // Lấy 4 byte đầu tiên

		// Giải mã từng nhóm bit
		bms->almInfo.cell_overvolt =  (raw >> 0)  & 0x0003;   // Quá áp
		bms->almInfo.cell_undervolt = (raw >> 2)  & 0x0003;   // Thấp áp
		bms->almInfo.delta_over =     (raw >> 8)  & 0x0003;   // ΔV quá lớn
		bms->almInfo.dchg_oc =        (raw >> 10) & 0x0003;   // Dòng xả quá
		bms->almInfo.chg_oc =         (raw >> 12) & 0x0003;   // Dòng sạc quá
		bms->almInfo.temp_high =      (raw >> 14) & 0x0003;   // Nhiệt cao
		bms->almInfo.temp_low =       (raw >> 16) & 0x0003;   // Nhiệt thấp
		bms->almInfo.soc_low =        (raw >> 20) & 0x0003;   // SOC thấp
		bms->almInfo.comm_fault =     (raw >> 28) & 0x0003;   // Lỗi truyền thông
		break;
		}

//		/* ----------------------------------------------------------
//		* 6.5 BATT_ST2 - Thông tin dung lượng, số chu kỳ (ID: 0x18F128F4)
//		* ---------------------------------------------------------- */
//		case ID_BATT_ST2:
//			bms->batt2.cap_remain_Ah = GET_U16_LE(&d[0]) * 0.1f;  // Dung lượng còn lại
//			bms->batt2.cap_full_Ah   = GET_U16_LE(&d[2]) * 0.1f;  // Dung lượng đầy
//			bms->batt2.cycle_cap_Ah  = GET_U16_LE(&d[4]) * 0.1f;  // Dung lượng đã xả tích lũy
//			bms->batt2.cycle_count   = GET_U16_LE(&d[6]);         // Số chu kỳ sạc/xả
//			break;
//
//		/* ----------------------------------------------------------
//		* 6.6 ALL_TEMP - 5 cảm biến nhiệt độ (ID: 0x18F228F4)
//		*  Byte0: bitmask cảm biến nào tồn tại (bit0–bit4)
//		*  Byte1–5: nhiệt độ (offset -50°C, 0xFF = không tồn tại)
//		* ---------------------------------------------------------- */
//		case ID_ALL_TEMP:
//			bms->allTemp.mask = d[0]; // Bitmask: 1 = có cảm biến
//			for (int i = 0; i < 5; i++)
//				bms->allTemp.temp[i] = (d[i + 1] == 0xFF)
//										? -127                // -127 = cảm biến không tồn tại
//										: ((int8_t)d[i + 1] - 50);
//			break;
//
//		/* ----------------------------------------------------------
//		* 6.7 BMSERR_INFO - Lỗi bên trong BMS (ID: 0x18F328F4)
//		* ----------------------------------------------------------
//		* Frame này chứa các lỗi cứng (hardware fault) bên trong BMS.
//		* Mỗi lỗi chiếm 1 bit, giá trị:
//		*   0 = bình thường
//		*   1 = lỗi
//		* Tổng cộng 18 bit (3 byte đầu tiên).
//		* ---------------------------------------------------------- */
//		case ID_BMSERR_INFO: {
//			uint32_t raw = GET_U32_LE(&d[0]);   // ghép 4 byte (little-endian)
//
//			bms->bmsErrInfo.line_res_high     = (raw >> 0)  & 0x01;
//			bms->bmsErrInfo.mos_overtemp      = (raw >> 1)  & 0x01;
//			bms->bmsErrInfo.cell_count_mismatch = (raw >> 2)  & 0x01;
//			bms->bmsErrInfo.cur_sensor_fault  = (raw >> 3)  & 0x01;
//			bms->bmsErrInfo.cell_overvolt     = (raw >> 4)  & 0x01;
//			bms->bmsErrInfo.pack_overvolt     = (raw >> 5)  & 0x01;
//			bms->bmsErrInfo.chg_overcurrent   = (raw >> 6)  & 0x01;
//			bms->bmsErrInfo.chg_short         = (raw >> 7)  & 0x01;
//			bms->bmsErrInfo.chg_temp_high     = (raw >> 8)  & 0x01;
//			bms->bmsErrInfo.chg_temp_low      = (raw >> 9)  & 0x01;
//			bms->bmsErrInfo.comm_inner_fault  = (raw >> 10) & 0x01;
//			bms->bmsErrInfo.cell_undervolt    = (raw >> 11) & 0x01;
//			bms->bmsErrInfo.pack_undervolt    = (raw >> 12) & 0x01;
//			bms->bmsErrInfo.dchg_overcurrent  = (raw >> 13) & 0x01;
//			bms->bmsErrInfo.dchg_short        = (raw >> 14) & 0x01;
//			bms->bmsErrInfo.dchg_temp_high    = (raw >> 15) & 0x01;
//			bms->bmsErrInfo.chg_mos_fault     = (raw >> 16) & 0x01;
//			bms->bmsErrInfo.dchg_mos_fault    = (raw >> 17) & 0x01;
//			break;
//		}
//
//		/* ----------------------------------------------------------
//		* 6.8 BMS_INFO - Thông tin hệ thống (ID: 0x18F428F4)
//		*  Bao gồm thời gian chạy, dòng sưởi, và SOH (%)
//		* ---------------------------------------------------------- */
//		case ID_BMS_INFO:
//			bms->info.runtime_s = GET_U32_LE(&d[0]);          // Thời gian hoạt động (s)
//			bms->info.heat_current_A = GET_U16_LE(&d[4]) / 1000.0f; // Dòng sưởi (mA → A)
//			bms->info.soh_percent = d[6];                     // SOH (%)
//			break;
//
//		/* ----------------------------------------------------------
//		* 6.9 BMSSW_STA - Trạng thái MOS & thiết bị (ID: 0x18F528F4)
//		*  Bit định nghĩa trong Byte0:
//		*    bit0: ChgMOS  (1=bật)
//		*    bit1: DchgMOS (1=bật)
//		*    bit2: Balance (1=đang cân bằng)
//		*    bit3: HeatMOS (1=bật)
//		*    bit4: ChargerPlug (1=đã cắm)
//		*    bit5: ACC (1=bật)
//		* ---------------------------------------------------------- */
//		case ID_BMSSW_STA:
//			bms->swSta.chgMOS  = (d[0] >> 0) & 1;
//			bms->swSta.dchgMOS = (d[0] >> 1) & 1;
//			bms->swSta.balance = (d[0] >> 2) & 1;
//			bms->swSta.heat    = (d[0] >> 3) & 1;
//			bms->swSta.chgPlug = (d[0] >> 4) & 1;
//			bms->swSta.acc     = (d[0] >> 5) & 1;
//			break;
//				/* ----------------------------------------------------------
//		* 7.0 CELLVOL - Điện áp từng cell (ID: 0x18E0xxF4)
//		*  Dải ID: 0x18E028F4 – 0x18E628F4
//		*  Mỗi frame chứa 4 cell, 2 byte/cell, Little-endian
//		*  → Frame 0: cell 1–4
//		*    Frame 1: cell 5–8, ...
//		* ---------------------------------------------------------- */
//		case ID_CELLVOL_BASE:
//		case ID_CELLVOL_BASE + 0x10000:
//		case ID_CELLVOL_BASE + 0x20000:
//		case ID_CELLVOL_BASE + 0x30000:
//		case ID_CELLVOL_BASE + 0x40000:
//		case ID_CELLVOL_BASE + 0x50000:
//		case ID_CELLVOL_BASE + 0x60000:
//		{
//			uint8_t frame_no = (id - ID_CELLVOL_BASE) >> 8;  // Tính số thứ tự frame
//			for (int i = 0; i < 4; i++) {
//				uint8_t cell_idx = frame_no * 4 + i;          // Số cell tuyệt đối
//				if (cell_idx < 25)
//					bms->cellArray.cell_mV[cell_idx] = GET_U16_LE(&d[i * 2]); // Điện áp mV
//			}
//			bms->cellArray.cell_count = 25; // Mặc định 25 cell (max)
//			break;
//		}
//
//		/* ----------------------------------------------------------
//		* 7.2 BMSChgINFO - Thông tin yêu cầu sạc (ID: 0x1806E5F4)
//		* ----------------------------------------------------------
//		* Gửi từ BMS → Charger (chu kỳ 500ms)
//		* Byte0–1: ChgVol (0.1V / bit)
//		* Byte2–3: ChgCur (0.1A / bit)
//		* Byte4:   ChgDevSw (0=ON, 1=OFF)
//		* Byte6:   ChgAndHeat (0=sạc, 1=sưởi)
//		* ---------------------------------------------------------- */
//		case ID_BMSCHG_INFO:
//			bms->chgInfo.chg_volt_V   = GET_U16_LE(&d[0]) * 0.1f;  // Điện áp yêu cầu sạc
//			bms->chgInfo.chg_curr_A   = GET_U16_LE(&d[2]) * 0.1f;  // Dòng yêu cầu sạc
//			bms->chgInfo.chg_dev_sw   = d[4];                      // Trạng thái bộ sạc
//			bms->chgInfo.chg_and_heat = d[6];                      // 0=sạc, 1=sưởi
//			break;
	}
}

/* ==============================================================
*                   CONTROL COMMAND FUNCTION
* ==============================================================
* 7.1 CTRL_INFO - Lệnh điều khiển gửi từ MCU → BMS (ID: 0x18F0F428)
* --------------------------------------------------------------
* Byte0: MaskCode (bit0=cho phép điều khiển charge, bit1=discharge, bit2=balance)
* Byte1: ChgSw (0=OFF, 1=ON)
* Byte2: DchgSw (0=OFF, 1=ON)
* Byte3: BalSw (0=OFF, 1=ON)
* -------------------------------------------------------------- */
void BMS_Jikong_RequestControl(CAN_HandleTypeDef *hcan,
							uint8_t mask, uint8_t chg, uint8_t dchg, uint8_t bal)
{
	CAN_TxHeaderTypeDef hdr;
	uint8_t data[8] = {0};
	uint32_t mailbox;

	hdr.IDE = CAN_ID_EXT;               // Frame mở rộng (29 bit)
	hdr.ExtId = ID_CTRL_INFO;           // ID lệnh điều khiển
	hdr.RTR = CAN_RTR_DATA;             // Khung dữ liệu (không phải remote)
	hdr.DLC = 8;                        // Độ dài 8 byte
	hdr.TransmitGlobalTime = DISABLE;

	// Ghi dữ liệu điều khiển
	data[0] = mask;  // bit0=chg, bit1=dchg, bit2=bal
	data[1] = chg;   // Trạng thái sạc
	data[2] = dchg;  // Trạng thái xả
	data[3] = bal;   // Trạng thái cân bằng

	// Gửi qua CAN
	HAL_CAN_AddTxMessage(hcan, &hdr, data, &mailbox);
}
