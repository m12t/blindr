#ifndef BLINDR_H
#define BLINDR_H

#include "gnss.h"
#include "utils.h"
#include "stepper.h"
#include "toggle.h"
#include "solar.h"

// main program global vars, some of which are accessed and modified elsewhere
uint low_boundary_set=0, high_boundary_set=0;
int BOUNDARY_LOW=0, BOUNDARY_HIGH=0, MIDPOINT=0, current_position=0;  // stepper positioning. midpoint and num_steps can be calculated
int8_t sec, min, hour, day, month, utc_offset;
int16_t year;
double latitude=0.0, longitude=0.0;  // use atof() on these. float *should* be sufficient
int north, east, gnss_fix=0;  // 1 for North and East, 0 for South and West, respectively. GGA fix quality
int automation_enabled=1;  // flag useful for whether or not to operate the blinds automatically.

#endif

// prototypes
void disable_all_interrupts_for(uint gpio);
void reenable_interrupts_for(uint gpio, int event);
void set_automation_state(void);
void normalize_boundaries(void);
void find_boundary(uint gpio);
void toggle_callback(uint gpio, uint32_t event);
void disable_automation(void);
void enable_automation(void);
void daily_loop(void);