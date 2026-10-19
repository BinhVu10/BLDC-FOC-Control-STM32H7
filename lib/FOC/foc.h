/*
 * Author: Binhvt22
**/

#ifndef HAND_MOTOR_CONTROL_FOC_H
#define HAND_MOTOR_CONTROL_FOC_H

#include <MotorApp.h>
#include <stdbool.h>
#include <stdint.h>
#include "pid.h"
#include "encoder_mt6826.h"
#include "pwm_port.h"
#include "stm32h7xx_hal.h"
#include "config.h"


typedef struct {

	float omega_ref_rpm;                                        // LỆNH RPM TỪ PI VỊ TRÍ, KHÔNG PHẢI FEEDBACK ENCODER

	float id;													// OUTPUT OF SPEED CONTROLLER

	float iq;													// OUTPUT OF SPEED CONTROLLER

    float vd;													// OUTPUT OF CURRENT CONTROLLER

    float vq;													// OUTPUT OF CURRENT CONTROLLER

    float v_alpha;												// OUTPUT OF INVERSED PARK

    float v_beta;												// OUTPUT OF INVERSED PARK

    float id_fb;												// FEEDBACK CURRENT D

    float iq_fb;												// FEEDBACK CURRENT Q

    float ia_fb;												// FEEDBACK CURRENT PHASE A

    float ib_fb;												// FEEDBACK CURRENT PHASE B

    float ic_fb;												// FEEDBACK CURRENT PHASE C

    float duty_a;												// DUTY CYCLE CẤP CHO PHA A

    float duty_b;												// DUTY CYCLE CẤP CHO PHA B

    float duty_c;												// DUTY CYCLE CẤP CHO PHA C

    float offset_current_a;										// OFFSET DÒNG PHA A

    float offset_current_b;										// OFFSET DÒNG PHA B

    float v_bus;												// ĐIỆN ÁP BUS

    float voltage_limit_max;									// VDC/CĂN(3)

    float current_limit_max;                                    // GIỚI HẠN VECTOR DÒNG ĐẶT, A

    uint32_t speed_cnt;                                         // CHIA NHỊP PI VẬN TỐC; KHÁC BỘ ĐẾM ĐO VẬN TỐC TRONG ENCODER

    uint32_t pos_cnt;											// CHIA NHỊP PI VỊ TRÍ

    // CONTROLLER
     pi_t pi_id;												// BỘ ĐIỀU KHIỂN DÒNG ID

     pi_t pi_iq;												// BỘ ĐIỀU KHIỂN DÒNG IQ

     pi_t pi_speed;												// BỘ ĐIỀU KHIỂN VẬN TỐC

     pi_t pi_pos;												// BỘ ĐIỀU KHIỂN VỊ TRÍ

} foc_t;

void foc_init(foc_t *foc);

// CÁC HÀM CONTROL ĐƯỢC GỌI THEO NHỊP FOC_FREQ_HZ, ENCODER ĐÃ CÓ MẪU HỢP LỆ.
void foc_current_ctrl_run(foc_t *foc, const encoder_t *encoder, float id_ref, float iq_ref);

void foc_speed_ctrl_run(foc_t *foc, const encoder_t *encoder, float rpm_ref);

void foc_ctrl_pos_run(foc_t *foc, const encoder_t *encoder, float pos_ref_rad);

void foc_open_loop_control(foc_t *foc, float vd, float vq, float theta_e);

void foc_voltage_limit_dq(foc_t *foc, float *v_d, float *v_q);

void foc_current_limit_dq(foc_t *foc, float *id, float *iq);

void foc_zero_pwm_output(foc_t *foc);

void foc_off(foc_t *foc);

void foc_voltage_bus_read(foc_t *foc);

#endif /* HAND_MOTOR_CONTROL_FOC_H */
