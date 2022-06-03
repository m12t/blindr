#include "blindr.h"



int main(void) {
    // main program loop for blindr

    stdio_init_all();  // rbf - used for debugging
    printf("blindr initializing...!\n");

    stepper_init();
    toggle_init(&toggle_callback);
    gnss_init();

    // wait for the rtc to come online and then set the next alarm
    while (!rtc_running())
        sleep_ms(1000);
    set_next_alarm();

    while (1) {
        // just keep the program alive
        tight_loop_contents();
    }
}


void set_next_alarm(void) {
    rtc_get_datetime(&now);
    calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                           now.year, now.month, now.day, utc_offset, latitude, longitude);
    int16_t next_year, tomorrow_year=now.year;
    int8_t next_month, next_day, next_dotw, next_hour, next_min, tomorrow_month = now.month, tomorrow_day = now.day;

    // get tomorrow's day, month, and even year
    today_is_tomorrow(&tomorrow_year, &tomorrow_month, &tomorrow_day, NULL, utc_offset);
    int8_t tomorrow_dotw = get_dotw(tomorrow_year, tomorrow_month, tomorrow_day);

    if (rise_hour == now.hour && rise_minute == now.min) {
        step_to_position(&current_position, MIDPOINT, BOUNDARY_HIGH);  // open the blinds
        // next alarm will be sunset
        next_year = now.year;
        next_month = now.month;
        next_day = now.day;
        next_dotw = now.dotw,
        next_hour = set_hour;
        next_min = set_minute;
    } else if (set_hour == now.hour && set_minute == now.min) {
        step_to_position(&current_position, 0, BOUNDARY_HIGH);  // close the blinds
        // the next event is a sunrise and will occur on the next calendar day. get that day.
        calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                               tomorrow_year, tomorrow_month, tomorrow_day, utc_offset,
                               latitude, longitude);
        // next alarm will be sunrise
        next_year = tomorrow_year;
        next_month = tomorrow_month;
        next_day = tomorrow_day;
        next_dotw = tomorrow_dotw,
        next_hour = rise_hour;
        next_min = rise_minute;
    } else {
        // it's currently neither a sunrise or a sunset
        // get the earliest hour that is still >= now.hour
        if (rise_hour >= now.hour && rise_minute >= now.min) {
            // the next valid event is a sunrise
            next_year = now.year;
            next_month = now.month;
            next_day = now.day;
            next_dotw = now.dotw,
            next_hour = rise_hour;
            next_min = rise_minute;
            
        } else if (set_hour >= now.hour && set_minute >= now.min) {
            // the next valid event is a sunset
            next_year = now.year;
            next_month = now.month;
            next_day = now.day;
            next_dotw = now.dotw,
            next_hour = set_hour;
            next_min = set_minute;
        } else {
            // the time is after both the sunrise and sunset (or there were nether)...
            // get the sunrise time tomorrow. if that's still not a thing, sleep for 1 minute
            calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                                   tomorrow_year, tomorrow_month, tomorrow_day, utc_offset, latitude, longitude);
            next_year = tomorrow_year;
            next_month = tomorrow_month;
            next_day = tomorrow_day;
            next_dotw = tomorrow_dotw,
            next_hour = rise_hour;
            next_min = rise_minute;
        }
    }

    datetime_t next_alarm = {
        .year  = next_year,
        .month = next_month,
        .day   = next_day,
        .dotw  = next_dotw,
        .hour  = next_hour,
        .min   = next_min,
        .sec   = 00
    };

    // printf("setting the next alarm for: %d/%d/%d %d:%d:00\n", next_month, next_day, next_year, next_hour, next_min);  // rbf
    rtc_set_alarm(&next_alarm, &set_next_alarm);
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
    // todo: find BOTH boundaries, then
    // wait for down toggle
    busy_wait_ms(100);  // combar switch bounce
    int stepped = 0;
    // printf("gpio: %d\n", gpio);
    uint dir = gpio == GPIO_TOGGLE_UP_PIN ? 0 : 1;
    // printf("dir: %d\n", dir);
    wake_stepper();
    while (gpio_get(gpio) == 0) {
        // while the switch is still pressed
        single_step(&current_position, dir, 1000);
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
