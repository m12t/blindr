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


void disable_interrupts_for(uint gpio, int event) {
    gpio_set_irq_enabled(gpio, event, false);  // disable further interrupts during execution to mitigate mechanical switch bounce
}


void reenable_interrupts_for(uint gpio, int event) {
    gpio_set_irq_enabled_with_callback(gpio, event, true, &toggle_callback);
}


void toggle_callback(uint gpio, uint32_t event) {
    disable_interrupts_for(gpio, 0x0C);  // prevent further irqs while handling this one

    if (event == 0x04) {
        // Falling edge detected. disable all interrupts until done
        step_indefinitely(&current_position, BOUNDARY_HIGH, gpio);
        // by now we're done with the falling action whether it's because
        // we reached a boundary of the blinds or because of a rising edge.
        // we need to know the current state to be able to configure the next
        // interrupt. if it's still 0, this means we never reset; listen for rising edge.
        // otherwise, we are reset (check that automation is enabled) and listen on another fall.
        busy_wait_ms(50);
        if (gpio_get(gpio) == 0) {
            // the pin is still low, check that automation was disabled and reenable IRQs for
            // rising edge which will happen next
            disable_automation();
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_RISE);
        } else {
            // the pin is high again, listen for another falling edge next
            enable_automation();
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_FALL);
        }
        printf("handled falling edge IRQ\n");
    } else if (event == 0x08) {
        // Rising edge detected
        enable_automation();
        busy_wait_ms(50);  // to be safe
        if (gpio_get(gpio) == 1) {
            // the pin is still high, listen for a falling edge next
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_FALL);
        }
        else {
            // rising edge detected yet the toggle is still down,
            // listen for another rising edge
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_RISE);
        }
        printf("handled rising edge IRQ\n");
    } else {
        // both detected, ignore
        printf("massive error!\n");
        reenable_interrupts_for(gpio, 0x0C);
        busy_wait_ms(10);
    }
}



void disable_automation() {
    // todo: after debugging, make this a one-liner:  automation_enabled = 0;
    if (automation_enabled == 1) {
        printf("disabling automation...\n");  // rbf
        automation_enabled = 0;
    }
    printf("automation status: %d\n", automation_enabled);
}


void enable_automation() {
    if (automation_enabled == 0) {
        printf("enabling automation...\n");  // rbf
        automation_enabled = 1;
    }
    printf("automation status: %d\n", automation_enabled);
}