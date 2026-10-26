/*
 * Author: Binhvt22
**/

#ifndef PID_H
#define PID_H

typedef struct{

    float kp;

    float ki;

    float kd;

    float kd_filter;												// HỆ SỐ LỌC ĐẠO HÀM

    float aw_gain;													// ANTI-WINDUP GAIN

    float integral;													// THÀNH PHẦN TÍCH PHÂN

    float derivative;												// THÀNH PHẦN ĐẠO HÀM

    float ts;

    float output;													// TÍN HIỆU ĐIỀU KHIỂN

    float output_max;												// OUTPUT MAX

    float output_min;												// OUTPUT MIN

    float error;													// SAI SỐ

}pid_t;



typedef struct {

    float kp;

    float ki;

    float integral;

    float ts;

    float output;

    float error;

} pi_t;

void pi_init(pi_t *pi, float kp, float ki, float ts);

float pi_update(pi_t *pi, float setpoint, float feedback);

void pi_anti_windup(pi_t *pi, float output_limited, float aw_gain);

void pi_reset(pi_t *pi);

void pid_init(pid_t *pid, float kp, float ki, float kd, float kd_filter, float aw_gain, float output_max, float output_min, float ts);

void pid_reset(pid_t *pid);

float pid_run(pid_t *pid, float setpoint, float feedback);


#endif /* PID_H */
