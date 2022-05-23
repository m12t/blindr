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

void parse_time(char *time, int *hour, int *min, int *sec) {
    // convert the char to int and formulate the int using tens and ones places.
    *hour = 10 * (time[0] - '0') + (time[1] - '0');
    *min = 10 * (time[2] - '0') + (time[3] - '0');
    *sec = 10 * (time[4] - '0') + (time[5] - '0');
}

int main(void) {
    char *zda_msg[7];
    zda_msg[0] = "$GNZDA";
    zda_msg[1] = "220929.00";
    zda_msg[2] = "21";
    zda_msg[3] = "05";
    zda_msg[4] = "2022";
    zda_msg[5] = "0";
    zda_msg[6] = "0";

    int hour, min, sec;
    parse_time(zda_msg[1], &hour, &min, &sec);

    printf("ID: %s\n", zda_msg[0]);
    printf("hour: %d\n", hour);
    printf("min: %d\n", min);
    printf("sec: %d\n", sec);
    printf("day: %d\n", atoi(zda_msg[2]));
    printf("month: %d\n", atoi(zda_msg[3]));
    printf("year: %d\n", atoi(zda_msg[4]));
}