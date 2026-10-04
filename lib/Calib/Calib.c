/*
 * Calib.c
 *
 *  Created on: Sep 26, 2026
 *      Author: binhvt22
 */

#include "Calib.h"
#include "config.h"
#include "pwm_port.h"

bool run_calib_encoder(foc_t *foc, encoder_t *encoder, uint8_t cycle_count){

	// CYCLE_COUNT LÀ SỐ CHU KỲ CALIBRATION

	float angle_offset_avg = 0.0f;

	float newest_angle_avg;

	float delta_angle;

	float angle1 = 0.0f;

	float angle2 = 0.0f;

	bool success = false;

	if ((foc == NULL) || (encoder == NULL) || (cycle_count < 2U)) {
		return false;
	}

	foc_voltage_bus_read(foc);

	if (foc->v_bus <= 0.0f) {
		return false;
	}

	for(uint8_t i = 0U; i < cycle_count; i++){

		// BƠM PHA B
		foc_open_loop_control(foc, VD_CALIB, VQ_CALIB, DEG_TO_RAD(120.0f));

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		foc_open_loop_control(foc, VD_CALIB/2.0f, VQ_CALIB, DEG_TO_RAD(120.0f));

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		foc_open_loop_control(foc, VD_CALIB/4.0f, VQ_CALIB, DEG_TO_RAD(120.0f));

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		// BƠM PHA A
		foc_open_loop_control(foc, VD_CALIB, VQ_CALIB, 0.0f);

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		foc_open_loop_control(foc, VD_CALIB/2.0f, VQ_CALIB, 0.0f);

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		foc_open_loop_control(foc, VD_CALIB/4.0f, VQ_CALIB, 0.0f);

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		// GỬI LỆNH ĐỌC ENCODER ĐỂ LẤY OFFSET
		if (encoder_Read_Polling(encoder) != HAL_OK) {
			goto calib_done;
		}

		angle1 = encoder->theta_m;

		// BƠM PHA C

		foc_open_loop_control(foc, VD_CALIB, VQ_CALIB, DEG_TO_RAD(-120.0f));

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		foc_open_loop_control(foc, VD_CALIB/2.0f, VQ_CALIB, DEG_TO_RAD(-120.0f));

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		foc_open_loop_control(foc, VD_CALIB/4.0f, VQ_CALIB, DEG_TO_RAD(-120.0f));

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		// BƠM PHA A
		foc_open_loop_control(foc, VD_CALIB, VQ_CALIB, 0.0f);

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		foc_open_loop_control(foc, VD_CALIB/2.0f, VQ_CALIB, 0.0f);

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		foc_open_loop_control(foc, VD_CALIB/4.0f, VQ_CALIB, 0.0f);

		HAL_Delay(200);										// DELAY ĐỂ GIỮ ỔN ĐỊNH ROTOR

		// GỬI LỆNH ĐỌC ENCODER ĐỂ LẤY OFFSET
		if (encoder_Read_Polling(encoder) != HAL_OK) {
			goto calib_done;
		}

		angle2 = encoder->theta_m;

		if(i == 0){
			continue;
		}

		delta_angle = angle2 - angle1;

		if (delta_angle > PI) {

			angle2 -= 2.0f * PI;

		}

		else if (delta_angle < -PI) {

			angle2 += 2.0f * PI;

		}

		// KẾT THÚC 1 CHU KỲ LẤY TRUNG BÌNH GÓC
		newest_angle_avg = (angle1 + angle2) / 2.0f;

		if (i > 0U) {
			delta_angle = newest_angle_avg - angle_offset_avg;

			if (delta_angle > PI) {

				newest_angle_avg -= 2.0f * PI;

			}
			else if (delta_angle < -PI) {

				newest_angle_avg += 2.0f * PI;
			}
		}

		angle_offset_avg += (newest_angle_avg - angle_offset_avg) / (float)(i);
	}

	// UPDATE GÓC OFFSET CUỐI CÙNG
	if (angle_offset_avg < 0.0f) {

		angle_offset_avg += 2.0f * PI;

	}
	else if (angle_offset_avg >= 2.0f * PI) {

		angle_offset_avg -= 2.0f * PI;

	}

	encoder->offset_encoder = angle_offset_avg;
	// DÙNG LẠI FRAME HỢP LỆ CUỐI ĐỂ TÍNH GÓC ĐIỆN VỚI OFFSET MỚI.
	encoder_Read_Angle(encoder);
	success = true;

calib_done:
	// KẾT THÚC CẢ KHI THÀNH CÔNG LẪN KHI LỖI; KHÔNG THAY ĐỔI STATE/MODE.
	foc_zero_pwm_output(foc);

	pwm_set(0U, 0U, 0U);
	return success;
}
