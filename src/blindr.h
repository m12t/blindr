#ifndef BLINDR_H
#define BLINDR_H

#include "gnss.h"
#include "utils.h"
#include "stepper.h"
#include "toggle.h"
#include "solar.h"

#include "pico/multicore.h"

#define MAX_CONSEC_CONN_FAILURES 3


void disable_all_interrupts_for(uint gpio);
void reenable_interrupts_for(uint gpio, int event);
void set_automation_state(void);
void normalize_boundaries(void);
void find_boundary(uint gpio);
void toggle_callback(uint gpio, uint32_t event);
void disable_automation(void);
void enable_automation(void);
int set_next_alarm(void);
void set_first_alarm(void);
void core1_entry(void);

#endif