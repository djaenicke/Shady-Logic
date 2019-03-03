#include <stdio.h>
#include <string.h>

#include "blinds_control.h"
#include "motor_controller.h"
#include "light_sensor.h"
#include "fsl_port.h"
#include "pin_mux.h"
#include "MK64F12.h"
#include "fsl_uart_freertos.h"
#include "fsl_uart.h"
#include "fsl_clock.h"
#include "app.h"

#define MIN_LIGHT  (0)
#define MAX_LIGHT  (700)

#define HYSTERESIS (15)

typedef struct Position_Tag
{
    uint8_t fully_open;
    uint8_t fully_closed;
    uint8_t current;
} Position_T;

static char UART_TX_Buffer[DEBUG_TX_BUFFER_SIZE];

static volatile Control_State_T Ctrl_State;
static Motor Stepper_Motor;
static Position_T Blinds_Position = {0};
static bool Toggle_State = false;

static volatile bool Print_Closed_Pos_Info = false;
static volatile bool Print_Open_Pos_Info = false;

static LightSensor Light_Sensor;

static void Run_Position_Learning(void);
static void Boolean_Position_Control(void);
void Light_Control(void);
static uint8_t Map_Light_To_Position(uint16_t light_val);

extern "C"
{
void PORTA_IRQHandler(void)
{
    Run_Position_Learning();
    PORT_ClearPinsInterruptFlags(PORTA, 0xFFFFFFFF);
}

void PORTC_IRQHandler(void)
{
    if (SEEK_OPEN_POSITION == Ctrl_State)
    {
        Ctrl_State = POSITION_LEARNED;
    }
    else if (MANUAL_CONTROL == Ctrl_State)
    {
        Toggle_State = true;
    }
    else
    {
        /* Do Nothing */
    }

    PORT_ClearPinsInterruptFlags(PORTC, 0xFFFFFFFF);
}
}

void Init_Blinds_Control(void)
{
    Ctrl_State = POSITION_NOT_LEARNED;
    Stepper_Motor.Disable_Driver();
    Stepper_Motor.Sleep();
    BOARD_Enable_SW_Interrupts();

    sprintf(UART_TX_Buffer, "Blinds controller initialized. Press SW3 to start position learning procedure.\r\n\n");
    Add_Debug_Message(UART_TX_Buffer);
}

void Run_Position_Learning(void)
{
    switch (Ctrl_State)
    {
        case POSITION_NOT_LEARNED:
            Ctrl_State = REQUEST_CLOSED_POSITION;
            Print_Closed_Pos_Info = true;
            break;
        case REQUEST_CLOSED_POSITION:
            Ctrl_State = SEEK_OPEN_POSITION;
            Print_Open_Pos_Info = true;
            break;
        case SEEK_OPEN_POSITION:
            /* Step forward by 30 degrees every button press */
            Stepper_Motor.Rotate(30);
            break;
        default:
            /* Restart position learning procedure */
            Ctrl_State = POSITION_NOT_LEARNED;
            break;
    }
}

void Ctrl_State_Machine(void)
{
    switch (Ctrl_State)
    {
        case POSITION_NOT_LEARNED:
            /* Do nothing */
            break;
        case REQUEST_CLOSED_POSITION:
            if (Print_Closed_Pos_Info)
            {
                sprintf(UART_TX_Buffer, "Place blinds in fully closed position and press SW3 when finished.\r\n\n");
                Add_Debug_Message(UART_TX_Buffer);
                Stepper_Motor.Zero_Position();
                Blinds_Position.fully_closed = (uint8_t) Stepper_Motor.Get_Position();
                Print_Closed_Pos_Info = false;
            }
            break;
        case SEEK_OPEN_POSITION:
            if (Print_Open_Pos_Info)
            {
                Stepper_Motor.Enable_Driver();
                Stepper_Motor.Wakeup();
                sprintf(UART_TX_Buffer, "Closed position learned!\r\nPress SW3 to begin seeking for open position.\r\n"
                                         "When fully open, press SW2 to finish the procedure.\r\n\n");
                Add_Debug_Message(UART_TX_Buffer);
                Print_Open_Pos_Info = false;
            }
            break;
        case POSITION_LEARNED:
            sprintf(UART_TX_Buffer, "Position learned!\r\n");
            Add_Debug_Message(UART_TX_Buffer);
            Blinds_Position.fully_open = (uint8_t) Stepper_Motor.Get_Position();
            Blinds_Position.current = Blinds_Position.fully_open;
            Stepper_Motor.Sleep();
            Ctrl_State = MANUAL_CONTROL;
            break;
        case IDLE:
            break;
        case LIGHT_CONTROL:
            Light_Control();
            break;
        case TIME_CONTROL:
            break;
        case MANUAL_CONTROL:
            if (Toggle_State)
            {
                Boolean_Position_Control();
            }
            Toggle_State = false;
            break;
    }
}

void Boolean_Position_Control(void)
{
    float deg_change;

    if (Blinds_Position.current == Blinds_Position.fully_open)
    {
        /* Close */
        sprintf(UART_TX_Buffer, "Closing blinds...\r\n");
        Add_Debug_Message(UART_TX_Buffer);

        deg_change = (float) Blinds_Position.fully_closed - Blinds_Position.fully_open;
        Stepper_Motor.Wakeup();
        Stepper_Motor.Rotate(deg_change);
        Stepper_Motor.Sleep();

        Blinds_Position.fully_closed = (uint8_t) Stepper_Motor.Get_Position();
        Blinds_Position.current = Blinds_Position.fully_closed;
    }
    else if (Blinds_Position.current == Blinds_Position.fully_closed)
    {
        /* Open */
        sprintf(UART_TX_Buffer, "Opening blinds...\r\n");
        Add_Debug_Message(UART_TX_Buffer);

        deg_change = Blinds_Position.fully_open - Blinds_Position.fully_closed;
        Stepper_Motor.Wakeup();
        Stepper_Motor.Rotate(deg_change);
        Stepper_Motor.Sleep();

        Blinds_Position.fully_open = (uint8_t) Stepper_Motor.Get_Position();
        Blinds_Position.current = Blinds_Position.fully_open;
    }
}

void Toggle_Blinds_State(void)
{
    Toggle_State = true;
}

void Change_Control_State(Control_State_T new_state)
{
    float deg_change;

    switch(new_state)
    {
        case LIGHT_CONTROL:
            Ctrl_State = LIGHT_CONTROL;
            break;
        case MANUAL_CONTROL:
            if (LIGHT_CONTROL == Ctrl_State)
            {
                deg_change = ((float)Blinds_Position.current - Blinds_Position.fully_open)*-1;
                Stepper_Motor.Wakeup();
                Stepper_Motor.Rotate(deg_change);
                Stepper_Motor.Sleep();
                Blinds_Position.current = Stepper_Motor.Get_Position();
            }
            Ctrl_State = MANUAL_CONTROL;
            break;
        default:
            /* Do Nothing */
            break;
    }
}

void Light_Control(void)
{
    uint16_t new_position = 0;
    float deg_change;
    bool new_position_requested = false;

    new_position = Map_Light_To_Position(Light_Sensor.Get_Value());

    sprintf(UART_TX_Buffer, "Current Position = %d, New Position = %d\r\n", Blinds_Position.current, new_position);
    Add_Debug_Message(UART_TX_Buffer);

    if (abs(new_position - Blinds_Position.current) > HYSTERESIS)
    {
        new_position_requested = true;
    }
    else if (new_position > (Blinds_Position.fully_open-HYSTERESIS))
    {
        new_position = Blinds_Position.fully_open;
        new_position_requested = true;
    }
    else if (new_position < (Blinds_Position.fully_closed+HYSTERESIS))
    {
        new_position = Blinds_Position.fully_closed;
        new_position_requested = true;
    }

    if (new_position_requested)
    {
        deg_change = ((float)Blinds_Position.current - new_position)*-1;

        Stepper_Motor.Wakeup();
        Stepper_Motor.Rotate(deg_change);
        Stepper_Motor.Sleep();

        Blinds_Position.current = Stepper_Motor.Get_Position();
    }
}

uint8_t Map_Light_To_Position(uint16_t light_val)
{
    uint8_t ret_val;

    if (light_val > MAX_LIGHT)
    {
        light_val = MAX_LIGHT;
    }

    ret_val = ((float)light_val/MAX_LIGHT)*Blinds_Position.fully_open;
    return(ret_val);
}


