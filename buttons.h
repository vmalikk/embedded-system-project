//------------------------------------------------------------------------------
//  Module: buttons.h
//  Description: Debounced button interface for Project 8
//  Author: GitHub Copilot (MSP430 Project 8 integration)
//------------------------------------------------------------------------------

#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdint.h>

// Buttons available on the platform. These map to SW1 (P4.1) and SW2 (P2.3).
typedef enum {
    BUTTON_ID_SW1 = 0,
    BUTTON_ID_SW2 = 1,
    BUTTON_COUNT
} button_id_t;

// Event identifiers returned to the application loop.
typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_SW1_PRESS,
    BUTTON_EVENT_SW2_PRESS,
} button_event_t;

void buttons_init(void);
button_event_t buttons_get_event(void);
void buttons_post_event(button_event_t event);

#endif // BUTTONS_H_
