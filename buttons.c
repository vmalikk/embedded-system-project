//------------------------------------------------------------------------------
//  Module: buttons.c
//  Description: Debounced button interface for Project 8
//------------------------------------------------------------------------------

#include "msp430.h"
#include "buttons.h"
#include "ports.h"

// Bitmask for pending events (bit0 = SW1, bit1 = SW2)
static volatile uint8_t g_button_event_mask = 0;

#define BUTTON_EVENT_MASK_SW1   (1u << 0)
#define BUTTON_EVENT_MASK_SW2   (1u << 1)

void buttons_init(void) {
    // Configure SW1 (P4.1)
    P4DIR &= ~SW1;
    P4OUT |= SW1;   // Pull-up
    P4REN |= SW1;
    P4IES |= SW1;   // High-to-low transition => press
    P4IFG &= ~SW1;
    P4IE  |= SW1;

    // Configure SW2 (P2.3)
    P2DIR &= ~SW2;
    P2OUT |= SW2;
    P2REN |= SW2;
    P2IES |= SW2;
    P2IFG &= ~SW2;
    P2IE  |= SW2;

    g_button_event_mask = 0;
}

button_event_t buttons_get_event(void) {
    uint16_t gie = __get_SR_register() & GIE;
    __disable_interrupt();

    button_event_t event = BUTTON_EVENT_NONE;
    if (g_button_event_mask & BUTTON_EVENT_MASK_SW1) {
        g_button_event_mask &= ~BUTTON_EVENT_MASK_SW1;
        event = BUTTON_EVENT_SW1_PRESS;
    } else if (g_button_event_mask & BUTTON_EVENT_MASK_SW2) {
        g_button_event_mask &= ~BUTTON_EVENT_MASK_SW2;
        event = BUTTON_EVENT_SW2_PRESS;
    }

    if (gie) {
        __enable_interrupt();
    }

    return event;
}

void buttons_post_event(button_event_t event) {
    switch (event) {
        case BUTTON_EVENT_SW1_PRESS:
            g_button_event_mask |= BUTTON_EVENT_MASK_SW1;
            break;
        case BUTTON_EVENT_SW2_PRESS:
            g_button_event_mask |= BUTTON_EVENT_MASK_SW2;
            break;
        default:
            break;
    }
}
