#include "Calib.h"
#include "spi.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

SPI_HandleTypeDef hspi3;
static GPIO_PinState cs;
static unsigned reads, fail_at, fail_kind, align_calls, delay_calls, stop_calls;
static bool powered;
static float bus_voltage;
static const uint16_t raw_samples[] = {1000U, 1000U, 32760U, 8U, 32752U, 16U};

static void near(float actual, float expected)
{
    assert(fabsf(actual - expected) < 0.0001f);
}

static void frame(uint8_t *rx, uint16_t raw, uint8_t status)
{
    rx[2] = (uint8_t)(raw >> 7);
    rx[3] = (uint8_t)(raw << 1);
    rx[4] = status;
    uint32_t remainder = ((uint32_t)rx[2] << 24) | ((uint32_t)rx[3] << 16)
                       | ((uint32_t)status << 8);
    for (int bit = 31; bit >= 8; --bit) {
        if (remainder & (UINT32_C(1) << bit))
            remainder ^= UINT32_C(0x107) << (bit - 8);
    }
    rx[5] = (uint8_t)remainder;
}

void HAL_GPIO_WritePin(void *port, uint16_t pin, GPIO_PinState state)
{
    assert(port == ENCODER_CS_GPIO_Port && pin == ENCODER_CS_Pin);
    cs = state;
}

HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(
    SPI_HandleTypeDef *spi, uint8_t *tx, uint8_t *rx, uint16_t length)
{
    (void)spi; (void)tx; (void)rx; (void)length;
    assert(0 && "CALIB MUST NOT START DMA");
    return HAL_ERROR;
}

HAL_StatusTypeDef HAL_SPI_TransmitReceive(
    SPI_HandleTypeDef *spi, uint8_t *tx, uint8_t *rx, uint16_t length, uint32_t timeout)
{
    assert(spi == &hspi3 && cs == GPIO_PIN_RESET && length == 6U && timeout == 1U);
    assert(tx[0] == 0xA0U && tx[1] == 0x03U && reads < 6U);
    assert(align_calls == (reads + 1U) * 6U && delay_calls == align_calls);
    uint16_t raw = raw_samples[reads++];
    frame(rx, raw, reads == fail_at && fail_kind == 2U ? 1U : 0U);
    if (reads == fail_at) {
        if (fail_kind == 1U) rx[5] ^= 1U;
        if (fail_kind == 3U) return HAL_TIMEOUT;
    }
    return HAL_OK;
}

uint32_t __get_PRIMASK(void) { return 0U; }
void __disable_irq(void) { assert(0 && "CALIB MUST NOT MASK INTERRUPTS"); }
void __set_PRIMASK(uint32_t value) { (void)value; }

void HAL_Delay(uint32_t delay)
{
    assert(delay == 200U && powered);
    delay_calls++;
}

void foc_voltage_bus_read(foc_t *foc) { foc->v_bus = bus_voltage; }
void foc_open_loop_control(foc_t *foc, float vd, float vq, float theta)
{
    (void)foc;
    const float phases[] = {DEG_TO_RAD(120.0f), 0.0f, DEG_TO_RAD(-120.0f), 0.0f};
    const float voltages[] = {VD_CALIB, VD_CALIB / 2.0f, VD_CALIB / 4.0f};
    near(theta, phases[(align_calls % 12U) / 3U]);
    near(vd, voltages[align_calls % 3U]);
    near(vq, VQ_CALIB);
    powered = true;
    align_calls++;
}

void foc_zero_pwm_output(foc_t *foc)
{
    foc->duty_a = foc->duty_b = foc->duty_c = 0.0f;
}

void pwm_set(uint32_t a, uint32_t b, uint32_t c)
{
    assert(a == 0U && b == 0U && c == 0U);
    powered = false;
    stop_calls++;
}

static void setup(encoder_t *encoder, foc_t *foc)
{
    reads = align_calls = delay_calls = stop_calls = fail_at = fail_kind = 0U;
    powered = false;
    bus_voltage = 12.0f;
    memset(foc, 0, sizeof(*foc));
    encoder_init(encoder);
    frame(encoder->rx_buffer, 8192U, 0U);
    encoder_Read_Angle(encoder);
    encoder->speed_after_gear = 12.0f;
    encoder->speed_filter_after_gear = 10.0f;
}

int main(void)
{
    encoder_t encoder;
    foc_t foc;
    setup(&encoder, &foc);
    assert(run_calib_encoder(&foc, &encoder, 3U));
    assert(reads == 6U && align_calls == 36U && delay_calls == 36U);
    assert(!powered && stop_calls == 1U && cs == GPIO_PIN_SET);
    near(encoder.offset_encoder, 0.0f); // Circular average across 0/2*pi.
    near(encoder.theta_e, 16.0f * TWO_PI / ENCODER_CPR);
    near(encoder.mechanical_zero_angle, 0.0f);
    near(encoder.speed_after_gear, 12.0f); // Mode entry/exit owns estimator reset.
    near(encoder.speed_filter_after_gear, 10.0f);

    // Reject a bad frame at every read position, including the discarded first cycle.
    for (unsigned kind = 1U; kind <= 3U; ++kind) {
        for (unsigned position = 1U; position <= 6U; ++position) {
            setup(&encoder, &foc);
            fail_kind = kind;
            fail_at = position;
            assert(!run_calib_encoder(&foc, &encoder, 3U));
            assert(reads == position && !powered && stop_calls == 1U);
            assert(cs == GPIO_PIN_SET && !encoder.success);
            near(encoder.offset_encoder, ENCODER_OFFSET);
            near(encoder.mechanical_zero_angle, 0.0f);
            float last_angle = position == 1U ? PI / 2.0f
                : (float)raw_samples[position - 2U] * TWO_PI / ENCODER_CPR;
            near(encoder.theta_m, last_angle);
        }
    }

    setup(&encoder, &foc);
    assert(!run_calib_encoder(&foc, &encoder, 0U));
    assert(!run_calib_encoder(&foc, &encoder, 1U));
    bus_voltage = 0.0f;
    assert(!run_calib_encoder(&foc, &encoder, 3U));
    assert(reads == 0U && align_calls == 0U && !powered);
    near(encoder.offset_encoder, ENCODER_OFFSET);
    puts("PASS: unchanged alignment sequence, polling, circular offset, zero preservation, failure cleanup");
    return 0;
}
