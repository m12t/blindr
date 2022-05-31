#include "toggle.h"
#include <stdio.h>  // rbf

int setup_toggle(void *toggle_callback) {

    // initialize the toggle switch pins tied high and throwing the switch pulls them to ground
    gpio_pull_up(18);  // change the pin number as needed
    gpio_pull_up(19);

    // default to enabling listening for falling edges, and only listen to rising edges when
    // a falling edge has occurred. This happens in `blindr.c`
    gpio_set_irq_enabled_with_callback(18, GPIO_IRQ_EDGE_FALL, true, toggle_callback);
    gpio_set_irq_enabled_with_callback(19, GPIO_IRQ_EDGE_FALL, true, toggle_callback);
    gpio_set_irq_enabled_with_callback(18, GPIO_IRQ_EDGE_RISE, false, toggle_callback);
    gpio_set_irq_enabled_with_callback(19, GPIO_IRQ_EDGE_RISE, false, toggle_callback);


    return 0;
}
