#include "gnss.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define UART_ID uart1
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define true 1
#define false 0

void parse_buffer(char *buffer, char **sentences) {
    /*
    split out the buffer into individual NMEA sentences
    which are terminated by <cr><lf> aka `\r\n`
    */
    int i = 0;
    char *eol;  // end of line
    eol = strtok(buffer, "\n\r");
    while (eol != NULL) {
		sentences[i++] = eol;
        eol = strtok(NULL, "\n\r");  // https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-strtok-tokenize-string
    }
	sentences[i-1] = NULL;  // NULL out the last entered row as it can't be guaranteed to be complete due to strtok()
}


int parse_line(char *string, char **fields, int num_fields) {
    int i = 0;
    fields[i++] = string;
    // search for the numebr of `,` in the sentence to create the appropriate size array?
    while ((i < num_fields) && NULL != (string = strchr(string, ','))) {
        *string = '\0';
        fields[i++] = ++string;
    }
	return --i;  // move index back 1 to return the num_populated fields
}

int hexchar2int(char c) {
    // from: https://github.com/craigpeacock/NMEA-GPS/blob/master/gps.c
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

int hex2int(char *c) {
    // from: https://github.com/craigpeacock/NMEA-GPS/blob/master/gps.c
	int value;
	value = hexchar2int(c[0]);
	value = value << 4;
	value += hexchar2int(c[1]);
	return value;
}


int checksum_valid(char *string) {
    // from: https://github.com/craigpeacock/NMEA-GPS/blob/master/gps.c
    char *checksum_str;
	int checksum;
	unsigned char calculated_checksum = 0;

	// Checksum is postcede by *
	checksum_str = strchr(string, '*');
	if (checksum_str != NULL){
		// Remove checksum from string
		*checksum_str = '\0';
		// Calculate checksum, starting after $ (i = 1)
		for (int i = 1; i < strlen(string); i++) {
			calculated_checksum = calculated_checksum ^ string[i];
		}
        // printf("Calculated checksum: %c", calculated_checksum);
		checksum = hex2int((char *)checksum_str+1);
		if (checksum == calculated_checksum) {
			// printf("Checksum OK");
			return 1;
		}
	} else {
		// printf("Error: Checksum missing or NULL NMEA message\r\n");
		return 0;
	}
	return 0;
}

// RX interrupt handler
void on_uart_rx(void) {
    size_t len = 255;
    char buffer[len];  // make a buffer of size `len` for the raw message
    char *sentences[8];  // array of pointers pointing to the location of the start of each sentence within buffer

    uart_read_blocking(UART_ID, buffer, len);  // read the message into the buffer
    parse_buffer(buffer, sentences);  // split the monolithic buffer into discrete sentences

    int i = 0;
	while (sentences[i] != NULL) {
        int num_fields = 0;
        int num_populated = 0;  // reset each iteration
        int valid = false;
		if (strstr(sentences[i], "GGA")) {
			// https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf
			num_fields = 18;  // 1 more
            valid = true;  // run the below. temporarily false for testing VTG
		} else if (strstr(sentences[i], "ZDA")) {
			num_fields = 10;  // 1 more
            valid = true;  // run the below. temporarily false for testing VTG
		} else {
            num_fields = 24;
            valid = true;  // risky... ok for debugging.
        }
        // todo: read the raw data and look for ZDA sentences and check the FIFO UART/read the raw chars one by one

        if (valid && checksum_valid(sentences[i])) {
            char *fields[num_fields];
            num_populated = parse_line(sentences[i], fields, num_fields);
            printf("\e[1;1H\e[2J");  // clear screen
            for (int j = 0; j <= num_populated; j++) {
                printf("%d: %s\n", j, fields[j]);  // extract values or whatever.
            }
        }
		i++;
	}
}

void setup(void) {
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // See datasheet for more information on function select
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

    // todo: change RTC time.

    while (1)
        tight_loop_contents();
}
