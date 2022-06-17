#include "blindr.h"


uint alarm_detected=1, toggle_detected=0;  // flag that gets set by the alarm callback to cause a runthrough of the gnss read, blind actuation, and next alarm setting sequence.
uint toggle_gpio=-1;
uint32_t toggle_event=-1;  // variables supplied by the toggle IRQ callback

int main(void) {
    // -------------------------------- initialize main program vars --------------------------------
    // blind variables
    uint low_boundary_set=0, high_boundary_set=0;  // flag for whether the respective boundary is set or not
    int boundary_low=0, boundary_high=0, current_position=0;  // stepper positioning
    int event = -1;       // flag for what the next solar event is: 0=sunset, 1=sunrise, -1=NULL/Invalid
    uint automation_enabled = 1;  // flag useful for whether or not to operate the blinds automatically.

    // gnss variables
    double latitude=0.0, longitude=0.0;
    int north=-1, east=-1;
    int utc_offset;
    uint baud_rate=9600, new_baud=115200, gnss_configured=0, consec_conn_failures=0;

    // -------------------------------- initialize the stepper and toggle switch --------------------------------
    stdio_init_all();  // rbf - used for debugging
    printf("blindr initializing...\n");
    stepper_init();
    toggle_init(&toggle_callback);

    // -------------------------------- run the main program loop indefinitely --------------------------------
    while (1) {
        // keep the program alive indefinitely while listening for interrupts and handling them accordingly.
        if (toggle_detected) {
            // * check this first so that toggle switch gets priority over scheduled alarms since
            //   the timing on alarms can wait for human input to finish should they collide
            handle_toggle(&low_boundary_set, &high_boundary_set, &boundary_low,
                          &boundary_high, &current_position, &automation_enabled,
                          toggle_gpio, toggle_event);
            toggle_detected = 0;
            toggle_gpio = -1;
            toggle_event = -1;

        } else if (alarm_detected) {
            read_actuate_alarm_sequence(&boundary_low, &boundary_high, &current_position, &event,
                                        &automation_enabled, &latitude, &longitude, &north, &east,
                                        &utc_offset, &baud_rate, new_baud, &gnss_configured,
                                        &consec_conn_failures);
            alarm_detected = 0;

        } else {
            sleep_ms(250);  // doubles as the requisite switch bounce delay in the event of toggle_detected=1
        }
    }
}


void alarm_callback(void) {
    printf("alarm detected in callback\n");
    alarm_detected = 1;
}


void toggle_callback(uint gpio, uint32_t event) {  // TODO: build out the gpio and event variable handling...
    // NOTE: this may cause a delay of up to 1000ms before actuaing the blinds which is undesirable...
    //       reduce the main while sleep to 250ms which is what the switch bounce delay was... just remove that and
    //       change the main sleep to 250ms.additionally, this setup is needed for the vars from `main()`
    //       to be sent via pointers (automation_enabled, boundaries, etc.)
    toggle_detected = 1;
    toggle_gpio = gpio;
    toggle_event = event;
}


void read_actuate_alarm_sequence(int *boundary_low, int *boundary_high, int *current_position,
                                 int *event, uint *automation_enabled, double *latitude,
                                 double *longitude, int *north, int *east, int *utc_offset,
                                 uint *baud_rate, uint new_baud, uint *gnss_configured,
                                 uint *consec_conn_failures) {
    // handle the current alarm/event and set the next one.

    datetime_t now = { 0 };    // blank datetime struct to be pupulated by get_rtc_datetime(&now) calls
    rtc_get_datetime(&now);    // grab the year, month, day for the solar calculations below

    int16_t year = now.year;
    int8_t month = now.month;
    int8_t day = now.day;
    int8_t dotw = now.dotw;
    int8_t hour = now.hour;
    int8_t min = now.min;
    int8_t sec = 0;
    int8_t rise_hour, rise_minute, set_hour, set_minute;  // solar event times that are populated by `calculate_solar_events()`
    uint config_gnss = 0;
    uint time_only = 1;
    uint gnss_read_successful = 0;

    if (*event == 1) {  // it's a sunrise right now
        if (automation_enabled) {  // only move the blinds if automation is enabled
            step_to_position(current_position, (uint)(*boundary_high - *boundary_low) / 2, *boundary_high);  // open the blinds
        }
        calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                               year, month, day, *utc_offset, *latitude, *longitude);
        *event = 0;  // next alarm will be sunset
        hour = set_hour;
        min = set_minute;
    } else {
        // * the next event is a sunrise (or invalid) and will occur tomorrow. Get tomorrow's solar events:
        //   - initialize tomorrow's variables to today and send the pointers to today_is_tomorrow() which will
        //     update them as needed. Use tomorrow's date to set an alarm for either the sunrise time or 00:00
        today_is_tomorrow(&year, &month, &day, NULL, *utc_offset);  // modify the day, month (if applicable), year (if applicable) to tomorrow's dd/mm/yyyy
        dotw = get_dotw(year, month, day);  // get tomorrow's day of the week using the freshly changed values for year, month, day

        calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                               year, month, day, *utc_offset, *latitude, *longitude);

        if (*event == 0) {  // it's a sunset right now
            if (automation_enabled) {
                step_to_position(current_position, 0, *boundary_high);  // close the blinds
            }
            *event = 1;  // next alarm will be sunrise
        } else {
            // the time is after both the sunrise and sunset (or there were neither)...
            // get the sunrise time tomorrow. In the edge case there is none (ie. high
            // latitudes around the summer solstice), sleep until 0:00 tomorrow and try again.
            if (rise_hour || rise_minute) {
                *event = 1;
            } else {
                *event = -1;  // invalid again
            }
        }
        hour = rise_hour;
        min = rise_minute;
    }

    if (!gnss_configured || consec_conn_failures > 0) {
        config_gnss = 1;
        time_only = 0;
        *baud_rate = 9600;  // reset the starting baud to default so the config messages work
        uart_set_baudrate(UART_ID, *baud_rate);
    }

    gnss_init(latitude, longitude, north, east, &year, &month, &day, &hour, &min, &sec, utc_offset, baud_rate,
              &gnss_read_successful, gnss_configured, config_gnss, new_baud, time_only);
    if (!gnss_read_successful) {
        printf("failed read!\n");
        consec_conn_failures += 1;
        if (*event == 1 || *event == -1) {
            // the above code already calculated tomorrow's date and rise time, use it.
        } else {
            // get tomorrow's date and set the alarm for 00:00 tomorrow
            today_is_tomorrow(&year, &month, &day, NULL, *utc_offset);  // modify the day, month (if applicable), year (if applicable) to tomorrow's dd/mm/yyyy
            dotw = get_dotw(year, month, day);  // get tomorrow's day of the week using the freshly changed values for year, month, day
            hour = 0;
            min = 0;
        }
        *event = -1;  // set the next event to invalid
    } else {  // rbf
        printf("gnss init successful on repeat alarm!\n");
    }

    // datetime_t next_alarm = {  // the `real` version
    //     .year  = year,
    //     .month = month,
    //     .day   = day,
    //     .dotw  = dotw,
    //     .hour  = hour,
    //     .min   = min,
    //     .sec   = 00
    // };

    /* ---------------- all the below can be deleted after testing ---------------- */
    rtc_get_datetime(&now);  // get the newly set time
    int sleep_time = 10;
    if (now.sec + sleep_time > 59) {  // for debugging only
        min = now.min + 1;
        sec = (now.sec + sleep_time) % 60;
    } else {
        min = now.min;
        sec = now.sec + sleep_time;
    }
    datetime_t next_alarm = {  // for debugging multiple alarm cycles
        .year  = now.year,
        .month = now.month,
        .day   = now.day,
        .dotw  = now.dotw,
        .hour  = now.hour,
        .min   = min,
        .sec   = sec
    };
    /* ---------------- all the above can be deleted after testing ---------------- */

    if (*consec_conn_failures < MAX_CONSEC_CONN_FAILURES) {
        printf("local time is: %d/%d/%d %d:%d:%d\n", now.month, now.day, now.year, now.hour, now.min, now.sec);  // rbf
        printf("setting the next alarm for: %d/%d/%d %d:%d:%d\n", next_alarm.month, next_alarm.day, next_alarm.year,
               next_alarm.hour, next_alarm.min, next_alarm.sec);  // rbf
        utils_set_rtc_alarm(&next_alarm, &alarm_callback);
    } else {
        printf("max consec failures reached. No further alarms will be set\n");
    }
}


void disable_all_interrupts_for(uint gpio) {
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);  // disable further interrupts during execution to mitigate mechanical switch bounce
}


void reenable_interrupts_for(uint gpio, int event) {
    gpio_set_irq_enabled_with_callback(gpio, event, true, &toggle_callback);
}


void set_automation_state(uint *automation_enabled) {
    // read the state of the toggle gpio pins
    busy_wait_ms(250);  // eliminate mechanical switch bounce
    if (gpio_get(GPIO_TOGGLE_DOWN_PIN) == 0 || gpio_get(GPIO_TOGGLE_UP_PIN) == 0) {
        disable_automation(automation_enabled);
    } else {
        enable_automation(automation_enabled);
    }
}


void normalize_boundaries(int *boundary_low, int *boundary_high, int *current_position) {
    // set low boundary to 0
    printf("normalizing...\n");  // rbf
    printf("current pos before: %d\n", current_position);  // rbf
    printf("--------------\n");  // rbf
    *current_position += abs(*boundary_low);
    *boundary_high += abs(*boundary_low);
    *boundary_low += abs(*boundary_low);  // must do this *after* other shifts
    printf("new low boundary: %d\n", boundary_low);  // rbf
    printf("new high boundary: %d\n", boundary_high);  // rbf
    printf("current pos after: %d\n", current_position);  // rbf

}


void find_boundary(uint *low_boundary_set, uint *high_boundary_set, int *boundary_low,
                   int *boundary_high, int *current_position, uint gpio) {
    // wait for down toggle
    busy_wait_ms(100);  // combar switch bounce
    int stepped = 0;
    // printf("gpio: %d\n", gpio);
    uint dir = gpio == GPIO_TOGGLE_UP_PIN ? 0 : 1;
    // printf("dir: %d\n", dir);
    wake_stepper();
    while (gpio_get(gpio) == 0) {
        // while the switch is still pressed
        single_step(current_position, dir, 1500);
        stepped = 1;
    }
    sleep_stepper();
    // update the respective boundary
    if (stepped && dir == 0) {
        *boundary_high = *current_position;
        *high_boundary_set = 1;
        // printf("Upper boundary found: %d\n", boundary_high);  // rbf
    } else if (stepped && dir == 1) {
        *boundary_low = *current_position;
        *low_boundary_set = 1;
        // printf("Lower boundary found: %d\n", boundary_low);  // rbf
    } else {
        // was just switch bounce, ignore it.
    }
    if (*low_boundary_set && *high_boundary_set) {
        normalize_boundaries(boundary_low, boundary_high, current_position);
    }
}


void handle_toggle(uint *low_boundary_set, uint *high_boundary_set, int *boundary_low,
                   int *boundary_high, int *current_position, uint *automation_enabled,
                   uint gpio, uint32_t event) {
    disable_all_interrupts_for(gpio);  // prevent further irqs while handling this one

    if (event == 0x04) {
        // Falling edge detected. disable all interrupts until done
        if (!low_boundary_set || !high_boundary_set) {
            find_boundary(low_boundary_set, high_boundary_set, boundary_low,
                          boundary_high, current_position, gpio);
        } else {
            step_indefinitely(current_position, *boundary_high, gpio);
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
    set_automation_state(automation_enabled);
}


void enable_automation(uint *automation_enabled) {
    *automation_enabled = 1;
    // printf("automation state: %d\n", automation_enabled);  // rbf
}


void disable_automation(uint *automation_enabled) {
    *automation_enabled = 0;
    // printf("automation state: %d\n", automation_enabled);  // rbf
}
