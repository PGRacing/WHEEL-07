#include "FreeRTOS.h"
#include "queue.h"
#include "can.h"


typedef struct
{
    CAN_TxHeaderTypeDef header;
    uint8_t* data;
}CAN_TxPackageType;

/* Can tasks */
void can1TaskStart(void *argument);
void can2TaskStart(void *argument);

extern xQueueHandle can1QueueHandle;
extern xQueueHandle can2QueueHandle;