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


int get_dotw(int16_t year, int8_t month, int8_t date) {
    // https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week#Keith
    return (date += month < 3 ? year-- : year - 2, 23*month/9 + date + 4 + year/4- year/100 + year/400)%7;
}

int is_leapyear(int16_t year) {
    // if a year is divisibly by 4 and it is NOT divisible by 100
    // or if it is divisible by 100, it's also divisible by 400
    if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
        return 1;
    return 0;
}

int last_day_of_month_on(uint8_t month, int16_t year) {
    // return the last valid date of a given month
    int months_with_31[] = {1, 3, 5, 7, 8, 10, 12};
    int months_with_30[] = {4, 6, 9, 11};
    switch (month) {
        case 1:
            return 31;
        case 2:
            if (is_leapyear(year))
                return 29;
            return 28;
        case 3:
            return 31;
        case 4:
            return 30;
        case 5:
            return 31;
        case 6:
            return 30;
        case 7:
            return 31;
        case 8:
            return 31;
        case 9:
            return 30;
        case 10:
            return 31;
        case 11:
            return 30;
        case 12:
            return 31;

    }
}

void today_is_yesterday(int16_t *year, int8_t *month, int8_t *date, int8_t *hour, uint utc_offset) {
    *hour = 24 - abs(hour + utc_offset);
    if (date == 1) {
        // it's the first of the month, yesterday was the last of the previous month
        if (month == 1) {
            // it's January 1st and yesterday was a differrent year... we need to go back a year
            *year -= 1;
            *month = 12;
        } else {
            // month isn't january, just subtract 1
            *month -= 1;
        }
        // need to go back a month
        *date = last_day_of_month_on(month, year);
    } else {
        // date is a regular month day, simply subtract 1
        *date -= 1;
    }
    *hour = 24 - (hour + utc_offset);
}

void today_is_tomorrow(int16_t *year, int8_t *month, int8_t *date, int8_t *hour, uint utc_offset) {
    if (date == last_day_of_month_on(month, year)) {
        if (month == 12) {
            *year += 1;
            *month = 1;
            *date = 1;
        } else {
            *month += 1;
            *date = 1;
        }
    } else {
        // date is a regular month day, simply subtract 1
        *date += 1;
    }
    *hour = (hour + utc_offset) - 24  // `(hour + utc_offset) % 24` would also work
}

void get_local_dt(int16_t *year, int8_t *month, int8_t *date, int8_t *hour, uint utc_offset) {
    // must call this before get_dotw()!
    // if adding the utc offset results in a different date, change accordingly
    if (hour + utc_offset < 0) {
        today_is_yesterday(year, month, date, hour, utc_offset);  // these are already pointers, no need to use `*` again.
    } else if (hour + utc_offset > 24) {
        today_is_tomorrow(year, month, date, hour, utc_offset);
    } else {
        // no complications will arise, simply add the offset
        *hour += utc_offset;
    }
}
