#include <stdio.h>
#include "pico/stdlib.h"
// #include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/irq.h"


#define UART_ID uart1
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

#define UART_TX_PIN 6
#define UART_RX_PIN 7

static int chars_rxed = 0;
const uint LED = PICO_DEFAULT_LED_PIN;


void parse_uart() {
    if (UART_ID == "$") {
        printf("\n");
        gpio_put(LED, 0);
    } else {
        printf("%c", uart_getc(UART_ID));
        gpio_put(LED, 1);
    }
}

int main() {
    // LED inits
    stdio_init_all();
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    // UART inits
    uart_init(uart1, 9600);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, false);

    // OK, all set up.
    while (1)
        parse_uart();

    // while (true) {
    //     printf("gps on uart 1!\n");
    //     // printf("%s");
    //     gpio_put(LED, 1);
    //     sleep_ms(50);
    //     gpio_put(LED, 0);
    //     sleep_ms(50);
    // }
    return 0;
}