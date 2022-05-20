#include <string.h>
#include <stdio.h>

int parse_comma_delimited_str(char *string, char **fields, int max_fields);
void parse_buffer(char *buffer, char **sentences);
int parse_line(char *string, char **fields, int max_fields);
int get_num_fields(char *string);


void parse_buffer(char *buffer, char **sentences) {
    /*
    split out the buffer into individual NMEA sentences
    which are terminated by <cr><lf> aka `\r\n`
    */
    int i = 0;
    char *eol;  // end of line
    eol = strtok(buffer, "\n\r");
	printf("%s", eol);
    while (eol != NULL) {
		sentences[i++] = eol;
        eol = strtok(NULL, "\n\r");  // https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-strtok-tokenize-string
    }
	sentences[i-1] = NULL;  // remove the last entered row as it can't be guaranteed to be complete
}

int get_num_fields(char *string) {
    // search for the numebr of `,` in the sentence to create the appropriate size array
	int count = 0;
	int len;
	len = strlen(string);
	for (int i = 0; i < len; i++) {
		if (string[i] == ",") {
			count++;
		}
	}
	return ++count;  // field count is one greater than the number of commas
}


int parse_line(char *string, char **fields, int num_fields) {
    int i = 0;
    fields[i++] = string;
    while ((i < num_fields) && NULL != (string = strchr(string, ','))) {
        *string = '\0';
        fields[i++] = ++string;
    }
    printf("ip1: %s\n", fields[0]);  // DAT
    printf("ip2: %s\n", fields[1]);  // DAT
    return --i;
}

int main() {
	char buffer[] = "$GNGGA,4554,2322,1111,5555\r\n$GNVTG,aa,bb,cc,dd,ee,ff\r\n$GARMC,q1,q2,q3,q4,q5,q6\r\n$BAD, 44";  // simulated NMEA data
	char *sentences[8];  // array of pointers pointing to the location of the start of each sentence within buffer
	
	parse_buffer(buffer, sentences);  // split the monolithic buffer into discrete sentences

	// debug: print the sentences
	printf("1: %s\n\n", sentences[0]);
	printf("2: %s\n\n", sentences[1]);
	printf("3: %s\n\n", sentences[2]);
	printf("4: %s\n\n", sentences[3]);

	int i = 0;
	while (sentences[i] != NULL) {
		if (strstr(sentences[i], "GGA")) {
			// https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf
			int num_gga_fields = 17;
			int actual_num_fields;
			actual_num_fields = get_num_fields(string);
			num_fields = actual_num_fields < max_fields ? actual_num_fields : max_fields;  // min of the two
			char *gga_fields[num_fields];
			// parse_comma_delimited_str(sentences[i], ggafields, num_gga_fields);
			int gga_populated;
			gga_populated = parse_line(sentences[i], gga_fields, num_gga_fields);
			printf("found GGA:\n%s\n", sentences[i]);  // DAT
			for (int j = 0; j <= gga_populated; j++) {
				printf("%d: %s\n", j, gga_fields[j]);
			}
		} else if (strstr(sentences[i], "VTG")) {
			printf("loop: %s\n", sentences[i]);
		}
		i++;
	}
	
	// parse_comma_delimited_str(sentences[0], field, 20);
	// printf("%s\n", field[0]);
	// printf("%s\n", field[1]);
	// printf("%s\n", field[2]);
	// printf("%s\n", field[3]);
	// printf("%s\n", field[4]);
}
