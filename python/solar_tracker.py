""" a module for tracking the sunrise and sunset using the formula found here:
https://gml.noaa.gov/grad/solcalc/solareqns.PDF
"""

import math


PI = math.pi
cos = math.cos
sin = math.sin
arccos = math.acos
tan = math.tan


def is_leapyear(year: int) -> bool:
    return (year % 4 == 0 and year % 100 != 0) or year % 400 == 0

# ignore leapyears for now
def get_gamma(day: int, hour: int, leapyear: bool) -> float:
    # return the fractional year, gamma
    # find out if it is a leapyear
    if leapyear:
        return 2 * PI / 366 * (day - 1 + (hour - 12)/24)
    return 2 * PI / 365 * (day - 1 + (hour - 12)/24)

def get_eqtime(gamma: float) -> float:
    return (229.18*(
        0.000075+0.001868*cos(gamma)-
        0.032077*sin(gamma)-
        0.014615*cos(2*gamma)-
        0.040849*sin(2*gamma)))

def get_decl(gamma: float) -> float:
    # in radians
    return (
        0.006918-0.399912*cos(gamma) +
        0.070257*sin(gamma) -
        0.006758*cos(2*gamma) +
        0.000907*sin(2*gamma) -
        0.002697*cos(3*gamma) +
        0.00148*sin(3*gamma))

def get_decl2(gamma: float):
    return 0.006918 - 0.399912*cos(gamma) + 0.070257*sin(gamma) - 0.006758*cos(2*gamma) + 0.000907*sin(2*gamma) - 0.002697*cos(3*gamma) + 0.00148*sin (3*gamma)

def get_time_offset(eqtime: float, longitude: float, timezone: int):
    return eqtime + 4 * longitude - 60 * timezone

def get_tst(time_offset: float, hour: int, min: int, sec: int):
    # return the true solar time, tst
    return hour * 60 + min + sec/60 + time_offset

def get_solar_angle(tst: float):
    # return the solar hour angle, ha, in degrees
    return tst/4 - 180

def undo_solar_angle(ha: float):
    # return the tst given the hour angle
    return (ha + 180) * 4

def get_solar_zenith(ha: float, decl: float, latitude: float):
    return arccos(sin(latitude)*sin(decl)+cos(latitude)*cos(decl)*cos(ha))

def get_solar_azimuth(decl: float, zenith: float, latitude: float):
    return -((sin(latitude)*cos(zenith) - sin(decl))/(cos(latitude)*sin(zenith)))

def get_sunrise(decl, eqtime, latitude, longitude):
    ha = calc_sunrise_sunset(decl, latitude)
    return 720 - 4 * (longitude + ha) - eqtime

def get_sunset(decl, eqtime, latitude, longitude):
    ha = -calc_sunrise_sunset(decl, latitude)
    return 720 - 4 * (longitude + ha) - eqtime

def calc_sunrise_sunset(decl, latitude: float):
    return arccos(
        cos(90.833)/(cos(latitude)*cos(decl)) -
        tan(latitude) * tan(decl)
    )

def calc_sunrise_sunset2(decl, latitude):
    return arccos(
        sin()
    )


# april 21 is day 11
day = 111
hour = 1
year = 2022
leapyear = is_leapyear(year)

minute = 0
second = 0
timezone = utc_offset = -6  # from UTC
latitude = 44.97
longitude = -93.25

gamma = get_gamma(108, 24, 0)
eqtime = get_eqtime(gamma)
print(f'gamma: {gamma}')
print(f'eqtime: {eqtime}')
decl = get_decl(gamma)
print(f'decl: {decl}')
decl2 = get_decl2(gamma)
assert decl == decl2

# time_offset =  eqtime + 4*longitude - 60*timezone

solar_noon = 720 - 4*longitude - eqtime
print(f"solar noon {solar_noon}")

# solar_zenith = sin(latitude)*sin(decl) + cos(latitude)*cos(decl)*cos(ha)
# print(f'solar_zenith1: {solar_zenith}')
# solar_zenith = arccos(solar_zenith)
# print(f'solar_zenith2: {solar_zenith}')




# # from wikipedia
# cosw = -tan(latitude)*tan(decl)
# rise = arccos(-cosw)
# set = arccos(cosw)

# rise_st = undo_solar_angle(rise)
# set_st = undo_solar_angle(set)

# print(f'cosw: {cosw}')
# print(f'sunrise: {rise_st}')
# print(f'sunset: {set_st}')



# to = get_time_offset(eqtime, longitude, timezone)
# print(f'time offset: {to}')
# tst = get_tst(to, hour, minute, second)
# print(f'tst: {tst}')
# ha = get_solar_angle(tst)
# print(f'ha: {ha}')
# zenith = get_solar_zenith(ha, decl, latitude)
# print(f'zenith: {zenith}')
# azimuth = get_solar_azimuth(decl, zenith, latitude)
# print(f'azimuth: {azimuth}')

# sunrise = get_sunrise(decl, eqtime, latitude, longitude)
# # sunset = get_sunset(decl, eqtime, latitude, longitude)
# print(f'sunrise: {sunrise}')
# # print(f'sunset: {sunset}')
