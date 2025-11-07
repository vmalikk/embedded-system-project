//------------------------------------------------------------------------------
//
//  Description: Project 8 entry point
//
//------------------------------------------------------------------------------

#include "msp430.h"
#include "macros.h"
#include "ports.h"
#include "clocks.h"
#include "init.h"
#include "timersB0.h"
#include "LCD.h"
#include "display.h"
#include "system.h"
#include "buttons.h"
#include "uart.h"
#include "app.h"

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    Init_Ports();
    Init_Clocks();
    Init_Conditions();
    Init_Timers();
    Init_LCD();

    buttons_init();
        uart_init(UART_PORT_UCA0, UART_BAUD_9600);
        uart_init(UART_PORT_UCA1, UART_BAUD_9600);

    app_init();

    enable_interrupts();

    for (;;) {
        Display_Process();
        app_process();
    }
}

