/*
    a program for sending NMEA PUBX and UBX protocol messages
    to a ublox gnss module to change things like baud rate,
    message rate, and desired NMEA sentences, etc.

    /* ublox m8 datasheet:
    https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf

    I'm using a M8030 chip on a quadcopter GPS module:
        - Team Blacksheep M8.2
        - BOM: ~$11

    NOTE: It's critical the chip is genuine ublox and not a clone. To detect this, connect to the module via
    UART and have a stream (eg. using minicom) to read the output in the terminal. Then disconnect power from
    only the module and reconnect it. A stream of `GPTXT` data will be emitted. You can look up what it should and
    shouldn't look like for a genuine chip. This program likely won't work on a clone chip.

    >>> after hours of digging, eventually removing the metal cover to see the m8030-KT chip, which has no internal
        flash storage. So the save commands can only save to BBR at best. The save UBX-CFG-CFG save commands are
        correct, there just isn't any onboard flash to save to on my chip.
*/
#include "gnss.h"

#define UART_ID uart1   // change as needed
#define BAUD_RATE 9600  // default BAUD rate for the module for initial connection. can be changed later.
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 4   // change as needed
#define UART_RX_PIN 5   // change as needed

void on_uart_rx(void);
int get_checksum(char *string);
void uart_tx_setup(void);
void uart_rx_setup(void);
void compile_message(char *nmea_msg, char *raw_msg, char *checksum,
                     char *terminator);
int extract_baud_rate(char *string);
void send_nmea(int testrun, int changing_baud);
void send_ubx(int testrun);
void fire_nmea_msg(char *msg);
void fire_ubx_msg(uint8_t *msg, size_t len);


int main(void) {
    uart_init(UART_ID, BAUD_RATE);
    uart_tx_setup();  // initialize UART Tx on the pico

    //  execution parameters ----------------------------------
    int testrun = 0;  // 1 to print the simulated transmission only, 0 to transmit it.
    int changing_baud = 1;  // only required for NMEA messages

    // send nmea, ubx, or both. simply uncomment what you want to send:
    send_nmea(testrun, changing_baud);  // make changes to desired sentences and/or baud rate
    send_ubx(testrun);   // save the configurations to non-volatile mem on the chip.
    // ---------------------------------- execution parameters

    uart_rx_setup();  // initialize UART Rx on the pico
    while (1)
        tight_loop_contents();
}


void on_uart_rx() {
    // just go line by line, no 
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        printf("%c", ch);
    }

    // size_t len = 256;  // size of the buffer in bytes
    // char buffer[len];  // make a buffer of size `len` for the raw message
    // uart_read_blocking(UART_ID, buffer, len);
    // for (int i=0; i<len-1; i++) {
    //     // print the hex message
    //     // printf("%02X", buffer[i]);
    // }
    // printf("s: %s\n-------------\n", buffer);
}


int get_checksum(char *string) {
    // adapted from: https://github.com/craigpeacock/NMEA-GPS/blob/master/gps.c
    char *checksum_str;
	// int checksum;
	int calculated_checksum = 0;
    // printf("calculating checksum\n");
    char duplicate[strlen(string)];
    strcpy(duplicate, string); // preserve the original string 

	// Checksum is postcede by *
	checksum_str = strchr(duplicate, '*');
	if (checksum_str != NULL){
		// Remove checksum from duplicate
		*checksum_str = '\0';
		// Calculate checksum, starting after $ (i = 1)
		for (int i = 1; i < strlen(duplicate); i++) {
			calculated_checksum = calculated_checksum ^ duplicate[i];  // exclusive OR
		}
        // printf("Calculated checksum (int): %u\n", calculated_checksum);
        return calculated_checksum;
	} else {
		// printf("Error: Checksum missing or NULL NMEA message\r\n");
		return 0;
	}
	return 0;
}


void compile_message(char *nmea_msg, char *raw_msg,
                     char *checksum, char *terminator) {
    // add each component to the `nmea_msg` array
    strcat(nmea_msg, raw_msg);     // add the base message
    strcat(nmea_msg, checksum);    // add the checksum
    strcat(nmea_msg, terminator);  // finally, add the termination sequence
    // printf("\ncatted: %s\n", nmea_msg);
}


void uart_tx_setup(void) {
    // initialize UART on the pico but only what's needed for transmission
    // so that the writes aren't interrupted by interrupts when the module
    // starts spitting out GNSS data.
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, false);
}


void uart_rx_setup(void) {
    // finish initializing the RX UART...
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
}


int extract_baud_rate(char *string) {
    // extract the new baud from the message
    printf("extracting baud rate...\n");
    char *token;
    token = strtok(string, ",");
    for (int i=0; i<5; i++) {
        // skip to the 5th field of the message
        token = strtok(NULL, ",");
    }
    return atoi(token);
}

void fire_ubx_msg(uint8_t *msg, size_t len) {
    printf("Firing UBX message...\n", msg);
    for (int i=0; i<100; i++) {
        uart_write_blocking(UART_ID, msg, len);
        busy_wait_ms(i*10);  // rbf
    }
}

void fire_nmea_msg(char *msg) {
    printf("Firing NMEA message: %s\n", msg);
    for (int k = 0; k < 5; k++) {
        // send out the message multiple times. BAUD_RATE in particular needs this treatment.
        for (int i=0; i<strlen(msg); i++) {
            // write the message char by char.
            uart_putc_raw(UART_ID, msg[i]);
        }
    }
}

void send_nmea(int testrun, int changing_baud) {
    // below are some NMEA PUBX messages to be modified as needed.
    // checksum values (immediately following `*`) are generated automatically

    // configure port scheme for NMEA PUBX messages and dynamically construct messages
    // 1 means enable, 0 for disable. Below, the 0th element is DDC, 1st is USART1,
    // 2nd is USART2, 3rd is USB, 4th is SPI. eg. "0,1,0,0" will enable all given identifiers on USART1
    char enable[] = ",0,1,0,0*";   // enable on USART1 and disable all other ports
    char disable[] = ",0,0,0,0*";  // disable on all ports
    char pub40_prefix[] = "$PUBX,40,";

    // modify these as needed:
    char *disable_identifiers[] = { "GSA", "RMC", "GSV", "VTG", "GLL"};  // combined with `disable`
    char *enable_identifiers[] = { "GGA", "ZDA" };  // gets combined these with `enable` char array

    // this is a PUBX 41 message, no automated composition, just append this to the messages array as-is
    char update_baud_rate[] = "$PUBX,41,1,3,3,115200,0*";  // update baud rate
    int num_disables = sizeof(disable_identifiers) / sizeof(disable_identifiers[0]);
    int num_enables = sizeof(enable_identifiers) / sizeof(enable_identifiers[0]);
    char *messages[num_enables + num_disables + changing_baud];
    int msg_count = 0;

    // construct the disabling messages
    for (int i=0; i < num_disables; i++) {
        char raw_msg[21];
        strcpy(raw_msg, ""); // get rid of junk values
        strcat(raw_msg, pub40_prefix);
        strcat(raw_msg, disable_identifiers[i]);
        strcat(raw_msg, disable);
        messages[msg_count++] = strdup(raw_msg);
    }

    // construct the enabling messages
    for (int i=0; i < num_enables; i++) {
        static char raw_msg[21];
        strcpy(raw_msg, "");  // get rid of junk values
        strcat(raw_msg, pub40_prefix);
        strcat(raw_msg, enable_identifiers[i]);
        strcat(raw_msg, enable);
        messages[msg_count++] = strdup(raw_msg);
    }

    if (changing_baud == 1) {
        // add the baud rate message, if applicable
        messages[msg_count++] = update_baud_rate;  // strdup not needed since `update_baud_rate` wont't be overwritten
    }

    for (int i=0; i < msg_count; i++) {
        int decimal_checksum;  // placeholder for the integer value checksum checksum
        decimal_checksum = get_checksum(messages[i]);  // calc the hex checksum and write it to the `checksum` array
        char checksum[2];  // placeholder for hexadecimal checksum
        strcpy(checksum, "");  // initialize to empty string to avoid junk values
        sprintf(checksum, "%X", decimal_checksum);  // convert the decimal checksum to hexadecimal
        // itoa(cs, checksum, 16);  // alternative to sprintf()
        // printf("%s", checksum);  // for debugging
        char msg_terminator[] = "\r\n";  // NMEA sentence terminator <cr><lr> == "\r\n"
        char nmea_msg[strlen(messages[i]) + strlen(msg_terminator) + strlen(checksum)];  // placeholder for final message
        strcpy(nmea_msg, "");  // initialize to empty string to avoid junk values
        compile_message(nmea_msg, messages[i], checksum, msg_terminator);  // assemble the components into the final msg

        if (!testrun) {
            fire_nmea_msg(nmea_msg);
            if (strncmp(nmea_msg, update_baud_rate, 23) == 0) {
                int new_baud;
                new_baud = extract_baud_rate(update_baud_rate);
                // update the pico's UART baud rate to the newly set value.
                printf("updating baud rate to %d\n", new_baud);
                uart_set_baudrate(UART_ID, new_baud);
            }
        }
    }
}

void send_ubx(int testrun) {
    // cfg_cfg_save_all will cause the changes to be permanent and persist through power cycles.

    // this is currently not working. the config will persist for a short period after disconnecting
    // the module, but after several hours the config will be lost. This is likely saving to battery-backed RAM
    // and once the battery discharges it will be erased. Need to configure to write it to flash.
    // the TBS m8.2 supposedly has onboard flash, run UBX-LOG-INFO to verify.
    uint8_t load_last_flash[] = {
        0xB5,0x62,0x06,0x09,0x0D,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x12,0x2C,0xC2
    };
    uint8_t cfg_cfg_flash_all[] = {
        0xB5,0x62,0x06,0x09,0x0D,0x00,0x00,0x00,0x00,0x00,0xFF,
        0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x12,0x2C,0xBA
    };

    uint8_t cfg_cfg_spi_flash[] = {
        0xB5,0x62,0x06,0x09,0x0D,0x00,0x00,0x00,0x00,0x00,0xFF,
        0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x2A,0xB8
    };

    // save to battery-backed RAM
    uint8_t cfg_cfg_bbr[] = {
        0xB5,0x62,0x06,0x09,0x0D,0x00,0x00,0x00,0x00,0x00,0xFF,
        0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x1B,0xA9
    };

    uint8_t cfg_cfg_bbr_flash_spiflash[] = {
        0xB5,0x62,0x06,0x09,0x0D,0x00,0x00,0x00,0x00,0x00,0xFF,
        0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x13,0x2D,0xBB
    };

    uint8_t cfg_cfg_i2c_eeprom[] = {
        0xB5,0x62,0x06,0x09,0x0D,0x00,0x00,0x00,0x00,0x00,0xFF,
        0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x1E,0xAC
    };
    uint8_t revert_bbr_to_default[] = { 
        0xB5,0x62,0x06,0x09,0x0D,0x00,0xFF,0xFF,0x00,0x00,0x00,
        0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x01,0x19,0x98
    };

    // below changes baud rate to `115200`
    uint8_t cfg_prt[] = {
        0xB5,0x62,0x06,0x00,0x14,0x00,0x01,0x00,0x00,0x00,
        0xD0,0x08,0x00,0x00,0x00,0xC2,0x01,0x00,0x07,0x00,
        0x03,0x00,0x00,0x00,0x00,0x00,0xC0,0x7E
    };
    
    // create a flash file
    uint8_t log_create[] = {
        0xB5,0x62,0x21,0x07,0x08,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x30,0x29
    };

    // pick the desired message and send it.
    if (!testrun) {
        fire_ubx_msg(cfg_cfg_bbr, sizeof(cfg_cfg_bbr));
        // // busy_wait_ms(500);
        
        printf("done firing UBX\n");
    }
}

