#ifndef TEST_SPI_H
#define TEST_SPI_H

#include <stdint.h>

typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
typedef struct { int unused; } SPI_HandleTypeDef;
extern SPI_HandleTypeDef hspi3;

#define ENCODER_CS_GPIO_Port ((void *)1)
#define ENCODER_CS_Pin 0x0800U

void HAL_GPIO_WritePin(void *port, uint16_t pin, GPIO_PinState state);
HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(
    SPI_HandleTypeDef *spi, uint8_t *tx, uint8_t *rx, uint16_t length);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(
    SPI_HandleTypeDef *spi, uint8_t *tx, uint8_t *rx, uint16_t length, uint32_t timeout);
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t value);

#endif
