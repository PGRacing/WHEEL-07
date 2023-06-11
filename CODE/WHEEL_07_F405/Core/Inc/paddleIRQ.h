#include "stm32f4xx.h"

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

void paddleIRQTaskStart(void *argument);