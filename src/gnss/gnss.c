#include "../blindr.h"
#include "gnss.h"


void parse_buffer(char *buffer, char **sentences, int max_sentences) {
    /*
    split out the buffer into individual NMEA sentences
    which are terminated by <cr><lf> aka `\r\n`
    */
    printf("max sentences: %d\n", max_sentences);
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
}

void get_utc_offset(double longitude, uint8_t *utc_offset, int8_t month, int8_t day) {
    *utc_offset = (int)(longitude / 15);
    // do the solar equations actually need daylight savings and offset??
    if ((month > 3 || (month == 3 && day >= 13)) && (month < 11 || (month == 11 && day <= 6))) {
        // a really hacky solution (not exactly on these dates every year)
        // but then again without a large table daylight savings itself is hacky.
        // for example, Arizona doesn't subscripe to daylight savings... so make a 
        // coordinate geobox around AZ? surely not...
        *utc_offset += 1;
        printf("DST found!\n");  // rbf
    }
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
    size_t len = 512;
    char buffer[len];  // make a buffer of size `len` for the raw message
    char *sentences[16] = {NULL};  //initialize an array of NULL pointers that will pointing to the location of the start of each sentence within buffer
    uart_read_blocking(UART_ID, buffer, len);  // read the message into the buffer
    parse_buffer(buffer, sentences, sizeof(sentences)/sizeof(sentences[0]));  // split the monolithic buffer into discrete sentences

    int i=0, valid=0, msg_type = 0, num_fields=0;
	while (sentences[i]) {
        printf("sentences[%d]: \n%s\n", i, sentences[i]);  // rbf
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
                // this only needs to run once on startup.
                parse_gga(fields, &latitude, &north, &longitude, &east, &gnss_fix);
                printf("latitude:  %f, longitude: %f\n", latitude, longitude);  // rbf
                printf("gnss_fix: %d\n", gnss_fix);  // rbf
            } else if (msg_type == 2 && gnss_fix) {
                // only parse if there is a fix
                // keep a variable clock_last_set to a date and only reset the clock after a certain amount of time
                // to avoid constantly changing it.
                parse_zda(fields, &year, &month, &day, &hour, &min, &sec);
                get_utc_offset(longitude, &utc_offset, month, day);
                // enact_utc_offset(&year, &month, &day, &hour, utc_offset);  // carful about bug where you get a negative hour...
                set_onboard_rtc(year, month, day, hour, min, sec, utc_offset);
                // TODO: fix bug where utc time is less than the offset and the clock time is negative...
                printf("%d/%d/%d %d:%d:%d\n", month, day, year, hour+utc_offset, min, sec);  // rbf
            } else {}
        }
		i++;
	}
    printf("-----------------------\n");
}

void setup(void) {
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


int gnss_main(void) {
    setup();

    while (1)
        tight_loop_contents();
}
