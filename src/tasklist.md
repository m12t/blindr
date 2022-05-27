❌
✅


CURRENT TASK:

TASKS:
______________________________________________________________________________
1. set pico RTC using parsed ZDA datetime data
    ✅ be able to parse NMEA data and manipulate variables into the desired types
1. use lat long to get the UTC offset to be able to use the ZDA-given UTC time for RTC

1. listen to and act on the 3 position toggle switch for manually controlling blindr
1. write out the code for stepper edge finding based on startup protocol.
    - on startup, use the three position toggle switch to move the blinds all the way to one extreme (eg. fully closed down) and then repeat this on the other edge. A count will be taken by the stepper to track the limits so it doesn't damage the blinds by over indexing when trying to maneuver them. incorporate a high sleep time between steps to move the blinds slowly so a precise edge can be found.
1. build out the solar functions (sunrise/sunset,etc.)
1. be able to set alarms? or whatever's the best method for sleeping between solar events (though still must listed for toggle switch input...)
1. be able to power on/off gnss module as needed.
1. evaluate the best way to manage the global variables like lat & long, etc. can they be written to non-volatile memory? which structure of storage is best, simple global vars or structs?
1. on startup, wait for satellite lock.
1. the configurations can't be saved to flash on the GNSS chip, so code will need to be added to change the configs every startup.


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
