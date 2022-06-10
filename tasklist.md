❌
✅

CURRENT TASK:

1. refactor to use DMA to write GNSS data instead of dedicating an entire core.
data flow:
✅ uart_received_IRQ ->  {this isn't actually firing during the busy wait... need to use PIO to listen}
✅ on_uart_rx() ->
3. write to DMA buffer ->
4. buffer_full_irq ->
5. parse_buffer ->
6. write to DMA sentences[] pointer array ->
7. [either set a flag that gets caught by the timeout loop | process it directly by calling parse_buffer()]

dma channels needed:
1. raw uart data -> raw `buffer[]`
2. parsed data -> parsed `sentences[]`

// be sure to unclaim dma channels on deinit.


REMAINING TASKS:
------------------------------------------------------------------------------



POTENTIAL FUTURE VERISON FEATURES:
------------------------------------------------------------------------------
1. add a watchdog to keep the loop alive
1. complicated blind openings like imperceptibly slowly opening and closing throughout the day, etc.
1. use DMA instead of the long and slow IRQ currently used for reading GNSS data. The current system works enough for the latency and throughput, though.
1. use PIO on the pico to control the stepper instead of bit banging the signals.

COMPLETE TASKS:
------------------------------------------------------------------------------
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
✅ Toggle. send and handle an interrupt on falling edge
✅ listen to and act on the 3 position toggle switch for manually controlling blinds
✅ evaluate the best way to manage the global variables like lat & long, etc. can they be written to non-volatile memory? which structure of storage is best, simple global vars or structs? (don't actually want non-volatile mem since you want new coordinates on each power cycle as a way to reset the coordinates, example if you move across the country.) > just use global vars for now
✅ since the reliability of the switch isn't perfect for triggering automation or not, assume automation is desired and perform an independent check before actually moving the blinds by evaluating current_position, and gpio_get(pin) value to see if the switch is flipper or not.
    > done in set_automation_state()
❌ use PIO and interrupts on GPIO (toggle switch) to efficiently wait on input instead of bit banging
    > see: logic_analyser.c example in pico-examples/pio
    >>> pio is overkill for toggle switch and stepper
❌ use PIO for controlling the stepper instead of bit banging the rising edge? >> no. GPIO is sufficient
✅ now that GNSS uart is only used on startup, place the logic in the `gnss.c` file and access the global vars lat, long, north, east, utc_offset, etc. via externs in `gnss.h`
✅ get gnss.c working to parse gnss data while the toggle functinoality runs simultaneously
✅ work through handing the latitude and longitude vars which get used in on_uart_rx() within gnss.c but are local to
the main() of blindr.c... and on_uart_rx() can't receive any parameters...
    > global vars and `extern` statements within `gnss.h`
✅ set pico RTC using parsed ZDA datetime data
    ✅ be able to parse NMEA data and manipulate variables into the desired types
❌ replace the GNSS uart interrupt architecture with PIO and DMA
    > since gnss is only used on startup
❌ precisely calculate daylight savings times using day of the week (see: https://cs.uwaterloo.ca/~alopez-o/math-faq/node73.html). This also means setting the DOTW in the datetime struct.
    > don't use DST right now. it's not even used by the solar calculations -- believe it or not, the sun doesn't observe daylight savings!
✅ write out the code for stepper edge finding based on startup protocol.
    - on startup, use the three position toggle switch to move the blinds all the way to one extreme (eg. fully closed down) and then repeat this on the other edge. A count will be taken by the stepper to track the limits so it doesn't damage the blinds by over indexing when trying to maneuver them. incorporate a high sleep time between steps to move the blinds slowly so a precise edge can be found.
❌ the configurations can't be saved to flash on the GNSS chip, so code will need to be added to change the configs every startup.
    > just assume it was configured manually -- don't configure it.
    ✅DON'T handle configuring the gnss module. config elsewhere and assume it has ZDA and GGA data.
✅ only rely on the gnss to set the module on startup. This allows the gnss to be disconnected and used for other tasks.
    a. be able to dismount the uart and irqs on gnss
✅ sort out the rat's nest of #includes and #defines
❌ be able to power on/off gnss module as needed.
    > no longer needed, uart will be shut off and the unit can be disconnected.
✅ build out alarm logic for solar events
✅ writing the main program loop of finding the next solar event and setting a timer on the rtc to service the event.
    ✅ be able to set alarms? or whatever's the best method for sleeping between solar events (though still must listed for toggle switch input...)
    ✅ build out the solar functions (sunrise/sunset,etc.)
✅ on startup, wait for satellite lock. -> gnss_fix=1
✅ bug where if `down` is the first toggle input, no `up` input is allowed.
    > was switch bounce triggering the function to run at set the boundary to 0 since the switch wouldn't stay down
    long enough to actually take steps. A sleep at the top of the function followed by another test for the pin's state eliminated this issue.
✅ test the alarm functionality to be able to actually trigger things
✅ perform integration testing with the stepper ... install the components.
✅ add the ability to power on/off gnss module and send configuratioins to it
    > wake it every week or so to ensure the onboard RTC stays accurate.
✅ allow for a mode that runs without gnss/uart on startup (implement a timeout on listening on uart??? if uart isn't found within 2 minutes, shut it down? -> set rtf to 01/01/2000 00:00:01? and once it reaches ...15:00?). this will just function as a manual-only blind for the cases a restart is triggered without a gnss module present and you still want to be able to control the blinds.
