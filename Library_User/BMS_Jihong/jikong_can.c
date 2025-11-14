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
			bms->cellVolt.max_cell_no = d[4];              // Số thứ tự cell cao nhất
			bms->cellVolt.min_cell_mV = GET_U16_LE(&d[2]); // mV cell thấp nhất
			bms->cellVolt.min_cell_no = d[5];              // Số thứ tự cell thấp nhất
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
	}
}