#ifndef TEST_HAL_H
#define TEST_HAL_H
#include <stdint.h>
#include <stddef.h>
#include "../encoder/spi.h"
void HAL_Delay(uint32_t delay);
uint32_t HAL_GetTick(void);
#endif
