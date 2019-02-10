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
    LIGHT_CONTROL,
    TIME_CONTROL,
    IDLE
} Control_State_T;


uint8_t UART_RX_Buffer[4];
char UART_TX_Buffer[128];

uart_rtos_handle_t Handle;
struct _uart_handle T_Handle;

uart_rtos_config_t uart_config = {
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

static float Closed_Position;
static float Open_Position;

static volatile bool Print_Closed_Pos_Info = false;
static volatile bool Print_Open_Pos_Info = false;

void Run_Position_Learning(void);

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

    PORT_ClearPinsInterruptFlags(PORTC, 0xFFFFFFFF);
}
}

void Init_Blinds_Control(void)
{
    Ctrl_State = POSITION_NOT_LEARNED;
    Stepper_Motor.Enable_Driver();
    BOARD_Enable_SW_Interrupts();

    /* Initialize a UART instance */
    uart_config.srcclk = CLOCK_GetFreq(UART0_CLK_SRC);
    UART_RTOS_Init(&Handle, &T_Handle, &uart_config);
    sprintf(UART_TX_Buffer, "Blinds controller initialized.\r\n\n");
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
                Closed_Position = Stepper_Motor.Get_Position();
                Print_Closed_Pos_Info = false;
            }
            break;
        case SEEK_OPEN_POSITION:
            if (Print_Open_Pos_Info)
            {
                sprintf(UART_TX_Buffer, "Closed position learned!\r\nPress SW3 to begin seeking for open position.\r\n"
                                         "When fully open, press SW2 to finish the procedure.\r\n\n");
                UART_RTOS_Send(&Handle, (uint8_t *)UART_TX_Buffer, strlen(UART_TX_Buffer));
                Print_Open_Pos_Info = false;
            }
            break;
        case POSITION_LEARNED:
            sprintf(UART_TX_Buffer, "Position learned!\r\n");
            UART_RTOS_Send(&Handle, (uint8_t *)UART_TX_Buffer, strlen(UART_TX_Buffer));
            Open_Position = Stepper_Motor.Get_Position();
            Stepper_Motor.Sleep();
            Ctrl_State = IDLE;
            break;
        case IDLE:
        case LIGHT_CONTROL:
        case TIME_CONTROL:
            break;
    }
}

