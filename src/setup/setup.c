#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

// setup the RTC on pico

int main(void) {
    stdio_init_all();
    printf("init`ng RTC\n");

    char datetime_buffer[256];
    char *datetime_str = &datetime_buffer[0];  // address of first element in buffer

    datetime_t dt = {
        .year = 2022,
        .month = 05,
        .day = 22,
        .dotw = 0,
        .hour = 17,
        .min = 45,
        .sec = 00
    };

    rtc_init();  // initialize the onboard RTC
    rtc_set_datetime(&dt);

    while (1) {
        rtc_get_datetime(&dt);
        datetime_to_str(datetime_str, sizeof(datetime_buffer), &dt);
        printf("\r%s\t", datetime_str);
        sleep_ms(100);
    }
    return 0;
}