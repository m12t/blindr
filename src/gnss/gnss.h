#ifndef GNSS_H
#define GNSS_H

#include "../utils/utils.h"

#include <stdio.h>      // rbf
#include <stdlib.h>
#include <stdint.h>     // for int8_t and int16_t
#include <ctype.h>  // atoi, atof, i think...
#include <string.h>  // definitely needed
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/dma.h"               // for UART DMA
#include "hardware/pio.h"
#include "hardware/structs/bus_ctrl.h"  // needed for bus priority access to the UART DMA


// uart config
#define UART_ID uart1
#define DMA_ID 0
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define BUFFER_LEN 256
#define SENTENCES_LEN 8


// init/deinit functions
void gnss_init(double *latitude, double *longitude, uint *north, uint *east, int *utc_offset,
               uint *baud_rate, uint *gnss_read_successful, uint *gnss_configured, uint config_gnss,
               uint new_baud_rate, uint time_only, uint *data_found);
int gnss_manage_connection(char *buffer, char **sentences, double *latitude,
                           double *longitude, uint *north, uint *east, int *utc_offset,
                           uint *gnss_read_successful, uint *gnss_configured, uint time_only,
                           PIO pio, uint sm, uint offset, uint *gnss_fix, uint *data_found);
void gnss_deinit(PIO pio, uint sm, uint offset);
void gnss_dma_init(char *buffer);
void gnss_dma_deinit(void);
void gnss_uart_tx_init(uint baud_rate);
void gnss_uart_deinit(void);


// data parsing functions
void parse_buffer(char *buffer, char **sentences, double *latitude, double *longitude,
                  uint *north, uint *east, int *utc_offset,
                  uint *gnss_read_successful, uint time_only, PIO pio,
                  uint sm, uint offset, uint *gnss_fix);
void split_buffer(char *buffer, char **sentences, int max_sentences, uint *sentences_pos);
void parse_line(char *string, char **fields, int num_fields);
void parse_utc_time(char *time, int8_t *hour, int8_t *min, int8_t *sec);
void get_utc_offset(double longitude, int *utc_offset);
void parse_zda(char **zda_msg, int16_t *year, int8_t *month, int8_t *day,
               int8_t *hour, int8_t *min, int8_t *sec);
void parse_gga(char **gga_msg, double *latitude, int *north,
               double *longitude, int *east, int *gnss_fix, uint time_only);
void to_decimal_degrees(double *position, int *direction);
int hexchar2int(char c);
int hex2int(char *c);
int checksum_valid(char *string);
int get_checksum(char *string);


// config functions
void configure_gnss(uint *baud_rate, uint new_baud_rate);
void compile_message(char *nmea_msg, char *raw_msg, char *checksum, char *terminator);
void build_baud_msg(char *msg, uint new_baud);
void fire_ubx_msg(uint8_t *msg, size_t msg_len);
void fire_nmea_msg(char *msg);
void wake_gnss(void);
void sleep_gnss(void);
void save_config(void);


#endif