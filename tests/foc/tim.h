#ifndef TEST_TIM_H
#define TEST_TIM_H
#include <stdint.h>
typedef struct { uint32_t arr; } TIM_HandleTypeDef;
extern TIM_HandleTypeDef htim1;
#define __HAL_TIM_GET_AUTORELOAD(timer) ((timer)->arr)
#endif
