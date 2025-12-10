/*
 * Charge.c
 *
 *  Created on: Nov 21, 2025
 *      Author: quang
 */

#include "Charge.h"

/* ==============================================================
 *                  FORWARD BMS TO CHARGER FUNCTION
 * ==============================================================
 * Hàm này chuyển tiếp tất cả gói tin từ BMS (CAN1) sang Charger
 * với cùng ID và dữ liệu, giữ nguyên định dạng (Standard/Extended)
 * ==============================================================
 */
HAL_StatusTypeDef Charge_Forward_BMS_To_Charger(CAN_HandleTypeDef *hcan_charger,
                                                 CAN_RxHeaderTypeDef *hdr,
                                                 uint8_t *data)
{
    CAN_TxHeaderTypeDef tx_hdr;
    uint32_t tx_mailbox;
    HAL_StatusTypeDef status;

    // Kiểm tra tham số đầu vào
    if (hcan_charger == NULL || hdr == NULL || data == NULL) {
        return HAL_ERROR;
    }

    // Kiểm tra CAN đã được khởi động chưa
    if (hcan_charger->State != HAL_CAN_STATE_READY) {
        return HAL_ERROR;
    }

    // Sao chép header từ gói tin nhận được
    tx_hdr.IDE = hdr->IDE;                    // Standard hoặc Extended ID
    tx_hdr.RTR = CAN_RTR_DATA;                // Data frame (không phải remote)
    tx_hdr.DLC = hdr->DLC;                    // Độ dài dữ liệu (0-8)
    tx_hdr.TransmitGlobalTime = DISABLE;

    // Gán ID tương ứng (Standard hoặc Extended)
    if (hdr->IDE == CAN_ID_EXT) {
        tx_hdr.ExtId = hdr->ExtId;            // Extended ID (29-bit)
    } else {
        tx_hdr.StdId = hdr->StdId;            // Standard ID (11-bit)
    }

    // Gửi gói tin tới Charger
    status = HAL_CAN_AddTxMessage(hcan_charger, &tx_hdr, data, &tx_mailbox);

    return status;
}

/* ==============================================================
 *                  SEND CUSTOM FRAME TO CHARGER FUNCTION
 * ==============================================================
 * Hàm này cho phép người dùng tự tạo và gửi gói tin tùy chỉnh
 * tới Charger với ID và dữ liệu do người dùng định nghĩa
 * ==============================================================
 */
HAL_StatusTypeDef Charge_Send_Custom_Frame(CAN_HandleTypeDef *hcan_charger,
                                            uint32_t id,
                                            uint8_t is_extended,
                                            uint8_t *data,
                                            uint8_t dlc)
{
    CAN_TxHeaderTypeDef tx_hdr;
    uint32_t tx_mailbox;
    HAL_StatusTypeDef status;

    // Kiểm tra tham số đầu vào
    if (hcan_charger == NULL || data == NULL) {
        return HAL_ERROR;
    }

    // Kiểm tra DLC hợp lệ (0-8)
    if (dlc > 8) {
        return HAL_ERROR;
    }

    // Kiểm tra CAN đã được khởi động chưa
    if (hcan_charger->State != HAL_CAN_STATE_READY) {
        return HAL_ERROR;
    }

    // Thiết lập header
    tx_hdr.IDE = is_extended ? CAN_ID_EXT : CAN_ID_STD;
    tx_hdr.RTR = CAN_RTR_DATA;                // Data frame
    tx_hdr.DLC = dlc;                          // Độ dài dữ liệu
    tx_hdr.TransmitGlobalTime = DISABLE;

    // Gán ID tương ứng
    if (is_extended) {
        tx_hdr.ExtId = id;                     // Extended ID (29-bit)
    } else {
        // Kiểm tra Standard ID hợp lệ (11-bit, tối đa 0x7FF)
        if (id > 0x7FF) {
            return HAL_ERROR;
        }
        tx_hdr.StdId = (uint16_t)id;          // Standard ID (11-bit)
    }

    // Gửi gói tin tới Charger
    status = HAL_CAN_AddTxMessage(hcan_charger, &tx_hdr, data, &tx_mailbox);

    return status;
}
