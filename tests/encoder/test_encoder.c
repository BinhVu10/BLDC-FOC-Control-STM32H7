/* Host tests: the real encoder source is linked with a small HAL substitute. */
#include "encoder_mt6826.h"
#include "config.h"
#include "spi.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

SPI_HandleTypeDef hspi3;
static HAL_StatusTypeDef transfer_result;
static GPIO_PinState cs;
static unsigned transfers;
static uint32_t irq_mask;
static uint8_t polling_frame[MT6826_FRAME_LENGTH];
static unsigned polling_calls;

void HAL_GPIO_WritePin(void *port, uint16_t pin, GPIO_PinState state)
{
    assert(port == ENCODER_CS_GPIO_Port && pin == ENCODER_CS_Pin);
    cs = state;
}

HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(
    SPI_HandleTypeDef *spi, uint8_t *tx, uint8_t *rx, uint16_t length)
{
    assert(spi == &hspi3 && cs == GPIO_PIN_RESET && rx != NULL);
    assert(length == 6U && tx[0] == 0xA0U && tx[1] == 0x03U);
    for (unsigned i = 2; i < 6; ++i) assert(tx[i] == 0U);
    transfers++;
    return transfer_result;
}

uint32_t __get_PRIMASK(void) { return irq_mask; }
void __disable_irq(void) { irq_mask = 1U; }
void __set_PRIMASK(uint32_t value) { irq_mask = value; }

HAL_StatusTypeDef HAL_SPI_TransmitReceive(
    SPI_HandleTypeDef *spi, uint8_t *tx, uint8_t *rx, uint16_t length, uint32_t timeout)
{
    assert(spi == &hspi3 && cs == GPIO_PIN_RESET && timeout == 1U);
    assert(length == MT6826_FRAME_LENGTH && tx[0] == 0xA0U && tx[1] == 0x03U);
    polling_calls++;
    memcpy(rx, polling_frame, length);
    return transfer_result;
}

static void near(float actual, float expected)
{
    assert(fabsf(actual - expected) < 0.0001f);
}

/* Independent polynomial-division reference for CRC-8, polynomial 0x107. */
static void frame(encoder_t *e, uint16_t raw, uint8_t status)
{
    e->rx_buffer[2] = (uint8_t)(raw >> 7);
    e->rx_buffer[3] = (uint8_t)(raw << 1);
    e->rx_buffer[4] = status;
    uint32_t remainder = ((uint32_t)e->rx_buffer[2] << 24)
                       | ((uint32_t)e->rx_buffer[3] << 16)
                       | ((uint32_t)status << 8);
    for (int bit = 31; bit >= 8; --bit) {
        if (remainder & (UINT32_C(1) << bit))
            remainder ^= UINT32_C(0x107) << (bit - 8);
    }
    e->rx_buffer[5] = (uint8_t)remainder;
}

static void sample(encoder_t *e, uint16_t raw)
{
    frame(e, raw, 0U);
    encoder_Read_Angle(e);
    assert(e->success && !e->crc_fault);
}

int main(void)
{
    encoder_t e;
    memset(&e, 0xA5, sizeof(e));
    encoder_init(&e);
    assert(cs == GPIO_PIN_SET && !e.dma_busy && !e.success);
    assert(!e.angle_initialized && e.speed_cnt == 0U && e.cnt_fault == 0U);
    near(e.offset_encoder, ENCODER_OFFSET);
    near(e.speed_after_gear, 0.0f);
    near(e.speed_filter_after_gear, 0.0f);
    for (unsigned i = 0; i < SPEED_DIV; ++i) encoder_Update_Speed(&e);
    assert(e.speed_cnt == 0U);

    // A bad first frame must not initialize angle tracking.
    frame(&e, 100U, 0U);
    e.rx_buffer[5] ^= 1U;
    encoder_Read_Angle(&e);
    assert(!e.angle_initialized && !e.success && e.crc_fault);
    sample(&e, 8192U);
    near(e.theta_m, PI / 2.0f);
    near(e.multi_angle_m, PI / 2.0f);
    near(e.mechanical_zero_angle, 0.0f);
    near(e.mechanical_angle, (PI / 2.0f) / GEAR_RATIO);
    float expected_theta_e = PI / 2.0f - ENCODER_OFFSET;
    while (expected_theta_e < 0.0f) expected_theta_e += TWO_PI;
    while (expected_theta_e >= TWO_PI) expected_theta_e -= TWO_PI;
    near(e.theta_e, expected_theta_e);
    sample(&e, 8192U);
    near(e.mechanical_angle, (PI / 2.0f) / GEAR_RATIO);

    // Forward and reverse crossing must remain continuous at the gear output.
    encoder_init(&e);
    sample(&e, 32760U);
    float angle_before_crossing = e.mechanical_angle;
    sample(&e, 8U);
    assert(e.turn_count == 1);
    near(e.mechanical_angle - angle_before_crossing,
         16.0f * TWO_PI / ENCODER_CPR / GEAR_RATIO);
    sample(&e, 32760U);
    assert(e.turn_count == 0);
    near(e.mechanical_angle, 32760.0f * TWO_PI / ENCODER_CPR / GEAR_RATIO);
    encoder_init(&e);
    sample(&e, 8U);
    angle_before_crossing = e.mechanical_angle;
    sample(&e, 32760U);
    assert(e.turn_count == -1);
    near(e.mechanical_angle - angle_before_crossing,
         -16.0f * TWO_PI / ENCODER_CPR / GEAR_RATIO);

    // Exercise every raw code and two complete turns.
    encoder_init(&e);
    for (unsigned i = 0; i <= 65536U; ++i) {
        sample(&e, (uint16_t)(i % ENCODER_CPR));
        assert(e.theta_m >= 0.0f && e.theta_m < TWO_PI);
        assert(e.theta_e >= 0.0f && e.theta_e < TWO_PI);
        near(e.mechanical_angle, (float)i * TWO_PI / ENCODER_CPR / GEAR_RATIO);
    }
    assert(e.turn_count == 2);
    e.offset_encoder = -4.0f * TWO_PI - PI;
    sample(&e, 0U);
    near(e.theta_e, PI);
    e.offset_encoder = 4.0f * TWO_PI + PI;
    sample(&e, 0U);
    near(e.theta_e, PI);

    // CRC/status errors preserve angles and accumulated motion; valid data recovers.
    encoder_t previous = e;
    frame(&e, 1000U, 0U);
    e.rx_buffer[5] ^= 1U;
    encoder_Read_Angle(&e);
    assert(!e.success && e.crc_fault && e.cnt_fault == 1U);
    near(e.theta_m, previous.theta_m);
    near(e.theta_e, previous.theta_e);
    near(e.mechanical_angle, previous.mechanical_angle);
    near(e.angle_delta_sum, previous.angle_delta_sum);
    for (uint8_t status = 1U; status <= 4U; status <<= 1) {
        frame(&e, 1000U, status);
        encoder_Read_Angle(&e);
        assert(!e.success && !e.crc_fault && e.status == status);
        near(e.mechanical_zero_angle, previous.mechanical_zero_angle);
        near(e.theta_m, previous.theta_m);
    }
    sample(&e, 0U);
    assert(e.cnt_fault == 4U); // Counter is cumulative, not consecutive.
    e.cnt_fault = CNT_ENCODER_ERROR_MAX;
    frame(&e, 1000U, 1U);
    encoder_Read_Angle(&e);
    assert(e.cnt_fault == CNT_ENCODER_ERROR_MAX);

    // Same 1 kHz window and LPF as the previous FOC speed estimator, both in RPM.
    encoder_init(&e);
    sample(&e, 0U);
    for (unsigned i = 1; i <= SPEED_DIV; ++i) {
        sample(&e, (uint16_t)(i * 10U));
        encoder_Update_Speed(&e);
        if (i < SPEED_DIV) near(e.speed_after_gear, 0.0f);
    }
    float expected_rpm = 200.0f * TWO_PI / ENCODER_CPR
                       / (FOC_SPEED_TS * GEAR_RATIO) * RADS_TO_RPM;
    near(e.speed_after_gear, expected_rpm);
    near(e.speed_filter_after_gear, SPEED_FILTER_ALPHA * expected_rpm);
    near(e.angle_delta_sum, 0.0f);
    assert(e.speed_cnt == 0U && irq_mask == 0U);
    // With no new valid frames, the ADC window still expires.
    irq_mask = 1U;
    for (unsigned i = 0; i < SPEED_DIV; ++i) encoder_Update_Speed(&e);
    near(e.speed_after_gear, 0.0f);
    near(e.speed_filter_after_gear, (1.0f - SPEED_FILTER_ALPHA)
                                  * SPEED_FILTER_ALPHA * expected_rpm);
    assert(irq_mask == 1U);
    irq_mask = 0U;
    for (unsigned i = 1; i <= SPEED_DIV; ++i) {
        sample(&e, (uint16_t)(200U - i * 10U));
        encoder_Update_Speed(&e);
    }
    near(e.speed_after_gear, -expected_rpm);

    // DMA launch is nonblocking; busy does not restart or change CS.
    transfer_result = HAL_OK;
    assert(encoder_Start_Read_DMA(&e) == HAL_OK);
    assert(e.dma_busy && cs == GPIO_PIN_RESET && transfers == 1U);
    float old_angle = e.theta_m;
    assert(encoder_Start_Read_DMA(&e) == HAL_BUSY);
    assert(transfers == 1U && cs == GPIO_PIN_RESET);
    near(e.theta_m, old_angle);
    encoder_DMA_Error(&e);
    assert(!e.dma_busy && !e.success && cs == GPIO_PIN_SET);
    near(e.theta_m, old_angle);
    for (unsigned result = HAL_ERROR; result <= HAL_TIMEOUT; ++result) {
        transfer_result = (HAL_StatusTypeDef)result;
        assert(encoder_Start_Read_DMA(&e) == result);
        assert(!e.dma_busy && !e.success && cs == GPIO_PIN_SET);
    }
    transfer_result = HAL_OK;
    assert(encoder_Start_Read_DMA(&e) == HAL_OK);
    // Simulate the ordering of the production SPI complete callback.
    HAL_GPIO_WritePin(ENCODER_CS_GPIO_Port, ENCODER_CS_Pin, GPIO_PIN_SET);
    sample(&e, 100U);
    e.dma_busy = false;
    assert(e.success && cs == GPIO_PIN_SET);

    // Polling shares decoding but does not stop/wait for DMA or change its state.
    unsigned dma_calls_before = transfers;
    frame(&e, 8192U, 0U);
    memcpy(polling_frame, e.rx_buffer, sizeof(polling_frame));
    assert(encoder_Read_Polling(&e) == HAL_OK);
    near(e.theta_m, PI / 2.0f);
    assert(cs == GPIO_PIN_SET && polling_calls == 1U && transfers == dma_calls_before);
    e.dma_busy = true;
    cs = GPIO_PIN_RESET;
    assert(encoder_Read_Polling(&e) == HAL_BUSY);
    assert(e.dma_busy && cs == GPIO_PIN_RESET && polling_calls == 1U);
    e.dma_busy = false;
    polling_frame[5] ^= 1U;
    assert(encoder_Read_Polling(&e) == HAL_ERROR);
    near(e.theta_m, PI / 2.0f);
    assert(!e.success && e.crc_fault && cs == GPIO_PIN_SET);
    frame(&e, 0U, 1U);
    memcpy(polling_frame, e.rx_buffer, sizeof(polling_frame));
    assert(encoder_Read_Polling(&e) == HAL_ERROR);
    near(e.theta_m, PI / 2.0f);
    assert(!e.success && !e.crc_fault);
    for (unsigned result = HAL_ERROR; result <= HAL_TIMEOUT; ++result) {
        transfer_result = (HAL_StatusTypeDef)result;
        assert(encoder_Read_Polling(&e) == result);
        near(e.theta_m, PI / 2.0f);
        assert(cs == GPIO_PIN_SET && !e.success && !e.dma_busy);
    }
    sample(&e, 8192U);
    float delta_before_offset = e.angle_delta_sum;
    e.offset_encoder = PI / 2.0f;
    encoder_Read_Angle(&e);
    near(e.theta_e, 0.0f);
    near(e.angle_delta_sum, delta_before_offset);
    puts("PASS: init, all raw angles, multi-turn, zero/offset, CRC/status, RPM/LPF, DMA and polling");
    return 0;
}
