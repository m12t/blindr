/* a program for sending UBX-CFG messages to the gnss module
   to change things like baud rate and desired NMEA sentences */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define UART_ID uart1
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 4
#define UART_RX_PIN 5

int add_checksum(char *string);
void on_uart_rx(void);
char* char2hex(unsigned char calculated_checksum);

/* ublox m8 datasheet:
https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf
*/

// set baud rate to 115200
// char *bitArray = {0xB5,0x62,0x06,0x00,


// 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x4F, 0xF6, 0x1F,
// 0x7B, 0x60, 0xCB, 0x07, 0x6F, 0x09, 0x1F, 0xE5, 0x13, 0xFE, 0xA4, 0x19, 0x00, 0x00};


// uint8_t msg[] = { 0xB5,0x62,0x0A,0x04,0x00,0x00,0x0E,0x34,\0 };


void on_uart_rx() {
    size_t len = 1024;  // size of the buffer in bytes
    char buffer[len];  // make a buffer of size `len` for the raw message

    uart_read_blocking(UART_ID, buffer, len);
    printf("received:\n%s\n-------------\n", buffer);
}

char* char2hex(unsigned char calculated_checksum) {
    long quotient, remainder;
    int i, j = 0;
    char hexadecimalnum[100]; 
    quotient = calculated_checksum;
 
    while (quotient != 0)
    {
        remainder = quotient % 16;
        if (remainder < 10)
            hexadecimalnum[j++] = 48 + remainder;
        else
            hexadecimalnum[j++] = 55 + remainder;
        quotient = quotient / 16;
    }
    printf("%s", hexadecimalnum);
    return hexadecimalnum;
}

int add_checksum(char *string) {
    // adapted from: https://github.com/craigpeacock/NMEA-GPS/blob/master/gps.c
    char *checksum_str;
	int checksum;
	unsigned char calculated_checksum = 0;
    printf("calculating checksum\n");

	// Checksum is postcede by *
	checksum_str = strchr(string, '*');
	if (checksum_str != NULL){
        printf("not null!\n");
		// Remove checksum from string
		*checksum_str = '\0';
		// Calculate checksum, starting after $ (i = 1)
		for (int i = 1; i < strlen(string); i++) {
			calculated_checksum = calculated_checksum ^ string[i];
		}
        printf("Calculated checksum: %u\n", calculated_checksum);
        char cs[] = char2hex(calculated_checksum);
        printf("hex check: %s\n", cs);
        string[strlen(string) - 4] = cs[0];
        string[strlen(string) - 3] = cs[1];
        printf("full msg: %s\n", string);
        return calculated_checksum;
	} else {
		// printf("Error: Checksum missing or NULL NMEA message\r\n");
		return 0;
	}
	return 0;
}


int main(void) {
    stdio_init_all();  // so printf works
    printf("initialized stdio\n");
    uart_init(UART_ID, 9600);
    printf("initialized uart on 9600\n");
    // char nmea_msg[] = "$PUBX,41,1,3,3,115200,0*1C\r\n";
    // char nmea_msg[] = "$PUBX,41,1,0007,0003,115200,0*24\r\n";  // update baud rate
    // char nmea_msg[] = "$PUBX,40,ZDA,1,1,1,0*45\r\n";  // enable ZDA
    char nmea_msg[] = "$PUBX,40,GLL,0,1,0,0*5D\r\n";  // disable GLL messages  // TODO: test selectively enabling USART GLL...
    add_checksum(nmea_msg);
    printf("%s", nmea_msg);
    /*
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);

    // send these prior to interrupts
    printf("----\n");
    // for (int i=0; i<strlen(nmea_msg); i++) {
    //     uart_putc_raw(UART_ID, nmea_msg[i]);
    // }
    printf("----\n");

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
    // char msg[] = "0xB5,0x62,0x0A,0x04,0x00,0x00,0x0E,0x34,\0";
    // uint8_t msg[] = { 0xB5,0x62,0x0A,0x04,0x00,0x00,0x0E,0x34,00 };

    sleep_ms(2000);
    // int __unused actual = uart_set_baudrate(UART_ID, 115200);
    while (1)
        tight_loop_contents();

    // */
}