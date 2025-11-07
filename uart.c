//------------------------------------------------------------------------------
//  Module: uart.c
//  Description: UART abstraction for MSP430FR2355 Project 8
//------------------------------------------------------------------------------

#include "msp430.h"
#include "uart.h"
#include "ports.h"

#if (UART_RX_BUFFER_SIZE & (UART_RX_BUFFER_SIZE - 1))
#error "UART_RX_BUFFER_SIZE must be a power of two"
#endif
#if (UART_TX_BUFFER_SIZE & (UART_TX_BUFFER_SIZE - 1))
#error "UART_TX_BUFFER_SIZE must be a power of two"
#endif

#define UART_RX_BUFFER_MASK    (UART_RX_BUFFER_SIZE - 1u)
#define UART_TX_BUFFER_MASK    (UART_TX_BUFFER_SIZE - 1u)

typedef struct {
    volatile uint8_t buffer[UART_RX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} uart_ring_t;

typedef struct {
    uart_ring_t rx;
    uart_ring_t tx;
} uart_state_t;

typedef struct {
    volatile uint16_t *ctlw0;
    volatile uint16_t *brw;
    volatile uint16_t *mctlw;
    volatile uint16_t *statw;
    volatile uint16_t *rxbuf;
    volatile uint16_t *txbuf;
    volatile uint16_t *ie;
    volatile uint16_t *ifg;
} uart_hw_t;

static uart_state_t g_uart_state[UART_PORT_COUNT];

static const uart_hw_t g_uart_hw[UART_PORT_COUNT] = {
    [UART_PORT_UCA0] = {
        .ctlw0 = &UCA0CTLW0,
        .brw   = &UCA0BRW,
        .mctlw = &UCA0MCTLW,
        .statw = &UCA0STATW,
        .rxbuf = &UCA0RXBUF,
        .txbuf = &UCA0TXBUF,
        .ie    = &UCA0IE,
        .ifg   = &UCA0IFG,
    },
    [UART_PORT_UCA1] = {
        .ctlw0 = &UCA1CTLW0,
        .brw   = &UCA1BRW,
        .mctlw = &UCA1MCTLW,
        .statw = &UCA1STATW,
        .rxbuf = &UCA1RXBUF,
        .txbuf = &UCA1TXBUF,
        .ie    = &UCA1IE,
        .ifg   = &UCA1IFG,
    },
};

static volatile uint8_t g_bridge_enabled = 1u;

static inline uint16_t critical_enter(void) {
    uint16_t sr = __get_SR_register();
    __disable_interrupt();
    return sr;
}

static inline void critical_exit(uint16_t sr) {
    if (sr & GIE) {
        __enable_interrupt();
    }
}

static void uart_configure_pins(uart_port_t port) {
    if (port == UART_PORT_UCA0) {
        // UCA0 on P1.6 (RXD) / P1.7 (TXD)
        P1SEL0 |= (UCA0RXD | UCA0TXD);
        P1SEL1 &= ~(UCA0RXD | UCA0TXD);
    } else {
        // UCA1 on P4.2 (RXD) / P4.3 (TXD)
        P4SEL0 |= (UCA1RXD | UCA1TXD);
        P4SEL1 &= ~(UCA1RXD | UCA1TXD);
    }
}

static inline void uart_reset_state(uart_state_t *state) {
    state->rx.head = state->rx.tail = 0u;
    state->tx.head = state->tx.tail = 0u;
}

static void uart_apply_baud(uart_port_t port, uart_baud_t baud) {
    uint16_t brw = 0;
    uint16_t mctlw = 0;
    uart_calculate_dividers(baud, &brw, &mctlw);
    *g_uart_hw[port].brw = brw;
    *g_uart_hw[port].mctlw = mctlw;
}

void uart_calculate_dividers(uart_baud_t baud, uint16_t *brw, uint16_t *mctlw) {
    if (!brw || !mctlw) {
        return;
    }
    switch (baud) {
        case UART_BAUD_115200:
            // 8 MHz SMCLK, oversampling. Error ~0.64%
            *brw = 4u;
            *mctlw = (uint16_t)((0x55u << 8) | (5u << 4) | UCOS16);
            break;
        case UART_BAUD_9600:
            // 8 MHz SMCLK, low-frequency mode. Error ~0.04%
            *brw = 833u;
            *mctlw = 0u;
            break;
        default:
            *brw = 4u;
            *mctlw = (uint16_t)((0x55u << 8) | (5u << 4) | UCOS16);
            break;
    }
}

void uart_init(uart_port_t port, uart_baud_t baud) {
    if (port >= UART_PORT_COUNT) {
        return;
    }

    uart_state_t *state = &g_uart_state[port];
    const uart_hw_t *hw = &g_uart_hw[port];

    uart_configure_pins(port);
    uart_reset_state(state);

    *hw->ctlw0 = UCSWRST;
    *hw->ctlw0 |= UCSSEL__SMCLK;     // SMCLK source
    *hw->ctlw0 &= ~(UCPEN | UCMSB | UCSPB | UCSYNC | UC7BIT);
    *hw->ctlw0 |= UCMODE_0;          // UART mode

    uart_apply_baud(port, baud);

    *hw->ctlw0 &= ~UCSWRST;          // Release for operation
    *hw->ifg &= ~(UCTXIFG | UCRXIFG);
    *hw->ie |= UCRXIE;               // Enable RX interrupt
}

void uart_set_baud(uart_port_t port, uart_baud_t baud) {
    if (port >= UART_PORT_COUNT) {
        return;
    }

    const uart_hw_t *hw = &g_uart_hw[port];
    uint16_t sr = critical_enter();

    *hw->ctlw0 |= UCSWRST;
    uart_apply_baud(port, baud);
    *hw->ctlw0 &= ~UCSWRST;
    *hw->ifg &= ~(UCTXIFG | UCRXIFG);
    *hw->ie |= UCRXIE;

    critical_exit(sr);
}

static inline void uart_enable_tx_interrupt(const uart_hw_t *hw) {
    *hw->ie |= UCTXIE;
}

static inline void uart_disable_tx_interrupt(const uart_hw_t *hw) {
    *hw->ie &= ~UCTXIE;
}

static void uart_tx_enqueue(uart_port_t port, uint8_t byte) {
    uart_state_t *state = &g_uart_state[port];
    const uart_hw_t *hw = &g_uart_hw[port];

    uint16_t next = (uint16_t)((state->tx.head + 1u) & UART_TX_BUFFER_MASK);
    if (next == state->tx.tail) {
        // Buffer full; drop oldest byte
        state->tx.tail = (uint16_t)((state->tx.tail + 1u) & UART_TX_BUFFER_MASK);
    }
    state->tx.buffer[state->tx.head] = byte;
    state->tx.head = next;
    uart_enable_tx_interrupt(hw);
}

void uart_tx_byte(uart_port_t port, uint8_t byte) {
    if (port >= UART_PORT_COUNT) {
        return;
    }

    const uart_hw_t *hw = &g_uart_hw[port];
    uart_state_t *state = &g_uart_state[port];

    uint16_t sr = critical_enter();

    if (state->tx.head == state->tx.tail && (*hw->ifg & UCTXIFG)) {
        *hw->txbuf = byte;
    } else {
        uart_tx_enqueue(port, byte);
    }

    critical_exit(sr);
}

void uart_tx_buffer(uart_port_t port, const uint8_t *data, uint16_t length) {
    if (!data) {
        return;
    }
    while (length--) {
        uart_tx_byte(port, *data++);
    }
}

void uart_tx_string(uart_port_t port, const char *text) {
    if (!text) {
        return;
    }
    while (*text) {
        uart_tx_byte(port, (uint8_t)*text++);
    }
}

uint16_t uart_rx_available(uart_port_t port) {
    if (port >= UART_PORT_COUNT) {
        return 0u;
    }
    uart_state_t *state = &g_uart_state[port];
    return (uint16_t)((state->rx.head - state->rx.tail) & UART_RX_BUFFER_MASK);
}

int uart_rx_pop(uart_port_t port, uint8_t *byte) {
    if (port >= UART_PORT_COUNT || !byte) {
        return 0;
    }
    uart_state_t *state = &g_uart_state[port];

    uint16_t sr = critical_enter();
    if (state->rx.head == state->rx.tail) {
        critical_exit(sr);
        return 0;
    }

    *byte = state->rx.buffer[state->rx.tail];
    state->rx.tail = (uint16_t)((state->rx.tail + 1u) & UART_RX_BUFFER_MASK);

    critical_exit(sr);
    return 1;
}

void uart_flush_rx(uart_port_t port) {
    if (port >= UART_PORT_COUNT) {
        return;
    }
    uint16_t sr = critical_enter();
    g_uart_state[port].rx.head = 0u;
    g_uart_state[port].rx.tail = 0u;
    critical_exit(sr);
}

void uart_bridge_enable(uint8_t enable) {
    g_bridge_enabled = enable ? 1u : 0u;
}

static void uart_forward_to_peer(uart_port_t source, uint8_t byte) {
    if (!g_bridge_enabled) {
        return;
    }
    uart_port_t peer = (source == UART_PORT_UCA0) ? UART_PORT_UCA1 : UART_PORT_UCA0;
    uart_tx_enqueue(peer, byte);
}

void uart_wait_tx_complete(uart_port_t port) {
    if (port >= UART_PORT_COUNT) {
        return;
    }

    const uart_hw_t *hw = &g_uart_hw[port];
    uart_state_t *state = &g_uart_state[port];

    for (;;) {
        uint16_t sr = critical_enter();
        uint16_t head = state->tx.head;
        uint16_t tail = state->tx.tail;
        uint16_t done = (uint16_t)(*hw->ifg & UCTXIFG);
        critical_exit(sr);

        if ((head == tail) && done) {
            break;
        }
    }
}

static void uart_isr_rx(uart_port_t port) {
    uart_state_t *state = &g_uart_state[port];
    const uart_hw_t *hw = &g_uart_hw[port];
    uint8_t byte = (uint8_t)(*hw->rxbuf);

    uint16_t next = (uint16_t)((state->rx.head + 1u) & UART_RX_BUFFER_MASK);
    if (next == state->rx.tail) {
        state->rx.tail = (uint16_t)((state->rx.tail + 1u) & UART_RX_BUFFER_MASK);
    }
    state->rx.buffer[state->rx.head] = byte;
    state->rx.head = next;

    uart_forward_to_peer(port, byte);
}

static void uart_isr_tx(uart_port_t port) {
    uart_state_t *state = &g_uart_state[port];
    const uart_hw_t *hw = &g_uart_hw[port];

    if (state->tx.head == state->tx.tail) {
        uart_disable_tx_interrupt(hw);
        return;
    }

    uint8_t byte = state->tx.buffer[state->tx.tail];
    state->tx.tail = (uint16_t)((state->tx.tail + 1u) & UART_TX_BUFFER_MASK);
    *hw->txbuf = byte;
}

#pragma vector=EUSCI_A0_VECTOR
__interrupt void eUSCI_A0_ISR(void) {
    switch (__even_in_range(UCA0IV, 0x08)) {
        case 0x00:
            break;
        case 0x02:
            uart_isr_rx(UART_PORT_UCA0);
            break;
        case 0x04:
            uart_isr_tx(UART_PORT_UCA0);
            break;
        default:
            break;
    }
}

#pragma vector=EUSCI_A1_VECTOR
__interrupt void eUSCI_A1_ISR(void) {
    switch (__even_in_range(UCA1IV, 0x08)) {
        case 0x00:
            break;
        case 0x02:
            uart_isr_rx(UART_PORT_UCA1);
            break;
        case 0x04:
            uart_isr_tx(UART_PORT_UCA1);
            break;
        default:
            break;
    }
}
