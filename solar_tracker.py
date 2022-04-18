""" a module for tracking the sunrise and sunset using the formula found here:
https://gml.noaa.gov/grad/solcalc/solareqns.PDF
"""

import math


PI = math.pi
cos = math.cos
sin = math.sin
arccos = math.acos
tan = math.tan

# ignore leapyears for now
def get_gamma(day: int, hour: int) -> float:
    # return the fractional year, gamma
    return 2 * PI / 365 * (day - 1 + (hour - 12)/24)

def get_eqtime(gamma: float) -> float:
    return (229.18*(
        0.000075+0.001868*cos(gamma)-
        0.032077*sin(gamma)-
        0.014615*cos(2*gamma)-
        0.040849*sin(2*gamma)))

def get_decl(gamma: float) -> float:
    return (0.006918-0.399912*cos(gamma)+
    0.070257*sin(gamma)-
    0.006758*cos(2*gamma)+
    0.000907*sin(2*gamma)-
    0.002697*cos(3*gamma)+
    0.00148*sin(3*gamma))

def get_time_offset(eqtime: float, longitude: float, timezone: int):
    return eqtime + 4 * longitude - 60 * timezone

def get_tst(time_offset: float, hour: int, minute: int, sec: int):
    # return the true solar time, tst
    return hour * 60 + minute + sec/60 + time_offset

def get_solar_angle(tst: float):
    # return the solar hour angle, ha
    return tst/4 - 180

def get_solar_zenith(ha: float, decl: float, latitude: float):
    return sin(latitude)*sin(decl)+cos(latitude)*cos(decl)*cos(ha)

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


# today, april 18 is day 108. Sunrise is at 6:23
day = 108
hour = 11

minute = 4
second = 2
timezone = -5  # from UTC
# latitude = 0
# longitude = 0

gamma = get_gamma(108, 0)
print(f'gamma: {gamma}')
eqtime = get_eqtime(gamma)
print(f'eqtime: {eqtime}')
decl = get_decl(gamma)
print(f'decl: {decl}')
to = get_time_offset(eqtime, longitude, timezone)
print(f'time offset: {to}')
tst = get_tst(to, hour, minute, second)
print(f'tst: {tst}')
ha = get_solar_angle(tst)
print(f'ha: {ha}')
zenith = get_solar_zenith(ha, decl, latitude)
print(f'zenith: {zenith}')
azimuth = get_solar_azimuth(decl, zenith, latitude)
print(f'azimuth: {azimuth}')

sunrise = get_sunrise(decl, eqtime, latitude, longitude)
sunset = get_sunset(decl, eqtime, latitude, longitude)
print(f'sunrise: {sunrise}')
print(f'sunset: {sunset}')
