/* STM32H7 UART binary logger: SOF A5 5A + 10 float32 + CRC16-CCITT-FALSE.
 * CRC covers SOF and payload; low CRC byte is transmitted first.
 */
#include "uart_logger.h"
#include <math.h>
#include <string.h>

#define FRAME_SIZE  ((uint32_t)sizeof(UART_LogFrame_t))

#define DMA_SIZE    ((FRAME_SIZE + 31U) & ~31U)

/* The DMA buffer must be in RAM accessible to the selected UART DMA engine.
 * On STM32H7, do not place this in DTCM if the DMA controller cannot access it.
 */
static uint8_t tx_buf[DMA_SIZE] __attribute__((aligned(32)));

_Static_assert(sizeof(float) == 4U, "Protocol requires 32-bit float");

_Static_assert(sizeof(UART_LogFrame_t) == (4U + 4U * UART_LOG_N), "Unexpected UART frame layout");

static uint16_t CRC16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;

    for (uint16_t i = 0; i < len; ++i)
    {
        crc ^= (uint16_t)data[i] << 8;

        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

void UART_Log_Init(float data[UART_LOG_N])
{
    if (data == NULL)

        return;

    for (uint32_t i = 0; i < UART_LOG_N; ++i)

        data[i] = NAN;
}

HAL_StatusTypeDef UART_Log_Send(UART_HandleTypeDef *huart, const float data[UART_LOG_N])
{
    if (huart == NULL || data == NULL)

        return HAL_ERROR;

    /* Do not overwrite the DMA buffer while it is being transmitted. */
    if (huart->gState != HAL_UART_STATE_READY)

        return HAL_BUSY;

    tx_buf[0] = 0xA5U;

    tx_buf[1] = 0x5AU;

    memcpy(&tx_buf[2], data, UART_LOG_N * sizeof(float));

    const uint16_t crc = CRC16(tx_buf, (uint16_t)(FRAME_SIZE - 2U));

    tx_buf[FRAME_SIZE - 2U] = (uint8_t)(crc & 0xFFU);

    tx_buf[FRAME_SIZE - 1U] = (uint8_t)(crc >> 8);

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)

    if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U)

        SCB_CleanDCache_by_Addr((uint32_t *)tx_buf, (int32_t)DMA_SIZE);

#endif

    return HAL_UART_Transmit_DMA(huart, tx_buf, (uint16_t)FRAME_SIZE);
}
