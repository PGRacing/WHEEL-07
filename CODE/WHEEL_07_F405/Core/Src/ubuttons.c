#include "ubuttons.h"
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "main.h"
#include "cmsis_os2.h"
#include "timers.h"
#include "canhandler.h"

uint8_t ubuttonsModuleInitalized = 0x00;

typedef enum
{
    UBUTTON_CLICKED = 0x00,
    UBUTTON_UNCLICKED = 0x01,
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
    void (*clickedCallback)(ubuttonType);
    void (*unclickedCallback)(ubuttonType);
    uint8_t debounceActive;
    uint8_t debounceCount;
    GPIO_PinState pinState;
};

/* Here configure buttons */
volatile ubuttonType buttonsRegister[UBUTTONS_COUNT] = {
    [BUTTON_A] = {
            .GPIO_Port = WHEEL_BTN2_GPIO_Port,
            .GPIO_Pin  = WHEEL_BTN2_Pin,
            .state     = UBUTTON_UNKNOWN,
            .clickedCallback  = NULL,
            .unclickedCallback = NULL,
            .debounceActive = 0x00,
            .debounceCount = 0,
            .pinState = GPIO_PIN_SET
    },
   /* [BUTTON_B] = {
            .GPIO_Port = WHEEL_BTN1_GPIO_Port,
            .GPIO_Pin  = WHEEL_BTN1_Pin,
            .state     = UBUTTON_UNKNOWN,
            .callback  = NULL,
            .debounceActive = 0x00,
            .debounceCount = 0,
            .pinState = GPIO_PIN_SET
    },
    [BUTTON_C] = {
            .GPIO_Port = WHEEL_BTN3_GPIO_Port,
            .GPIO_Pin  = WHEEL_BTN3_Pin,
            .state     = UBUTTON_UNKNOWN,
            .callback  = NULL,
            .debounceActive = 0x00,
            .debounceCount = 0,
            .pinState = GPIO_PIN_SET
    },*/
    };

/* *** BUTTON CAN PART *** */
const CAN_TxHeaderTypeDef ubuttonsCANHeader =
        {
                .DLC = 5,
                .ExtId = 0,
                .IDE = CAN_ID_STD,
                .RTR = CAN_RTR_DATA,
                .StdId = 0x447,
                .TransmitGlobalTime = DISABLE,
        };

/*!!!! CANData should be global or declared on root level!!!! */
uint8_t ubuttonsCANData[5] = {0xFF};

/* muxsw CAN package that will be used for queue */
CAN_TxPackageType ubuttonsCANPackage =
        {
                .header = ubuttonsCANHeader,
                .data = ubuttonsCANData
        };

void ubuttonsPrepareCANPackage(volatile ubuttonType* ubuttonPtr)
{
    /*!< Data disclaimer
     * First byte --> trigger info
     * Following bytes -->
     * 0x55 -> No trigger, just notification pass
     * 0x??  -> Bit relative to rotary adcID is set for the triggering switch (switch position changed)
     * */

    ubuttonsCANPackage.data[0] = 0x55;
    for(uint8_t i = 0; i < UBUTTONS_COUNT; i++)
    {
        /* Set byte about which switch triggered the change */
        if(ubuttonPtr != NULL && ubuttonPtr->GPIO_Pin == buttonsRegister[i].GPIO_Pin)
        {
            ubuttonsCANPackage.data[0] = i + 1;
        }
        ubuttonsCANPackage.data[i + 1] = buttonsRegister[i].state;
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


void ubuttonsReadState()
{
    for(int i=0; i < UBUTTONS_COUNT; i++)
    {
        GPIO_PinState gpioState = HAL_GPIO_ReadPin(buttonsRegister[i].GPIO_Port, buttonsRegister[i].GPIO_Pin);
        buttonsRegister[i].pinState = gpioState;

        /* Can be optimized with reading whole port at once */
        /*!< Start debouncing if button pushed registred */
        if(buttonsRegister[i].debounceCount == 0
            && gpioState == GPIO_PIN_RESET
            && buttonsRegister[i].debounceActive == 0x00)
        {
            /* If button is pushed set button debouncing state to active */
            buttonsRegister[i].debounceCount = 0;
            buttonsRegister[i].debounceActive = 0x01;
            buttonsRegister[i].state = UBUTTON_DEBOUNCING;
        }

        /*!< Reset if not in debouncing and not clicked at the moment */
        if(!buttonsRegister[i].debounceActive && gpioState == GPIO_PIN_SET)
        {
            if(buttonsRegister[i].state != UBUTTON_UNCLICKED && buttonsRegister[i].unclickedCallback != NULL)
                    buttonsRegister[i].unclickedCallback(buttonsRegister[i]);

            buttonsRegister[i].state = UBUTTON_UNCLICKED;
            buttonsRegister[i].debounceActive = 0x00;
            buttonsRegister[i].debounceCount = 0;
        }

        /*!< If debouncing treshold was met check second time for next state*/
        if(buttonsRegister[i].debounceActive == 0x01
            && buttonsRegister[i].debounceCount > UBUTTONS_DEBOUNCE_THRESHOLD)
        {
            /*!< Debouncing finished successfully */
            if(gpioState == GPIO_PIN_RESET)
            {
                if(buttonsRegister[i].state != UBUTTON_CLICKED && buttonsRegister[i].clickedCallback != NULL)
                    buttonsRegister[i].clickedCallback(buttonsRegister[i]);

                buttonsRegister[i].state = UBUTTON_CLICKED;
            }
            /*!< Debouncing gone wrong */
            buttonsRegister[i].debounceActive = 0x00;
        }

        /*!< Increment debounce counter */
        if(buttonsRegister[i].debounceActive == 0x01)
            buttonsRegister[i].debounceCount++;
    }
}

void ubuttonClickedCallback(ubuttonType btn)
{
    ubuttonsPrepareCANPackage(&btn);
    ubuttonsPushCANPackageToQueue();
}

void ubuttonUnclickedCallback(ubuttonType btn)
{
    ubuttonsPrepareCANPackage(&btn);
    ubuttonsPushCANPackageToQueue();
}

void ubuttonsTaskStart(void* arguments)
{
    buttonsRegister[0].clickedCallback = ubuttonClickedCallback;
    buttonsRegister[0].unclickedCallback = ubuttonUnclickedCallback;

    ubuttonsModuleInitalized = 0x01;

    for(;;)
    {
        ubuttonsReadState();
        osDelay(1);
    }
}

/*!< ubtnNotifyTask entry point */
void ubuttonsNotifyTaskStart(void *argument)
{
    /* Infinite loop */
    for (;;)
    {
        if(ubuttonsModuleInitalized == 0x01)
        {
            ubuttonsPrepareCANPackage(NULL);
            ubuttonsPushCANPackageToQueue();
        }
        osDelay(pdMS_TO_TICKS(UBUTTONS_NOTIFICATION_PERIOD));
    }
}

