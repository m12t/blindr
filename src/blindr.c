#include "blindr.h"


int main(void) {
    stdio_init_all();  // rbf - used for debugging
    printf("blindr initializing...!\n");

    stepper_init();
    toggle_init(&toggle_callback);
    gnss_preinit();

    while (1) {
        // keep the program alive indefinitely
        tight_loop_contents();
    }
}

void gnss_preinit(void) {
    gnss_init();
    while (gnss_running) {
        busy_wait_ms(50);
    }

    if (gnss_read_successful) {
        set_first_alarm();
    }
}

void set_first_alarm(void) {
    // this function is similar to set_next_alarm() except the way it finds the next
    // event is different (it compares times instad of going off `next_event`). It
    // also doesn't initialize the GNSS again as that was just done moments prior.
    printf("read must have succeeded...\n");
    int16_t year;
    int8_t month, day, dotw, hour, min;

    // todo: check for the timeout counter, cancel it if needed.

    rtc_get_datetime(&now);
    calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                           now.year, now.month, now.day, utc_offset, latitude, longitude);

    // it's currently neither a sunrise or a sunset
    // get the earliest event that is still > now
    if ((rise_hour > now.hour) || (rise_hour == now.hour && rise_minute > now.min)) {
        next_event = 1;  // the next valid event is a sunrise
        year = now.year;
        month = now.month;
        day = now.day;
        dotw = now.dotw,
        hour = rise_hour;
        min = rise_minute;
    } else if ((set_hour > now.hour) || (set_hour == now.hour && set_minute > now.min)) {
        next_event = 0;  // the next valid event is a sunset
        year = now.year;
        month = now.month;
        day = now.day;
        dotw = now.dotw,
        hour = set_hour;
        min = set_minute;
    } else {
        int16_t tomorrow_year=now.year;
        int8_t tomorrow_month=now.month, tomorrow_day=now.day;
        today_is_tomorrow(&tomorrow_year, &tomorrow_month, &tomorrow_day, NULL, utc_offset);
        int8_t tomorrow_dotw = get_dotw(tomorrow_year, tomorrow_month, tomorrow_day);
        // the time is after both the sunrise and sunset (or there were neither)...
        // get the sunrise time tomorrow. In the edge case there is none (ie. high
        // latitudes around the summer solstice), sleep until 0:00 tomorrow and try again.
        calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                                tomorrow_year, tomorrow_month, tomorrow_day, utc_offset, latitude, longitude);
        next_event = -1;  // invalid again
        year = tomorrow_year;
        month = tomorrow_month;
        day = tomorrow_day;
        dotw = tomorrow_dotw,
        hour = rise_hour;
        min = rise_minute;
    }

    datetime_t first_alarm = {
        .year  = year,
        .month = month,
        .day   = day,
        .dotw  = dotw,
        .hour  = hour,
        .min   = min,
        .sec   = 00
    };

    rtc_set_alarm(&first_alarm, &set_next_alarm);
    printf("setting the first alarm for: %d/%d/%d %d:%d:00\n", month, day, year, hour, min);  // rbf

}


void set_next_alarm(void) {
    // handle the current alarm/event and set the next one.

    int16_t year, tomorrow_year=now.year;
    int8_t month, day, dotw, hour, min, tomorrow_month=now.month, tomorrow_day=now.day;

    rtc_get_datetime(&now);
    calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                           now.year, now.month, now.day, utc_offset, latitude, longitude);

    printf("rise %d:%d, set %d:%d\n", rise_hour, rise_minute, set_hour, set_minute);

    // get tomorrow's day, month, and even year
    today_is_tomorrow(&tomorrow_year, &tomorrow_month, &tomorrow_day, NULL, utc_offset);
    int8_t tomorrow_dotw = get_dotw(tomorrow_year, tomorrow_month, tomorrow_day);

    if (next_event == 1) {
        step_to_position(&current_position, MIDPOINT, BOUNDARY_HIGH);  // open the blinds
        next_event = 0;  // next alarm will be sunset
        year = now.year;
        month = now.month;
        day = now.day;
        dotw = now.dotw,
        hour = set_hour;
        min = set_minute;
    } else if (next_event == 0) {
        step_to_position(&current_position, 0, BOUNDARY_HIGH);  // close the blinds
        // the next event is a sunrise and will occur tomorrow. Get tomorrow's solar events:
        calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                               tomorrow_year, tomorrow_month, tomorrow_day, utc_offset,
                               latitude, longitude);
        next_event = 1;  // next alarm will be sunrise
        year = tomorrow_year;
        month = tomorrow_month;
        day = tomorrow_day;
        dotw = tomorrow_dotw,
        hour = rise_hour;
        min = rise_minute;
    } else {
        // the time is after both the sunrise and sunset (or there were neither)...
        // get the sunrise time tomorrow. In the edge case there is none (ie. high
        // latitudes around the summer solstice), sleep until 0:00 tomorrow and try again.
        calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                                tomorrow_year, tomorrow_month, tomorrow_day, utc_offset, latitude, longitude);
        next_event = -1;  // invalid again
        year = tomorrow_year;
        month = tomorrow_month;
        day = tomorrow_day;
        dotw = tomorrow_dotw,
        hour = rise_hour;
        min = rise_minute;

    }

    gnss_init();  // get the latest accurate time from gnss data to recalibrate the onboard RTC
    while (gnss_running) {
        busy_wait_ms(50);
    }
    if (!gnss_read_successful) {
        consec_conn_failures += 1;

        next_event = -1;  // invalid
        year = tomorrow_year;
        month = tomorrow_month;
        day = tomorrow_day;
        dotw = tomorrow_dotw,
        hour = rise_hour;
        min = rise_minute;
    }

    datetime_t next_alarm = {
        .year  = year,
        .month = month,
        .day   = day,
        .dotw  = dotw,
        .hour  = hour,
        .min   = min,
        .sec   = 00
    };
    if (consec_conn_failures < MAX_CONSEC_CONN_FAILURES) {
        printf("local time is: %d/%d/%d %d:%d:00\n", now.month, now.day, now.year, now.hour, now.min);  // rbf
        printf("setting the next alarm for: %d/%d/%d %d:%d:00\n", month, day, year, hour, min);  // rbf
        rtc_set_alarm(&next_alarm, &set_next_alarm);
    }
}


void disable_all_interrupts_for(uint gpio) {
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);  // disable further interrupts during execution to mitigate mechanical switch bounce
}


void reenable_interrupts_for(uint gpio, int event) {
    gpio_set_irq_enabled_with_callback(gpio, event, true, &toggle_callback);
}


void set_automation_state(void) {
    // read the state of the toggle gpio pins
    busy_wait_ms(250);  // eliminate mechanical switch bounce
    if (gpio_get(GPIO_TOGGLE_DOWN_PIN) == 0 || gpio_get(GPIO_TOGGLE_UP_PIN) == 0) {
        disable_automation();
    } else {
        enable_automation();
    }
}


void normalize_boundaries(void) {
    // set low boundary to 0
    printf("normalizing...\n");  // rbf
    printf("current pos before: %d\n", current_position);  // rbf
    printf("--------------\n");  // rbf
    current_position += abs(BOUNDARY_LOW);
    BOUNDARY_HIGH += abs(BOUNDARY_LOW);
    BOUNDARY_LOW += abs(BOUNDARY_LOW);  // must do this *after* other shifts
    MIDPOINT = (BOUNDARY_HIGH - BOUNDARY_LOW) / 2;
    printf("new low boundary: %d\n", BOUNDARY_LOW);  // rbf
    printf("new high boundary: %d\n", BOUNDARY_HIGH);  // rbf
    printf("current pos after: %d\n", current_position);  // rbf
    printf("middy: %d\n", MIDPOINT);  // rbf

}


void find_boundary(uint gpio) {
    // wait for down toggle
    busy_wait_ms(100);  // combar switch bounce
    int stepped = 0;
    // printf("gpio: %d\n", gpio);
    uint dir = gpio == GPIO_TOGGLE_UP_PIN ? 0 : 1;
    // printf("dir: %d\n", dir);
    wake_stepper();
    while (gpio_get(gpio) == 0) {
        // while the switch is still pressed
        single_step(&current_position, dir, 1500);
        stepped = 1;
    }
    sleep_stepper();
    // update the respective boundary
    if (stepped && dir == 0) {
        BOUNDARY_HIGH = current_position;
        high_boundary_set = 1;
        // printf("Upper boundary found: %d\n", BOUNDARY_HIGH);  // rbf
    } else if (stepped && dir == 1) {
        BOUNDARY_LOW = current_position;
        low_boundary_set = 1;
        // printf("Lower boundary found: %d\n", BOUNDARY_LOW);  // rbf
    } else {
        // was just switch bounce, ignore it.
    }
    if (low_boundary_set && high_boundary_set) {
        normalize_boundaries();
    }
}


void toggle_callback(uint gpio, uint32_t event) {
    disable_all_interrupts_for(gpio);  // prevent further irqs while handling this one

    if (event == 0x04) {
        // Falling edge detected. disable all interrupts until done
        if (!low_boundary_set || !high_boundary_set) {
            find_boundary(gpio);
        } else {
            step_indefinitely(&current_position, BOUNDARY_HIGH, gpio);
        }
        // by now we're done with the falling action whether it's because
        // we reached a boundary of the blinds or because of a rising edge.
        // we need to know the current state to be able to configure the next
        // interrupt. if it's still 0, this means we never reset; listen for rising edge.
        // otherwise, we are reset (check that automation is enabled) and listen on another fall.
        if (gpio_get(gpio) == 0) {
            // the pin is still low, check that automation was disabled and reenable IRQs for
            // rising edge which will happen next
            // disable_automation();
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_RISE);
        } else {
            // the pin is high again, listen for another falling edge next
            // enable_automation();
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_FALL);
        }
    } else if (event == 0x08) {
        // Rising edge detected
        if (gpio_get(gpio) == 1) {
            // the pin is still high, listen for a falling edge next
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_FALL);
        }
        else {
            // rising edge detected yet the toggle is still down,
            // listen for another rising edge
            reenable_interrupts_for(gpio, GPIO_IRQ_EDGE_RISE);
        }
    } else {
        // both detected, ignore
        reenable_interrupts_for(gpio, 0x0C);
    }
    set_automation_state();
}


void disable_automation(void) {
    // this is an idempotent action: https://en.wikipedia.org/wiki/Idempotence
    automation_enabled = 0;
    // printf("automation state: %d\n", automation_enabled);  // rbf
}


void enable_automation(void) {
    // this is an idempotent action: https://en.wikipedia.org/wiki/Idempotence
    automation_enabled = 1;
    // printf("automation state: %d\n", automation_enabled);  // rbf
}
