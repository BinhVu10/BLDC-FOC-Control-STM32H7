/*
 * Author: Binhvt22
**/

#include "pid.h"


void pi_init(pi_t *pi, float kp, float ki, float ts)
{
    pi->kp = kp;

    pi->ki = ki;

    pi->ts = ts;

    pi->integral = 0.0f;

    pi->output = 0.0f;

    pi->error = 0.0f;
}

float pi_update(pi_t *pi, float setpoint, float feedback)
{
    pi->error = setpoint - feedback;

    float kp_term, integral_temp;

    kp_term = pi->kp * pi->error;

    integral_temp = pi->ki * pi->error * pi->ts;

    if (pi->ki == 0.0f) {

        pi->integral = 0.0f;

    }

    else {

        pi->integral = pi->integral + integral_temp;

    }

    pi->output = kp_term + pi->integral;


    return pi->output;
}

void pi_anti_windup(pi_t *pi, float output_limited, float aw_gain)
{
    if (pi->ki != 0.0f) {

        pi->integral += aw_gain * (output_limited - pi->output);

    }
}

void pi_reset(pi_t *pi)
{
    pi->integral = 0.0f;

    pi->output = 0.0f;

    pi->error = 0.0f;

}

void pid_init(pid_t *pid, float kp, float ki, float kd, float kd_filter, float aw_gain, float output_max, float output_min, float ts){

	pid->kp = kp;

	pid->ki = ki;

	pid->kd = kd;

	if (kd_filter < 0.0f) {

		pid->kd_filter = 0.0f;

	}

	else if (kd_filter > 1.0f) {

		pid->kd_filter = 1.0f;

	}

	else {

		pid->kd_filter = kd_filter;

	}

	pid->aw_gain = aw_gain;

	pid->integral = 0.0f;

	pid->derivative = 0.0f;

	pid->output = 0.0f;

	pid->error = 0.0f;

	pid->output_max = output_max;

	pid->output_min = output_min;

	pid->ts = ts;

}

void pid_reset(pid_t *pid){

	pid->error = 0.0f;

	pid->integral = 0.0f;

	pid->derivative = 0.0f;

	pid->output = 0.0f;

}

float pid_run(pid_t *pid, float setpoint, float feedback){

	float error = setpoint - feedback;

	float kp_term = pid->kp * error;

	if ((pid->ki != 0.0f) && (pid->ts > 0.0f)) {

		pid->integral += pid->ki * error * pid->ts;

	}

	else {

		pid->integral = 0.0f;

	}

    if ((pid->kd != 0.0f) && (pid->ts > 0.0f)) {

		float derivative_term = pid->kd * (error - pid->error) / pid->ts;

		pid->derivative = (1.0f - pid->kd_filter) * pid->derivative + pid->kd_filter * derivative_term;

    }

    else {

		pid->derivative = 0.0f;
    }

	// CONTROL OUTPUT
	float output = kp_term + pid->integral + pid->derivative;

	// SATURATION OUTPUT

	if(output > pid->output_max){

		pid->output = pid->output_max;

	}

	else if(output < pid->output_min){

		pid->output = pid->output_min;

	}

	else{

		pid->output = output;
	}


	// BACK-CALCULATION ANTI-WINUP
	if ((pid->ki != 0.0f) && (pid->ts > 0.0f)) {

		pid->integral += pid->aw_gain* (pid->output - output);

	}

	pid->error = error;

	return pid->output;

}
