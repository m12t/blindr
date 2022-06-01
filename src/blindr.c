#include "gnss.h"
#include "utils.h"
#include "stepper.h"
#include "toggle.h"
// #include "hardware/gpio.h"  // is there a cleaner way to include this??

#include <stdio.h>  // rbf
// #include <stdint.h>  // needed for int8_t and int16_t
// #include <ctype.h>  // needed?
// #include <string.h>  // needed?

// main program global vars, some of which are accessed and modified elsewhere
uint low_boundary_set=0, high_boundary_set=0;
int BOUNDARY_LOW=0, BOUNDARY_HIGH=0, MIDPOINT=0, current_position=0;  // stepper positioning. midpoint and num_steps can be calculated
int8_t sec, min, hour, day, month, utc_offset;
int16_t year;
double latitude=0.0, longitude=0.0;  // use atof() on these. float *should* be sufficient
int north, east, gnss_fix=0;  // 1 for North and East, 0 for South and West, respectively. GGA fix quality
int automation_enabled=1;  // flag useful for whether or not to operate the blinds automatically.


// prototypes for this file
void disable_all_interrupts_for(uint gpio);
void reenable_interrupts_for(uint gpio, int event);
void set_automation_state(void);
void normalize_boundaries(void);
void find_boundary(uint gpio);
void toggle_callback(uint gpio, uint32_t event);
void disable_automation(void);
void enable_automation(void);
void daily_loop(void);


int main(void) {
    // main program loop for blindr

    stdio_init_all();  // rbf - used for debugging

    // setup_uart();  // for connecting to GNSS module
    stepper_init();
    toggle_init(&toggle_callback);
    gnss_init();


    while (1) {
        printf("%d/%d/%d %d:%d:%d\n", month, day, year, hour+utc_offset, min, sec);  // rbf
        printf("latitude:  %f, longitude: %f\n", latitude, longitude);  // rbf
        printf("gnss_fix: %d\n", gnss_fix);  // rbf
    }
}


void disable_all_interrupts_for(uint gpio) {
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);  // disable further interrupts during execution to mitigate mechanical switch bounce
}


void reenable_interrupts_for(uint gpio, int event) {
    gpio_set_irq_enabled_with_callback(gpio, event, true, &toggle_callback);
}

void set_automation_state(void) {
    // read the state of the toggle gpio pins
    busy_wait_ms(250);  // eliminate mechanical switch bounce
    if (gpio_get(GPIO_TOGGLE_DOWN_PIN) == 0 || gpio_get(GPIO_TOGGLE_UP_PIN) == 0) {
        disable_automation();
    } else {
        enable_automation();
    }
}

void normalize_boundaries(void) {
    // set low boundary to 0
    printf("normalizing...\n");  // rbf
    printf("current pos before: %d\n", current_position);  // rbf
    current_position += abs(BOUNDARY_LOW);
    BOUNDARY_HIGH += abs(BOUNDARY_LOW);
    BOUNDARY_LOW += abs(BOUNDARY_LOW);  // must do this *after* other shifts
    MIDPOINT = (BOUNDARY_HIGH - BOUNDARY_LOW) / 2;
    printf("new low boundary: %d\n", BOUNDARY_LOW);  // rbf
    printf("new high boundary: %d\n", BOUNDARY_HIGH);  // rbf
    printf("current pos after: %d\n", current_position);  // rbf
    printf("middy: %d\n", MIDPOINT);  // rbf

}

void find_boundary(uint gpio) {
    // todo: find BOTH boundaries, then
    // wait for down toggle
    uint dir = gpio == GPIO_TOGGLE_DOWN_PIN ? 0 : 1;
    while (gpio_get(gpio) == 0) {
        // while the switch is still pressed
        single_step(&current_position, dir, 30);
    }
    // update the respective boundary
    if (gpio == GPIO_TOGGLE_UP_PIN) {
        BOUNDARY_HIGH = current_position;
        high_boundary_set = 1;
        printf("Upper boundary found: %d\n", BOUNDARY_HIGH);  // rbf
    } else {
        BOUNDARY_LOW = current_position;
        low_boundary_set = 1;
        printf("Lower boundary found: %d\n", BOUNDARY_LOW);  // rbf
    }
    if (low_boundary_set && high_boundary_set) {
        normalize_boundaries();
    }
}

void toggle_callback(uint gpio, uint32_t event) {
    disable_all_interrupts_for(gpio);  // prevent further irqs while handling this one
    // busy_wait_ms(100);

    if (event == 0x04) {
        // Falling edge detected. disable all interrupts until done
        if (!low_boundary_set || !high_boundary_set) {
            find_boundary(gpio);
        } else {
            step_indefinitely(&current_position, BOUNDARY_HIGH, gpio);
        }
        // by now we're done with the falling action whether it's because
        // we reached a boundary of the blinds or because of a rising edge.
        // we need to know the current state to be able to configure the next
        // interrupt. if it's still 0, this means we never reset; listen for rising edge.
        // otherwise, we are reset (check that automation is enabled) and listen on another fall.
        if (gpio_get(gpio) == 0) {
            // the pin is still low, check that automation was disabled and reenable IRQs for
            // rising edge which will happen next
            // disable_automation();
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_RISE);
        } else {
            // the pin is high again, listen for another falling edge next
            // enable_automation();
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_FALL);
        }
    } else if (event == 0x08) {
        // Rising edge detected
        if (gpio_get(gpio) == 1) {
            // the pin is still high, listen for a falling edge next
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_FALL);
        }
        else {
            // rising edge detected yet the toggle is still down,
            // listen for another rising edge
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_RISE);
        }
    } else {
        // both detected, ignore
        reenable_interrupts_for(gpio, 0x0C);
    }
    set_automation_state();
}


void disable_automation(void) {
    // todo: after debugging, make this a one-liner:  automation_enabled = 0;
    // this is an idempotent action: https://en.wikipedia.org/wiki/Idempotence
    automation_enabled = 0;
    printf("automation state: %d\n", automation_enabled);  // rbf
}


void enable_automation(void) {
    automation_enabled = 1;
    printf("automation state: %d\n", automation_enabled);  // rbf
}

void daily_loop(void);