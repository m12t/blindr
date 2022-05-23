#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// 0: $xxZDA
// 1: 220929.00 (utc time hhmmss.ss)
// 2: 21  (day, dd)
// 3: 05  (month, mm)
// 4: 2022  (year, yyyy)
// 5: 00 (Local time zone hours; always 00)
// 6: 00 (Local time zone minutes; always 00)

void parse_zda_time(char **zda_msg, int16_t *year, int8_t *month, int8_t *day,
                    int8_t *hour, int8_t *min, int8_t *sec) {
    // convert the char to int and formulate the int using tens and ones places.
    /* 
    an example of calling this function where `zda_msg` is a pointer array:
    int16_t year;
    int8_t month, day, hour, min, sec;
    parse_time(zda_msg, &year, &month, &day, &hour, &min, &sec);
    */
    *year = atoi(zda_msg[4]);
    *month = atoi(zda_msg[3]);
    *day = atoi(zda_msg[2]);
    char *time = zda_msg[1];
    *hour = 10 * (time[0] - '0') + (time[1] - '0');
    *min = 10 * (time[2] - '0') + (time[3] - '0');
    *sec = 10 * (time[4] - '0') + (time[5] - '0');
}

int main(void) {
    /* used for debugging--- */
    char *zda_msg[7];
    zda_msg[0] = "$GNZDA";
    zda_msg[1] = "191910.00";
    zda_msg[2] = "21";
    zda_msg[3] = "05";`
    zda_msg[4] = "2022";
    zda_msg[5] = "0";
    zda_msg[6] = "0";
    /* ---used for debugging */

    int16_t year;
    int8_t month, day, hour, min, sec;
    parse_time(zda_msg, &year, &month, &day, &hour, &min, &sec);

    /* used for debugging--- */
    printf("ID: %s\n", zda_msg[0]);
    printf("hour: %d\n", hour);
    printf("min: %d\n", min);
    printf("sec: %d\n", sec);
    printf("day: %d\n", day);
    printf("month: %d\n", month);
    printf("year: %d\n", year);
    /* ---used for debugging */
}