/*
 * Charge.h
 *
 *  Created on: Nov 21, 2025
 *      Author: quang
 */

#ifndef CHARGER_CHARGE_H_
#define CHARGER_CHARGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ==============================================================
 *                  CHARGE MODULE DEFINITIONS
 * ==============================================================
 * Module này dùng để chuyển tiếp gói tin từ BMS (CAN1) sang Charger
 * và gửi các gói tin tùy chỉnh tới Charger
 * ==============================================================
 */

/**
 * @brief Chuyển tiếp gói tin từ BMS (CAN1) sang Charger
 * @param hcan_charger  CAN handle của Charger (có thể là hcan1, hcan2, hcan3...)
 * @param hdr           Header CAN của gói tin nhận được từ BMS
 * @param data          Mảng 8 byte dữ liệu từ BMS
 * @retval HAL_StatusTypeDef  Trạng thái gửi (HAL_OK nếu thành công)
 * 
 * @note Hàm này sẽ chuyển tiếp tất cả gói tin từ BMS sang Charger
 *       với cùng ID và dữ liệu, hỗ trợ cả Standard ID và Extended ID
 */
HAL_StatusTypeDef Charge_Forward_BMS_To_Charger(CAN_HandleTypeDef *hcan_charger,
                                                 CAN_RxHeaderTypeDef *hdr,
                                                 uint8_t *data);

/**
 * @brief Gửi gói tin tùy chỉnh tới Charger
 * @param hcan_charger  CAN handle của Charger (có thể là hcan1, hcan2, hcan3...)
 * @param id            CAN ID (Standard hoặc Extended tùy vào is_extended)
 * @param is_extended   1 = Extended ID (29-bit), 0 = Standard ID (11-bit)
 * @param data          Mảng 8 byte dữ liệu cần gửi
 * @param dlc           Data Length Code (0-8, số byte dữ liệu thực tế)
 * @retval HAL_StatusTypeDef  Trạng thái gửi (HAL_OK nếu thành công)
 * 
 * @note Hàm này cho phép người dùng tự tạo và gửi gói tin tùy chỉnh tới Charger
 *       Ví dụ: Charge_Send_Custom_Frame(&hcan2, 0x123, 0, my_data, 8);
 */
HAL_StatusTypeDef Charge_Send_Custom_Frame(CAN_HandleTypeDef *hcan_charger,
                                            uint32_t id,
                                            uint8_t is_extended,
                                            uint8_t *data,
                                            uint8_t dlc);

#ifdef __cplusplus
}
#endif

#endif /* CHARGER_CHARGE_H_ */
