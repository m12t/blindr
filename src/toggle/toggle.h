#include "pico/stdlib.h"
// #include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

int setup_toggle(gpio_irq_callback_t *callback);
void gpio_callback(uint gpio, uint32_t events);
