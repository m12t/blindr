#include "gnss.h"
#include "gnss.pio.h"
#include <inttypes.h>  // rbf - for printing uint32_t


/* >>>>>>>>>>>>>>>>>>>>>>>>>>>> BEGIN INIT/DEINIT FUNCTIONS >>>>>>>>>>>>>>>>>>>>>>>>>>>> */

void gnss_init(double *latitude, double *longitude, uint *north, uint *east, int *utc_offset,
               uint *baud_rate, uint *gnss_read_successful, uint *gnss_configured, uint config_gnss,
               uint new_baud_rate, uint time_only, uint *data_found) {
    char buffer[BUFFER_LEN] __attribute__((aligned(BUFFER_LEN))) = { 0 };
    char *sentences[SENTENCES_LEN] = { NULL };
    uint gnss_fix = 0;

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &uart_rx_program);

    gnss_uart_tx_init(*baud_rate);
    wake_gnss();

    if (config_gnss) {
        configure_gnss(baud_rate, new_baud_rate);
    }

    uart_rx_program_init(pio, sm, offset, UART_RX_PIN, *baud_rate);

    gnss_dma_init(buffer);
    gnss_manage_connection(buffer, sentences, latitude, longitude, north, east,
                           utc_offset, gnss_read_successful, gnss_configured,
                           time_only, pio, sm, offset, &gnss_fix, data_found);
}


int gnss_manage_connection(char *buffer, char **sentences, double *latitude,
                           double *longitude, uint *north, uint *east, int *utc_offset,
                           uint *gnss_read_successful, uint *gnss_configured, uint time_only,
                           PIO pio, uint sm, uint offset, uint *gnss_fix, uint *data_found) {
    printf("managing gnss connection\n");
    char buffer_copy[BUFFER_LEN];
    for (int i=0; i<240; i++) {  // the main timeout loop of ~2 mins
        printf("--------------------------------\n");
        busy_wait_ms(500);
        if (*gnss_read_successful) {
            *gnss_configured = 1;
            return 1;  // deinit was already called, simply return.
        }
        if (!buffer[0] || !buffer[BUFFER_LEN-1]) {  // crude check for some values being found
            if (i > 60 && !*data_found) {
                printf("no signal found... this was buffer:\n----\n%s\n---\n", buffer);
                // ~30 second timeout for any signal. if nothing detected on UART, abort.
                gnss_deinit(pio, sm, offset);
                return 0;
            }
        } else {
            // copy buffer to avoid race condition
            *data_found = 1;
            *gnss_configured = 1;
            memcpy(buffer_copy, buffer, BUFFER_LEN);  // duplicate the buffer to avoid a race with DMA
            parse_buffer(buffer_copy, sentences, latitude, longitude, north, east, utc_offset,
                         gnss_read_successful, time_only, pio, sm, offset, gnss_fix);
        }
    }
    return 0;
}


void gnss_deinit(PIO pio, uint sm, uint offset) {
    printf("deinitializing gnss\n");  // rbf
    sleep_gnss();
    gnss_uart_deinit();
    gnss_dma_deinit();

    uart_rx_program_deinit(pio, sm);  // deinit the pio program
    pio_remove_program(pio, &uart_rx_program, offset);  // remove the pio assembly program to avoid the `NO PROGRAM SPACE` error

}


void gnss_dma_init(char *buffer) {
    printf("initializing DMA\n");
    dma_channel_claim(DMA_ID);
    dma_channel_config c = dma_channel_get_default_config(DMA_ID);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);  // test if DMA_SIZE_8 works here... plus simplification of uart_rx
    channel_config_set_read_increment(&c, false);    // read from the same spot (RX FIFO)
    channel_config_set_write_increment(&c, true);    // write around the ring buffer
    channel_config_set_ring(&c, true, 8);            // 2**8 = 256
    channel_config_set_dreq(&c, DREQ_PIO0_RX0);      // pio_get_dreq(pio, sm, false)

    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_W_BITS | BUSCTRL_BUS_PRIORITY_DMA_R_BITS;  // Grant high bus priority to the DMA

    dma_channel_configure(
        DMA_ID,
        &c,
        buffer,                   // Write address (only need to set this once)
        &pio0->rxf,
        (uint32_t)1e8,            // useful for a timeout in case dma_channel_abort() fails 1e8/115200 ~= 15 mins
        false
    );
    dma_channel_start(DMA_ID);
}


void gnss_dma_deinit(void) {
    printf("deinitializing DMA\n");
    if (dma_channel_is_busy(DMA_ID)) {
        dma_channel_abort(DMA_ID);  // CRITICAL SO THE NEXT DMA CAN START UP SUCCESSFULLY
    }
    if (dma_channel_is_claimed(DMA_ID)) {
        printf("UNCLAIMING>>>\n");
        dma_channel_unclaim(DMA_ID);
    }
}


void gnss_uart_tx_init(uint baud_rate) {
    printf("initializing UART\n");
    if (!uart_is_enabled(UART_ID)) {
        uart_init(UART_ID, baud_rate);
        gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);

        // // Set our data format
        uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
        uart_set_fifo_enabled(UART_ID, false);  // enabled by default but this helps the readability
        // uart_getc(UART_ID);  // clear one junk value... needed?
    }
}


void gnss_uart_deinit(void) {
    printf("inside deinit UART...\n");
    if (uart_is_enabled(UART_ID)) {
        printf("...deinitializing UART\n");
        uart_deinit(UART_ID);
    }
    // TODO: deinit the pio as well...
}


/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< END INIT/DEINIT FUNCTIONS <<<<<<<<<<<<<<<<<<<<<<<<<<<<< */


/* >>>>>>>>>>>>>>>>>>>>>>>>>>>> BEGIN DATA PARSING FUNCTIONS >>>>>>>>>>>>>>>>>>>>>>>>>>>> */
void parse_buffer(char *buffer, char **sentences, double *latitude, double *longitude,
                  uint *north, uint *east, int *utc_offset,
                  uint *gnss_read_successful, uint time_only, PIO pio,
                  uint sm, uint offset, uint *gnss_fix) {
    printf("parsing...\n");
    uint sentences_pos = 0;
    split_buffer(buffer, sentences, sizeof(sentences)/sizeof(sentences[0]), &sentences_pos);
    int i=0, valid=0, msg_type = 0, num_fields=0;
    int16_t year;
    int8_t month, day, hour, min, sec;
	while (sentences[i]) {
        printf("sentences[%d]: \n%s\n", i, sentences[i]);  // rbf
        printf("gnss fix: %d\n", *gnss_fix);
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
                parse_gga(fields, latitude, north, longitude, east, gnss_fix, time_only);
                // printf("lon: %f, lat: %f\n", longitude, latitude);  // rbf
            } else if (msg_type == 2 && *gnss_fix) {
                // only parse if there is a fix
                parse_zda(fields, &year, &month, &day, &hour, &min, &sec);
                printf("%d/%d/%d %d:%d:%d\n", year, month, day, hour, min, sec);
                if ((time_only || *latitude && *longitude) && year && month && day && (day || hour || sec)) {
                    // last check the values are atleast nonzero.
                    // NOTE: for the (day || hour || sec), day hour and sec == 0 are valid,
                    // but the next second won't be for long (<1s) so loop again until it is.
                    get_utc_offset(*longitude, utc_offset);
                    localize_datetime(&year, &month, &day, &hour, *utc_offset);
                    set_onboard_rtc(year, month, day, hour, min, sec);
                    // rtc is running and lat and long are set by nature of gnss_fix=1. deinitialize...
                    printf("final lat long: %f, %f\n", *latitude, *longitude);  // rbf
                    printf("final time: %d/%d/%d %d:%d:%d\n", year, month, day, hour, min, sec);
                    *gnss_read_successful = 1;
                    gnss_deinit(pio, sm, offset);
                }
            } else {}
        }
		i++;
	}
}


void split_buffer(char *buffer, char **sentences, int max_sentences, uint *sentences_pos) {
    /*
    split the monolithic buffer into discrete NMEA
    sentences which are terminated by <cr><lf> aka `\r\n`
    */
    int i = 0;
    int first_valid_index;
    char *bol, *eol;  // beginning of line, end of line
    // printf("full buffer: %s\n", buffer);
    bol = strchr(buffer, '$');  // remove all chars from the last incomplete sentence
    if (bol) {
        first_valid_index = (int)(bol - buffer);
        // printf("char at fvi: %c\n", buffer[first_valid_index]);
        // printf("first valid index: %d\n", first_valid_index);
        eol = strtok(&buffer[first_valid_index], "\n\r");
        while (eol != NULL && i < max_sentences) {
            sentences[*sentences_pos++] = eol;
            *sentences_pos %= SENTENCES_LEN;
            eol = strtok(NULL, "\n\r");  // https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-strtok-tokenize-string
            i++;
        }
        // for (int j=0; j < i; j++) {
        //     printf("parsed: %s\n", sentences[j]);
        // }
    }
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


void parse_utc_time(char *time, int8_t *hour, int8_t *min, int8_t *sec) {
    // extract the substrings for hh, mm, ss from the UTC string of format: hhmmss.ss
    // used with both ZDA and GGA data
    *hour = 10 * (time[0] - '0') + (time[1] - '0');
    *min  = 10 * (time[2] - '0') + (time[3] - '0');
    *sec  = 10 * (time[4] - '0') + (time[5] - '0');
}


void get_utc_offset(double longitude, int *utc_offset) {
    *utc_offset = longitude / 15;
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


void parse_gga(char **gga_msg, double *latitude, int *north,
               double *longitude, int *east, int *gnss_fix, uint time_only) {
    // id   : gga_msg[0]
    // time : gga_msg[1]
    // lat  : [2]
    // ns   : [3]  ('N' or 'S')
    // long : [4]
    // ew   : [5]  ('E' or 'W')
    *gnss_fix = atoi(gga_msg[6]);  // this runs regardless of the value of `time_only`, which is desirable.
    if (*gnss_fix && !(*latitude || *longitude) && !time_only) {
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
/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<< END DATA PARSING FUNCTIONS <<<<<<<<<<<<<<<<<<<<<<<<<<<< */


/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> BEGIN CONFIG FUNCTIONS >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
void configure_gnss(uint *baud_rate, uint new_baud_rate) {
    // * below are some NMEA PUBX messages to be modified as needed.
    // * checksum values (immediately following `*`) are generated automatically

    // * configure port scheme for NMEA PUBX messages and dynamically construct messages:
    //   1 means enable, 0 for disable. Below, the 0th element is enable on DDC, 1st for USART1,
    //   2nd for USART2, 3rd for USB, 4th for SPI. eg. "0,1,0,0" will enable all given identifiers on USART1
    char enable[] = ",0,1,0,0*";         // enable on USART1 and disable all other ports
    char disable[] = ",0,0,0,0*";        // disable on all ports
    char pub40_prefix[] = "$PUBX,40,";   // the prefix for all pub40 type NMEA messages

    // modify these as needed depending on the desired sentences:
    char *disable_identifiers[] = { "GSA", "RMC", "GSV", "VTG", "GLL" };  // combined with `disable`
    char *enable_identifiers[] = { "GGA", "ZDA" };  // gets combined these with `enable` char array

    uint msg_count = 0;
    uint num_disables = sizeof(disable_identifiers) / sizeof(disable_identifiers[0]);
    uint num_enables = sizeof(enable_identifiers) / sizeof(enable_identifiers[0]);
    char *messages[num_enables + num_disables + (new_baud_rate > 0)];  // +1 if the baud needs to be updated

    printf("Firing NMEA messages:\n----------------------\n\n");

    if (new_baud_rate > 0) {  // new_baud_rate of -1 is used to ignore this and not update the baud
        char update_baud_rate[32];  // to be populated
        build_baud_msg(update_baud_rate, new_baud_rate);
        messages[msg_count++] = strdup(update_baud_rate);
        // printf("update baud message: %s\n", update_baud_rate);
    }

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

        fire_nmea_msg(nmea_msg);
        printf("%s\n", nmea_msg);
        if ((new_baud_rate != *baud_rate) && new_baud_rate > 0) {
            printf("updating the baud rate to: %d on message number: %d\n", new_baud_rate, i);
            uart_set_baudrate(UART_ID, new_baud_rate);
            *baud_rate = new_baud_rate;  // update the global baud
        }
    }
    save_config();  // save the configuration to battery-backed RAM on the gnss module
}


void compile_message(char *nmea_msg, char *raw_msg,
                     char *checksum, char *terminator) {
    // add each component to the `nmea_msg` array
    strcat(nmea_msg, raw_msg);     // add the base message
    strcat(nmea_msg, checksum);    // add the checksum
    strcat(nmea_msg, terminator);  // finally, add the termination sequence
    // printf("\ncatted: %s\n", nmea_msg);
}


void build_baud_msg(char *msg, uint new_baud) {
    char update_baud_prefix[] = "$PUBX,41,1,3,3,";
    char update_baud_suffix[] = ",0*";  // checksum gets added later
    char new_baud_buff[8];
    snprintf(new_baud_buff, 8, "%d", new_baud);  // stringify new_baud so it can be used in `strcat` below

    strcpy(msg, "");  // get rid of junk values
    strcat(msg, update_baud_prefix);
    strcat(msg, new_baud_buff);
    strcat(msg, update_baud_suffix);
    // printf("built the new baud message: %s\n", msg);  // rbf
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


void wake_gnss(void) {
    // put the GNSS module in `continuous` mode
    printf("waking the gnss module\n");  // rbf
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


void save_config(void) {
    printf("Saving gnss configurations to BBR...\n");
    uint8_t cfg_save[] = {
        0xB5,0x62,0x06,0x09,0x0D,0x00,0x00,0x00,0x00,0x00,0xFF,
        0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x1B,0xA9
    };
    fire_ubx_msg(cfg_save, sizeof(cfg_save));
    busy_wait_ms(250);  // give the gnss chip a chance to save the config
}
/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< END CONFIG FUNCTIONS <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */