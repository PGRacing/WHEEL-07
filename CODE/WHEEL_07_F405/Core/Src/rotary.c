#include <stdlib.h>
#include <memory.h>
#include "FreeRTOS.h"
#include "rotary.h"
#include "adc.h"
#include "cmsis_os2.h"


typedef struct {
    const uint16_t adcExpectedReading;
    const uint16_t adcResolution;
} muxswConfigType;

typedef struct {
    uint8_t adcID;
    uint16_t* swRawData;
    muxswStatesEnum state;
    const muxswConfigType *muxswConfigRegister;
    uint16_t validityCount[MUXSW_STATE_COUNT];
    uint8_t validityThresh;
} muxswType;


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
                    .validityThresh = 20
                }
        };


void muxswStateChanged(volatile muxswType* muxswPtr)
{

}

void muxswInRange(volatile muxswType* muxswPtr)
{
    uint8_t plausibiltiyCounter = 0;
    for(uint8_t i = 0; i < MUXSW_STATE_COUNT - 1; i++)
    {
        const muxswConfigType* currentType = &(muxswPtr->muxswConfigRegister[i]);
        if(abs(currentType->adcExpectedReading - *muxswPtr->swRawData) < currentType->adcResolution)
        {
            muxswPtr->validityCount[i]++;
            plausibiltiyCounter++;
        }

        if(muxswPtr->validityCount[i] > muxswPtr->validityThresh)
        {

            /* State changed! */
            if(muxswPtr->state != i)
            {
                /* Changing state to the actual */
                muxswPtr->state = i;


            }

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


void muxswTaskStart(void *argument)
{
    /* USER CODE BEGIN adcTaskStart */
    /* Infinite loop */
    for(;;)
    {
        if(xSemaphoreTake(adcConvReadySemaphore, 0) == pdTRUE)
        {
            for(uint8_t i = 0; i < MUXSW_COUNT; i++)
            {
                muxswInRange(&muxswRegister[0]);
            }
        }
        /* Do something with data */
    }
    /* USER CODE END adcTaskStart */
}