#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/irq.h"

#include "gnss.h"
#include "gnss.pio.h"

// global vars
#define UART_ID uart1
#define UART_IRQ UART1_IRQ
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define BAUD_RATE 9600

char buffer[256];

void gnss_uart_init();


// for debugging and showing that gnss works on the given pins. not used by PIO
void on_uart_rx(void) {
    uint8_t ch;
    while (uart_is_readable(UART_ID)) {
        if ((ch = uart_getc(UART_ID))) {
            printf("%c", ch);
        }
    }
}

void gnss_uart_init() {
    // is this even needed or does PIO handle all of this?
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);
}

int main(void) {
    stdio_init_all();
    printf("pio gnss\n");
    // gnss_uart_init();
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &uart_rx_program);
    uart_rx_program_init(pio, sm, offset, UART_RX_PIN, BAUD_RATE);
    while (1) {
        char c = uart_rx_program_getc(pio, sm);
        printf("%c", c);
        // putchar(c);
    }
    printf("\n");
}