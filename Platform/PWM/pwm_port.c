/*
 * timer.c
 *
 *  Created on: Sep 10, 2026
 *      Author: binhvt22
 */
#include "pwm_port.h"
#include "tim.h"

void pwm_start(void)
{
    (void)HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);							// BẬT TIMER TẠO PWM KÊNH 2
    (void)HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);							// BẬT TIMER TẠO PWM KÊNH 3
    (void)HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);							// BẬT TIMER TẠO PWM KÊNH 4
    (void)HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_1);							// BẬT TIMER SO SÁNH KÊNH 1 (ĐỂ NGẮT TRGO - ADC)
    (void)HAL_TIM_Base_Start(&htim2);										// BẬT TIMER ĐỊNH THỜI ĐO THỜI GIAN
    (void)HAL_TIM_Base_Start_IT(&htim7);									// BẬT TIMER ĐỊNH THỜI ĐỂ TRUYỀN UART
}

void pwm_set(uint32_t duty_a, uint32_t duty_b, uint32_t duty_c)
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, duty_a);					// XÉT DUTY PWM
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, duty_b);					// XÉT DUTY PWM
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, duty_c);					// XÉT DUTY PWM
}

