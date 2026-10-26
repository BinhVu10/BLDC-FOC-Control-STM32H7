/*
 * adrc.c
 *
 *  Created on: Oct 21, 2026
 *      Author: THANH BINH
 */


#include "adrc.h"
#include "config.h"
#include <string.h>
#include "arm_math.h"
#include "stdint.h"

// KHỞI TẠO BỘ ĐIỀU KHIỂN LINEAR ADRC
void ladrc_init(LADRC_t *ladrc, float output_max, float output_min){

	// RESET TẤT CẢ THAM SỐ VỀ 0

	memset(ladrc, 0, sizeof(*ladrc));

	ladrc->eso.eso_bw = ESO_BW;												// BĂNG THÔNG BỘ QUAN SÁT

	ladrc->eso.eso_gain_l1 = 2*ESO_BW;										// GAIN BỘ QUAN SÁT L1

	ladrc->eso.eso_gain_l2 = ESO_BW*ESO_BW;									// GAIN BỘ QUAN SÁT L2

	ladrc->output_max = output_max;											// GIÁ TRỊ MAX OUTPUT

	ladrc->output_min = output_min;											// GIÁ TRỊ MIN OUTPUT

	ladrc->Ts = ADRC_TS;													// CHU KỲ LẤY MẪU BỘ ĐIỀU KHIỂN ADRC

	ladrc->b0 = ADRC_B0;													// THAM SỐ ĐỐI TƯỢNG

	ladrc->kp = ADRC_KP;													// THAM SỐ BỘ ĐIỀU KHIẾN

	ladrc->ki = ADRC_KI;													// THAM SỐ BỘ ĐIỀU KHIẾN KHÂU TÍCH PHÂN

	ladrc->aw_gain = ADRC_AW;												// HỆ SỐ ANTI-WINDUP

}

// CẬP NHẬT CONTROL OUTPUT


//  THIẾT KẾ THAM SỐ L1, L2 THEO WC, ĐƯA VÀO LUỒNG CHẠY ĐỂ TEST
float ladrc_update(LADRC_t *ladrc, float setpoint, float feedback){

	// PREDICTION

	ladrc->eso.x1_hat = ladrc->eso.prev_x1_hat + (ladrc->eso.prev_x2_hat * 1 + ladrc->b0 * ladrc->output) * ladrc->Ts ;

	ladrc->eso.x2_hat = ladrc->eso.prev_x2_hat;

	ladrc->eso.y_hat = 1 * ladrc->eso.x1_hat;									// PREDICTION OUTPUT

	// CORRECTION (BY USING MEASUREMENT SENSOR)

	ladrc->eso.e_observer = feedback - ladrc->eso.y_hat ;						// e = y - y_hat

	ladrc->eso.x1_hat = ladrc->eso.x1_hat + ladrc->eso.eso_gain_l1 * ladrc->eso.e_observer * ladrc->Ts;

	ladrc->eso.x2_hat = ladrc->eso.x2_hat + ladrc->eso.eso_gain_l2 * ladrc->eso.e_observer * ladrc->Ts;

	// CONTROL LAW UPDATE

	float error = setpoint - ladrc->eso.x1_hat;

	float output;

	if (ladrc->ki == 0.0f) {

		ladrc->integral = 0.0f;

	}

	// BẢO VỆ TRÁNH CHIA CHO 0

	if(ladrc-> b0 != 0){

		ladrc->integral += ladrc->ki * error * ladrc->Ts ;

		output = (ladrc->kp * (error) + ladrc->integral - ladrc-> eso.x2_hat) / ladrc->b0 ;

	}

	else{

		output = 0 ;

	}


	// SATURATION

	if (output > ladrc->output_max){

		ladrc->output = ladrc->output_max;

	}

	else if (output < ladrc->output_min){

		ladrc->output = ladrc->output_min;

	}

	else{

		ladrc->output = output;

	}


	// AW_GAIN LÀ HỆ SỐ RỜI RẠC; B0 ĐỔI SAI LỆCH OUTPUT VỀ MIỀN TÍCH PHÂN.
	if ((ladrc->ki != 0.0f) && (ladrc->Ts > 0.0f)) {

		ladrc->integral += ladrc->aw_gain * ladrc->b0 * (ladrc->output - output);

	}

	// UPDATE STATE

	ladrc->eso.prev_x1_hat = ladrc->eso.x1_hat;

	ladrc->eso.prev_x2_hat = ladrc->eso.x2_hat;

	ladrc->error = error;

	return ladrc->output;

}

// RESET LADRC
void ladrc_reset(LADRC_t *ladrc){

	ladrc->error = 0 ;

	ladrc->output = 0;

	ladrc->eso.x1_hat = 0 ;

	ladrc->eso.x2_hat = 0 ;

	ladrc->eso.prev_x1_hat = 0;

	ladrc->eso.prev_x2_hat = 0 ;

	ladrc->eso.y_hat = 0 ;

	ladrc->eso.e_observer = 0.0f;

	ladrc->integral = 0 ;

}
