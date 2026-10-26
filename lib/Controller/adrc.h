/*
 * adrc.h
 *
 *  Created on: Oct 21, 2026
 *      Author: THANH BINH
 */

#ifndef CONTROLLER_ADRC_H_
#define CONTROLLER_ADRC_H_


#include "config.h"
#include <string.h>
#include "arm_math.h"
#include "stdint.h"


/*
 * SECOND ORDER ESO - LINEAR ACTIVE DISTURBANCE REJECTION (LADRC)
 * */

typedef struct{

	float eso_bw;												  		// BĂNG THÔNG BỘ QUAN SÁT TA THIẾT KẾ

	float eso_gain_l1;													// GAIN BỘ QUAN SÁT ESO

	float eso_gain_l2;													// GAIN BỘ QUAN SÁT ESO

	float x1_hat;														// STATE 1, SPEED

	float x2_hat;														// STATE 2, UNKNOWN DISTURBANCES

	float prev_x1_hat;													// PREVIOUS VALUE X1

	float prev_x2_hat;													// PREVIOUS VALUE X2

	float y_hat;														// OUTPUT Y = CX

	float e_observer;													// SAI SỐ ƯỚC LƯỢNG

}eso_observer_t;

typedef struct{

	// PARAMETER

	float Ts;															// CHU KỲ THỰC THI ADRC

	float b0;															// THAM SỐ MÔ HÌNH Kt/J

	// ESO

	eso_observer_t eso;													// BỘ QUAN SÁT EXTENDED STATE OBSERVER

	// CONTROLLER PARAMETER

	float kp;															// GAIN CỦA CONTROL LAW OUTPUT, b0

	float ki;															// GAIN TÍCH PHÂN

	float integral;														// TÍCH PHÂN

	float aw_gain;														// GAIN ANTI-WINDUP

	float output;														// CONTROL COMMAND OUTPUT, u

	float output_max;													// MAX OUTPUT, FOR SATURATION

	float output_min;													// MIN OUTPUT, FOR SATURATION

	float error;														// SAI SỐ GIÁ TRỊ ĐẶT VÀ FEEDBACK

}LADRC_t;

void ladrc_init(LADRC_t* ladrc, float output_max, float output_min);	// KHỞI TẠO BỘ ĐIỀU KHIỂN LINEAR ADRC

float ladrc_update(LADRC_t* ladrc, float setpoint, float feedback);		// CẬP NHẬT CONTROL OUTPUT

void ladrc_reset(LADRC_t *ladrc);

#endif /* CONTROLLER_ADRC_H_ */
