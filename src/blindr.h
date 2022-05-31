#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>  // needed for int8_t and int16_t
#include <ctype.h>
#include <string.h>
#include "pico/stdlib.h"

// stepper configs
#define SLEEP_PIN 13
#define STEP_PIN 14
#define DIRECTION_PIN 15
// toggle configs
#define GPIO_TOGGLE_DOWN_PIN 18
#define GPIO_TOGGLE_UP_PIN 19


void disable_all_interrupts_for(uint gpio);
void reenable_interrupts_for(uint gpio, int event);
void toggle_callback(uint gpio, uint32_t event);
void disable_automation();
void enable_automation();
void set_automation_status(void);
