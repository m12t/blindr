#include <string.h>
#include <stdio.h>

int parse_comma_delimited_str(char *string, char **fields, int max_fields);
void parse_comma_delimited_str_inplace(char **string, int max_fields);
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
    while (eol != NULL) {
		sentences[i++] = eol;
        eol = strtok(NULL, "\n\r");  // https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-strtok-tokenize-string
    }
	sentences[i-1] = NULL;  // NULL out the last entered row as it can't be guaranteed to be complete due to strtok()
}


void parse_comma_delimited_str_inplace(char **string, int max_fields) {
	printf("jj");
    printf("%s", *string);
	int i = 0;
	char *fields[max_fields];
	fields[i] = string[0];
	while ((i < max_fields-1) && NULL != (*string = strchr(*string, ','))) {
    	**string = '\0';
    	fields[++i] = ++*string;
    }
    printf("\nip1: %s\n", fields[0]);
    printf("ip2: %s\n", fields[1]);
    printf("ip3: %s\n", fields[2]);
    printf("ip4: %s\n", fields[3]);
    printf("ip5: %s\n", fields[4]);
    printf("ip6: %s\n", fields[5]);
    // string = fields;  // ??
}

int main() {
	char buffer[] = "$GNGGA,4554,2322,,1111,5555\r\n$GNVTG,aa,bb,cc,dd,ee,ff\r\n$GARMC,q1,q2,q3,q4,q5,q6\r\n$BAD, 44";  // simulated NMEA data
	char *sentences[8];  // array of pointers pointing to the location of the start of each sentence
	// char **fields[8];  // array of pointers to pointers
	
	parse_buffer(buffer, sentences);  // split the monolithic buffer into discrete sentences

	// printf("1: %s\n\n", sentences[0]);
	// printf("2: %s\n\n", sentences[1]);
	// printf("3: %s\n\n", sentences[2]);
	// printf("4: %s\n\n", sentences[3]);

	int i = 0;
	while (sentences[i] != NULL) {
		if (strstr(sentences[i], "GGA")) {
			// https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf
			int num_gga_fields = 17;
			// char *ggafields[num_gga_fields];
			// parse_comma_delimited_str(sentences[i], ggafields, num_gga_fields);
			printf("h1");
			parse_comma_delimited_str_inplace(&sentences[i], num_gga_fields);
			// printf("found GGA:\n%s\n", sentences[i]);
			// printf("\n%s\n", sentences[i+1]);
			// printf("\n%s\n", sentences[i+2]);
			// printf("\n%s\n", sentences[i+3]);
			// printf("%s\n", ggafields[0]);
			// printf("%s\n", ggafields[1]);
			// printf("%s\n", ggafields[2]);
			// printf("%s\n", ggafields[3]);
			// printf("%s\n", ggafields[4]);
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
