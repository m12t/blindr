#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"


void set_onboard_rtc(int year, int month, int day, int hour, int min, int sec) {
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

    rtc_init();
    rtc_set_datetime(&dt);
}
