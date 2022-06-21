#include "stepper.h"


void stepper_init(void) {
    // printf("Initializing Stepper\n");
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
    busy_wait_ms(2);  // per instructions, leave >1ms after sleep before step input
}


void sleep_stepper() {
    // save power and reduce heat/stress by shutting down the stepper when it isn't
    // needed since stepper motors pull full current even when stationary.
    busy_wait_ms(250);  // to combat bounce in position of the blinds when moving them really fast.
    gpio_put(SLEEP_PIN, 0);
    busy_wait_ms(2);  // purely a safety margin
}


void single_step(int *current_position, uint direction, uint sleep_time) {
    // create a single rising edge to trigger a single
    // step and update the current position accordingly
    if (direction == 0 || direction == 1) {
        gpio_put(DIRECTION_PIN, direction);
        // it's a valid direction, take a step
        gpio_put(STEP_PIN, 0);
        busy_wait_us(sleep_time);  // give a healthy margin between signals - busy wait needed during interrupt
        gpio_put(STEP_PIN, 1);
        *current_position += -(2*direction - 1);  // map [0, 1] to [-1, 1] and flip the sign since 0 is up
        // printf("single step -- dir: %d, pos: %d\n", direction, *current_position);  // rbf
    }
}


int step_indefinitely(int *current_position, uint BOUNDARY_HIGH, uint toggle_pin) {
    // alternate to single_step() where this function reads the toggle switch value
    // directly and also modifies the current position automatically.
    // NOTE: this function can only be called once the boundary are already set
    //       since the startup routine automatically reads the initial toggle inputs
    //       as setting the lower and upper boundaries (in no particular order)
    // printf("stepping indefinitely...\n");
    wake_stepper();
    uint direction = toggle_pin == GPIO_TOGGLE_UP_PIN ? 0 : 1;  // change to whatever pin ends up being used...
    while ((gpio_get(toggle_pin) == 0) &&
           (*current_position <= BOUNDARY_HIGH) &&
           (*current_position >= 0)) {
        // the pin is still pulled high and the position is within the range, steep
        if ((*current_position >= BOUNDARY_HIGH && direction == 0) ||
            (*current_position <= 0 && direction == 1)) {
            // the current position is at a boundary and the direction is trying
            // to move it out of bounds. Disallow and exit the loop.
            break;
        } else {
            // a valid step can be taken, do so:
            single_step(current_position, direction, 1500);
        }
    }
    sleep_stepper();
}


int step_to_position(int *current_position, uint desired_position, uint BOUNDARY_HIGH) {
    // receive the current position, desired_position, and high boundary (low boundary is normalized to 0)
    // and step to the desired destination, updating the current position pointer along the way
    // return 0 for no action taken, 1 for step(s) were performed. It's not necessary to return the number
    // or net number of steps taken as *current_position is updated each step here.

    // check that the inputs are valid
    if ((desired_position < 0) ||
        (desired_position > BOUNDARY_HIGH) ||
        (desired_position == *current_position)) {
        // invalid input or no movement necessary
        return 1;  // no steps taken
    }

    wake_stepper();
    uint direction;

    while (*current_position != desired_position) {
        // printf("stepping to position\n");
        direction = *current_position > desired_position ? 1 : 0;  // change this to whatever ends up being up and down on the blinds
        single_step(current_position, direction, 250);
    }

    sleep_stepper();
    return 0;
}
