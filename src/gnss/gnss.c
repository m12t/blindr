#include "../blindr.h"  // for global defines
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



// int setup_gnss_dma(char *buffer) {
//     const char src[] = "$GNZDA,2022,05,29,11,01,51,0,0\r\n,$GNGGA,4243.323,01100.0332,00,12\r\n";  // rbf
//     char dst[256];  // rbf - this is an address passed in by main

//     uint offset = pio_add_program(pio0, &pio_serialiser_program);
//     pio_serialiser_program_init(pio0, 0, offset, 17, 18);

//     int chan = dma_claim_unused_channel(true);
//     dma_channel_config c = dma_channel_get_default_config(chan);
//     channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
//     channel_config_set_write_increment(&c, true);
//     channel_config_set_dreq(&c, DREQ_PIO0_RX0);

//     dma_channel_configure(
//         chan,          // Channel to be configured
//         &c,            // The configuration we just created
//         buffer,        // The initial write address             <-- this is a variable in main() whose address gets passed into the gnss subdir to solve the issue of global vars...
//         src,           // The initial read address
//         count_of(src), // Number of transfers; in this case each is 1 byte.   <-- 256??
//         true           // Start immediately.
//     );

// }