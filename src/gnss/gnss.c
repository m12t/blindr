/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
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


// function prototypes
int split_lines(char *buffer, bool print);
int parse_line(char *buffer, char **fields, int max_fields);
int console_print(char *buffer);
void on_uart_rx();

int split_lines(char *buffer, bool print) {
    int i = 0;
    uint8_t ch;
    char *field[20];
    uart_read_blocking(UART_ID, buffer, 255);
    // while (uart_is_readable(UART_ID) && (ch = uart_getc(UART_ID)) != '$') {
    //     if (!uart_is_readable(UART_ID))
    //         printf("uart not readable");
    //     buffer[i++] = ch;
    // }
    if (print)  // print the buffer to the console
        console_print(buffer);
    parse_line(buffer, field, 20);
    // printf("MSG type  :%s\r\n",field[0]);
    // printf("UTC Time  :%s\r\n",field[1]);
    // printf("Latitude  :%s\r\n",field[2]);
    // printf("Longitude :%s\r\n",field[4]);
    // printf("Satellites:%s\r\n",field[7]);
}

int parse_line(char *buffer, char **fields, int max_fields) {
    int i = 0;
	fields[i++] = buffer;

	while ((i < max_fields) && (*buffer = strchr(buffer, ',') != NULL)) {
		*buffer = '\0';  // change the comma to an end of string NULL character
		fields[i++] = ++buffer;
	}
	return --i;
}

int console_print(char *buffer) {
    printf("%s\n", buffer);
}

// RX interrupt handler
void on_uart_rx() {
    char buffer[255];
    split_lines(buffer, true);
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
