#ifndef SETUP_H
#define SETUP_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

#endif


void set_onboard_rt(int16_t *year, int8_t *month, int8_t day,
                    int8_t *hour, int8_t *min, int8_t *sec);