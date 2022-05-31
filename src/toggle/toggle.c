#include "toggle.h"
#include <stdio.h>  // rbf

int setup_toggle(void *callback) {
    printf("running toggle\n");

    gpio_pull_up(18);
    gpio_pull_up(19);

    gpio_set_irq_enabled_with_callback(18, GPIO_IRQ_EDGE_FALL, true, callback);
    gpio_set_irq_enabled_with_callback(19, GPIO_IRQ_EDGE_FALL, true, callback);

    // gpio_set_irq_enabled_with_callback(18, GPIO_IRQ_EDGE_RISE, true, {enable_automation_callback})  // re-enable automatioun
    // gpio_set_irq_enabled_with_callback(19, GPIO_IRQ_EDGE_RISE, true, {enable_automation_callback})  // re-enable automatioun

    return 0;
}
