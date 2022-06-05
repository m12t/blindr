#ifndef GNSS_H
#define GNSS_H

#include <stdio.h>      // rbf
#include <stdlib.h>
#include <stdint.h>     // for int8_t and int16_t
#include <ctype.h>  // atoi, atof, i think...
#include <string.h>  // definitely needed
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"     // needed??
#include "hardware/rtc.h"

// uart config
#define UART_ID uart1
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define buffer_len 256
#define sentences_len 8

extern int8_t sec, min, hour, day, month, utc_offset;
extern int16_t year;
extern double latitude, longitude;  // use atof() on these. float *should* be sufficient
extern int north, east, gnss_fix;
extern uint baud_rate;
extern uint gnss_running, gnss_read_successful;

#endif

void parse_buffer();
void split_buffer(char *buffer, char **sentences, int max_sentences);
void parse_line(char *string, char **fields, int num_fields);
int checksum_valid(char *string);
int hex2int(char *c);
int hexchar2int(char c);
void parse_zda(char **zda_msg, int16_t *year, int8_t *month, int8_t *day,
               int8_t *hour, int8_t *min, int8_t *sec);
void parse_gga(char **gga_msg, double *latitude, int *north,
               double *longitude, int *east, int *gnss_fix);
void get_utc_offset(double longitude, int8_t *utc_offset);
void gnss_init(void);
void gnss_deinit(int fully);

// config functions
int get_checksum(char *string);
void uart_tx_setup(void);
void uart_rx_setup(void);
void compile_message(char *nmea_msg, char *raw_msg, char *checksum,
                     char *terminator);
uint extract_baud_rate(char *string);
void config_gnss();
void fire_nmea_msg(char *msg);
void fire_ubx_msg(uint8_t *msg, size_t msg_len);
void wake_gnss(void);
void sleep_gnss(void);
void abort_gnss_timeout(void);
int manage_gnss_connection(void);
