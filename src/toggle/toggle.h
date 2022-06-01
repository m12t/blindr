#ifndef TOGGLE_H
#define TOGGLE_H

#include "hardware/gpio.h"

#define GPIO_TOGGLE_DOWN_PIN 18
#define GPIO_TOGGLE_UP_PIN 19

#endif

int toggle_init(void *toggle_callback);
