/*
 * io_abstraction.cpp
 *
 *  Created on: Feb 9, 2019
 *      Author: djaenicke
 */

#include "io_abstraction.h"
#include "assert.h"

const Pin_Cfg_T Pin_Cfgs[NUM_IO] =
{
    {PORTC, GPIOC,  0, kPORT_MuxAsGpio, kGPIO_DigitalOutput, LOW},  /* MOTOR_STEP_CTRL */
    {PORTC, GPIOC,  2, kPORT_MuxAsGpio, kGPIO_DigitalOutput, HIGH}, /* MOTOR_DIR_CTRL  - HIGH=CW, LOW=CCW */
    {PORTC, GPIOC,  3, kPORT_MuxAsGpio, kGPIO_DigitalOutput, HIGH}, /* MOTOR_SLEEP     - Active Low       */
    {PORTC, GPIOC,  5, kPORT_MuxAsGpio, kGPIO_DigitalOutput, LOW},  /* MOTOR_DRV_EN    */
    {PORTC, GPIOC,  7, kPORT_MuxAsGpio, kGPIO_DigitalOutput, HIGH}, /* MOTOR_DRV_RESET - Active Low       */
    {PORTB, GPIOB, 21, kPORT_MuxAsGpio, kGPIO_DigitalOutput, HIGH}  /* BLUE_LED        - Active Low       */
};

void Set_GPIO(IO_Map_T gpio, GPIO_State_T state)
{
    if (gpio < NUM_IO)
    {
        if ((kPORT_MuxAsGpio == Pin_Cfgs[gpio].mux) &&
            (kGPIO_DigitalOutput == Pin_Cfgs[gpio].dir))
            GPIO_PinWrite(Pin_Cfgs[gpio].gbase, Pin_Cfgs[gpio].pin, (uint8_t) state);
        else
            assert(false);
    }
    else
        assert(false);
}

uint32_t Read_GPIO(IO_Map_T gpio)
{
    uint32_t ret_val;

    if (gpio < NUM_IO)
    {
        if ((kPORT_MuxAsGpio == Pin_Cfgs[gpio].mux) &&
            (kGPIO_DigitalInput == Pin_Cfgs[gpio].dir))
            ret_val = GPIO_PinRead(Pin_Cfgs[gpio].gbase, Pin_Cfgs[gpio].pin);
        else
            assert(false);
    }
    else
        assert(false);

    return (ret_val);
}

