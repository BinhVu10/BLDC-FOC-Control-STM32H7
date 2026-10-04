/*
 * Author: Binhvt22
**/

#include "foc.h"
#include "foc_math.h"
#include "config.h"
#include "pwm_port.h"
#include "tim.h"
#include <string.h>
#include <math.h>
#include "arm_math.h"
#include "encoder_mt6826.h"
#include "adc_port.h"


// KHỞI TẠO FOC
void foc_init(foc_t *foc){

	if (foc == NULL) {
		return;
	}

	memset(foc, 0, sizeof(*foc));

	// LIMIT ĐIỆN ÁP
	foc->voltage_limit_max = foc->v_bus * ONE_BY_SQRT3;

	foc->current_limit_max = 3.0f;


	// KHỞI TẠO THAM SỐ BỘ ĐIỀU KHIỂN
	pi_init(&foc->pi_id, KP_Id, KI_Id, FOC_TS);

	pi_init(&foc->pi_iq, KP_Iq, KI_Iq, FOC_TS);

	pi_init(&foc->pi_speed, KP_SPEED, KI_SPEED, FOC_SPEED_TS);

	pi_init(&foc->pi_pos, KP_POS, KI_POS, FOC_POS_TS);

}

// ĐIỀU KHIỂN DÒNG ĐIỆN (TORQUE)
void foc_current_ctrl_run(foc_t *foc, const encoder_t *encoder, float id_ref, float iq_ref){

	uint32_t pwm_arr;

	if (foc == NULL) {
		return;
	}

	if (foc->v_bus <= 0.0f) {

		foc_off(foc);

		return;
	}

	// DÙNG CÙNG MỘT GÓC ĐIỆN CHO PARK VÀ INVERSE PARK DÙ SPI CẬP NHẬT GIỮA CHU KỲ.
	const float theta_e = encoder->theta_e;

	const float sin_theta = arm_sin_f32(theta_e);

	const float cos_theta = arm_cos_f32(theta_e);

	// CLARKE - PARK TRANSFORM
	clarke_park_transform(foc->ia_fb, foc->ib_fb, foc->ic_fb, sin_theta, cos_theta, &foc->id_fb, &foc->iq_fb);

	// RUN CONTROLLER
	foc->vd = pi_update(&foc->pi_id, id_ref, foc->id_fb);

	foc->vq = pi_update(&foc->pi_iq, iq_ref, foc->iq_fb);

	// CHẶN VD/CĂN(3)
	foc_voltage_limit_dq(foc, &foc->vd, &foc->vq);

	// ANTI-WINDUP BACK-CALCULATION
	pi_anti_windup(&foc->pi_id, foc->vd, AW_GAIN_CURRENT);

	pi_anti_windup(&foc->pi_iq, foc->vq, AW_GAIN_CURRENT);

	// INVERSED PARK
	inverse_park_transform(foc->vd, foc->vq, sin_theta, cos_theta, &foc->v_alpha, &foc->v_beta);

	// BĂM XUNG
	svpwm(foc->v_alpha, foc->v_beta, foc->v_bus, &foc->duty_a, &foc->duty_b, &foc->duty_c);

	// CẬP NHẬT CCR
	pwm_arr = __HAL_TIM_GET_AUTORELOAD(&htim1);

	pwm_set((uint32_t)(foc->duty_a * (float)pwm_arr),

			(uint32_t)(foc->duty_b * (float)pwm_arr),

			(uint32_t)(foc->duty_c * (float)pwm_arr));
}

// ĐIỀU KHIỂN TỐC ĐỘ (SAU GEAR)
void foc_speed_ctrl_run(foc_t *foc, const encoder_t *encoder, float rpm_ref){

	if (foc == NULL) {
		return;
	}

	foc->speed_cnt++;

	if(foc->speed_cnt >= SPEED_DIV){

		// ĐIỀU KHIỂN VẬN TỐC
		foc->iq = pi_update(&foc->pi_speed, rpm_ref, encoder->speed_filter_after_gear);

		foc->id = 0.0f;

		// GIỚI HẠN DÒNG - STATURATION CURRENT
		foc_current_limit_dq(foc, &foc->id, &foc->iq);

		// ANTI-WINDUP BỘ PI VẬN TỐC
		pi_anti_windup(&foc->pi_speed, foc->iq, AW_GAIN_SPEED);

		foc->speed_cnt = 0U;
	}

	// ĐIỀU KHIỂN DÒNG
	foc_current_ctrl_run(foc, encoder, foc->id, foc->iq);
}

// ĐIỀU KHIỂN VỊ TRÍ (SAU GEAR)
void foc_ctrl_pos_run(foc_t *foc, const encoder_t *encoder, float pos_ref_rad){

	if (foc == NULL) {
		return;
	}

	foc->pos_cnt++;

	if (foc->pos_cnt >= POS_DIV) {

		foc->omega_ref_rpm = pi_update(&foc->pi_pos, pos_ref_rad, encoder->angle_after_gear);

		// GIỚI HẠN VẬN TỐC ĐẦU RA BỘ VỊ TRÍ
		foc->omega_ref_rpm = CONSTRAIN(foc->omega_ref_rpm,-SPEED_LIMIT, SPEED_LIMIT);

		foc->pos_cnt = 0U;
	}

	foc_speed_ctrl_run(foc, encoder, foc->omega_ref_rpm);
}

// ĐIỂU KHIỂN VÒNG HỞ ĐIỆN ÁP
void foc_open_loop_control(foc_t *foc, float vd, float vq, float theta_e){

	uint32_t pwm_arr;

	float sin_theta;

	float cos_theta;

	if ((foc == NULL) || (foc->v_bus <= 0.0f)) {
		return;
	}

	foc_voltage_limit_dq(foc, &vd, &vq);

	sin_theta = arm_sin_f32(theta_e);

	cos_theta = arm_cos_f32(theta_e);

	inverse_park_transform(vd, vq, sin_theta, cos_theta, &foc->v_alpha, &foc->v_beta);

	svpwm(foc->v_alpha, foc->v_beta, foc->v_bus, &foc->duty_a, &foc->duty_b, &foc->duty_c);

	pwm_arr = __HAL_TIM_GET_AUTORELOAD(&htim1);

	pwm_set((uint32_t)(foc->duty_a * (float)pwm_arr),

			(uint32_t)(foc->duty_b * (float)pwm_arr),

			(uint32_t)(foc->duty_c * (float)pwm_arr));
}

// LIMIT ĐIỆN ÁP DQ TRONG BÁN KÍNH VDC/CĂN(3)
void foc_voltage_limit_dq(foc_t *foc, float *v_d, float *v_q){

	float voltage_square;

	float voltage_magnitude;

	float scale;

	if ((foc == NULL) || (v_d == NULL) || (v_q == NULL)) {
		return;
	}

	if (foc->voltage_limit_max <= 0.0f) {

		*v_d = 0.0f;

		*v_q = 0.0f;

		return;
	}

	voltage_square = (*v_d * *v_d) + (*v_q * *v_q);

	if (voltage_square > foc->voltage_limit_max * foc->voltage_limit_max) {

        voltage_magnitude = sqrtf(voltage_square);

		scale = foc->voltage_limit_max / voltage_magnitude;

		*v_d *= scale;

		*v_q *= scale;
	}
}

// LIMIT DÒNG ĐIỆN DÒNG GIỚI HẠN
void foc_current_limit_dq(foc_t *foc, float *id, float *iq){

	float current_square;

	float current_magnitude;

	float scale;

	if ((foc == NULL) || (id == NULL) || (iq == NULL)) {
		return;
	}

	if (foc->current_limit_max <= 0.0f) {

		*id = 0.0f;

		*iq = 0.0f;

		return;
	}

	current_square = (*id * *id) + (*iq * *iq);

	if (current_square > foc->current_limit_max * foc->current_limit_max) {

		current_magnitude = sqrtf(current_square);

		scale = foc->current_limit_max / current_magnitude;

		*id *= scale;

		*iq *= scale;
	}
}

// KHÔNG XUẤT XUNG PWM --> KHÔNG CẤP ÁP
void foc_zero_pwm_output(foc_t *foc)
{
    if (foc == NULL) {
        return;
    }

    foc->duty_a = 0.0f;

    foc->duty_b = 0.0f;

    foc->duty_c = 0.0f;
}

// HÀM TẮT FOC
void foc_off(foc_t *foc){

	if (foc == NULL) {

		return;
	}

	foc_zero_pwm_output(foc);

	pwm_set(0U, 0U, 0U);

	pi_reset(&foc->pi_id);

	pi_reset(&foc->pi_iq);

	pi_reset(&foc->pi_speed);

	pi_reset(&foc->pi_pos);

	foc->id = 0.0f;

	foc->iq = 0.0f;

	foc->vd = 0.0f;

	foc->vq = 0.0f;

	foc->omega_ref_rpm = 0.0f;

	foc->speed_cnt = 0U;

	foc->pos_cnt = 0U;
}

// HÀM ĐỌC GIÁ TRỊ VOLTAGE BUS
void foc_voltage_bus_read(foc_t *foc){

	uint16_t voltage_bus_raw;

	if (foc == NULL) {
		return;
	}

	voltage_bus_raw = read_voltage_bus_raw();

	foc->v_bus = (float)voltage_bus_raw * ADC_VREF * VBUS_DIV_GAIN / ADC_MAX;

	foc->voltage_limit_max = foc->v_bus * ONE_BY_SQRT3;
}
