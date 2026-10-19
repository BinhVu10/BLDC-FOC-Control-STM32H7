#ifndef UART_LOGGER_H
#define UART_LOGGER_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

/* Fixed channel order: must match CHANNEL_NAMES in the Python app.
 * Previous seven slots are preserved; the new slots are appended. No temperature channel yet.
 */
typedef enum
{
    LOG_IQ = 0,          /* Channel 1: measured Iq (A) */

    LOG_IQ_REF,          /* Channel 2: active Iq reference (A) */

    LOG_VQ,              /* Channel 3: commanded Vq (V) */

    LOG_SPEED,           /* Channel 4: measured speed */

    LOG_POSITION,        /* Channel 5: measured position */

    LOG_SPEED_REF,       /* Channel 6: active speed reference */

    LOG_POSITION_REF,    /* Channel 7: active position reference */

    LOG_VD,              /* Channel 8: commanded Vd (V) */

    LOG_ID,              /* Channel 9: measured Id (A) */

    LOG_ID_REF,          /* Channel 10: active Id reference (A) */

    UART_LOG_N           /* Always the number of channels = 10 */

} UART_LogChannel_t;

/* 2-byte SOF + UART_LOG_N IEEE-754 float32 + 2-byte CRC16 (little-endian).
 * Frame length is 44 bytes for ten channels.
 */
typedef struct __attribute__((packed))
{
    uint8_t sof[2];

    float payload[UART_LOG_N];

    uint16_t crc;

} UART_LogFrame_t;

/* Call every sample before assigning selected channels: unused fields -> NaN. */
void UART_Log_Init(float data[UART_LOG_N]);

/* Non-blocking UART DMA TX. The caller's data[] is copied before return.
 * Returns HAL_BUSY if the previous DMA transfer has not finished.
 */
HAL_StatusTypeDef UART_Log_Send(UART_HandleTypeDef *huart, const float data[UART_LOG_N]);

#endif /* UART_LOGGER_H */
