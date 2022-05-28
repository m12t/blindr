#include "../blindr.h"
#include "utils.h"  // needed??

// ZDA field format
// 0: $xxZDA
// 1: 220929.00 (utc time hhmmss.ss)
// 2: 21  (day, dd)
// 3: 05  (month, mm)
// 4: 2022  (year, yyyy)
// 5: 00 (Local time zone hours; always 00)
// 6: 00 (Local time zone minutes; always 00)

// void set_onboard_rtc(int year, int month, int day, int hour, int min, int sec) {
void set_onboard_rtc(int16_t year, int8_t month, int8_t day,
                     int8_t hour, int8_t min, int8_t sec) {
    /* receive parsed ZDA data and populate set the RTC to the current time */

    // construct the datetime_t struct and populte with the parameters data
    datetime_t dt = {
        .year = year,
        .month = month,
        .day = day,
        .hour = hour,
        .min = min,
        .sec = sec,
    };

    rtc_init();  // what's the behavior of calling this once the rtc is already initialized?
    rtc_set_datetime(&dt);
}