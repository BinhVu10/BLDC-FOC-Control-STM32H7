/*
 * encoder_mt6826.c
 *
 *  Created on: Oct 3, 2026
 *      Author: THANH BINH
 */

#include "encoder_mt6826.h"
#include "spi.h"
#include "config.h"
#include <string.h>

#define CMD_BURST_READ     0xA0U     // LỆNH ĐỌC GÓC
#define ANGLE_ADDRESS_REG  0x03U     // ĐỊA CHỈ CỦA THANH GHI LƯU GÓC

static uint8_t encoder_crc_calc(const uint8_t *data, uint8_t len);

// HÀM KHỞI TẠO ĐỐI TƯỢNG ENCODER
void encoder_init(encoder_t *encoder)
{
    memset(encoder, 0, sizeof(*encoder));

    encoder->offset_encoder = ENCODER_OFFSET;

    encoder->tx_buffer[0] = CMD_BURST_READ;

    encoder->tx_buffer[1] = ANGLE_ADDRESS_REG;

    // CS Ở MỨC HIGH KHI CHƯA TRUYỀN FRAME.
    HAL_GPIO_WritePin(ENCODER_CS_GPIO_Port, ENCODER_CS_Pin, GPIO_PIN_SET);

}

// HÀM BẮT ĐẦU ĐỌC ENCODER SPI DMA
uint8_t encoder_Start_Read_DMA(encoder_t *encoder)
{
    if (encoder->dma_busy) {

        return HAL_BUSY;
    }

    encoder->dma_busy = true;

    // KÉO CHÂN XUỐNG 0 ĐỂ BẮT ĐẦU FRAME TRUYỀN DMA
    HAL_GPIO_WritePin(ENCODER_CS_GPIO_Port, ENCODER_CS_Pin, GPIO_PIN_RESET);

    // BẮT ĐẦU TRUYỀN/NHẬN SPI DMA
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(&hspi3, encoder->tx_buffer, encoder->rx_buffer, MT6826_FRAME_LENGTH);

    if (status != HAL_OK) {

        encoder_DMA_Error(encoder);
    }

    return (uint8_t)status;
}

// POLLING CHỈ ĐỌC SPI; VIỆC DỪNG DMA VÀ CONTROL THUỘC PHẦN CHUYỂN MODE.
uint8_t encoder_Read_Polling(encoder_t *encoder)
{
    if (encoder->dma_busy) {

        return HAL_BUSY;

    }

    HAL_GPIO_WritePin(ENCODER_CS_GPIO_Port, ENCODER_CS_Pin, GPIO_PIN_RESET);

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(  &hspi3, encoder->tx_buffer, encoder->rx_buffer, MT6826_FRAME_LENGTH, 1U);

    HAL_GPIO_WritePin(ENCODER_CS_GPIO_Port, ENCODER_CS_Pin, GPIO_PIN_SET);

    if (status != HAL_OK) {

        encoder->success = false;

        if (encoder->cnt_fault < CNT_ENCODER_ERROR_MAX) {


        	encoder->cnt_fault++;
        }

        return (uint8_t)status;
    }

    encoder_Read_Angle(encoder);

    return encoder->success ? HAL_OK : HAL_ERROR;

}

// HÀM GIẢI MÃ GÓC TRẢ VỀ TỪ SPI DMA/POLLING
void encoder_Read_Angle(encoder_t *encoder)
{
    // BYTE STATUS VÀ CRC
    encoder->status = encoder->rx_buffer[4] & 0x07U;

    encoder->crc = encoder->rx_buffer[5];

    encoder->crc_fault = (encoder->crc != encoder_crc_calc(&encoder->rx_buffer[2], 3U));

    encoder->success = false;

    if (encoder->crc_fault || (encoder->status != 0U)) {

        // GIỮ NGUYÊN TOÀN BỘ GÓC VÀ MỐC ZERO KHI FRAME KHÔNG HỢP LỆ.
        if (encoder->cnt_fault < CNT_ENCODER_ERROR_MAX) {

            encoder->cnt_fault++;

        }

        return;
    }

    uint16_t raw_angle = ((uint16_t)encoder->rx_buffer[2] << 7U) | ((uint16_t)encoder->rx_buffer[3] >> 1U);

    float new_angle = (float)raw_angle * TWO_PI / (float)ENCODER_CPR;

    // KHỞI TẠO GÓC 0 CƠ KHÍ TỪ MẪU HỢP LỆ ĐẦU TIÊN
    if (!encoder->is_angle_initlize) {

        encoder->mechanical_zero_angle = new_angle;

        encoder->theta_m = new_angle;

        encoder->turn_count = 0;

        encoder->angle_delta_sum = 0.0f;

        encoder->speed_cnt = 0U;

        encoder->is_angle_initlize = true;

    }

    // UPDATE MULTI-ANGLE TRƯỚC KHI GHI ĐÈ GÓC CƠ CŨ
    update_multi_angle(encoder, new_angle);

    encoder->theta_m = new_angle;

    // GÓC ĐIỆN DÙNG OFFSET ALIGN ROTOR, KHÔNG DÙNG MỐC ZERO VỊ TRÍ
    encoder->theta_e = (encoder->theta_m - encoder->offset_encoder) * (float)POLE_PAIRS;

    while (encoder->theta_e < 0.0f) {

        encoder->theta_e += TWO_PI;

    }
    while (encoder->theta_e >= TWO_PI) {

        encoder->theta_e -= TWO_PI;

    }

    encoder->success = true;
}

// HÀM UPDATE MULTI_ANGLE
void update_multi_angle(encoder_t *encoder, float current_angle)
{
    // TÍNH SỰ CHÊNH LỆCH GIỮA CURRENT ANGLE VÀ PREV ANGLE
    float delta_angle = current_angle - encoder->theta_m;

    // GÓC TĂNG VƯỢT BIÊN 2*PI -> 0: TĂNG SỐ VÒNG.
    if (delta_angle < -PI) {

        delta_angle += TWO_PI;

        encoder->turn_count++;

    }
    // GÓC GIẢM VƯỢT BIÊN 0 -> 2*PI: GIẢM SỐ VÒNG.
    else if (delta_angle > PI) {

        delta_angle -= TWO_PI;

        encoder->turn_count--;

    }

    encoder->multi_angle_m = current_angle + TWO_PI * (float)encoder->turn_count;

    encoder->angle_after_gear =  (encoder->multi_angle_m - encoder->mechanical_zero_angle) / (float)GEAR_RATIO;

    encoder->angle_after_gear_deg = RAD_TO_DEG(encoder->angle_after_gear);

    encoder->angle_delta_sum += delta_angle;
}

// CẬP NHẬT VẬN TỐC THEO CÙNG NHỊP SPEED_DIV VÀ LPF
void encoder_Update_Speed(encoder_t *encoder)
{
    if (!encoder->is_angle_initlize) {

        return;

    }

    encoder->speed_cnt++;

    if (encoder->speed_cnt < SPEED_DIV) {

    	return;
    }

    // SPI CÓ THỂ NGẮT NGANG ADC: LẤY VÀ XÓA TỔNG DELTA TRONG CÙNG MỘT ĐOẠN NGẮN.
    uint32_t primask = __get_PRIMASK();

    __disable_irq();										// TẠM NGƯNG NGẮT ĐỂ TRÁNH XUNG ĐỘT

    float delta_angle = encoder->angle_delta_sum;

    encoder->angle_delta_sum = 0.0f;

    __set_PRIMASK(primask);

    encoder->speed_after_gear = delta_angle / (FOC_SPEED_TS * (float)GEAR_RATIO) * RADS_TO_RPM;

    encoder->speed_filter_after_gear = SPEED_FILTER_ALPHA * encoder->speed_after_gear + (1.0f - SPEED_FILTER_ALPHA) * encoder->speed_filter_after_gear;

    encoder->speed_cnt = 0U;
}

// KẾT THÚC GIAO DỊCH LỖI, KHÔNG CẬP NHẬT GÓC TỪ RX BUFFER
void encoder_DMA_Error(encoder_t *encoder)
{
    HAL_GPIO_WritePin(ENCODER_CS_GPIO_Port, ENCODER_CS_Pin, GPIO_PIN_SET);

    encoder->success = false;

    if (encoder->cnt_fault < CNT_ENCODER_ERROR_MAX) {

        encoder->cnt_fault++;

    }

    encoder->dma_busy = false;
}

// HÀM CHECK CRC TỪ BẢN TIN
static uint8_t encoder_crc_calc(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0U;

    for (uint8_t i = 0; i < len; i++) {

        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++) {

            if (crc & 0x80U) {

                crc = (uint8_t)((crc << 1) ^ 0x07U);

            }
            else {

                crc <<= 1;

            }
        }
    }

    return crc;
}
