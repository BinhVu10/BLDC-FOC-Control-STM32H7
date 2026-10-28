/*
 * adrc.c
 *
 *  Created on: Oct 21, 2026
 *      Aoutputthor: THANH BINH
 */


#include  "adrc_discrete.h"
#include  "config.h"
#include  <string.h>
#include  "arm_math.h"

static void ladrc_eso_init(LADRC_DISCRETE_t *ladrc){

	ladrc->eso.w_Observer = ladrc->eso.k_eso * ladrc->w_CL;						// BĂNG THÔNG BỘ QUAN SÁT

	if (ladrc->Ts <= 0.0f) {

		return;

	}

	float z_CL, z_ESO;															// Z-DOMAIN POLE PLACEMENT

	float tmp = -ladrc->w_CL * ladrc->Ts;

	arm_vexp_f32(&tmp, &z_CL, 1);										  		// TÍNH z_CL RỜI RẠC, Z-DOMAIN POLE PLACEMENT

	tmp = -ladrc->eso.w_Observer * ladrc->Ts;

	arm_vexp_f32(&tmp, &z_ESO, 1);											   	// TÍNH z_OBSERVER RỜI RẠC, Z-DOMAIN POLE PLACEMENT

	// GAIN BỘ ĐIỀU KHIỂN

	ladrc->kp = (ADRC_DISCRETE_KP != 0.0f) ? ADRC_DISCRETE_KP : (1.0f - z_CL) / ladrc->Ts;

	// GAIN L LUENBERGER OBSERVER

	ladrc->eso.gain_eso_l1 = (ESO_L1_DISCRETE != 0.0f) ? ESO_L1_DISCRETE : 1.0f - z_ESO * z_ESO;

	ladrc->eso.gain_eso_l2 = (ESO_L2_DISCRETE != 0.0f) ? ESO_L2_DISCRETE : (1.0f - z_ESO) * (1.0f - z_ESO) / ladrc->Ts;

	// OBSERVER MATRIX A, B

	ladrc-> eso.A_ESO11 = 1.0 - ladrc->eso.gain_eso_l1;

	ladrc->eso.A_ESO12 = ladrc->Ts * (1.0 - ladrc->eso.gain_eso_l1);

	ladrc-> eso.A_ESO21 = - ladrc->eso.gain_eso_l2;

	ladrc->eso.A_ESO22 = 1.0 - ladrc->eso.gain_eso_l2 * ladrc->Ts;

	ladrc->eso.b_ESO1 = ladrc->Ts * ladrc->b0 * (1 - ladrc->eso.gain_eso_l1);

	ladrc->eso.b_ESO2 = - ladrc->b0 * ladrc-> eso.gain_eso_l2 * ladrc->Ts;

}

// KHỞI TẠO BỘĐIỀU KHIỂN LINEAR ADRC
void ladrc_discrete_init(LADRC_DISCRETE_t *ladrc, float output_max, float output_min){

	memset(ladrc, 0, sizeof(*ladrc));										// RESET TẤT CẢ THAM SỐ VỀ 0

	ladrc->f_bw_controller = ADRC_DISCRETE_BW_HZ;							// BĂNG THÔNG BỘ ĐIỀU KHIỂN ADRC, HZ

	ladrc-> w_CL = TWO_PI * ladrc-> f_bw_controller;						// BĂNG THÔNG BỘ ĐIỀU KHIỂN ADRC, RAD/S

	ladrc->eso.k_eso = K_ESO;												// HỆ SỐ TƯƠNG ĐỐI GIỮA BỘ QUAN SÁT VÀ BỘ ĐIỀU KHIỂN

	ladrc->output_max = output_max;											// GIÁ TRỊ MAX output

	ladrc->output_min = output_min;											// GIÁ TRỊ MIN output

	ladrc->Ts = ADRC_DISCRETE_TS;										    // CHoutput KỲ LẤY MẪU BỘ ĐIỀU KHIỂN ADRC

	ladrc->b0 = ADRC_DISCRETE_B0;								            // THAM SỐ ĐỐI TƯỢNG

	ladrc->ki = ADRC_DISCRETE_KI;											// THAM SỐ BỘ ĐIỀU KHIẾN KHÂU TÍCH PHÂN

	ladrc->aw_gain = ADRC_DISCRETE_AW;										// HỆ SỐ ANTI-WINDUP

	ladrc_eso_init(ladrc);													// INIT BỘ QUAN SÁT

}

float ladrc_discrete_update(LADRC_DISCRETE_t *ladrc, float setpoint, float feedback){

	// BẢO VỆ CHIA CHO 0
	if ((ladrc->Ts <= 0.0f) || (ladrc->b0 == 0.0f)) {

		ladrc->error = setpoint - feedback;

		ladrc->integral = 0.0f;

		ladrc->output = 0.0f;

		return ladrc->output;

	}

	// CORRECTION DỰA TRÊN MEASUREMENT HIỆN TẠI
	float x1_hat_now  = ladrc->eso.x1_hat + ladrc->eso.gain_eso_l1 * feedback;

	float x2_hat_now = ladrc->eso.x2_hat + ladrc->eso.gain_eso_l2 * feedback;

	// COMPUTE CONTROL OUTPUT
	ladrc->error = setpoint - x1_hat_now;

	if (ladrc->ki == 0.0f) {

		ladrc->integral = 0.0f;

	}

	else {

		ladrc->integral += ladrc->ki * ladrc->error * ladrc->Ts;

	}

	float output = (ladrc->kp * ladrc->error + ladrc->integral - x2_hat_now)/ ladrc->b0;

	// SATURATION output
	ladrc->output = (output > ladrc->output_max) ? ladrc->output_max : ((output < ladrc->output_min) ? ladrc->output_min : output);

	// ANTI-WINDUP
	if ((ladrc->ki != 0.0f) && (ladrc->Ts > 0.0f)) {

		ladrc->integral += ladrc->aw_gain * ladrc->b0 * (ladrc->output - output);

	}

	// PRECTION FOR THE NEXT STEP CONTROL

	ladrc->eso.x1_hat = ladrc->eso.A_ESO11 * x1_hat_now + ladrc->eso.A_ESO12 * x2_hat_now + ladrc->eso.b_ESO1 * ladrc->output;

	ladrc->eso.x2_hat = ladrc->eso.A_ESO21 * x1_hat_now + ladrc->eso.A_ESO22 * x2_hat_now + ladrc->eso.b_ESO2 * ladrc->output;

	return ladrc->output;

}

// RESET LADRC
void ladrc_discrete_reset(LADRC_DISCRETE_t *ladrc){

	ladrc->error = 0 ;

	ladrc->output = 0;

	ladrc->eso.x1_hat = 0 ;

	ladrc->eso.x2_hat = 0 ;

	ladrc->integral = 0 ;

}
