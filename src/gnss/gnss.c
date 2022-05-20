#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define UART_ID uart1
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE
#define UART_TX_PIN 4
#define UART_RX_PIN 5


// function prototypes
int split_lines(char *buffer, bool print, size_t buffer_len);  // TODO: needs changes...
int parse_line(char *buffer, char **fields, int max_fields);
void split_sentences(char *buffer);
int console_print(char *buffer);
void on_uart_rx(void);
void setup(void);



void split_sentences(char *buffer) {
    /*
    split out the buffer into individual NMEA sentences
    which are terminated by <cr><lf> aka `\r\n`
    */
    char *eol;
    eol = strtok(buffer, "\n\r");
    while (eol != NULL) {
        printf( "line:%s\n", eol );  // DAT
        eol = strtok(NULL, "\n\r");
    }
}

int split_lines(char *buffer, bool print, size_t buffer_len) {
    char *lines[8];  // individual NMEA sentneces within the buffer
    char *field[20];  // comma-delimited values for each sentence
    split_sentences(buffer);
    if (print)  // print the buffer to the console
        console_print(buffer);
    // char *gga = strstr(buffer, "GGA");
    // // TODO: stop reading at "\r\n"
    // if (gga) {
    //     printf("found GGA!\n");
    //     console_print(gga-2);
    //     i = parse_line(gga-2, field, 20);  // -2 to include the `talker ID`
    //     printf("\n\nMSG type  :%s\r\n",field[0]);
    //     printf("UTC Time  :%s\r\n",field[1]);
    //     printf("Latitude  :%s\r\n",field[2]);
    //     printf("Longitude :%s\r\n",field[4]);
    //     printf("Altitude  :%s\r\n",field[9]);
    //     printf("Satellites:%s\r\n\n",field[7]);
    // }
}

int parse_line(char *buffer, char **fields, int max_fields) {
    // for splitting sentences into parts
    int i = 0;
	fields[i++] = buffer;

	while ((i < max_fields) && NULL != (buffer = strchr(buffer, ','))) {
        if (buffer[0] == '$') {
            printf("breaking out\n");
            break;
        }
		*buffer = '\0';  // change the comma to an end of string NULL character
		fields[i++] = ++buffer;
	}
	return --i;
}

int console_print(char *buffer) {
    printf("\n%s\n", buffer);
}

// RX interrupt handler
void on_uart_rx(void) {
    // todo: only read valid lines when splitting them up to sentences or values.
    size_t len = 256;  // size of the buffer in bytes
    char buffer[len];  // make a buffer of size `len` for the raw message
    char *sentences[8];  // buffer for holding pointers to each sentence
    uart_read_blocking(UART_ID, buffer, len);  // read the message into the buffer
    split_message(buffer, sentences, len);  // split the message into individual sentences
    split_sentences();  // split the sentences into values
}

void setup(void) {
    printf("\n\n Initializing... \n");
    stdio_init_all();
    uart_init(UART_ID, 9600);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    // Set UART flow control CTS/RTS to `false`
    uart_set_hw_flow(UART_ID, false, false);
    // Set data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    // Turn on FIFO's - throughput is valued over latency.
    uart_set_fifo_enabled(UART_ID, true);
    // Set up a RX interrupt
    int UART_IRQ = UART1_IRQ;
    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
}


int main(void) {
    setup();
    while (1)
        tight_loop_contents();
}

