#include "../blindr.h"
#include "utils.h"
#include <math.h>


void set_onboard_rtc(int16_t year, int8_t month, int8_t day, int8_t dotw,
                     int8_t hour, int8_t min, int8_t sec) {
    /* receive datetime data and set the RTC to the current time */

    // construct the datetime_t struct and populte it data
    datetime_t dt = {
        .year = year,
        .month = month,
        .day = day,
        .dotw = dotw,
        .hour = hour,
        .min = min,
        .sec = sec,
    };

    rtc_init();  // what's the behavior of calling this once the rtc is already initialized?
    rtc_set_datetime(&dt);
}


int get_dotw(int16_t year, int8_t month, int8_t day, int8_t dotw,
             int8_t hour, int8_t min, int8_t sec) {
    // https://cs.uwaterloo.ca/~alopez-o/math-faq/node73.html
    uint C = year / 100;  // Century: 1987 -> 19
    uint Y = year - 100*C;  // 2 digit year: 1987 -> 87

    return (uint)(day + floor(2.6*month - 0.2) - 2*C + Y + floor(Y/4) + floor(C/4)) % 7;
}