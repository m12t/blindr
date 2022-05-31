#include "pico/stdlib.h"
// #include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "../blindr.h"

int setup_toggle(void *toggle_callback);
void gpio_callback(uint gpio, uint32_t events);
