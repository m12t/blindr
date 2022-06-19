#include "blindr.h"

// global vars
uint alarm_detected=1;  // flag that gets set by the alarm callback to cause a runthrough of the gnss read, blind actuation, and next alarm setting sequence.
uint low_boundary_set=0, high_boundary_set=0;  // flag for whether the respective boundary is set or not
int boundary_low=0, boundary_high=0, current_position=0;  // stepper positioning
uint automation_enabled = 1;  // flag useful for whether or not to operate the blinds automatically.

int main(void) {
    // -------------------------------- initialize main program vars --------------------------------

    // gnss variables
    double latitude=0.0, longitude=0.0;
    int north=-1, east=-1;
    int utc_offset;
    uint baud_rate=9600, new_baud=115200, gnss_configured=0, consec_conn_failures=0, data_found=0, time_only=0, config_gnss=1;
    int solar_event = -1;       // flag for what the next solar_event is: 0=sunset, 1=sunrise, -1=NULL/Invalid

    // -------------------------------- initialize the stepper and toggle switch --------------------------------
    // stdio_init_all();  // rbf - used for debugging
    // printf("blindr initializing...\n");
    stepper_init();
    toggle_init(&toggle_callback);

    // -------------------------------- run the main program loop indefinitely --------------------------------
    while (1) {
        // keep the program alive indefinitely while listening for interrupts and handling them accordingly.
       if (alarm_detected) {
            alarm_detected = 0;
            read_actuate_alarm_sequence(&solar_event, &latitude, &longitude, &north, &east,
                                        &utc_offset, &baud_rate, new_baud, &gnss_configured, &config_gnss,
                                        &consec_conn_failures, &data_found, &time_only);
        }
        sleep_ms(60000);  // sleep for 1 min
    }
}


void alarm_callback(void) {
    alarm_detected = 1;
}


void toggle_callback(uint gpio, uint32_t event) {
    // * unfortunately, this callback has to contain the toggle logic instead of simply
    //   setting a flag that's caught in the main loop like alarm_callback(). The reason
    //   is so that toggle can run concurrently with the gnss module. Otherwise,
    //   it would be blocked by read_actuate_alarm_sequence()
    disable_all_interrupts_for(gpio);  // prevent further irqs while handling this one
    busy_wait_ms(100);    // combat switch bounce.

    if (event == 0x04) {
        // Falling edge detected. disable all interrupts until done
        if (!low_boundary_set || !high_boundary_set) {
            find_boundary(gpio);
        } else {
            step_indefinitely(&current_position, boundary_high, gpio);
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
    set_automation_state();  // this is idempotent, so just call it every time.
}


void actuate(int solar_event) {
    if (automation_enabled) {
        if (solar_event == 1) {
            // it's a sunrise right now... open the blinds
            step_to_position(&current_position, (uint)(boundary_high - boundary_low) / 2, boundary_high);
        } else if (solar_event == 0) {
            // it's a sunset right now... close the blinds
            step_to_position(&current_position, 0, boundary_high);
        }
    }
}


void read_actuate_alarm_sequence(int *solar_event, double *latitude, double *longitude,
                                 int *north, int *east, int *utc_offset, uint *baud_rate,
                                 uint new_baud, uint *gnss_configured, uint *config_gnss,
                                 uint *consec_conn_failures, uint *data_found, uint *time_only) {
    // handle the current alarm/solar_event and set the next one.
    uint gnss_read_successful = 0;

    actuate(*solar_event);

    gnss_init(latitude, longitude, north, east, utc_offset, baud_rate, &gnss_read_successful,
              gnss_configured, *config_gnss, new_baud, *time_only, data_found);

    if (gnss_read_successful) {
        *time_only = 1;  // now that the first runthrough has gathered lat/long, idempotently set time_only to true for future requests.
        *config_gnss = 0;
        // printf("good read\n");
    } else {
        // * the read failed. reset UART baud to the gnss module's
        //   default and set the flag to run the configuration on the module
        // printf("unsuccessful read\n");
        consec_conn_failures += 1;
        *config_gnss = 1;
        *baud_rate = 9600;  // reset the starting baud to default so the config messages work
        uart_set_baudrate(UART_ID, *baud_rate);
    }

    datetime_t now = { 0 };    // blank datetime struct to be pupulated by get_rtc_datetime(&now) calls
    rtc_get_datetime(&now);    // grab the year, month, day for the solar calculations below


    int16_t year = now.year;
    int8_t month = now.month;
    int8_t day = now.day;
    int8_t hour = now.hour;
    int8_t min = now.min;
    int8_t rise_hour, rise_minute, set_hour, set_minute;  // solar event times that are populated by `calculate_solar_events()`


    if (*solar_event == 1) {  // it's a sunrise right now
        calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                               year, month, day, *utc_offset, *latitude, *longitude);
        *solar_event = 0;  // next alarm will be sunset
        hour = set_hour;
        min = set_minute;
    } else {
        if (*solar_event == 0) {  // it's a sunset right now
            // * the next solar_event is a sunrise and will occur tomorrow. Get tomorrow's solar events:
            //   - initialize tomorrow's variables to today and send the pointers to today_is_tomorrow() which will
            //     update them as needed. Use tomorrow's date to set an alarm for either the sunrise time or 00:00
            today_is_tomorrow(&year, &month, &day, NULL, *utc_offset);  // modify the day, month (if applicable), year (if applicable) to tomorrow's dd/mm/yyyy

            calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                                   year, month, day, *utc_offset, *latitude, *longitude);
            *solar_event = 1;  // next alarm will be sunrise
            hour = rise_hour;
            min = rise_minute;
        } else {  // solar_event == -1 meaning it's startup or the last solar_event was invalid.
            // the time is after both the sunrise and sunset (or there were neither)...
            // get the sunrise time tomorrow. In the edge case there is none (ie. high
            // latitudes around the summer solstice), sleep until 0:00 tomorrow and try again.
            if (check_for_solar_events_today(year, month, day, *utc_offset, *latitude, *longitude)) {
                // there is a valid solar_event today. find the next one (if applicable)
                calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                                       year, month, day, *utc_offset, *latitude, *longitude);
                if (rise_hour > now.hour || (rise_hour == now.hour && rise_minute > now.min)) {
                    // the next solar event is a sunrise today.
                    *solar_event = 1;
                    hour = rise_hour;
                    min = rise_minute;
                } else if (set_hour > now.hour || (set_hour == now.hour && set_minute > now.min)) {
                    // the next solar event is a sunset today.
                    *solar_event = 0;
                    hour = set_hour;
                    min = set_minute;
                } else {
                    // the next event is a rise tomorrow
                    today_is_tomorrow(&year, &month, &day, NULL, *utc_offset);  // modify the day, month (if applicable), year (if applicable) to tomorrow's dd/mm/yyyy
                    calculate_solar_events(&rise_hour, &rise_minute, &set_hour, &set_minute,
                                           year, month, day, *utc_offset, *latitude, *longitude);
                    *solar_event = 1;
                    hour = rise_hour;
                    min = rise_minute;
                }
            } else {
                *solar_event = -1;  // no valid solar_event times were found, try again first thing tomorrow morning
                hour = 00;
                min = 01;
            }
        }

    }

    datetime_t next_alarm = {  // the `real` version
        .year  = year,
        .month = month,
        .day   = day,
        .hour  = hour,
        .min   = min,
        .sec   = 00
    };

    if (*consec_conn_failures < MAX_CONSEC_CONN_FAILURES) {
        // set the next alarm. else abort alarm sequence.
        utils_set_rtc_alarm(&next_alarm, &alarm_callback);
        // printf("local time is: %d/%d/%d %d:%d:%d\n", now.month, now.day, now.year, now.hour, now.min, now.sec);  // rbf
        // printf("setting the next alarm for: %d/%d/%d %d:%d:%d\n", next_alarm.month, next_alarm.day, next_alarm.year,
        //     next_alarm.hour, next_alarm.min, next_alarm.sec);  // rbf
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
    // printf("normalizing...\n");  // rbf
    // printf("current pos before: %d\n", current_position);  // rbf
    // printf("--------------\n");  // rbf
    current_position += abs(boundary_low);
    boundary_high += abs(boundary_low);
    boundary_low += abs(boundary_low);  // must do this *after* other shifts
    // printf("new low boundary: %d\n", boundary_low);  // rbf
    // printf("new high boundary: %d\n", boundary_high);  // rbf
    // printf("current pos after: %d\n", current_position);  // rbf
}


void find_boundary(uint gpio) {
    // printf("finding boundary\n");
    // wait for down toggle
    // busy_wait_ms(100);  // combar switch bounce  -- not needed now that main loop has a delay
    int stepped = 0;
    // printf("gpio: %d\n", gpio);
    uint dir = gpio == GPIO_TOGGLE_UP_PIN ? 0 : 1;
    // printf("dir: %d\n", dir);
    wake_stepper();
    while (gpio_get(gpio) == 0) {
        // printf("stepping\n");
        // while the switch is still pressed
        single_step(&current_position, dir, 1500);
        stepped = 1;
    }
    sleep_stepper();
    // update the respective boundary
    if (stepped && dir == 0) {
        // printf("stepped in dir 0\n");
        boundary_high = current_position;
        high_boundary_set = 1;
        // printf("Upper boundary found: %d\n", boundary_high);  // rbf
    } else if (stepped && dir == 1) {
        // printf("stepped in dir 1\n");
        boundary_low = current_position;
        low_boundary_set = 1;
        // printf("Lower boundary found: %d\n", boundary_low);  // rbf
    } else {
        // was just switch bounce, ignore it.
    }
    // printf("low_boundary_set: %d, high_boundary_set: %d\n", low_boundary_set, high_boundary_set);
    if (low_boundary_set && high_boundary_set) {
        normalize_boundaries();
    }
}


void enable_automation(void) {
    automation_enabled = 1;
    // printf("automation state: %d\n", automation_enabled);  // rbf
}


void disable_automation(void) {
    automation_enabled = 0;
    // printf("automation state: %d\n", automation_enabled);  // rbf
}
