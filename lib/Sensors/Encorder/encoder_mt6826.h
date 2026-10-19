/*
 * encoder_mt6826.h
 *
 *  Created on: Oct 3, 2026
 *      Author: THANH BINH
 */

#ifndef SENSORS_ENCORDER_ENCODER_MT6826_H_
#define SENSORS_ENCORDER_ENCODER_MT6826_H_

#include <stdbool.h>
#include <stdint.h>

#define MT6826_FRAME_LENGTH 6U

typedef struct
{
    uint8_t tx_buffer[MT6826_FRAME_LENGTH];

    uint8_t rx_buffer[MT6826_FRAME_LENGTH];

    float theta_m;                                               // GÓC CƠ ROTOR [0, 2*PI), RAD

    float theta_e;                                               // GÓC ĐIỆN [0, 2*PI), RAD

    float multi_angle_m;                                         // GÓC ROTOR ĐA VÒNG, RAD (CHƯA TRỪ MỐC ZERO)

    float mechanical_angle;                                      // GÓC CƠ KHÍ SAU GEAR SO VỚI MỐC ZERO, RAD

    float mechanical_angle_deg;						    		 // GÓC CƠ KHÍ SAU GEAR SO VỚI MỐC ZERO, DEG

    float mechanical_zero_angle;								 // GÓC 0 CƠ KHÍ (LẤY LÀM MỐC)

    float speed_after_gear;                                      // VẬN TỐC SAU GEAR, RPM

    float speed_filter_after_gear;                               // VẬN TỐC SAU GEAR FILTER LPF, RPM

    float offset_encoder;										 // GIÁ TRỊ OFFSET ENCODER

    uint8_t status;                                              // 3 BIT STATUS CỦA MT6826S

    uint8_t crc;												 // BYTE CRC KIỂM TRA TÍNH HỢP LỆ BẢN TIN

    uint8_t crc_fault;											 // CỜ KIỂM TRA CRC FAULT

    uint16_t cnt_fault;                                          // TỔNG LỖI SPI/CRC/STATUS, CHẶN TẠI CNT_ENCODER_ERROR_MAX

    uint8_t success;                                             // KẾT QUẢ LẦN ĐỌC ĐÃ KẾT THÚC GẦN NHẤT

    int32_t turn_count;											 // BIẾN ĐẾM MULTI-ANGLE

    volatile float angle_delta_sum;                              // TỔNG DELTA GÓC ROTOR CHO VÒNG VẬN TỐC, RAD

    uint32_t speed_cnt;                                          // ĐẾM CHU KỲ ADC ĐỂ CẬP NHẬT VẬN TỐC

    volatile uint8_t dma_busy;                                   // CỜ DMA_BUSY, DÙNG CHUNG GIỮA NGẮT ADC VÀ SPI

    volatile uint8_t angle_initialized;                         // FIRST VALID ANGLE FRAME RECEIVED

}encoder_t;


// HÀM KHỞI TẠO ĐỐI TƯỢNG ENCODER, GỌI TRƯỚC KHI BẮT ĐẦU DMA
void encoder_init(encoder_t *encoder);

// HÀM BẮT ĐẦU ĐỌC ENCODER SPI DMA, TRẢ VỀ HAL_OK / HAL_BUSY / HAL_ERROR
uint8_t encoder_Start_Read_DMA(encoder_t *encoder);

// POLLING DÙNG CHO CALIB
uint8_t encoder_Read_Polling(encoder_t *encoder);

// GIẢI MÃ FRAME DMA/POLLING, CHỈ GỌI KHI ĐÃ NHẬN ĐỦ FRAME
void encoder_Read_Angle(encoder_t *encoder);

// HÀM UPDATE MULTI_ANGLE
void update_multi_angle(encoder_t* encoder, float angle);

// GỌI MỖI CHU KỲ ADC (FOC_FREQ_HZ), TRƯỚC KHI BẮT ĐẦU DMA MỚI
void encoder_Update_Speed(encoder_t *encoder);

// GỌI KHI HAL BÁO LỖI DMA/SPI; KHÔNG GIẢI MÃ FRAME CHƯA HOÀN THÀNH
void encoder_DMA_Error(encoder_t *encoder);

#endif /* SENSORS_ENCORDER_ENCODER_MT6826_H_ */
