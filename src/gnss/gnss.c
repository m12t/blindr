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
void parse_buffer(char *buffer, char **sentences);
int parse_line(char *string, char **fields, int num_fields);
void on_uart_rx(void);
int console_print(char *buffer);
void setup(void);
int main(void);

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
	return i-2;  // exclude the last row and move index back 1
}

// RX interrupt handler
void on_uart_rx(void) {
    // todo: only read valid lines when splitting them up to sentences or values.
    size_t len = 256;  // size of the buffer in bytes
    char buffer[len];  // make a buffer of size `len` for the raw message
    char *sentences[8];  // array of pointers pointing to the location of the start of each sentence within buffer
    uart_read_blocking(UART_ID, buffer, len);  // read the message into the buffer
    parse_buffer(buffer, sentences);  // split the monolithic buffer into discrete sentences

    int i = 0; 
	while (sentences[i] != NULL) {
		if (strstr(sentences[i], "GGA")) {
			// https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf
			int num_gga_fields = 18;  // 1 more
			char *gga_fields[num_gga_fields];
			int gga_populated;
			gga_populated = parse_line(sentences[i], gga_fields, num_gga_fields);
			printf("found GGA:\n%s\n", sentences[i]);  // DAT
			for (int j = 0; j <= gga_populated; j++) {
				printf("%d: %s\n", j, gga_fields[j]);
			}
		} else if (strstr(sentences[i], "ZDA")) {
			int num_zda_fields = 10;  // 1 more
			char *zda_fields[num_zda_fields];
			int zda_populated;
			zda_populated = parse_line(sentences[i], zda_fields, num_zda_fields);
			printf("found ZDA:\n%s\n", sentences[i]);  // DAT
			for (int j = 0; j <= zda_populated; j++) {
				printf("%d: %s\n", j, zda_fields[j]);
			}
		}
		// } else if (strstr(sentences[i], "VTG")) {
		// 	printf("loop: %s\n", sentences[i]);
		// }
		i++;
	}

}


int console_print(char *buffer) {
    printf("\n%s\n", buffer);
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

