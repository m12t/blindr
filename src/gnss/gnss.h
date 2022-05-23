
void parse_buffer(char *buffer, char **sentences);
int parse_line(char *string, char **fields, int num_fields);
int checksum_valid(char *string);
void on_uart_rx(void);
void setup(void);
int hex2int(char *c);
int hexchar2int(char c);
void parse_zda(char **zda_msg, int16_t *year, int8_t *month, int8_t *day,
               int8_t *hour, int8_t *min, int8_t *sec);
void parse_gga(char **gga_msg, float *latitude, int *north,
               float *longitude, int *east);

