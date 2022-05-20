
void parse_buffer(char *buffer, char **sentences);
int parse_line(char *string, char **fields, int num_fields);
int checksum_valid(char *string);
void on_uart_rx(void);
void setup(void);
int main(void);
int hex2int(char *c);
int hexchar2int(char c);