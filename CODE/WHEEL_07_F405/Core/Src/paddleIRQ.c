#include "paddleIRQ.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "task.h"
#include "canhandler.h"

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

/*!< *** PADDLE CAN PART *** */
CAN_TxHeaderTypeDef paddlesCANHeader =
        {
                .DLC = 2,
                .ExtId = 0,
                .IDE = CAN_ID_STD,
                .RTR = CAN_RTR_DATA,
                .StdId = 0x100,
                .TransmitGlobalTime = DISABLE,
        };

uint8_t paddlesCANData[2] = {0xFF};

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == paddles[PADDLE_UP].GPIO_Pin)
    {
        HAL_TIM_Base_Start_IT(&htim13);
        paddles[PADDLE_UP].state = PADDLE_DEBOUNCING;
    }

    if(GPIO_Pin == paddles[PADDLE_DOWN].GPIO_Pin)
    {
        HAL_TIM_Base_Start_IT(&htim14);
        paddles[PADDLE_DOWN].state = PADDLE_DEBOUNCING;
    }
}

/*!< Translate current GPIO state to our paddle state */
void HALtoPaddleState(paddleStatusType* paddle)
{
    switch (HAL_GPIO_ReadPin(paddle->GPIO_Port, paddle->GPIO_Pin)){
        case GPIO_PIN_RESET:
            paddle->state = PADDLE_CLICKED;
            break;
        case GPIO_PIN_SET:
            paddle->state = PADDLE_UNCLICKED;
            break;
    };
}

/*!< Send data to CAN Asap*/
void paddlesCANInstant()
{
    /* As simple as possible */
    paddlesCANData[0] = paddles[0].state;
    paddlesCANData[1] = paddles[1].state;
    HAL_CAN_AddTxMessage(&hcan1, &paddlesCANHeader, paddlesCANData, can1TxMailbox);
}

/*!< Alternative way to send data to can through queue */
void paddlesCANQueue()
{
    CAN_TxPackageType paddlesCANPackage = {.header = paddlesCANHeader, .data = paddlesCANData};
    if(can1QueueHandle != NULL && xQueueSend(can1QueueHandle, &paddlesCANData, portMAX_DELAY) != pdPASS)
    {
        /* Problem with pushing data to queue */
        __NOP();
    }else if(can1QueueHandle == NULL)
    {
        /* Queue handle does not exist yet */
        __NOP();
    };
}

/*!< paddleIRQTask entry point */
void paddleIRQTaskStart(void *argument)
{
    paddleTaskHandlerLocal = xTaskGetCurrentTaskHandle();

    /* Set Initial paddle state */
    for(uint8_t i = 0; i < 2; i++)
    {
        HALtoPaddleState(&(paddles[i]));
    }

    for(;;)
    {
        //suspend this task
        vTaskSuspend(NULL);
        for(uint8_t i = 0; i < 2; i++)
        {
            if(paddles[i].state == PADDLE_DEBOUNCING)
            {
                HALtoPaddleState(&(paddles[i]));
                /* Send CAN data about paddle asap */
                paddlesCANInstant();
            }
        }
    }
}

void debouncingTIMCallback(TIM_HandleTypeDef *htim)
{
    if(htim == &htim13)
    {
        HAL_TIM_Base_Stop_IT(&htim13);
    }

    if(htim == &htim14)
    {
        HAL_TIM_Base_Stop_IT(&htim14);
    }

    if(paddleTaskHandlerLocal != NULL)
    {
        BaseType_t checkIfYieldRequired;
        checkIfYieldRequired = xTaskResumeFromISR(paddleTaskHandlerLocal);
        /* Resume running higher priority task when IRQ finished */
        portYIELD_FROM_ISR(checkIfYieldRequired);
    }
}