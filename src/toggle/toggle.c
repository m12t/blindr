#include "toggle.h"
#include <stdio.h>  // rbf

int setup_toggle(gpio_irq_callback_t *callback) {
    printf("runnign toggle\n");

    gpio_pull_up(18);
    gpio_pull_up(19);

    gpio_set_irq_enabled_with_callback(18, GPIO_IRQ_EDGE_FALL, true, callback);
    gpio_set_irq_enabled_with_callback(19, GPIO_IRQ_EDGE_FALL, true, callback);

    return 0;
}
