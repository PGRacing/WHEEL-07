#include "stm32f4xx.h"
#include "cmsis_os2.h"


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

void paddleIRQTaskStart(void *argument);

/* Debouncing timers handler */
void debouncingTIMCallback(TIM_HandleTypeDef *htim);