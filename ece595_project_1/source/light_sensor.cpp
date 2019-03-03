#include "light_sensor.h"
#include "fsl_adc16.h"

uint16_t LightSensor::Get_Value(void)
{
    return(ADC16_GetChannelConversionValue(ADC0, 0));
}
