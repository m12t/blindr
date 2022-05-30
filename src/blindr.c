#include "blindr.h"
#include "gnss.h"
#include "utils.h"
#include "stepper.h"


int main(void) {
    // main program loop for blindr

    const uint BOUNDARY_LOW = 0;
    uint BOUNDARY_HIGH, current_position;  // stepper positioning. midpoint and num_steps can be calculated
    int8_t sec, min, hour, day, month, utf_offset;
    int16_t year;
    double latitude=0.0, longitude=0.0;  // use atof() on these. float *should* be sufficient
    int north, east, gnss_fix=0;  // 1 for North and East, 0 for South and West, respectively. GGA fix quality

    stdio_init_all();  // rbf - used for debugging

    setup_uart();  // for connecting to GNSS module
    stepper_init();

    while (1) {
        // main program loop
        tight_loop_contents();
    }
}


void on_uart_rx(double longitude, double latitude, int north, int east, int gnss_fix) {
    // RX interrupt handler

    // checkpoint: todo: minimize this function as much as possible and remove all application logic from this
    // isr. use a ring buffer (in future use DMA/PIO if possible)
    // although... just get this running and then worry about making it better.

    size_t len = 255;
    char buffer[len];  // make a buffer of size `len` for the raw message
    char *sentences[16] = { NULL };  //initialize an array of NULL pointers that will pointing to the location of the start of each sentence within buffer
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
                // parse_zda(fields, &year, &month, &day, &hour, &min, &sec);
                // get_utc_offset(longitude, &utc_offset, month, day);
                // // enact_utc_offset(&year, &month, &day, &hour, utc_offset);  // carful about bug where you get a negative hour...
                // set_onboard_rtc(year, month, day, hour, min, sec, utc_offset);
                // TODO: fix bug where utc time is less than the offset and the clock time is negative...
                // printf("%d/%d/%d %d:%d:%d\n", month, day, year, hour+utc_offset, min, sec);  // rbf
            } else {}
        }
		i++;
	}
    printf("-----------------------\n");  // rbf
}


int setup_uart() {
    // setup uart comms between the GNSS module and pico


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

    // todo: config the gnss module (baud rate, add ZDA, etc.)
    // todo: wait for a fix

    return 0;
}
