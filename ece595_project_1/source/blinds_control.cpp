#include <stdio.h>
#include <string.h>

#include "blinds_control.h"
#include "motor_controller.h"
#include "fsl_port.h"
#include "pin_mux.h"
#include "MK64F12.h"
#include "fsl_uart_freertos.h"
#include "fsl_uart.h"
#include "fsl_clock.h"

typedef enum
{
    POSITION_NOT_LEARNED=0,
    REQUEST_CLOSED_POSITION,
    SEEK_OPEN_POSITION,
    POSITION_LEARNED,
    MANUAL_CONTROL,
    LIGHT_CONTROL,
    TIME_CONTROL,
    IDLE
} Control_State_T;

typedef struct Position_Tag
{
    uint8_t fully_open;
    uint8_t fully_closed;
    uint8_t current;
} Position_T;


static uint8_t UART_RX_Buffer[4];
static char UART_TX_Buffer[128];

static uart_rtos_handle_t Handle;
static struct _uart_handle T_Handle;

uart_rtos_config_t UART_Config =
{
    UART0,
    0,
    115200,
    kUART_ParityDisabled,
    kUART_OneStopBit,
    UART_RX_Buffer,
    sizeof(UART_RX_Buffer)
};

static volatile Control_State_T Ctrl_State;
static Motor Stepper_Motor;
static Position_T Blinds_Position = {0};
static bool Toggle_State = false;

static volatile bool Print_Closed_Pos_Info = false;
static volatile bool Print_Open_Pos_Info = false;

void Run_Position_Learning(void);
void Boolean_Position_Control(void);

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

    /* Initialize a UART instance */
    UART_Config.srcclk = CLOCK_GetFreq(UART0_CLK_SRC);
    UART_RTOS_Init(&Handle, &T_Handle, &UART_Config);
    sprintf(UART_TX_Buffer, "Blinds controller initialized. Press SW3 to start position learning procedure.\r\n\n");
    UART_RTOS_Send(&Handle, (uint8_t *)UART_TX_Buffer, strlen(UART_TX_Buffer));
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
                UART_RTOS_Send(&Handle, (uint8_t *)UART_TX_Buffer, strlen(UART_TX_Buffer));
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
                UART_RTOS_Send(&Handle, (uint8_t *)UART_TX_Buffer, strlen(UART_TX_Buffer));
                Print_Open_Pos_Info = false;
            }
            break;
        case POSITION_LEARNED:
            sprintf(UART_TX_Buffer, "Position learned!\r\n");
            UART_RTOS_Send(&Handle, (uint8_t *)UART_TX_Buffer, strlen(UART_TX_Buffer));
            Blinds_Position.fully_open = (uint8_t) Stepper_Motor.Get_Position();
            Blinds_Position.current = Blinds_Position.fully_open;
            Stepper_Motor.Sleep();
            Ctrl_State = MANUAL_CONTROL; /* !!! TEMPORARY  !!!*/
            break;
        case IDLE:
        case LIGHT_CONTROL:
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
        UART_RTOS_Send(&Handle, (uint8_t *)UART_TX_Buffer, strlen(UART_TX_Buffer));

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
        UART_RTOS_Send(&Handle, (uint8_t *)UART_TX_Buffer, strlen(UART_TX_Buffer));

        deg_change = Blinds_Position.fully_open - Blinds_Position.fully_closed;
        Stepper_Motor.Wakeup();
        Stepper_Motor.Rotate(deg_change);
        Stepper_Motor.Sleep();

        Blinds_Position.fully_open = (uint8_t) Stepper_Motor.Get_Position();
        Blinds_Position.current = Blinds_Position.fully_open;
    }
}

