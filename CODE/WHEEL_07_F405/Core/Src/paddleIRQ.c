#include "paddleIRQ.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "task.h"

#define DEBOUNCING_COUNT 10

TaskHandle_t paddleTaskHandlerLocal = NULL;

typedef enum
{
    PADDLE_UP = 0x00,
    PADDLE_DOWN = 0x01
}paddleTypeEnum;

typedef enum
{
    PADDLE_CLICKED = 0x00,
    PADDLE_UNCLICKED = 0x01,
    PADDLE_DEBOUNCING = 0x02,
    PADDLE_ERROR = 0x03,
    PADDLE_UNKNOWN = 0x04,
}paddleStateEnum;

typedef struct
{
    const paddleTypeEnum type;
    GPIO_TypeDef* GPIO_Port;
    uint16_t GPIO_Pin;
    paddleStateEnum state;
    uint16_t debouncingCnt;
}paddleStatusType;

paddleStatusType paddles[2] =
        {
        [PADDLE_UP] = {.type = PADDLE_UP,
         .GPIO_Port = PADDLE_UP_GPIO_Port,
         .GPIO_Pin = PADDLE_UP_Pin,
         .state = PADDLE_UNKNOWN,
         .debouncingCnt = 0},

        [PADDLE_DOWN] = {.type = PADDLE_DOWN,
         .GPIO_Port = PADDLE_DOWN_GPIO_Port,
         .GPIO_Pin = PADDLE_DOWN_Pin,
         .state = PADDLE_UNKNOWN,
         .debouncingCnt = 0},

        };

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
            HAL_TIM_Base_Start_IT(&htim13);
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
        paddles[PADDLE_UP].state = PADDLE_DEBOUNCING;
        for(uint8_t debounce = 0; debounce < DEBOUNCING_COUNT; debounce++)
        {
            osDelay(1);
        }
        if(HAL_GPIO_ReadPin(PADDLE_UP_GPIO_Port, PADDLE_UP_Pin) == GPIO_PIN_SET){
            paddles[PADDLE_UP].state = PADDLE_CLICKED;
        }else if(HAL_GPIO_ReadPin(PADDLE_UP_GPIO_Port, PADDLE_UP_Pin) == GPIO_PIN_RESET)
        {
            paddles[PADDLE_UP].state = PADDLE_UNCLICKED;
        }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim == &htim13)
    {
        __NOP();
    }

    if(htim == &htim14)
    {
        __NOP();
    }
}