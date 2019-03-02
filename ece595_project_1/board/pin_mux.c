/*
 * Copyright 2017-2018 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!GlobalInfo
product: Pins v4.0
* BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS ***********/

/**
 * @file    pin_mux.c
 * @brief   Board pins file.
 */
 
/* This is a template for board specific configuration created by MCUXpresso IDE Project Wizard.*/

#include "pin_mux.h"
#include "fsl_port.h"
#include "io_abstraction.h"

#define SOPT5_UART0TXSRC_UART_TX      0x00u   /*!< UART 0 transmit data source select: UART0_TX pin */

/**
 * @brief Set up and initialize all required blocks and functions related to the board hardware.
 */
void BOARD_InitBootPins(void)
{
    port_interrupt_t p_int_cfg;
    uint8_t i;

    /* Enable Port Clock Gate Controls */
    CLOCK_EnableClock(kCLOCK_PortA);
    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortC);


    for (i=0; i<NUM_IO; i++)
    {
        PORT_SetPinMux(Pin_Cfgs[i].pbase, Pin_Cfgs[i].pin, Pin_Cfgs[i].mux);
    }

    /* Enable interrupts for SW2 and SW3 */
    p_int_cfg = kPORT_InterruptRisingEdge;
    PORT_SetPinInterruptConfig(Pin_Cfgs[SW_2].pbase, Pin_Cfgs[SW_2].pin, p_int_cfg);
    PORT_SetPinInterruptConfig(Pin_Cfgs[SW_3].pbase, Pin_Cfgs[SW_3].pin, p_int_cfg);

    CLOCK_EnableClock(kCLOCK_Uart0);
    CLOCK_EnableClock(kCLOCK_Uart4);

    NVIC_SetPriority(UART0_RX_TX_IRQn, 5);
    NVIC_SetPriority(UART4_RX_TX_IRQn, 3);
}

void BOARD_Enable_SW_Interrupts(void)
{
    /* Enable switch interrupts */
    PORT_ClearPinsInterruptFlags(PORTC, 0xFFFFFFFF);
    NVIC_EnableIRQ(PORTC_IRQn);

    PORT_ClearPinsInterruptFlags(PORTA, 0xFFFFFFFF);
    NVIC_EnableIRQ(PORTA_IRQn);
}

