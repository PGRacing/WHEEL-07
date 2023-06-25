#include "ubuttons.h"
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "main.h"
#include "cmsis_os2.h"
#include "timers.h"

#define BUTTONS_COUNT 3

typedef enum
{
    BUTTON_CLICKED = 0x00,
    BUTTON_UNCLICKED = 0x01,
    BUTTON_DEBOUNCING = 0x02,
    BUTTON_ERROR = 0x03,
    BUTTON_UNKNOWN = 0x04,
}buttonStateEnum;

/* Descriptors for easier button recognition */
typedef enum
{
    BUTTON_A = 0x00,
    BUTTON_B = 0x01,
    BUTTON_C = 0x02,
    BUTTON_D = 0x03,
    BUTTON_E = 0x04,
    BUTTON_F = 0x05
}buttonDescriptorEnum;

typedef struct
{
    GPIO_TypeDef* GPIO_Port;
    uint16_t GPIO_Pin;
    buttonStateEnum state;
    void (*callback)(void);
}buttonType;

/* Here configure buttons */
buttonType buttonsArray[BUTTONS_COUNT] = {
    [BUTTON_A] = {
            .GPIO_Port = WHEEL_BTN1_GPIO_Port,
            .GPIO_Pin  = WHEEL_BTN1_Pin,
            .state     = BUTTON_UNKNOWN,
            .callback  = NULL
    },
    [BUTTON_B] = {
            .GPIO_Port = WHEEL_BTN2_GPIO_Port,
            .GPIO_Pin  = WHEEL_BTN2_Pin,
            .state     = BUTTON_UNKNOWN,
            .callback  = NULL
    },
    [BUTTON_C] = {
            .GPIO_Port = WHEEL_BTN3_GPIO_Port,
            .GPIO_Pin  = WHEEL_BTN3_Pin,
            .state     = BUTTON_UNKNOWN,
            .callback  = NULL
    },
    };

void vTimerCallback(TimerHandle_t xTimer)
{
    HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
}

void ubuttonsTaskStart(void* arguments)
{
    TimerHandle_t  timHandle = NULL;
    timHandle = xTimerCreate("btn_pooling_tim", 100, pdTRUE, (void *) 1, vTimerCallback);
    if(xTimerStart(timHandle, 0) != pdPASS)
    {
        __NOP();
    }
    for(;;)
    {
        osDelay(10);
    }
}



