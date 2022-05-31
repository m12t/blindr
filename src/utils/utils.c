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


int get_dotw(int16_t year, int8_t month, int8_t day) {
    // https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week#Keith
    return (date += month < 3 ? year-- : year - 2, 23*month/9 + date + 4 + year/4- year/100 + year/400)%7;
}

void get_local_dt(int16_t year, int8_t month, int8_t day,
                  int8_t hour, int8_t min, int8_t sec, uint utc_offset) {
    // must call this before get_dotw()!

}