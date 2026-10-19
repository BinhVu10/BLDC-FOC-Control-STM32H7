#include "foc.h"
#include "tim.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

TIM_HandleTypeDef htim1 = {4999U};
static encoder_t *interrupt_encoder;
static float trig_angle, park_sin, park_cos, inverse_sin, inverse_cos;
static unsigned current_calls;
static uint32_t pwm[3];

static void near(float actual, float expected)
{
    assert(fabsf(actual - expected) < 0.0001f);
}

uint16_t read_voltage_bus_raw(void) { return 40000U; }
void pwm_set(uint32_t a, uint32_t b, uint32_t c)
{
    pwm[0] = a; pwm[1] = b; pwm[2] = c;
}

float arm_sin_f32(float angle)
{
    trig_angle = angle;
    return angle + 10.0f;
}

float arm_cos_f32(float angle)
{
    assert(angle == trig_angle);
    return angle + 20.0f;
}

// OBSERVE ORCHESTRATION; TRANSFORM/SVPWM IMPLEMENTATIONS ARE TESTED BY THE FIRMWARE BUILD.
void clarke_park_transform(float a, float b, float c, float sin_theta, float cos_theta,
                           float *d, float *q)
{
    (void)c;
    current_calls++;
    park_sin = sin_theta;
    park_cos = cos_theta;
    *d = a; *q = b;
    if (interrupt_encoder) interrupt_encoder->theta_e += 0.5f;
}

void inverse_park_transform(float d, float q, float sin_theta, float cos_theta,
                            float *a, float *b)
{
    inverse_sin = sin_theta;
    inverse_cos = cos_theta;
    *a = d; *b = q;
}

void svpwm(float a, float b, float bus, float *da, float *db, float *dc)
{
    (void)a; (void)b; (void)bus;
    *da = 0.25f; *db = 0.5f; *dc = 0.75f;
}

int main(void)
{
    foc_t f;
    encoder_t e = {0};
    foc_init(&f);
    foc_voltage_bus_read(&f);
    assert(FOC_FREQ_HZ == 20000U && FOC_SPEED_LOOP_HZ == 1000U);
    assert(FOC_POS_LOOP_HZ == 100U && SPEED_DIV == 20U && POS_DIV == 200U);
    near(f.pi_id.ts, 0.00005f);
    near(f.pi_speed.ts, 0.001f);
    near(f.pi_pos.ts, 0.01f);
    near(f.current_limit_max, CURRENT_MAX);

    e.theta_e = 1.0f;
    f.ia_fb = 0.2f;
    f.ib_fb = -0.3f;
    interrupt_encoder = &e;
    foc_current_ctrl_run(&f, &e, 0.4f, 0.5f);
    near(trig_angle, 1.0f);
    near(park_sin, 11.0f);
    near(park_cos, 21.0f);
    near(inverse_sin, park_sin);
    near(inverse_cos, park_cos); // SAME TRIG VALUES EVEN WHEN DMA PUBLISHES IN BETWEEN.
    near(e.theta_e, 1.5f);
    near(f.pi_id.error, 0.2f);
    near(f.pi_iq.error, 0.8f);
    assert(pwm[0] == 1249U && pwm[1] == 2499U && pwm[2] == 3749U);
    interrupt_encoder = NULL;

    // Independent position/speed counters retain their original scheduling model.
    foc_off(&f);
    unsigned start_calls = current_calls;
    float previous_speed_error = 0.0f;
    float previous_pos_error = 0.0f;
    for (unsigned tick = 1; tick <= 400U; ++tick) {
        e.mechanical_angle = (float)tick * 0.0001f;
        e.speed_filter_after_gear = (float)tick * 0.001f;
        foc_ctrl_pos_run(&f, &e, 0.1f);
        if (tick % POS_DIV == 0U) previous_pos_error = 0.1f - e.mechanical_angle;
        if (tick % SPEED_DIV == 0U)
            previous_speed_error = f.omega_ref_rpm - e.speed_filter_after_gear;
        near(f.pi_pos.error, previous_pos_error);
        near(f.pi_speed.error, previous_speed_error);
        assert(f.pos_cnt == tick % POS_DIV && f.speed_cnt == tick % SPEED_DIV);
        assert(current_calls == start_calls + tick);
    }
    near(f.pi_pos.error, 0.06f);

    // Speed-only mode uses encoder RPM and preserves current clamp/anti-windup.
    foc_off(&f);
    e.speed_filter_after_gear = 12.0f;
    for (unsigned tick = 0; tick < SPEED_DIV; ++tick)
        foc_speed_ctrl_run(&f, &e, 1000.0f);
    near(f.pi_speed.error, 988.0f);
    near(f.iq, f.current_limit_max);
    near(f.id, 0.0f);
    assert(f.pi_speed.integral < 0.0f); // Back-calculation follows the clamp.

    // Stopping FOC resets controllers but does not re-zero encoder or its estimator.
    e.mechanical_zero_angle = 0.7f;
    e.turn_count = 3;
    e.angle_delta_sum = 0.12f;
    encoder_t saved = e;
    foc_off(&f);
    assert(memcmp(&saved, &e, sizeof(e)) == 0);
    assert(pwm[0] == 0U && pwm[1] == 0U && pwm[2] == 0U);
    near(f.pi_speed.integral, 0.0f);
    near(f.omega_ref_rpm, 0.0f);
    puts("PASS: encoder feedback, latched Park angle, 20k/1k/100Hz, clamp/anti-windup, FOC reset");
    return 0;
}
