""" a module for tracking the sunrise and sunset using the formula found here:
https://gml.noaa.gov/grad/solcalc/solareqns.PDF
"""

import math


PI = math.pi

date = 1  # Jan 1
hour = 6
# ignore leapyears for now
fractional_year = 2 * PI / 365 * (date - 1 + (hour - 12)/24)
