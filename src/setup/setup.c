#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

// ZDA field format
// 0: $xxZDA
// 1: 220929.00 (utc time hhmmss.ss)
// 2: 21  (day, dd)
// 3: 05  (month, mm)
// 4: 2022  (year, yyyy)
// 5: 00 (Local time zone hours; always 00)
// 6: 00 (Local time zone minutes; always 00)

// void set_onboard_rtc(int year, int month, int day, int hour, int min, int sec) {
void set_onboard_rt(char *zda_sentence) {
    /* receive parsed ZDA data and populate set the RTC to the current time */

    // construct the datetime_t struct and populte with the parameters data
    datetime_t dt = {
        .year = atoi(zda_sentence[4]),
        .month = atoi(zda_sentence[3]),
        .day = atoi(zda_sentence[2]),
        .hour = atoi(zda_sentence[1]),
        .min = atoi(zda_sentence[1]),
        .sec = atoi(zda_sentence[1]),
    };

    rtc_init();
    rtc_set_datetime(&dt);
}
