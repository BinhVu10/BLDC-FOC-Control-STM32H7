/*
 * Author: Binhvt22
**/

#include "foc_math.h"
#include "config.h"

void clarke_transform(float ia, float ib, float ic, float *i_alpha, float *i_beta)
{
    *i_alpha = TWO_BY_THREE * (ia - 0.5f * ib - 0.5f * ic);

    *i_beta = TWO_BY_THREE * SQRT3_BY_TWO * (ib - ic);
}

void park_transform(float i_alpha, float i_beta, float sin_theta, float cos_theta,  float *i_d, float *i_q)
{
    *i_d = i_alpha * cos_theta + i_beta * sin_theta;

    *i_q = -i_alpha * sin_theta + i_beta * cos_theta;
}

void clarke_park_transform(float ia, float ib, float ic, float sin_theta, float cos_theta, float *i_d, float *i_q){

    float i_alpha = TWO_BY_THREE * (ia - 0.5f * ib - 0.5f * ic);

    float i_beta = TWO_BY_THREE * SQRT3_BY_TWO * (ib - ic);

    *i_d = i_alpha * cos_theta + i_beta * sin_theta;

    *i_q = -i_alpha * sin_theta + i_beta * cos_theta;
}

void inverse_park_transform(float v_d, float v_q, float sin_theta, float cos_theta, float *v_alpha, float *v_beta)
{
    *v_alpha = v_d * cos_theta - v_q * sin_theta;

    *v_beta = v_d * sin_theta + v_q * cos_theta;
}

void svpwm(float v_alpha, float v_beta, float v_bus, float *duty_a, float *duty_b, float *duty_c){

	// SECTOR DETERMINATION

	uint8_t sector;

	if (v_beta >= 0.0f) {

	    if (v_alpha >= 0.0f) {

	        sector = (ONE_BY_SQRT3 * v_beta > v_alpha) ? 2 : 1;  // 1/sqrt(3) ≈ 0.577

	    } else {

	        sector = (-ONE_BY_SQRT3 * v_beta > v_alpha) ? 3 : 2;
	    }
	}
	else {

	    if (v_alpha >= 0.0f) {

	        sector = (-ONE_BY_SQRT3 * v_beta > v_alpha) ? 5 : 6;
	    }
	    else {

	        sector = (ONE_BY_SQRT3 * v_beta > v_alpha) ? 4 : 5;
	    }
	}

	float t1, t2, t0h;

	float inv_vdc = 1.0f/ v_bus;

	// CALCULATE DUTY CYCLE
	switch (sector){

		case 1:

			t1 = 0.5f * (3.0f * v_alpha - SQRT3 * v_beta) * inv_vdc;

			t2 = (SQRT3 * v_beta) * inv_vdc;

			t0h = 0.5f * (1.0f - t1 - t2);

			*duty_a = t1 + t2 + t0h;

			*duty_b = t2 + t0h;

			*duty_c =t0h;

			break;


		case 2:

			t1 = 0.5f * (3.0f * v_alpha + SQRT3 * v_beta) * inv_vdc;

			t2 = 0.5f * (-3.0f * v_alpha + SQRT3 * v_beta) * inv_vdc;

			t0h = 0.5f * (1.0f - t1 - t2);

			*duty_a = t1 + t0h;

			*duty_b = t1 + t2 + t0h;

			*duty_c = t0h;

			break;

		case 3:

			t1 =(SQRT3 * v_beta) * inv_vdc;

			t2 = 0.5f * (-3.0f * v_alpha - SQRT3 * v_beta) * inv_vdc;

			t0h = 0.5f * (1.0f - t1 - t2);

			*duty_a = t0h;

			*duty_b = t1 + t2 + t0h;

			*duty_c = t2 + t0h;

			break;

		case 4:

			t1 = 0.5f * (-3.0f * v_alpha + SQRT3 * v_beta) * inv_vdc;

			t2 = (-SQRT3 * v_beta) * inv_vdc;

			t0h = 0.5f * (1.0f - t1 - t2);

			*duty_a = t0h;

			*duty_b =  t1 + t0h;

			*duty_c = t1 + t2 + t0h;

			break;

		case 5:

			t1 = 0.5f * (-3.0f * v_alpha - SQRT3 * v_beta) * inv_vdc;

			t2 = 0.5f * ( 3.0f * v_alpha - SQRT3 * v_beta) * inv_vdc;

			t0h = 0.5f * (1.0f - t1 - t2);

			*duty_a = t2 + t0h;

			*duty_b =  t0h;

			*duty_c = t1 + t2 + t0h;

			break;

		case 6:

			t1 = (-SQRT3 * v_beta) * inv_vdc;

			t2 = 0.5f * (3.0f * v_alpha + SQRT3 * v_beta) * inv_vdc;

			t0h = 0.5f * (1.0f - t1 - t2);

			*duty_a = t1 + t2 + t0h;

			*duty_b = t0h;

			*duty_c =  t1 + t0h;

			break;

		default:

			*duty_a = 0.5f;

			*duty_b = 0.5f;

			*duty_c = 0.5f;

			break;
	}

	// Clamp duty
	*duty_a = CONSTRAIN(*duty_a, 0.0f, 1.0f);

	*duty_b = CONSTRAIN(*duty_b, 0.0f, 1.0f);

	*duty_c = CONSTRAIN(*duty_c, 0.0f, 1.0f);
}
