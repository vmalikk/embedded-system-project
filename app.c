#include <string.h>
#include <stdint.h>
#include "app.h"
#include "uart.h"
#include "buttons.h"
#include "lcd_app.h"
#include "messages.h"

extern volatile unsigned int Time_Sequence;

#define APP_MAX_COMMAND_LENGTH    (32u)
#define SPLASH_DURATION_TICKS      (25u)  // 5s @ 200ms per tick
#define BAUD_CHANGE_DELAY_TICKS    (10u)  // 2s @ 200ms per tick
#define TRANSMIT_HOLD_TICKS        (3u)   // 0.6s transmit acknowledgement

static app_state_t g_state = APP_STATE_WAITING;
static uart_baud_t g_current_baud = UART_BAUD_9600;
static char g_last_command[APP_MAX_COMMAND_LENGTH + 1u];
static uint8_t g_last_length = 0u;
static uint8_t g_command_valid = 0u;
static uint8_t g_pending_test_tx = 0u;
static unsigned int g_baud_change_tick = 0u;
static unsigned int g_transmit_tick = 0u;
static uint8_t g_splash_active = 1u;
static unsigned int g_splash_start_tick = 0u;
static char g_rx_buffer[APP_MAX_COMMAND_LENGTH + 1u];
static uint8_t g_rx_index = 0u;

static void app_show_waiting(void) {
    lcd_status_waiting(messages_get_baud_label(g_current_baud));
    g_state = APP_STATE_WAITING;
}

static void app_handle_complete_command(const char *data, uint8_t length) {
    if (!data || length == 0u) {
        return;
    }
    if (length >= APP_MAX_COMMAND_LENGTH) {
        length = (uint8_t)(APP_MAX_COMMAND_LENGTH - 1u);
    }

    memcpy(g_last_command, data, length);
    g_last_command[length] = '\0';
    g_last_length = length;
    g_command_valid = 1u;
    g_state = APP_STATE_RECEIVED;
    lcd_status_received(g_last_command);
    lcd_show_baud(messages_get_baud_label(g_current_baud));
}

static void app_toggle_baud(void) {
    g_current_baud = (g_current_baud == UART_BAUD_115200) ? UART_BAUD_9600 : UART_BAUD_115200;
    uart_set_baud(UART_PORT_UCA0, g_current_baud);
    uart_set_baud(UART_PORT_UCA1, g_current_baud);
    uart_flush_rx(UART_PORT_UCA1);

    g_command_valid = 0u;
    g_last_length = 0u;
    g_rx_index = 0u;
    g_pending_test_tx = 1u;
    g_baud_change_tick = Time_Sequence;

    lcd_clear_line(1u);
    lcd_clear_line(2u);
    lcd_clear_line(4u);
    lcd_show_baud(messages_get_baud_label(g_current_baud));
    g_state = APP_STATE_WAITING;
}

static void app_send_test_string(void) {
    const char *test = messages_get_test_string();
    if (test) {
        uart_tx_string(UART_PORT_UCA1, test);
        uart_tx_string(UART_PORT_UCA1, "\r\n");
        uart_wait_tx_complete(UART_PORT_UCA1);

        uart_tx_string(UART_PORT_UCA0, test);
        uart_tx_string(UART_PORT_UCA0, "\r\n");
        uart_wait_tx_complete(UART_PORT_UCA0);
    }
    g_pending_test_tx = 0u;
    app_show_waiting();
}

static void app_handle_rx(void) {
    uint8_t byte = 0u;
    while (uart_rx_pop(UART_PORT_UCA1, &byte)) {
        if (byte == '\r' || byte == '\n') {
            if (g_rx_index > 0u) {
                g_rx_buffer[g_rx_index] = '\0';
                app_handle_complete_command(g_rx_buffer, g_rx_index);
                g_rx_index = 0u;
            }
            continue;
        }

        if (g_rx_index < (APP_MAX_COMMAND_LENGTH - 1u)) {
            g_rx_buffer[g_rx_index++] = (char)byte;
        }
    }
}

static void app_handle_buttons(void) {
    button_event_t event = buttons_get_event();
    if (event == BUTTON_EVENT_NONE) {
        return;
    }

    if (event == BUTTON_EVENT_SW2_PRESS) {
        app_toggle_baud();
    } else if (event == BUTTON_EVENT_SW1_PRESS && g_command_valid) {
        lcd_status_transmit(g_last_command);
        lcd_show_baud(messages_get_baud_label(g_current_baud));
        if (g_last_length > 0u) {
            uart_tx_string(UART_PORT_UCA1, g_last_command);
            uart_tx_string(UART_PORT_UCA1, "\r\n");
            uart_wait_tx_complete(UART_PORT_UCA1);

            uart_tx_string(UART_PORT_UCA0, g_last_command);
            uart_tx_string(UART_PORT_UCA0, "\r\n");
            uart_wait_tx_complete(UART_PORT_UCA0);
        }
        g_state = APP_STATE_TRANSMIT;
        g_transmit_tick = Time_Sequence;
    }
}

static void app_update_transmit_hold(void) {
    if (g_state != APP_STATE_TRANSMIT) {
        return;
    }
    if ((unsigned int)(Time_Sequence - g_transmit_tick) >= TRANSMIT_HOLD_TICKS) {
        if (g_command_valid) {
            g_state = APP_STATE_RECEIVED;
            lcd_status_received(g_last_command);
        } else {
            app_show_waiting();
        }
    }
}

static void app_check_pending_test_send(void) {
    if (!g_pending_test_tx) {
        return;
    }
    if ((unsigned int)(Time_Sequence - g_baud_change_tick) >= BAUD_CHANGE_DELAY_TICKS) {
        app_send_test_string();
    }
}

static void app_handle_splash(void) {
    if (!g_splash_active) {
        return;
    }
    if ((unsigned int)(Time_Sequence - g_splash_start_tick) >= SPLASH_DURATION_TICKS) {
        g_splash_active = 0u;
        app_show_waiting();
    }
}

void app_init(void) {
    g_state = APP_STATE_WAITING;
    g_current_baud = UART_BAUD_9600;
    g_command_valid = 0u;
    g_pending_test_tx = 0u;
    g_splash_active = 1u;
    g_splash_start_tick = Time_Sequence;
    g_rx_index = 0u;
    g_last_length = 0u;
    memset(g_last_command, 0, sizeof(g_last_command));

    uart_bridge_enable(1u);
    lcd_show_splash();
}

void app_process(void) {
    app_handle_splash();
    if (g_splash_active) {
        return;
    }

    app_handle_buttons();
    app_handle_rx();
    app_update_transmit_hold();
    app_check_pending_test_send();
}
