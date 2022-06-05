#include "gnss.h"
#include "../utils/utils.h"   // would be better to set the include path and just do utils/utils.h...


// implement a ring buffer for the UART messages received
uint buffer_pos = 0, sentences_pos = 0;
char buffer[buffer_len];  // make a buffer of size `buffer_len` for the raw message
char *sentences[sentences_len] = {NULL};


void split_buffer(char *buffer, char **sentences, int max_sentences) {
    /*
    split the monolithic buffer into discrete NMEA
    sentences which are terminated by <cr><lf> aka `\r\n`
    */
    int i = 0;
    int first_valid_index;
    char *bol, *eol;  // beginning of line, end of line
    // printf("full buffer: %s\n", buffer);
    bol = strchr(buffer, '$');  // remove all chars from the last incomplete sentence
    if (bol)
        printf("char at fvi: %c\n", buffer[first_valid_index]);
    // first_valid_index = (int)(bol - buffer);
    // printf("first valid index: %d\n", first_valid_index);
    eol = strtok(&buffer[first_valid_index], "\n\r");
    while (eol != NULL && i < max_sentences) {
		sentences[sentences_pos++] = eol;
        sentences_pos %= sentences_len;
        eol = strtok(NULL, "\n\r");  // https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-strtok-tokenize-string
        i++;
    }
    printf("parsed: %s\n", sentences[i]);
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
    int whole_degrees = (*position/100);  // implicit conversion to int after floating point arithmetic (faster)
    // get the last 2 digits before the decimal denoting whole minutes:
    int whole_minutes = (*position - 100*whole_degrees);
    // grab everything after the decimal (the partial minutes):
    double partial_minutes = *position - (int)*position;
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
    if (*gnss_fix && !(*latitude || *longitude)) {
        // * nothing has been set yet and there is a gnss fix so the data are presumed valid.
        // * the check for !latitude allows this function to be called after a sleep without
        //   modifying the latitude and longitude data unnecessarily yet still relaying if
        //   a gnss_fix has been found.
        *latitude = atof(gga_msg[2]);
        *north = (toupper(gga_msg[3][0]) == 'N') ? 1 : 0;
        *longitude = atof(gga_msg[4]);
        *east = (toupper(gga_msg[5][0]) == 'E') ? 1 : 0;

        to_decimal_degrees(latitude, north);
        to_decimal_degrees(longitude, east);
    }
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
        // printf("string: %s\n", string);
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


void parse_buffer() {
    // the buffer is full, extract its data
    // printf("parsing...\n");
    split_buffer(buffer, sentences, sizeof(sentences)/sizeof(sentences[0]));
    int i=0, valid=0, msg_type = 0, num_fields=0;
	while (sentences[i]) {
        // printf("sentences[%d]: \n%s\n", i, sentences[i]);  // rbf
        // printf("gnss fix: %d\n", gnss_fix);
        num_fields = 0;     // reset each iteration
		if (strstr(sentences[i], "GGA")) {
			num_fields = 18;  // 1 more
            msg_type = 1;
            valid = 1;
		} else if (strstr(sentences[i], "ZDA")) {
			num_fields = 10;  // 1 more
            msg_type = 2;
            valid = 1;
		} else {
            valid = 0;
        }
        if (valid && checksum_valid(sentences[i])) {
            char *fields[num_fields];
            parse_line(sentences[i], fields, num_fields);
            if (msg_type == 1) {
                // always parse this as it has the `gnss_fix` flag
                parse_gga(fields, &latitude, &north, &longitude, &east, &gnss_fix);
                // printf("lon: %f, lat: %f\n", longitude, latitude);  // rbf
            } else if (msg_type == 2 && gnss_fix) {
                // only parse if there is a fix
                parse_zda(fields, &year, &month, &day, &hour, &min, &sec);
                // printf("%d/%d/%d %d:%d:%d\n", year, month, day, hour, min, sec);
                if (latitude && longitude && year && month && day && (day || hour || sec)) {
                    // last check the values are atleast nonzero.
                    // NOTE: for the (day || hour || sec), day hour and sec == 0 are valid,
                    // but the next second won't be
                    get_utc_offset(longitude, &utc_offset);
                    localize_datetime(&year, &month, &day, &hour, utc_offset);
                    set_onboard_rtc(year, month, day, hour, min, sec);
                    // rtc is running and lat and long are set by nature of gnss_fix=1. shut down UART.
                    printf("final lat long: %f, %f\n", latitude, longitude);  // rbf
                    printf("final time: %d/%d/%d %d:%d:%d\n", year, month, day, hour, min, sec);
                    gnss_read_successful = 1;
                    gnss_deinit(0);
                }
            } else {}
        }
		i++;
	}
}


// RX interrupt handler
void on_uart_rx(void) {
    uint8_t ch;
    while (uart_is_readable(UART_ID)) {
        if (ch = uart_getc(UART_ID)) {
            printf("%c", ch);
            buffer[buffer_pos++] = ch;
            buffer_pos %= buffer_len;  // ring buffer
            if (buffer_pos == 255) {
                parse_buffer();
            }
        }
    }
}


void gnss_init(void) {
    gnss_running = 1;
    gnss_read_successful = 0;

    printf("initializing gnss\n");  // rbf
    if (!uart_is_enabled(UART_ID)) {
        uart_init(UART_ID, baud_rate);
    }

    uart_tx_setup();  // initialize UART Tx on the pico

    wake_gnss();
    config_gnss();  // make changes to desired sentences and baud rate

    uart_rx_setup();

    manage_gnss_connection();
}

int manage_gnss_connection(void) {
    // set an initial timeout for any signals on the specified UART port.
    // this is used to check for if the module is connected. If not, exit.
    // then, check for valid data for a longer timeout before eventually exiting.
    for (int i=0; i<500; i++) {  // 30 second timeout for whether or not a device is found
        if (buffer[0]) {  // see if anything was written  
            break;
        }
        sleep_ms(60);
    }
    if (!buffer[0]) {
        printf("no gnss found \n");
        gnss_deinit(1);
        return 0;
    }

    for (int i=0; i<600; i++) {  // i < 600
        // 10 minute timeout on getting a position fix
        if (gnss_read_successful) {
            break;
        }
        sleep_ms(1e3);
    }
    if (!gnss_read_successful) {
        // the connection timed out before a fix and therefore the gnss_deinit()
        // above was never reached. deinitialize the gnss here.
        printf("conn failed\n");
        gnss_deinit(0);
        return 0;
    }
    return 1;
}


void wake_gnss(void) {
    // put the GNSS module in `continuous` mode
    printf("waking the gnss module on baud: %d ...\n", baud_rate);  // rbf
    uint8_t wake_msg[] = {  // power on
        0xB5,0x62,0x02,0x41,0x10,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x08,0x00,
        0x00,0x00,0x61,0x6B
    };
    uint8_t wake_msg_force[] = {  // power on
        0xB5,0x62,0x02,0x41,0x10,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x08,0x00,
        0x00,0x00,0x5D,0x4B
    };
    fire_ubx_msg(wake_msg, sizeof(wake_msg));
    busy_wait_ms(250);  // give the gnss chip a chance to wake before sending config messages
}


void sleep_gnss(void) {
    // put the GNSS module in `power save` mode
    printf("sleeping the gnss module...\n");  // rbf
    uint8_t sleep_msg[] = {  // sleep indefinitely
        0xB5,0x62,0x02,0x41,0x08,0x00,0x00,0x00,0x00,0x00,
        0x02,0x00,0x00,0x00,0x4D,0x3B
    };
    fire_ubx_msg(sleep_msg, sizeof(sleep_msg));
    busy_wait_ms(250);  // give the gnss chip a chance to enter low power mode
}


void gnss_deinit(int fully) {
    // sleep the gnss module
    printf("deinitializing gnss\n");
    // disable IRQ *before* sending the final messages because the
    // module will send out loads of spam
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
    irq_set_enabled(UART_IRQ, false);
    sleep_gnss();

    if (fully) {
        // only fully deinitialize uart if there is no gnss at all.
        uart_deinit(UART_ID);
    }

    gnss_fix = 0;    // important to force a reset for the next startup
    gnss_running = 0;  // turn of the flag, freeing executing in set_next_alarm()
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
    // printf("done with rx setup\n");
}


void compile_message(char *nmea_msg, char *raw_msg,
                     char *checksum, char *terminator) {
    // add each component to the `nmea_msg` array
    strcat(nmea_msg, raw_msg);     // add the base message
    strcat(nmea_msg, checksum);    // add the checksum
    strcat(nmea_msg, terminator);  // finally, add the termination sequence
    // printf("\ncatted: %s\n", nmea_msg);
}


uint extract_baud_rate(char *string) {
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

void fire_ubx_msg(uint8_t *msg, size_t msg_len) {
    // printf("Firing UBX message 5 times...\n", msg);
    for (int i=0; i<5; i++) {
        uart_write_blocking(UART_ID, msg, msg_len);
        busy_wait_ms(i*10);
    }
}

void fire_nmea_msg(char *msg) {
    // printf("Firing NMEA message: %s\n", msg);
    for (int k=0; k<5; k++) {
        // send out the message multiple times. BAUD_RATE in particular needs this treatment.
        for (int i=0; i<strlen(msg); i++) {
            // write the message char by char.
            uart_putc_raw(UART_ID, msg[i]);
        }
    }
}

void config_gnss() {
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
    char update_baud_rate[] = "$PUBX,41,1,3,3,115200,0*";  // update baud rate to 115200
    int num_disables = sizeof(disable_identifiers) / sizeof(disable_identifiers[0]);
    int num_enables = sizeof(enable_identifiers) / sizeof(enable_identifiers[0]);
    char *messages[num_enables + num_disables];
    int msg_count = 0;

    messages[msg_count++] = update_baud_rate;

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

        // if the gnss is already config'd for baud=115200, none of the
        // messages will be sent, but they aren't needed because the gnss
        // is already properly config'd. Nonetheless, this will update the
        // pico uart baud to 115200 in order for it to extract lat, long,
        // and datetime from the incoming messages
        fire_nmea_msg(nmea_msg);
        if (strncmp(nmea_msg, update_baud_rate, 23) == 0) {
            baud_rate = extract_baud_rate(update_baud_rate);
            // update the pico's UART baud rate to the newly set value.
            // printf("updating baud rate to %d\n", new_baud);
            uart_set_baudrate(UART_ID, baud_rate);
        }
    }
}

