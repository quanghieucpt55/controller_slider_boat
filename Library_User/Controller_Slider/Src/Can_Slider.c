/*
 * Can_Slider.c
 *
 *  Created on: Nov 12, 2025
 *      Author: quang
 */

#include "Can_Slider.h"

#define GET_U16_LE(p)  ((uint16_t)((p)[1] | ((p)[0] << 8)))

Can_Slider_t can_slider;

Can_Slider_VCU_t can_slider_vcu;

void Can_Slider_Init(CAN_HandleTypeDef *hcan) {
	CAN_FilterTypeDef FilterCan;
	FilterCan.FilterBank = 1;
    FilterCan.FilterMode = CAN_FILTERMODE_IDLIST;
    FilterCan.FilterScale = CAN_FILTERSCALE_32BIT;
    FilterCan.FilterIdHigh = (CAN_SLIDER_1_ID << 5);
    FilterCan.FilterIdLow = 0x0000;
    FilterCan.FilterMaskIdHigh = (CAN_SLIDER_2_ID << 5);
    FilterCan.FilterMaskIdLow = 0x0000;
    FilterCan.FilterFIFOAssignment = CAN_RX_FIFO0;
    FilterCan.FilterActivation = ENABLE;
    FilterCan.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(hcan, &FilterCan);

	HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

	can_slider.last_time_accel = HAL_GetTick();
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
	if (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0) {
		CAN_RxHeaderTypeDef hdr;
		uint8_t d[8];
		HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, d);
		Can_Slider_Process(&can_slider, &hdr, d);
	}
}

void Can_Slider_Process(Can_Slider_t *can_slider, CAN_RxHeaderTypeDef *hdr_ptr, uint8_t *d) {
	switch (hdr_ptr->StdId) {
		case CAN_SLIDER_1_ID:
		{
			uint8_t err;
			err = Can_Slider_1_Process(&can_slider->slider_1, hdr_ptr, d);
		}
			break;
		case CAN_SLIDER_2_ID:
			Can_Slider_2_Process(&can_slider->slider_2, hdr_ptr, d);
			break;
	}
}

Can_Slider_Error_Code_t Can_Slider_1_Process(Can_Slider_1_t *slider_1, CAN_RxHeaderTypeDef *hdr, uint8_t *d) {
	slider_1->vehicle_mode.forward = d[0];
	slider_1->vehicle_mode.reverse = d[1];
	if (d[0] && !d[1]) {
		can_slider.motor_direc = 1;
	} else if (!d[0] && d[1]) {
		can_slider.motor_direc = 2;
	} else {
		can_slider.motor_direc = 0;
	}
	slider_1->vehicle_mode.brake = d[2];
	slider_1->motor_rpm = GET_U16_LE(&d[3]);
	if (HAL_GetTick() - can_slider.last_time_accel > 1000) 
{		if (slider_1->motor_rpm > can_slider.last_motor_rpm) {
			can_slider.last_time_accel = HAL_GetTick();
			can_slider.rpm_accel = slider_1->motor_rpm - can_slider.last_motor_rpm;
			can_slider.last_motor_rpm = slider_1->motor_rpm;
		} else {
			can_slider.rpm_accel = 0;
			can_slider.last_time_accel = HAL_GetTick();
			can_slider.last_motor_rpm = slider_1->motor_rpm;
		}
	}
	slider_1->motor_temp = d[5]-40;
	slider_1->controller_temp = d[6]-40;
	can_slider.raw_err_code = d[7];
	switch (can_slider.raw_err_code)
	{
	case OVER_CURRENT:
		slider_1->error_code = 0x0001;
		break;
	case CONTROLLER_TEMP_HIGH:
		slider_1->error_code = 0x0002;
		break;
	case MOTOR_ENCODER_ERROR:
		slider_1->error_code = 0x0004;
		break;
	case COMMUNICATION_ERROR:
		slider_1->error_code = 0x0008;
		break;
	case UNDER_VOLTAGE_BATTERY:
		slider_1->error_code = 0x0010;
		break;
	case OVER_VOLTAGE_BATTERY:
		slider_1->error_code = 0x0020;
		break;
	case MOTOR_TEMP_HIGH:
		slider_1->error_code = 0x0040;
		break;
	case MOTOR_TEMP_SENSOR_ERROR:
		slider_1->error_code = 0x0080;
		break;
	case ACCELERATOR_FAULT:
		slider_1->error_code = 0x0100;
		break;
	default:
		slider_1->error_code = 0x0000;
		break;
	}
	return slider_1->error_code;
}

void Can_Slider_2_Process(Can_Slider_2_t *slider_2, CAN_RxHeaderTypeDef *hdr, uint8_t *d) {
	slider_2->battery_voltage = (float)GET_U16_LE(&d[0])*0.1;
	slider_2->dc_current = (float)GET_U16_LE(&d[2])*0.1;
	slider_2->control_mode = d[4];
}

void Can_Vcu_Send_Slider(CAN_HandleTypeDef *hcan) {
	CAN_TxHeaderTypeDef hdr;
	uint32_t txMailbox;
	hdr.StdId = CAN_SLIDER_VCU_ID;
	hdr.IDE = CAN_ID_STD;
	hdr.RTR = CAN_RTR_DATA;
	hdr.DLC = 3;
	HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(hcan, &hdr, (uint8_t*)&can_slider_vcu, &txMailbox);
}
