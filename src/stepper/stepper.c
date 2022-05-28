#include "../blindr.h"  // for global defines
#include "stepper.h"
#include "pico/stdlib.h"  // GPIO_OUT, etc.


void stepper_init(void) {
    gpio_init(SLEEP_PIN);
    gpio_init(STEP_PIN);
    gpio_init(DIRECTION_PIN);

    gpio_set_dir(SLEEP_PIN, GPIO_OUT);
    gpio_set_dir(STEP_PIN, GPIO_OUT);
    gpio_set_dir(DIRECTION_PIN, GPIO_OUT);

    sleep_stepper();  // default to sleeping the stepper
}

void wake_stepper() {
    gpio_put(SLEEP_PIN, 1);  // current is now flowing throught the stepper
    sleep_ms(2);  // per instructions, leave >1ms after sleep before step input
}

void sleep_stepper() {
    gpio_put(SLEEP_PIN, 0);
}

int single_step(int direction) {
    wake_stepper();  // if calling this many times, you don't want to

    // set the stepper going in the correct direction
    gpio_put(DIRECTION_PIN, direction);
    // a single step is a rising edge. Do this by setting low then setting high
    gpio_put(STEP_PIN, 0);
    sleep_us(30);  // give a healthy margin between signals
    gpio_put(STEP_PIN, 1);

    sleep_stepper();

    return 2*direction-1;  // a step was completed, return the direction-aware step distance [-1, 1]
}

int step_to(int *current_position, int desired_position, int BOUNDARY_HIGH) {
    // receive the current position, desired_position, and high boundary (low boundary is normalized to 0)
    // and step to the desired destination, updating the current position pointer along the way

    // check that the inputs are valid
    if ((desired_position < 0) ||
        (desired_position > BOUNDARY_HIGH) ||
        (desired_position == *current_position)) {
        // invalid input or no movement necessary
        return 0;  // no steps taken
    }

    wake_stepper();
    int direction;
    int net_steps_taken = 0;  // not really needed

    while (*current_position != desired_position) {
        direction = *current_position > desired_position ? 0 : 1;  // change this to whatever ends up being up and down on the blinds
        gpio_put(DIRECTION_PIN, direction);
        gpio_put(STEP_PIN, 0);
        sleep_us(30);  // give a healthy margin between signals
        gpio_put(STEP_PIN, 1);
        *current_position += 2*direction - 1;  // map [0, 1] to [-1, 1]
        net_steps_taken += 2*direction - 1;
    }

    sleep_stepper();
    return net_steps_taken;  // not really needed since the value in current_position is updated via the pointer
}
