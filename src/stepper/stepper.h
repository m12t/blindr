#ifndef STEPPER_H
#define STEPPER_H

#include <stdio.h>              // rbf
#include <stdlib.h>             // needed??
#include <stdint.h>             // needed for int8_t and int16_t
#include <ctype.h>              // needed???
#include "pico/stdlib.h"        // GPIO_OUT, etc.
#include "../toggle/toggle.h"   // for GPIO_TOGGLE_DOWN_PIN


#define SLEEP_PIN 13
#define STEP_PIN 14
#define DIRECTION_PIN 15

#endif


void stepper_init(void);
void wake_stepper();
void sleep_stepper();
void single_step(int *current_position, uint direction, uint sleep_time);
int step_indefinitely(int *current_position, uint BOUNDARY_HIGH, uint toggle_pin);
int step_to_position(int *current_position, uint desired_position, uint BOUNDARY_HIGH);
