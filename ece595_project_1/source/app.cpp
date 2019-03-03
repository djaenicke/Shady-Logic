/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Application includes */
#include <stdio.h>
#include <string.h>
#include "app.h"
#include "assert.h"
#include "io_abstraction.h"
#include "blinds_control.h"
#include "fsl_uart_freertos.h"
#include "fsl_uart.h"
#include "fsl_clock.h"


typedef struct Task_Cfg_Tag
{
    TaskFunction_t func;
    const char name[configMAX_TASK_NAME_LEN];
    const configSTACK_DEPTH_TYPE stack_size;
    UBaseType_t priority;
} Task_Cfg_T;

static char Debug_Tx_Buffer[DEBUG_TX_BUFFER_SIZE];
static uint8_t Debug_Rx_Buffer[8];
static uint8_t Bt_Rx_Buffer[13];

static uart_rtos_handle_t Debug_Handle;
static struct _uart_handle Debug_T_Handle;
static uart_rtos_handle_t BT_Handle;
static struct _uart_handle BT_T_Handle;

uart_rtos_config_t Debug_UART_Config =
{
    UART0,
    0,
    115200,
    kUART_ParityDisabled,
    kUART_OneStopBit,
    Debug_Rx_Buffer,
    sizeof(Debug_Rx_Buffer)
};

uart_rtos_config_t BT_UART_Config =
{
    UART4,
    0,
    9600,
    kUART_ParityDisabled,
    kUART_OneStopBit,
    Bt_Rx_Buffer,
    sizeof(Bt_Rx_Buffer)
};

/* Task function declarations */
static void Init_App_Task(void *pvParameters);
static void Blinds_Control_Task(void *pvParameters);
static void BluetoothSetup(void);
static void BluetoothTask(void *pvParameters);

/* Task Configurations */
#define NUM_TASKS (3)
const Task_Cfg_T Task_Cfg_Table[NUM_TASKS] =
{
    /* Function,          Name,             Stack Size,  Priority */
    {Init_App_Task,       "Init_App",       100,         configMAX_PRIORITIES - 1},
    {Blinds_Control_Task, "Blinds Control", 500,         configMAX_PRIORITIES - 3},
    {BluetoothTask,       "BluetoothTask",  500,         configMAX_PRIORITIES - 2},
};

/* Local function declarations */
static void Init_OS_Tasks(void);

/* Function definitions */
void Init_OS_Tasks(void)
{
    uint8_t i;

    /* Initialize a UART instance */
    Debug_UART_Config.srcclk = CLOCK_GetFreq(UART0_CLK_SRC);
    UART_RTOS_Init(&Debug_Handle, &Debug_T_Handle, &Debug_UART_Config);

    for (i=0; i<NUM_TASKS; i++)
    {
        if (xTaskCreate(Task_Cfg_Table[i].func, Task_Cfg_Table[i].name,
                        Task_Cfg_Table[i].stack_size, NULL, Task_Cfg_Table[i].priority, NULL) != pdPASS)
        {
            printf("Task number %d creation failed!.\r\n", i);
            assert(false);
        }
    }

    Set_GPIO(BLUE_LED, LOW);
}

/*Bluetooth Serial Connection Setup */
static void BluetoothSetup(void)
{
    /* Initialize a UART instance */
    BT_UART_Config.srcclk = CLOCK_GetFreq(UART4_CLK_SRC);
    UART_RTOS_Init(&BT_Handle, &BT_T_Handle, &BT_UART_Config);
}

static void BluetoothTask(void *pvParameters)
{
    static uint16_t last_head=0;

    while(1)
    {
        if (strlen(Debug_Tx_Buffer))
        {
            UART_RTOS_Send(&Debug_Handle, (uint8_t *)Debug_Tx_Buffer, strlen(Debug_Tx_Buffer));
            (void) memset(Debug_Tx_Buffer, 0, sizeof(Debug_Tx_Buffer));
        }

        if (last_head != BT_T_Handle.rxRingBufferHead)
        {
            last_head = BT_T_Handle.rxRingBufferHead;

            UART_RTOS_Send(&Debug_Handle, (uint8_t *)Bt_Rx_Buffer, 13);
            UART_RTOS_Send(&Debug_Handle, (uint8_t *)"\r\n", sizeof("\r\n"));

            if (NULL != strstr((const char*)&Bt_Rx_Buffer, "TOGGLE"))
            {
                Change_Control_State(MANUAL_CONTROL);
                Toggle_Blinds_State();
            }
            else if (NULL != strstr((const char*)&Bt_Rx_Buffer, "AUTO"))
            {
                Change_Control_State(LIGHT_CONTROL);
            }
            else
            {

            }
        }

        vTaskDelay(pdMS_TO_TICKS(400));
    }
}

void Start_OS(void)
{
    Init_OS_Tasks();
    vTaskStartScheduler();
    while(1);
}

static void Init_App_Task(void *pvParameters)
{
    while(1)
    {
        printf("Initializing application...\r\n");
        Init_Blinds_Control();
        BluetoothSetup();
        vTaskSuspend(NULL);
    }
}

static void Blinds_Control_Task(void *pvParameters)
{
    while(1)
    {
        Ctrl_State_Machine();
        vTaskDelay(pdMS_TO_TICKS(900));
    }
}

void Add_Debug_Message(char *msg)
{
    if (strlen(msg)<DEBUG_TX_BUFFER_SIZE)
    {
        memcpy(Debug_Tx_Buffer, msg, strlen(msg));
    }
    else
        assert(false);
}
