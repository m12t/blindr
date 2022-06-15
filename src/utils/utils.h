#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>           // rbf
#include <stdint.h>          // for int8_t and int16_t
#include <stdlib.h>          // for abs()
#include "hardware/rtc.h"    // for initializing and setting the RTC
#include "pico/util/datetime.h"

#endif

void set_onboard_rtc(int16_t year, int8_t month, int8_t day, int8_t hour, int8_t min, int8_t sec);
int8_t get_dotw(int16_t year, int8_t month, int8_t day);
int is_leapyear(int16_t year);
uint last_day_of_month_on(int8_t month, int16_t year);
void today_is_yesterday(int16_t *year, int8_t *month, int8_t *day, int8_t *hour, int utc_offset);
void today_is_tomorrow(int16_t *year, int8_t *month, int8_t *day, int8_t *hour, int utc_offset);
void localize_datetime(int16_t *year, int8_t *month, int8_t *day, int8_t *hour, int utc_offset);
void utils_get_rtc_datetime(datetime_t *dt);
void utils_set_rtc_alarm(datetime_t *alarm, rtc_callback_t callback);
