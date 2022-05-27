#ifndef GNSS_H
#define GNSS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>  // needed for int8_t and int16_t
#include <ctype.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#endif


int gnss_main(void);
void parse_buffer(char *buffer, char **sentences, int max_sentences);
void parse_line(char *string, char **fields, int num_fields);
int checksum_valid(char *string);
void on_uart_rx(void);
void setup(void);
int hex2int(char *c);
int hexchar2int(char c);
void parse_zda(char **zda_msg, int16_t *year, int8_t *month, int8_t *day,
               int8_t *hour, int8_t *min, int8_t *sec);
void parse_gga(char **gga_msg, double *latitude, int *north,
               double *longitude, int *east, int *gnss_fix);
void get_utc_offset(double longitude, uint8_t *utc_offset, int8_t month, int8_t day);
