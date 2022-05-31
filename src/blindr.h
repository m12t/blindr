#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>  // needed for int8_t and int16_t
#include <ctype.h>
#include <string.h>
#include "pico/stdlib.h"

// uart config
#define UART_ID uart1
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 4
#define UART_RX_PIN 5
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
