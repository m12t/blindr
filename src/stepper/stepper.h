#include <stdlib.h>
#include <stdint.h>  // needed for int8_t and int16_t
#include <ctype.h>
#include "pico/stdlib.h"  // GPIO_OUT, etc.

#define SLEEP_PIN 13
#define STEP_PIN 14
#define DIRECTION_PIN 15

void stepper_init(void);
void wake_stepper();
void sleep_stepper();
void single_step(uint *current_position, uint direction, uint sleep_time);
int step_indefinitely(uint *current_position, uint BOUNDARY_HIGH, uint toggle_pin);
int step_to_position(uint *current_position, uint desired_position, uint BOUNDARY_HIGH);
