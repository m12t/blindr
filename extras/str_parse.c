#include <string.h>
#include <stdio.h>

void parse_buffer(char *buffer, char **sentences);
int parse_line(char *string, char **fields, int num_fields);


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

void extract_gga(char *sentence, float *longitude, float *latitude, char *time) {
	*latitude = sentence[2];
	*longitude = sentence[4];
	// GPStime = parts[1][0:2] + ":" + parts[1][2:4] + ":" + parts[1][4:6];
}

int main() {
	char buffer[] = "$GNGGA,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,w,w,w,\r\n$GNZDA,aa,bb,cc,dd,ee,ff\r\n$GARMC,q1,q2,q3,q4,q5,q6\r\n$BAD, 44";  // simulated NMEA data
	char *sentences[8];  // array of pointers pointing to the location of the start of each sentence	
	parse_buffer(buffer, sentences);  // split the monolithic buffer into discrete sentences
	float *longitude;  // can extract these anywhere by passing pointers
	float *latitude;   // can extract these anywhere by passing pointers
	char *time;

	int i = 0;
	while (sentences[i] != NULL) {
		if (strstr(sentences[i], "GGA")) {
			// https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf
			int num_gga_fields = 18;  // 1 more
			char *gga_fields[num_gga_fields];
			int gga_populated;
			gga_populated = parse_line(sentences[i], gga_fields, num_gga_fields);
			printf("found GGA:\n%s\n", sentences[i]);  // DAT
			extract_gga(sentences[i], longitude, latitude, time);
			printf("%f", *longitude);
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
	
	// parse_comma_delimited_str(sentences[0], field, 20);
	// printf("%s\n", field[0]);
	// printf("%s\n", field[1]);
	// printf("%s\n", field[2]);
	// printf("%s\n", field[3]);
	// printf("%s\n", field[4]);
}
