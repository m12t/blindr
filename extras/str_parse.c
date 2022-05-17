#include <string.h>
#include <stdio.h>

int parse_comma_delimited_str(char *string, char **fields, int max_fields)
{
   int i = 0;
   fields[i++] = string;
   while ((i < max_fields) && NULL != (string = strchr(string, ','))) {
      *string = '\0';
      fields[i++] = ++string;
   }
   return --i;
}

int main() {
	char line[] = "$GNSS,4554,2322,1111,5555";
	char *field[20];
	parse_comma_delimited_str(line, field, 20);
	printf("%s\n", field[0]);
	printf("%s\n", field[1]);
	printf("%s\n", field[2]);
	printf("%s\n", field[3]);
	printf("%s\n", field[4]);
}

	
