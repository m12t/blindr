#ifndef UTILS_H
#define UTILS_H

#include "pico/stdlib.h"  // needed??
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

#endif

void set_onboard_rtc(int16_t year, int8_t month, int8_t day, int8_t hour, int8_t min, int8_t sec);
int8_t get_dotw(int16_t year, int8_t month, int8_t day);
int is_leapyear(int16_t year);
int last_day_of_month_on(uint8_t month, int16_t year);
void today_is_yesterday(int16_t *year, int8_t *month, int8_t *day, int8_t *hour, uint utc_offset);
void today_is_tomorrow(int16_t *year, int8_t *month, int8_t *day, int8_t *hour, uint utc_offset);
void localize_datetime(int16_t *year, int8_t *month, int8_t *day, int8_t *hour, uint utc_offset);
