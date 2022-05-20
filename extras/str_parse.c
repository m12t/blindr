#include <string.h>
#include <stdio.h>

int parse_comma_delimited_str(char *string, char **fields, int max_fields);
void parse_buffer(char *buffer, char **sentences);


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
        eol = strtok(NULL, "\n\r");
    }
}

int parse_line(char *buffer, char **fields, int max_fields) {
    int i = 0;
	fields[i++] = buffer;

	while ((i < max_fields) && NULL != (buffer = strchr(buffer, ','))) {
        if (buffer[0] == '$') {  // is this needed now that lines are split by `\r\n` ???
            printf("breaking out\n");
            break;
        }
		*buffer = '\0';  // change the comma to an end of string NULL character
		fields[i++] = ++buffer;
	}
	return --i;
}

int parse_comma_delimited_str(char *string, char **fields, int max_fields) {
   int i = 0;
   fields[i++] = string;
   while ((i < max_fields) && NULL != (string = strchr(string, ','))) {
      *string = '\0';
      fields[i++] = ++string;
   }
   return --i;
}


int main() {
	char buffer[] = "$GNGSA,4554,2322,1111,5555\r\n$GNGPA,aa,bb,cc,dd,ee,ff\r\n$GARMC,q1,q2,q3,q4,q5,q6\r\n";  // simulated NMEA data
	char *sentences[8];  // array of pointers pointing to the location of the start of each sentence
	// char **fields[20];  // location of each field of each sentence
	// char line[] = "$GNSS,4554,2322,1111,5555";
	char *field[20];
	
	parse_buffer(buffer, sentences);  // split the monolithic buffer into discrete sentences

	// printf("1: %s\n\n", sentences[0]);
	// printf("2: %s\n\n", sentences[1]);
	// printf("3: %s\n\n", sentences[2]);
	// printf("4: %s\n\n", sentences[3]);

	int i = 0;
	while (sentences[i] != NULL) {
		printf("loop: %s\n", sentences[i++]);
	}
	
	// parse_comma_delimited_str(sentences[0], field, 20);
	// printf("%s\n", field[0]);
	// printf("%s\n", field[1]);
	// printf("%s\n", field[2]);
	// printf("%s\n", field[3]);
	// printf("%s\n", field[4]);
}

	
