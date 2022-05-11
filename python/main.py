""" pseudocode for blindr """

from machine import Pin
import utime

global sunrise, sunset
sunrise, sunset = solar.get_schedule()  # need to reset daily

time_to_next_phase = 15

while True:
    # load up stepper motor, run checks.
    time, longitude, lattitude = decode_gps()
    if time.date != date:
        sunrise, sunset = solar.get_schedule()
        time_to_next_phase = 15
        next_phase = sunrise
    if time > next_phase - time_to_next_phase:
        run_stepper_motor()  # open for sunrise, close for sunset
        time_to_next_phase -= 1
        # swap next phase
    utime.sleep(15)
