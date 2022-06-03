#include "gnss.h"
#include "../utils/utils.h"   // would be better to set the include path and just do utils/utils.h...


uint lat_long_set = 0;

void parse_buffer(char *buffer, char **sentences, int max_sentences) {
    /*
    split out the buffer into individual NMEA sentences
    which are terminated by <cr><lf> aka `\r\n`
    */
    int i = 0;
    char *eol;  // end of line
    eol = strtok(buffer, "\n\r");
    while (eol != NULL && i < max_sentences) {
		sentences[i++] = eol;
        eol = strtok(NULL, "\n\r");  // https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-strtok-tokenize-string
    }
    if (i > 0) {
        // NULL out the last entered row as it can't be guaranteed to be complete due to strtok()
	    sentences[i-1] = NULL;
    }
}

void parse_utc_time(char *time, int8_t *hour, int8_t *min, int8_t *sec) {
    // extract the substrings for hh, mm, ss from the UTC string of format: hhmmss.ss
    // used with both ZDA and GGA data
    *hour = 10 * (time[0] - '0') + (time[1] - '0');
    *min  = 10 * (time[2] - '0') + (time[3] - '0');
    *sec  = 10 * (time[4] - '0') + (time[5] - '0');
}


void parse_zda(char **zda_msg, int16_t *year, int8_t *month, int8_t *day,
               int8_t *hour, int8_t *min, int8_t *sec) {
    // convert the char to int and formulate the int using tens and ones places.
    *year = atoi(zda_msg[4]);
    *month = atoi(zda_msg[3]);
    *day = atoi(zda_msg[2]);
    // extract the substrings for hh, mm, ss from the UTC string of format: hhmmss.ss
    parse_utc_time(zda_msg[1], hour, min, sec);
}

void to_decimal_degrees(double *position, int *direction) {
    // convert degrees and minutes to decimal degrees
    // get the first 2 (latitude) or 3 (longitude) digits denoting the degrees:
    int whole_degrees = (*position/100);               // implicit conversion to int after floating point arithmetic (faster)
    // get the last 2 digits before the decimal denoting whole minutes:
    int whole_minutes = (*position - 100*whole_degrees);
    // grab everything after the decimal (the partial minutes):
    double partial_minutes = *position - (int)*position;  // implicit conversion to int after floating point arithmetic (faster)
    *position = whole_degrees + (whole_minutes + partial_minutes) / 60;
    if (*direction == 0) {
        // direction is `S` or `W` -- make the position negative
        *position *= -1;  // make it negative
    }
}

void parse_gga(char **gga_msg, double *latitude, int *north,
               double *longitude, int *east, int *gnss_fix) {
    // id   : gga_msg[0]
    // time : gga_msg[1]
    // lat  : [2]
    // ns   : [3]  ('N' or 'S')
    // long : [4]
    // ew   : [5]  ('E' or 'W')
    *gnss_fix = atoi(gga_msg[6]);
    if (gnss_fix) {
        *latitude = atof(gga_msg[2]);
        *north = (toupper(gga_msg[3][0]) == 'N') ? 1 : 0;
        *longitude = atof(gga_msg[4]);
        *east = (toupper(gga_msg[5][0]) == 'E') ? 1 : 0;

        to_decimal_degrees(latitude, north);
        to_decimal_degrees(longitude, east);
    }
    lat_long_set = 1;  // flag for whether lat long data are set
}

void get_utc_offset(double longitude, int8_t *utc_offset) {
    *utc_offset = longitude / 15;
    // printf("longitude:  %f\n", longitude);  // rbf
    // printf("utc offset: %d\n", *utc_offset); // rbf
}

void parse_line(char *string, char **fields, int num_fields) {
    int i = 0;
    fields[i++] = string;
    // search for the numebr of `,` in the sentence to create the appropriate size array?
    while ((i < num_fields) && NULL != (string = strchr(string, ','))) {
        *string = '\0';
        fields[i++] = ++string;
    }
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
    size_t len = 256;
    char buffer[len];  // make a buffer of size `len` for the raw message
    char *sentences[16] = {NULL};  //initialize an array of NULL pointers that will pointing to the location of the start of each sentence within buffer
    uart_read_blocking(UART_ID, buffer, len);  // read the message into the buffer
    parse_buffer(buffer, sentences, sizeof(sentences)/sizeof(sentences[0]));  // split the monolithic buffer into discrete sentences

    int i=0, valid=0, msg_type = 0, num_fields=0;
	while (sentences[i]) {
        // printf("sentences[%d]: \n%s\n", i, sentences[i]);  // rbf
        num_fields = 0;     // reset each iteration
		if (strstr(sentences[i], "GGA")) {
			num_fields = 18;  // 1 more
            msg_type = 1;
            valid = 1;  // run the below. temporarily false for testing VTG
		} else if (strstr(sentences[i], "ZDA")) {
			num_fields = 10;  // 1 more
            msg_type = 2;
            valid = 1;  // run the below. temporarily false for testing VTG
		} else {
            msg_type = 0;  // not really necessary
            valid = 0;
        }
        if (valid && checksum_valid(sentences[i])) {
            char *fields[num_fields];
            parse_line(sentences[i], fields, num_fields);
            if (msg_type == 1) {
                // always parse this as it has the `gnss_fix` flag
                if (!lat_long_set) {
                    parse_gga(fields, &latitude, &north, &longitude, &east, &gnss_fix);
                    // printf("lon: %f, lat: %f\n", longitude, latitude);  // rbf
                }
                if (rtc_running()) {
                    // rtc is running and lat and long are set. shut down UART.
                    gnss_deinit();
                }
            } else if (msg_type == 2 && gnss_fix) {
                // only parse if there is a fix
                parse_zda(fields, &year, &month, &day, &hour, &min, &sec);
                if (longitude) {
                    get_utc_offset(longitude, &utc_offset);
                    localize_datetime(&year, &month, &day, &hour, utc_offset);
                    if (!rtc_running()) {
                        set_onboard_rtc(year, month, day, hour, min, sec);
                    } else {
                        if (lat_long_set) {
                            // rtc is running and lat and long are set. shut down UART.
                            gnss_deinit();
                        }
                    }
                }
                // set_onboard_rtc(&year, &month, &day, &hour, &min, &sec, &utc_offset);
            } else {}
        }
		i++;
	}
    // printf("-----------------------\n");  // rbf
}

void gnss_init(void) {
    // stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);

    uart_tx_setup();  // initialize UART Tx on the pico

    send_nmea(0, 1);  // make changes to desired sentences and/or baud rate
    send_ubx(0);   // save the configurations to non-volatile mem on the chip.

    uart_rx_setup();
}


void gnss_deinit(void) {
    // printf("deinitializing uart!\n");  // rbf
    // deinit uart
    uart_deinit(UART_ID);
    // disable IRQ
    irq_set_enabled(UART1_IRQ, false);
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


void uart_tx_setup(void) {
    // initialize UART on the pico but only what's needed for transmission
    // so that the writes aren't interrupted by interrupts when the module
    // starts spitting out GNSS data.
    
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);
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
}


void compile_message(char *nmea_msg, char *raw_msg,
                     char *checksum, char *terminator) {
    // add each component to the `nmea_msg` array
    strcat(nmea_msg, raw_msg);     // add the base message
    strcat(nmea_msg, checksum);    // add the checksum
    strcat(nmea_msg, terminator);  // finally, add the termination sequence
    // printf("\ncatted: %s\n", nmea_msg);
}


int extract_baud_rate(char *string) {
    // extract the new baud from the message
    // printf("extracting baud rate...\n");
    char *token;
    token = strtok(string, ",");
    for (int i=0; i<5; i++) {
        // skip to the 5th field of the message
        token = strtok(NULL, ",");
    }
    return atoi(token);
}

void fire_ubx_msg(uint8_t *msg, size_t len) {
    // printf("Firing UBX message...\n", msg);
    for (int i=0; i<100; i++) {
        uart_write_blocking(UART_ID, msg, len);
        busy_wait_ms(i*10);  // rbf
    }
}

void fire_nmea_msg(char *msg) {
    // printf("Firing NMEA message: %s\n", msg);
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
            // if the gnss is already config'd for baud=115200, none of the
            // messages will be sent, but they aren't needed because the gnss
            // is already properly config'd. Nonetheless, this will update the
            // pico uart baud to 115200 in order for it to extract lat, long,
            // and datetime from the incoming messages
            fire_nmea_msg(nmea_msg);
            if (strncmp(nmea_msg, update_baud_rate, 23) == 0) {
                int new_baud;
                new_baud = extract_baud_rate(update_baud_rate);
                // update the pico's UART baud rate to the newly set value.
                // printf("updating baud rate to %d\n", new_baud);
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
        
        // printf("done firing UBX\n");
    }
}

