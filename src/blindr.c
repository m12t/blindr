#include "blindr.h"
#include "gnss.h"
#include "utils.h"
#include "stepper.h"
#include "toggle.h"
#include "hardware/gpio.h"


const uint BOUNDARY_LOW = 0;
extern uint BOUNDARY_HIGH, current_position;  // stepper positioning. midpoint and num_steps can be calculated
int8_t sec, min, hour, day, month, utf_offset;
int16_t year;
double latitude=0.0, longitude=0.0;  // use atof() on these. float *should* be sufficient
int north, east, gnss_fix=0;  // 1 for North and East, 0 for South and West, respectively. GGA fix quality

int main(void) {
    // main program loop for blindr


    stdio_init_all();  // rbf - used for debugging

    // setup_uart();  // for connecting to GNSS module
    // stepper_init();
    setup_toggle(&gpio_callback);

    while (1);  // rbf


    // while (1) {
    //     // main program loop
    //     tight_loop_contents();
    // }
}


void gpio_callback(uint gpio, uint32_t events) {
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL, false);  // disable further interrupts during execution to combat switch bounce
    printf("callback working! Event on gpio: %d\n", gpio);
    busy_wait_ms(1000); 
    gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);  // re-enable further interrupts during execution to combat switch bounce
}
