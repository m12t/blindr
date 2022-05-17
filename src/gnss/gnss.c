/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"


/// \tag::uart_advanced[]

#define UART_ID uart1
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 4
#define UART_RX_PIN 5


// RX interrupt handler
void on_uart_rx() {
    char buffer[255];
    int i = 0;
    uint8_t ch;
    char id[3];
    while (uart_is_readable(UART_ID) && (ch = uart_getc(UART_ID)) != '$') {
        buffer[i++] = ch;
    }
    // while (uart_is_readable(UART_ID) && (ch = uart_getc(UART_ID)) != '\n') {
    //     buffer[i++] = ch;
    // }
    // while (uart_is_readable(UART_ID)) {
    //     ch = uart_getc(UART_ID);
    //     if (ch == '\n')
    //         printf("found a newline\n");
    //     printf("%c", ch);
    // }
    printf("%s", buffer);
}

void setup() {
    printf("\n\n Initializing... \n");
    stdio_init_all();
    uart_init(UART_ID, 9600);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    // int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);


}


int main() {
    setup();
    while (1)
        tight_loop_contents();
}

/// \end:uart_advanced[]
