#include "rotary.h"

uint32_t adc_value[4];

uint32_t rotary_map[12] = {};

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    /* ADC callback */
}