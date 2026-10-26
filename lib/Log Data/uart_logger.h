#ifndef UART_LOGGER_H
#define UART_LOGGER_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

/* UART wire order: 15 IEEE-754 float32 fields, little-endian.
 * Keep strictly synchronized with CHANNEL_NAMES in uart_logger_15float.py.
 * Any future field is appended to this enum; ALL fields remain float32.
 */
typedef enum
{
    LOG_IQ = 0,          /*  0 feedback Iq [A] */
    LOG_IQ_REF,          /*  1 reference Iq [A] */
    LOG_ID,              /*  2 feedback Id [A] */
    LOG_ID_REF,          /*  3 reference Id [A] */
    LOG_VD,              /*  4 commanded Vd [V] */
    LOG_VQ,              /*  5 commanded Vq [V] */
    LOG_V_BUS,           /*  6 DC bus [V] */
    LOG_SPEED,           /*  7 speed feedback */
    LOG_SPEED_REF,       /*  8 speed reference */
    LOG_POSITION,        /*  9 position feedback */
    LOG_POSITION_REF,    /* 10 position reference */
    LOG_TEMPERATURE,     /* 11 temperature / extra debug float */
    LOG_SYS_STAGE,       /* 12 system state encoded as float */
    LOG_BOOT_STAGE,      /* 13 boot stage encoded as float */
    LOG_CONTROL_MODE,    /* 14 control mode encoded as float */
    UART_LOG_N           /* Number of float32 slots: 15 */
} UART_LogChannel_t;

#define LOG_TEMPORARY LOG_TEMPERATURE

/* SOF 2 bytes + 15*4-byte float32 + CRC16 2 bytes = 64 bytes. */
typedef struct __attribute__((packed))
{
    uint8_t sof[2];
    float payload[UART_LOG_N];
    uint16_t crc;
} UART_LogFrame_t;

/* Set all slots to NaN before assigning this frame's selected channels. */
void UART_Log_Init(float data[UART_LOG_N]);

/* Copies payload into an aligned internal UART DMA buffer, then transmits.
 * HAL_BUSY means previous DMA transfer is not finished. No queueing.
 */
HAL_StatusTypeDef UART_Log_Send(UART_HandleTypeDef *huart,
                                const float data[UART_LOG_N]);

#endif /* UART_LOGGER_H */
