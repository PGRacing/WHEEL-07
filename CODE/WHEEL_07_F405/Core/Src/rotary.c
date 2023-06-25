#include <stdlib.h>
#include <memory.h>
#include "FreeRTOS.h"
#include "rotary.h"
#include "adc.h"
#include "cmsis_os2.h"
#include "canhandler.h"

uint8_t muxswModuleInitalized = 0x00;

typedef struct {
    const uint16_t adcExpectedReading;
    const uint16_t adcResolution;
} muxswConfigType;

typedef struct muxswType_ muxswType;

struct muxswType_{
    uint8_t adcID;
    uint16_t* swRawData;
    muxswStatesEnum state;
    const muxswConfigType *muxswConfigRegister;
    uint16_t validityCount[MUXSW_STATE_COUNT];
    uint8_t validityThresh;
    /* Here using struct -> muxswType because it will be visible after typedef */
    void (*callback)(volatile muxswType* muxswPtr);
};

/*!< muxSW ADC translation register */
const muxswConfigType muxswConfigRegister1[MUXSW_STATE_COUNT] = {
        [SW_POSITION_1] = {
                .adcExpectedReading = 315U,
                .adcResolution = 110U,
        },
        [SW_POSITION_2] = {
                .adcExpectedReading = 630U,
                .adcResolution = 110U,
        },
        [SW_POSITION_3] = {
                .adcExpectedReading = 945U,
                .adcResolution = 110U,
        },
        [SW_POSITION_4] = {
                .adcExpectedReading = 1260U,
                .adcResolution = 110U,
        },
        [SW_POSITION_5] = {
                .adcExpectedReading = 1575U,
                .adcResolution = 110U,
        },
        [SW_POSITION_6] = {
                .adcExpectedReading = 1890U,
                .adcResolution = 220U,
        },
        [SW_POSITION_7] = {
                .adcExpectedReading = 2205U,
                .adcResolution = 110U,
        },
        [SW_POSITION_8] = {
                .adcExpectedReading = 2520U,
                .adcResolution = 110U,
        },
        [SW_POSITION_9] = {
                .adcExpectedReading = 2835U,
                .adcResolution = 110U,
        },
        [SW_POSITION_10] = {
                .adcExpectedReading = 3150U,
                .adcResolution = 110U,
        },
        [SW_POSITION_11] = {
                .adcExpectedReading = 3465U,
                .adcResolution = 110U,
        },
        [SW_POSITION_12] = {
                .adcExpectedReading = 3780U,
                .adcResolution = 110U,
        },
        [SW_UNKNOWN] = {
                .adcExpectedReading = 0xFFFFU,
                .adcResolution = 0U,
        },
        [SW_INVALID] = {
                .adcExpectedReading = 0U,
                .adcResolution = 0U,
        }
};

volatile muxswType muxswRegister[MUXSW_COUNT] =
        {
                {
                        .adcID = 0U,
                        .swRawData = (uint16_t *)&adcRawData[0],
                        .state = SW_UNKNOWN,
                        .muxswConfigRegister = muxswConfigRegister1,
                        .validityCount = {0},
                        .validityThresh = 20,
                        .callback = NULL,
                }
        };

/* *** MUXSW CAN PART *** */
const CAN_TxHeaderTypeDef muxswCANHeader =
        {
                .DLC = 5,
                .ExtId = 0,
                .IDE = CAN_ID_STD,
                .RTR = CAN_RTR_DATA,
                .StdId = 0x446,
                .TransmitGlobalTime = DISABLE,
        };

/*!!!!CANData should be global or declared on root level!!!! */
uint8_t muxswCANData[5] = {0xFF};

/* muxsw CAN package that will be used for queue */
CAN_TxPackageType muxswCANPackage =
        {
                .header = muxswCANHeader,
                .data = muxswCANData
        };


void muxswPrepareCANPackage(volatile muxswType* muxswPtr)
{
    /*!< Data disclaimer
     * First byte --> trigger info
     * Following bytes --> state of [adcID] mux switch
     * 0x55 -> No trigger, just notification pass
     * 0x??  -> Bit relative to rotary adcID is set for the triggering switch (switch position changed)
     * */

    muxswCANPackage.data[0] = 0x55;
    for(uint8_t i = 0; i < MUXSW_COUNT; i++)
    {
        /* Set byte about which switch triggered the change */
        if(muxswPtr != NULL && muxswPtr->adcID == i)
        {
            muxswCANPackage.data[0] = i + 1;
        }
        muxswCANPackage.data[i + 1] = muxswRegister[i].state;
    }
}

/*!< Add data to CAN2 queue */
HAL_StatusTypeDef muxswPushCANPackageToQueue()
{
    if(can2QueueHandle != NULL && xQueueSend(can2QueueHandle, &muxswCANPackage, portMAX_DELAY) != pdPASS)
    {
        /* Problem with pushing data to queue */
        __NOP();
    }else if(can2QueueHandle == NULL)
    {
        /* Queue handle does not exist yet */
        __NOP();
    };
}

void muxswStateChanged(volatile muxswType* muxswPtr)
{
    muxswPrepareCANPackage(muxswPtr);
    muxswPushCANPackageToQueue();
}


/*!< Operate whole checking process of single muxSwitch */
void muxswInRange(volatile muxswType* muxswPtr)
{
    uint8_t plausibiltiyCounter = 0;

    /* Checking for all states */
    for(uint8_t i = 0; i < MUXSW_STATE_COUNT - 1; i++)
    {
        /* Acquire information about current iterator selection for cleaner code */
        const muxswConfigType* currentType = &(muxswPtr->muxswConfigRegister[i]);

        /* Check if ADC reading for current channels ticks into expected reading w/ hysteresis */
       if(abs(currentType->adcExpectedReading - *muxswPtr->swRawData) < currentType->adcResolution)
        {
            muxswPtr->validityCount[i]++;
            plausibiltiyCounter++;
        }

        /* If ADC is debounced correctly for current state */
      if(muxswPtr->validityCount[i] > muxswPtr->validityThresh)
        {

            /* State changed! */
            if(muxswPtr->state != i)
            {
                /* Changing state to the actual */
                muxswPtr->state = i;

                if(muxswPtr->callback != NULL)
                {
                    muxswPtr->callback(muxswPtr);
                }

            }

            /* Zeroing validity counter */
            muxswPtr->validityCount[i] = 0;
            for(int j=0; j < MUXSW_STATE_COUNT; j++)
            {
                muxswPtr->validityCount[j] = 0;
            }
        }
    }

    if(plausibiltiyCounter < MUXSW_COUNT - 1)
    {
        /* If change read not valid increment counter */
        muxswPtr->validityCount[SW_INVALID]++;

        if(muxswPtr->validityCount[SW_INVALID] > 100)
            muxswPtr->validityCount[SW_INVALID] = 0;
    }
}

/*!< muxswTask entry point */
void muxswTaskStart(void *argument)
{
    /* Add same callback to all rotaries */
    for(uint8_t iter = 0; iter < MUXSW_COUNT; iter++)
    {
        muxswRegister[iter].callback = muxswStateChanged;
    }
    /* Infinite loop */
    for(;;)
    {
        /* Semaphore goes true after full ADC conversion */
        if(adcConvReadySemaphore != NULL && xSemaphoreTake(adcConvReadySemaphore, 0) == pdTRUE)
        {
            muxswModuleInitalized = 0x01;
            for(uint8_t i = 0; i < MUXSW_COUNT; i++)
            {
                muxswInRange(&muxswRegister[0]);
            }
        }
    }
}

/*!< muxswNotifyTask entry point */
void muxswNotifyTaskStart(void *argument)
{
    /* Infinite loop */
    for (;;)
    {
#ifdef MUXSW_NOTIFICATION_ENABLED
        if(muxswModuleInitalized == 0x01)
        {
            muxswPrepareCANPackage(NULL);
            muxswPushCANPackageToQueue();
        }
#endif
       osDelay(pdMS_TO_TICKS(MUXSW_NOTIFICATION_PERIOD));
    }
}