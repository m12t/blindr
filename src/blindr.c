#include "gnss.h"
// #include "setup.h"

// uart config
#define UART_ID uart1
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 4
#define UART_RX_PIN 5
// stepper configs
#define SLEEP_PIN 13
#define STEP_PIN 14
#define DIRECTION_PIN 15
// toggle configs
// #define UP_PIN {num}
// #define DOWN_PIN {num}


int main(void) {
    // main program loop for blindr
    uint BOUNDARY_LOW=0, BOUNDARY_HIGH, current_position;  // stepper positioning. midpoint and num_steps can be calculated
    int8_t sec, min, hour, day, month, utf_offset;
    int16_t year;
    double latitude=0.0, longitude=0.0;  // use atof() on these. float *should* be sufficient
    int north, east, gnss_fix=0;  // 1 for North and East, 0 for South and West, respectively. GGA fix quality

    stdio_init_all();  // rbf - used for debugging

    uart_setup()  // for connecting to GNSS module

    while (1) {
        // 
    }
}