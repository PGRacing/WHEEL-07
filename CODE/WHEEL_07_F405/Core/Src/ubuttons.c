#include "ubuttons.h"
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "main.h"
#include "cmsis_os2.h"
#include "timers.h"
#include "canhandler.h"

uint8_t ubuttonsModuleInitalized = 0x00;

const osTimerAttr_t ubuttonsHoldTimerAttr = {.name = "ubuttonsHold" };
osTimerId_t ubuttonsHoldTimer;

uint16_t timerCounter = 0;

typedef enum
{
    UBUTTON_UNCLICKED = 0x00,
    UBUTTON_CLICKED = 0x01,
    UBUTTON_DEBOUNCING = 0x02,
    UBUTTON_ERROR = 0x03,
    UBUTTON_UNKNOWN = 0x04,
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

typedef struct buttonType_ ubuttonType;

struct buttonType_
{
    GPIO_TypeDef* GPIO_Port;
    uint16_t GPIO_Pin;
    buttonStateEnum state;
    void (*clickedCallback)(volatile ubuttonType*);
    void (*unclickedCallback)(volatile ubuttonType*);
    uint8_t debounceActive;
    uint8_t debounceCount;
    GPIO_PinState pinState;
    uint8_t pressTime;
};

/* Here configure buttons */
volatile ubuttonType ubuttonsRegister[UBUTTONS_COUNT] = {
    [BUTTON_A] = {
            .GPIO_Port = WHEEL_BTN2_GPIO_Port,
            .GPIO_Pin  = WHEEL_BTN2_Pin,
            .state     = UBUTTON_UNKNOWN,
            .clickedCallback  = NULL,
            .unclickedCallback = NULL,
            .debounceActive = 0x00,
            .debounceCount = 0,
            .pinState = GPIO_PIN_SET,
            .pressTime = 0
    },
   [BUTTON_B] = {
           .GPIO_Port = WHEEL_BTN6_GPIO_Port,
           .GPIO_Pin  = WHEEL_BTN6_Pin,
           .state     = UBUTTON_UNKNOWN,
           .clickedCallback  = NULL,
           .unclickedCallback = NULL,
           .debounceActive = 0x00,
           .debounceCount = 0,
           .pinState = GPIO_PIN_SET,
           .pressTime = 0
    },
    [BUTTON_C] = {
            .GPIO_Port = WHEEL_BTN1_GPIO_Port,
            .GPIO_Pin  = WHEEL_BTN1_Pin,
            .state     = UBUTTON_UNKNOWN,
            .clickedCallback  = NULL,
            .unclickedCallback = NULL,
            .debounceActive = 0x00,
            .debounceCount = 0,
            .pinState = GPIO_PIN_SET,
            .pressTime = 0
    },
    };

/* *** BUTTON CAN PART *** */
const CAN_TxHeaderTypeDef ubuttonsCANHeader =
        {
                .DLC = 5,
                .ExtId = 0,
                .IDE = CAN_ID_STD,
                .RTR = CAN_RTR_DATA,
                .StdId = 0x201,
                .TransmitGlobalTime = DISABLE,
        };

/*!!!! CANData should be global or declared on root level!!!! */
uint8_t ubuttonsCANData[8] = {0xFF};

/* button CAN package that will be used for queue */
CAN_TxPackageType ubuttonsCANPackage =
        {
                .header = ubuttonsCANHeader,
                .data = ubuttonsCANData
        };

const CAN_TxHeaderTypeDef ubuttonsHoldCANHeader =
        {
                .DLC = 4,
                .ExtId = 0,
                .IDE = CAN_ID_STD,
                .RTR = CAN_RTR_DATA,
                .StdId = 0x449,
                .TransmitGlobalTime = DISABLE,
        };

uint8_t ubuttonsHoldCANData[8] = {0xFF};

/* button hold CAN package that will be used for queue */
CAN_TxPackageType ubuttonsHoldCANPackage =
        {
                .header = ubuttonsHoldCANHeader,
                .data = ubuttonsHoldCANData
        };


void ubuttonsPrepareCANPackage(volatile ubuttonType* ubuttonPtr)
{
    ubuttonsCANPackage.header = ubuttonsCANHeader;
    /*!< Data disclaimer
     * First byte --> trigger info
     * Following bytes -->
     * 0x55 -> No trigger, just notification pass
     * 0x??  -> Bit relative to button is set for the triggering switch (switch position changed)
     * */

    ubuttonsCANPackage.data[0] = 0x55;
    for(uint8_t i = 0; i < UBUTTONS_COUNT; i++)
    {
        /* Set byte about which switch triggered the change */
        if(ubuttonPtr != NULL && ubuttonPtr->GPIO_Pin == ubuttonsRegister[i].GPIO_Pin)
        {
            ubuttonsCANPackage.data[0] = i + 1;
        }
        ubuttonsCANPackage.data[i + 1] = ubuttonsRegister[i].state;
    }
}

void ubuttonsPrepareHoldCANPackage()
{
    /*!<
     * Hold time for each button as multiple of UBUTTONS_PRESS_NOTIFICATION_PERIOD
     * */
    for(uint8_t i = 0; i < UBUTTONS_COUNT; i++)
    {
        ubuttonsHoldCANPackage.data[i] = ubuttonsRegister[i].pressTime;
    }
}

/*!< Add data to CAN2 queue */
HAL_StatusTypeDef ubuttonsPushCANPackageToQueue()
{
    if(can2QueueHandle != NULL && xQueueSend(can2QueueHandle, &ubuttonsCANPackage, portMAX_DELAY) != pdPASS)
    {
        /* Problem with pushing data to queue */
        __NOP();
    }else if(can2QueueHandle == NULL)
    {
        /* Queue handle does not exist yet */
        __NOP();
    };
}

/*!< Add data to CAN2 queue */
HAL_StatusTypeDef ubuttonsPushHoldCANPackageToQueue()
{
    if(can2QueueHandle != NULL && xQueueSend(can2QueueHandle, &ubuttonsHoldCANPackage, portMAX_DELAY) != pdPASS)
    {
        /* Problem with pushing data to queue */
        __NOP();
    }else if(can2QueueHandle == NULL)
    {
        /* Queue handle does not exist yet */
        __NOP();
    };
}


void ubuttonsReadState()
{
    for(int i=0; i < UBUTTONS_COUNT; i++)
    {
        GPIO_PinState gpioState = HAL_GPIO_ReadPin(ubuttonsRegister[i].GPIO_Port, ubuttonsRegister[i].GPIO_Pin);
        ubuttonsRegister[i].pinState = gpioState;

        /* Can be optimized with reading whole port at once */
        /*!< Start debouncing if button pushed registred */
        if(ubuttonsRegister[i].debounceCount == 0
           && gpioState == GPIO_PIN_RESET
           && ubuttonsRegister[i].debounceActive == 0x00)
        {
            /* If button is pushed set button debouncing state to active */
            ubuttonsRegister[i].debounceCount = 0;
            ubuttonsRegister[i].debounceActive = 0x01;
            ubuttonsRegister[i].state = UBUTTON_DEBOUNCING;
        }

        /*!< Reset if not in debouncing and not clicked at the moment */
        if(ubuttonsRegister[i].debounceActive == 0x00 && gpioState == GPIO_PIN_SET)
        {
            if(ubuttonsRegister[i].state != UBUTTON_UNCLICKED && ubuttonsRegister[i].unclickedCallback != NULL)
            {
                ubuttonsRegister[i].state = UBUTTON_UNCLICKED;
                ubuttonsRegister[i].unclickedCallback(&(ubuttonsRegister[i]));
            }

            ubuttonsRegister[i].state = UBUTTON_UNCLICKED;
            ubuttonsRegister[i].debounceCount = 0;
        }

        /*!< If debouncing treshold was met check second time for next state*/
        if(ubuttonsRegister[i].debounceActive == 0x01
           && ubuttonsRegister[i].debounceCount > UBUTTONS_DEBOUNCE_THRESHOLD)
        {
            /*!< Debouncing finished successfully */
            if(gpioState == GPIO_PIN_RESET)
            {
                if(ubuttonsRegister[i].state != UBUTTON_CLICKED && ubuttonsRegister[i].clickedCallback != NULL)
                {
                    ubuttonsRegister[i].state = UBUTTON_CLICKED;
                    ubuttonsRegister[i].clickedCallback(&(ubuttonsRegister[i]));
                }
                ubuttonsRegister[i].state = UBUTTON_CLICKED;
            }
            /*!< Debouncing gone wrong */
            ubuttonsRegister[i].debounceActive = 0x00;
        }

        /*!< Increment debounce counter */
        if(ubuttonsRegister[i].debounceActive == 0x01)
            ubuttonsRegister[i].debounceCount++;
    }
}

void ubuttonClickedCallback(volatile ubuttonType* btn)
{
    btn->pressTime = 0;
    ubuttonsPrepareCANPackage(btn);
    ubuttonsPushCANPackageToQueue();
}

void ubuttonUnclickedCallback(volatile ubuttonType* btn)
{
    btn->pressTime = 0;
    ubuttonsPrepareCANPackage(btn);
    ubuttonsPushCANPackageToQueue();
}

void ubuttonsHoldTimerCallback(void* arguments)
{
    /*
    uint8_t anyActive = 0x00;
    for(int i=0; i < UBUTTONS_COUNT; i++)
    {
        if(ubuttonsRegister[i].state == UBUTTON_CLICKED)
        {
            ubuttonsRegister[i].pressTime++;
            anyActive++;
        }
    }

    if(anyActive != 0x00)
    {
        ubuttonsPrepareHoldCANPackage();
        ubuttonsPushHoldCANPackageToQueue();
    }
    */
}

_Noreturn void ubuttonsTaskStart(void* arguments)
{
    /* Remember that something is fucked up with timer priority - CMSIS v2 - > remember to change priority to higher */
    ubuttonsHoldTimer = osTimerNew(ubuttonsHoldTimerCallback, osTimerPeriodic, NULL, &ubuttonsHoldTimerAttr);

    if(ubuttonsHoldTimer != NULL)
    {
        if(osTimerStart(ubuttonsHoldTimer, pdMS_TO_TICKS(UBUTTONS_PRESS_NOTIFICATION_PERIOD)) != osOK)
            /*!< Problem occured when starting the timer */
            __NOP();

    }

    ubuttonsRegister[0].clickedCallback = ubuttonClickedCallback;
    ubuttonsRegister[0].unclickedCallback = ubuttonUnclickedCallback;

    ubuttonsRegister[1].clickedCallback = ubuttonClickedCallback;
    ubuttonsRegister[1].unclickedCallback = ubuttonUnclickedCallback;

    ubuttonsRegister[2].clickedCallback = ubuttonClickedCallback;
    ubuttonsRegister[2].unclickedCallback = ubuttonUnclickedCallback;

    ubuttonsModuleInitalized = 0x01;

    /* Infinite loop */
    for(;;)
    {
        ubuttonsReadState();
        osDelay(1);
    }
}

/*!< ubtnNotifyTask entry point */
_Noreturn void ubuttonsNotifyTaskStart(void *argument)
{
    /* Infinite loop */
    for (;;)
    {
#ifdef UBUTTONS_NOTIFICATION_ENABLED
        if(ubuttonsModuleInitalized == 0x01)
        {
            ubuttonsPrepareCANPackage(NULL);
            ubuttonsPushCANPackageToQueue();
        }
#endif
        osDelay(pdMS_TO_TICKS(UBUTTONS_NOTIFICATION_PERIOD));
    }
}

