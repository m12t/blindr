❌
✅


CURRENT TASK: parsing NMEA data, manipulating into variables

TASKS:
______________________________________________________________________________
1. set pico RTC using parsed ZDA datetime data
1. use lat long to get the UTC offset to be able to use the ZDA-given UTC time for RTC

1. listen to and act on the 3 position toggle switch for manually controlling blindr
1. write out the code for stepper edge finding based on startup protocol.
    - on startup, use the three position toggle switch to move the blinds all the way to one extreme (eg. fully closed down) and then repeat this on the other edge. A count will be taken by the stepper to track the limits so it doesn't damage the blinds by over indexing when trying to maneuver them. incorporate a high sleep time between steps to move the blinds slowly so a precise edge can be found.


1. be able to power on/off gnss module as needed.



COMPLETE:
______________________________________________________________________________
✅ get stepper motor turning
✅ read data from gnss module
✅ parse gnss data
✅ be able to configure gnss module to add ZDA data (done in separate project: `gnss_config`)
✅ set pico RTC to a given time