❌
✅


CURRENT TASK:

Toggle. Configure toggle on PIO and send an interrupt on falling edge (when activated)


- gnss architecture:
    - PIO listens on the uart pins and writes directly to a buffer via DMA. An IRQ is then generated for one of the main cores to parse the message and take further action.

so to break it down into components:
1. the PIO reads the UART and writes it to its RX FIFO
2. the DMA then reads from the RX FIFO and writes it to an array in RAM


toggle components:
1. PIO are set for the up and down pins of the toggle switch
2. they efficiently wait() for a change of state and trigger a IRQ to the main chip on a rising edge and end it with a falling edge


TASKS:
______________________________________________________________________________
1. work through handing the latitude and longitude vars which get used in on_uart_rx() within gnss.c but are local to
the main() of blindr.c... and on_uart_rx() can't receive any parameters...
    > handle uart within main() of blindr.c is the simplest solution, but also bad design.
1. set pico RTC using parsed ZDA datetime data
    ✅ be able to parse NMEA data and manipulate variables into the desired types

1. listen to and act on the 3 position toggle switch for manually controlling blindr
1. write out the code for stepper edge finding based on startup protocol.
    - on startup, use the three position toggle switch to move the blinds all the way to one extreme (eg. fully closed down) and then repeat this on the other edge. A count will be taken by the stepper to track the limits so it doesn't damage the blinds by over indexing when trying to maneuver them. incorporate a high sleep time between steps to move the blinds slowly so a precise edge can be found.
1. build out the solar functions (sunrise/sunset,etc.)
1. be able to set alarms? or whatever's the best method for sleeping between solar events (though still must listed for toggle switch input...)
1. be able to power on/off gnss module as needed.
1. evaluate the best way to manage the global variables like lat & long, etc. can they be written to non-volatile memory? which structure of storage is best, simple global vars or structs? (don't actually want non-volatile mem since you want new coordinates on each power cycle as a way to reset the coordinates, example if you move across the country.)
1. on startup, wait for satellite lock.
1. the configurations can't be saved to flash on the GNSS chip, so code will need to be added to change the configs every startup.
1. use PIO and interrupts on GPIO (toggle switch) to efficiently wait on input instead of bit banging
    > see: logic_analyser.c example in pico-examples/pio
1. use PIO for controlling the stepper instead of bit banging the rising edge? no. GPIO is sufficient
1. replace the GNSS uart interrupt architecture with PIO and DMA
1. precisely calculate daylight savings times using day of the week (see: https://cs.uwaterloo.ca/~alopez-o/math-faq/node73.html). This also means setting the DOTW in the datetime struct.
1. since the reliability of the switch isn't perfect for triggering automation or not, assume automation is desired and perform an independent check before actually moving the blinds by evaluating current_position, and gpio_get(pin) value to see if the switch is flipper or not.


COMPLETE:
______________________________________________________________________________
✅ get stepper motor turning
✅ read data from gnss module
✅ parse gnss data
✅ be able to configure gnss module to add ZDA data (done in separate project: `gnss_config`)
✅ set pico RTC to a given time
✅ parse ZDA NMEA data, manipulating into variables
✅ debugging parsing lat long in gnss.c
✅ outline the system architecture:
    - startup:
        1. wait for GNSS signal or timeout of 10 minutes
        1. listen for toggle switch input on boundaries
            - store these boundaries (and midpoint for fully open) in global variables
            - remember to sleep the stepper driver (and stepper) after each use
    - once GNSS signal is received:
        1. set onboard RTC
        1. store lat, long, ns, ew in global vars
    - enter main loop
        1. listen for toggle switches
        1. recalculate the next day's sunrise and sunset times at minight 00:00
        1. move the blinds to their position prior to the events
        1. power on and listen to the GNSS once each week to update the onboard RTC, then power the module off.
❌ write lat and long to flash... although you want it to search for new lat long on each power cycle of the uC.
✅ use lat long to get the UTC offset to be able to use the ZDA-given UTC time for RTC
    > daylight savings was *roughly* taken into account as well
✅ validate that the toggle switch works with a simple python script


