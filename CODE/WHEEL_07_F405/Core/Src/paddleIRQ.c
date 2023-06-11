#include "paddleIRQ.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "task.h"

TaskHandle_t paddleTaskHandlerLocal = NULL;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == PADDLE_UP_Pin || GPIO_Pin == PADDLE_DOWN_Pin)
    {
        if(paddleTaskHandlerLocal != NULL)
        {
            BaseType_t checkIfYieldRequired;
            checkIfYieldRequired = xTaskResumeFromISR(paddleTaskHandlerLocal);
            /* Resume running higher priority task when IRQ finished */
            portYIELD_FROM_ISR(checkIfYieldRequired);
        }

    }
}

void paddleIRQTaskStart(void *argument)
{
    paddleTaskHandlerLocal = xTaskGetCurrentTaskHandle();
    for(;;)
    {
        //suspend this task
        vTaskSuspend(NULL);
        __NOP();
    }
}