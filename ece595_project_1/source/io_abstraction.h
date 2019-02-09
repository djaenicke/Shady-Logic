#ifndef _IO_ABSTRACTION_H_
#define _IO_ABSTRSCTION_H_

#include <stdint.h>

#include "fsl_port.h"
#include "fsl_gpio.h"

typedef enum
{
    LOW=0,
    HIGH=1
} GPIO_State_T;

typedef struct
{
extern void Set_GPIO(IO_Map_T gpio, GPIO_State_T state);
extern uint32_t Read_GPIO(IO_Map_T gpio);

#endif /* _IO_ABSTRACTION_H_ */
