/*
 * adrc_discrete.c
 *
 *  Created on: Oct 28, 2026
 *
 *      Author: THANH BINH
 */

#ifndef CONTROLLER_ADRC_DISCRETE_H_
#define CONTROLLER_ADRC_DISCRETE_H_


/*
 * SECOND ORDER ESO
 * */
typedef struct{

	float k_eso;														// HỆ SỐ TƯƠNG ĐỐI BỘ QUAN SÁT VÀ BỘ ĐIỀU KHIỂN

	float w_Observer;												    // BĂNG THÔNG BỘ QUAN SÁT TA THIẾT KẾ

	float gain_eso_l1, gain_eso_l2;										// GAI BỘ QUAN SÁT

	float A_ESO11, A_ESO12, A_ESO21, A_ESO22;							// HỆ SỐ CỦA MA TRẬN A TRONG BỘ QUAN SÁT

	float b_ESO1, b_ESO2;												// HỆ SỐ MA TRẬN B TRONG BỘ QUAN SÁT

	float x1_hat, x2_hat;												// TRẠNG THÁI ƯỚC LƯỢNG

}eso_discrete_observer_t;

typedef struct{

	// PARAMETER
	float f_bw_controller;												// BĂNG THÔNG BỘ ĐIỀU KHIỂN ADRC, HZ

	float w_CL;															// BĂNG THÔNG BỘ DIỀU KHIỂN ADRC, RAD/S

	float Ts;															// CHU KỲ LẤY MẪU THỰC THI ADRC

	float b0;															// THAM SỐ MÔ HÌNH Kt/J - CRITICAL GAIN

	// ESO
	eso_discrete_observer_t eso;											// BỘ QUAN SÁT EXTENDED STATE OBSERVER

	// CONTROLLER PARAMETER

	float kp;															// GAIN CỦA CONTROL LAW OUTPUT, b0

	float ki;															// GAIN TÍCH PHÂN

	float integral;														// TÍCH PHÂN

	float aw_gain;														// GAIN ANTI-WINDUP

	float output;														// CONTROL COMMAND OUTPUT, u

	float output_max;													// MAX OUTPUT, FOR SATURATION

	float output_min;													// MIN OUTPUT, FOR SATURATION

	float error;														// SAI SỐ GIÁ TRỊ ĐẶT VÀ FEEDBACK

}LADRC_DISCRETE_t;

void ladrc_discrete_init(LADRC_DISCRETE_t* ladrc, float output_max, float output_min);		// KHỞI TẠO BỘ ĐIỀU KHIỂN LINEAR ADRC

float ladrc_discrete_update(LADRC_DISCRETE_t* ladrc, float setpoint, float feedback);		// CẬP NHẬT CONTROL OUTPUT

void ladrc_discrete_reset(LADRC_DISCRETE_t *ladrc);											// RESET BỘ ĐIỀU KHIỂN ADRC

#endif /* CONTROLLER_ADRC_DISCRETE_H_ */
