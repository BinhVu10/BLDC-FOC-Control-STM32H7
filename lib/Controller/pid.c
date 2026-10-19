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
