#include <stdio.h>
#include "pico/stdlib.h"

int main() {
#ifndef PICO_DEFAULT_LED_PIN
#warning blink example requires a board with a regular LED
#else
    const uint LED = PICO_DEFAULT_LED_PIN;
    stdio_init_all();
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);
    while (true) {
        printf("Hello, world!\n");
        gpio_put(LED, 1);
        sleep_ms(50);
        gpio_put(LED, 0);
        sleep_ms(50);
    }
    return 0;
#endif
}