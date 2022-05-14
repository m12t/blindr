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
    int count = 0;
    while (true) {
        printf("Hello, world! for the %d time\n", count++);
        gpio_put(LED, 1);
        sleep_ms(50);
        gpio_put(LED, 0);
        sleep_ms(1000);
    }
    return 0;
#endif
}