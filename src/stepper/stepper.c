#include <stdio.h>  // for debugging
#include "pico/stdlib.h"

// any free gpio pins can work for the below,
// orient wherever it is easiest for soldering
// and use with the GPS module.
#define SLEEP_PIN 13
#define STEP_PIN 14
#define DIRECTION_PIN 15

int stepper_main() {
    stdio_init_all();  // for debugging and prining to console
    gpio_init(SLEEP_PIN);
    gpio_init(STEP_PIN);
    gpio_init(DIRECTION_PIN);

    gpio_set_dir(SLEEP_PIN, GPIO_OUT);
    gpio_set_dir(STEP_PIN, GPIO_OUT);
    gpio_set_dir(DIRECTION_PIN, GPIO_OUT);

    gpio_put(SLEEP_PIN, 0);

    int counter = 0;
    while (true) {
        printf("inside main while count: %d\n", counter);
        if (counter % 2 == 0) {
            printf("going forwards\n");
            gpio_put(DIRECTION_PIN, 1);
        } else {
            printf("going backwards\n");
            gpio_put(DIRECTION_PIN, 0);
        }
        gpio_put(SLEEP_PIN, 1);
        sleep_ms(2);  // per instructions, leave >1ms after sleep before step input

        for (int i = 0; i < 10000; i++) {
            gpio_put(STEP_PIN, 0);
            sleep_us(30);
            gpio_put(STEP_PIN, 1);
        }
        gpio_put(SLEEP_PIN, 0);
        printf("sleeping...\n");
        sleep_ms(2000);
        counter++;
    }

    return 0;
}