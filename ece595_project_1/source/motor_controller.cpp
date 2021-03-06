/*
 * motor_controller.cpp
 *
 *  Created on: Feb 9, 2019
 *      Author: djaenicke
 */

#include <math.h>

#include "motor_controller.h"
#include "io_abstraction.h"
#include "clock_config.h"

#define STEPS_PER_REV       (1600)
#define DEG_PER_STEP        (360.0f/1600)
#define MICROSTEPS_PER_STEP (8)

#define INSTUCTIONS_PER_SEC (BOARD_BOOTCLOCKRUN_CORE_CLOCK)
#define ONE_MSEC_DELAY_CNT  ((uint32_t)(BOARD_BOOTCLOCKRUN_CORE_CLOCK/1000))

#define DEG_PER_SEC         (360.0f) /* Angular Velocity */
#define STEP_DELAY_CNT      ((uint32_t)(INSTUCTIONS_PER_SEC/(DEG_PER_SEC*MICROSTEPS_PER_STEP)))

static void Delay(uint32_t delay_cnt);


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
    Delay(ONE_MSEC_DELAY_CNT);
    Set_GPIO(MOTOR_DRV_RESET, HIGH);
}

void Motor::Rotate(float degrees)
{
    float steps_needed;
    uint16_t i;
    int8_t sign = 0;

    if (degrees > 0.0f)
    {
        Set_Direction(CW);
        sign = 1;
    }
    else
    {
        Set_Direction(CCW);
        sign = -1;
    }

    steps_needed = abs(degrees)/DEG_PER_STEP;

    for (i=0; i<=round(steps_needed); i++)
    {
        Set_GPIO(MOTOR_STEP_CTRL, HIGH);
        Delay(STEP_DELAY_CNT);
        Set_GPIO(MOTOR_STEP_CTRL, LOW);
        rel_position += sign * DEG_PER_STEP;
    }
}

void Motor::Zero_Position(void)
{
    rel_position = 0.0f;
}

float Motor::Get_Position(void)
{
    return(rel_position);
}

void Delay(uint32_t delay_cnt)
{
    volatile uint32_t i = 0;
    for (i = 0; i < (delay_cnt/8); i++) /* divide by 8 is here to account for other instructions */
    {
        __asm("NOP");
    }
}
