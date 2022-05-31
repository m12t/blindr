#ifndef UTILS_H
#define UTILS_H

#include "pico/stdlib.h"  // needed??
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

#endif

void set_onboard_rtc(int16_t year, int8_t month, int8_t day, int8_t dotw,
                     int8_t hour, int8_t min, int8_t sec);
int get_dotw(int16_t year, int8_t month, int8_t day, int8_t dotw,
             int8_t hour, int8_t min, int8_t sec);
void get_local_dt(int16_t year, int8_t month, int8_t day,
                  int8_t hour, int8_t min, int8_t sec, uint utc_offset)
