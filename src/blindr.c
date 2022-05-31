#include "blindr.h"
#include "gnss.h"
#include "utils.h"
#include "stepper.h"
#include "toggle.h"
#include "hardware/gpio.h"


const uint BOUNDARY_LOW = 0;
uint BOUNDARY_HIGH = 100, current_position = 50;  // stepper positioning. midpoint and num_steps can be calculated
int8_t sec, min, hour, day, month, utf_offset;
int16_t year;
double latitude=0.0, longitude=0.0;  // use atof() on these. float *should* be sufficient
int north, east, gnss_fix=0;  // 1 for North and East, 0 for South and West, respectively. GGA fix quality
int automation_enabled = 1;  // flag useful for whether or not to operate the blinds automatically.

int main(void) {
    // main program loop for blindr


    stdio_init_all();  // rbf - used for debugging

    // setup_uart();  // for connecting to GNSS module
    // stepper_init();
    setup_toggle(&toggle_callback);


    while (1) {
        // main program loop
        // tight_loop_contents();
    }
}


void disable_all_interrupts_for(uint gpio) {
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);  // disable further interrupts during execution to mitigate mechanical switch bounce
}


void reenable_interrupts_for(uint gpio, int event) {
    gpio_set_irq_enabled_with_callback(gpio, event, true, &toggle_callback);
}

void set_automation_status(void) {
    // read the state of the toggle gpio pins
    busy_wait_ms(250);  // eliminate mechanical switch bounce
    if (gpio_get(GPIO_TOGGLE_DOWN_PIN) == 0 || gpio_get(GPIO_TOGGLE_UP_PIN) == 0) {
        disable_automation();
    } else {
        enable_automation();
    }
}

void toggle_callback(uint gpio, uint32_t event) {
    disable_all_interrupts_for(gpio);  // prevent further irqs while handling this one
    // busy_wait_ms(100);

    if (event == 0x04) {
        // Falling edge detected. disable all interrupts until done
        step_indefinitely(&current_position, BOUNDARY_HIGH, gpio);
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
        // enable_automation();
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
    set_automation_status();
}


void disable_automation() {
    // todo: after debugging, make this a one-liner:  automation_enabled = 0;
    if (automation_enabled == 1) {
        automation_enabled = 0;
    }
    printf("automation status: %d\n", automation_enabled);  // rbf
}


void enable_automation() {
    if (automation_enabled == 0) {
        automation_enabled = 1;
    }
    printf("automation status: %d\n", automation_enabled);  // rbf
}
