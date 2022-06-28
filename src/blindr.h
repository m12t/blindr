#ifndef BLINDR_H
#define BLINDR_H

#include "gnss.h"
#include "utils.h"
#include "stepper.h"
#include "toggle.h"
#include "solar.h"

#define MAX_CONSEC_CONN_FAILURES 6


void alarm_callback(void);
void toggle_callback(uint gpio, uint32_t event);
void actuate(int solar_event);
void read_actuate_alarm_sequence(int *solar_event, double *latitude, double *longitude,
                                 int *north, int *east, int *utc_offset, uint *baud_rate,
                                 uint new_baud, uint *gnss_configured, uint *config_gnss,
                                 uint *consec_conn_failures, uint *data_found, uint *time_only);
void disable_all_interrupts_for(uint gpio);
void reenable_interrupts_for(uint gpio, int event);
void set_automation_state(void);
void normalize_boundaries(void);
void find_boundary(uint gpio);
void enable_automation(void);
void disable_automation(void);
void dance(uint sleep_time);
void set_rise_times(uint consistent_rise, int8_t *hour, int8_t *min,
                    int8_t consistent_rise_hour, int8_t consistent_rise_minute,
                    int8_t rise_hour, int8_t rise_minute);

#endif