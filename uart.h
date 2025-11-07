//------------------------------------------------------------------------------
//  Module: uart.h
//  Description: UART abstraction for MSP430FR2355 Project 8
//------------------------------------------------------------------------------

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

typedef enum {
    UART_PORT_UCA0 = 0,
    UART_PORT_UCA1 = 1,
    UART_PORT_COUNT
} uart_port_t;

typedef enum {
    UART_BAUD_115200 = 0,
    UART_BAUD_9600   = 1,
} uart_baud_t;

#define UART_RX_BUFFER_SIZE   (128u)
#define UART_TX_BUFFER_SIZE   (128u)

void uart_init(uart_port_t port, uart_baud_t baud);
void uart_set_baud(uart_port_t port, uart_baud_t baud);
void uart_calculate_dividers(uart_baud_t baud, uint16_t *brw, uint16_t *mctlw);
void uart_tx_byte(uart_port_t port, uint8_t byte);
void uart_tx_buffer(uart_port_t port, const uint8_t *data, uint16_t length);
void uart_tx_string(uart_port_t port, const char *text);
uint16_t uart_rx_available(uart_port_t port);
int uart_rx_pop(uart_port_t port, uint8_t *byte);
void uart_flush_rx(uart_port_t port);
void uart_bridge_enable(uint8_t enable);
void uart_wait_tx_complete(uart_port_t port);

#endif // UART_H_
