/*
 * motor_controller.cpp
 *
 *  Created on: Feb 9, 2019
 *      Author: djaenicke
 */

#include <math.h>

#include "motor_controller.h"
#include "io_abstraction.h"

#define STEPS_PER_REV      (1600)
#define DEG_PER_STEP       (360.0f/1600)


static void Delay(void);


void Motor::Set_Direction(Direction_T new_dir)
{
    if (CW == new_dir)
    {
        Set_GPIO(MOTOR_DIR_CTRL, HIGH);
        last_dir = CW;
    }
    else
    {
        Set_GPIO(MOTOR_DIR_CTRL, LOW);
        last_dir = CCW;
    }
}

void Motor::Enable_Driver(void)
{
    Set_GPIO(MOTOR_DRV_EN, LOW);
}

void Motor::Disable_Driver(void)
{
    Set_GPIO(MOTOR_DRV_EN, HIGH);
}

void Motor::Sleep(void)
{
    if (!is_asleep)
    {
        Set_GPIO(MOTOR_SLEEP, LOW);
        is_asleep = true;
    }
}

void Motor::Wakeup(void)
{
    if (is_asleep)
    {
        Set_GPIO(MOTOR_SLEEP, HIGH);
        is_asleep = false;
    }
}

void Motor::Reset_Driver(void)
{
    Set_GPIO(MOTOR_DRV_RESET, LOW);
    Delay();
    Set_GPIO(MOTOR_DRV_RESET, HIGH);
}

void Motor::Rotate(float degrees)
{
    float steps_needed;
    uint16_t i;

    if (degrees > 0)
        Set_Direction(CW);
    else
        Set_Direction(CCW);

    rel_position += degrees;
    steps_needed = abs(degrees)/DEG_PER_STEP;

    for (i=0; i<=round(steps_needed); i++)
    {
        Set_GPIO(MOTOR_STEP_CTRL, HIGH);
        Delay();
        Set_GPIO(MOTOR_STEP_CTRL, LOW);
    }
}

void Motor::Zero_Position(void)
{
    rel_position = 0.0f;
}

void Delay(void)
{
    volatile uint32_t i = 0;
    for (i = 0; i < 2500; i++)
    {
        __asm("NOP");
    }
}
