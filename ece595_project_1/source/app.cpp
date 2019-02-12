/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Application includes */
#include <stdio.h>
#include "app.h"
#include "assert.h"
#include "io_abstraction.h"
#include "blinds_control.h"

typedef struct Task_Cfg_Tag
{
    TaskFunction_t func;
    const char name[configMAX_TASK_NAME_LEN];
    const configSTACK_DEPTH_TYPE stack_size;
    UBaseType_t priority;
} Task_Cfg_T;

/* Task function declarations */
static void Init_App_Task(void *pvParameters);
static void UART_Task(void *pvParameters);
static void Blinds_Control_Task(void *pvParameters);

/* Task Configurations */
#define NUM_TASKS (2)
const Task_Cfg_T Task_Cfg_Table[NUM_TASKS] =
{
    /* Function,          Name,             Stack Size,  Priority */
    {Init_App_Task,       "Init_App",       110,         configMAX_PRIORITIES - 1},
    {Blinds_Control_Task, "Blinds Control", 1000,        configMAX_PRIORITIES - 2}
};

/* Local function declarations */
static void Init_OS_Tasks(void);

/* Function definitions */
void Init_OS_Tasks(void)
{
    uint8_t i;

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
        vTaskSuspend(NULL);
    }
}

static void Blinds_Control_Task(void *pvParameters)
{
    while(1)
    {
        Ctrl_State_Machine();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
