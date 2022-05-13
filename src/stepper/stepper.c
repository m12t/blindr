#include <stdio.h>
#include "pico/stdlib.h"

#define SLEEP_PIN 13
#define STEP_PIN 14
#define DIRECTION_PIN 15

int main() {
    gpio_init(SLEEP_PIN);
    gpio_init(STEP_PIN);
    gpio_init(DIRECTION_PIN);

    gpio_set_dir(SLEEP_PIN, GPIO_OUT);
    gpio_set_dir(STEP_PIN, GPIO_OUT);
    gpio_set_dir(DIRECTION_PIN, GPIO_OUT);

    gpio_put(SLEEP_PIN, 0);

    int counter = 0;
    while (counter < 10) {
        printf("inside main while count: %d\n", counter);
        if (counter % 2 == 0) {
            gpio_put(DIRECTION_PIN, 1);
        } else {
            gpio_put(DIRECTION_PIN, 0);
        }

        for (int i = 0; i < 2000; i++) {
            gpio_put(STEP_PIN, 0);
            sleep_ms(1);
            gpio_put(STEP_PIN, 1);
            sleep_ms(1);
        }

        if (counter > 4) {
            printf("Sleeping...\n");
            gpio_put(SLEEP_PIN, 1);
            sleep_ms(10000);
            printf("Awake?\n");

        }
        counter++;
    }

    return 0;
}