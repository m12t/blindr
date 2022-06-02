#ifndef SOLAR_H
#define SOLAR_H

#include <math.h>
#include <stdint.h>
#include "../utils/utils.h"
#define PI 3.14159265358979323846264338327950288

#endif

// prototypes:
double calculate_solar_event(int16_t year, int8_t month, int8_t day,
                             int8_t hour, int8_t min, int8_t sec, int utc_offset,
                             double latitude, double longitude);
double calcSunriseSet(double rise, double JD, double latitude, double longitude, double timezone);
double calcSunriseSetUTC(double rise, double JD, double latitude, double longitude);
double getJD(int16_t year, int8_t month, int8_t day);
double calcHourAngleSunrise(double lat, double solarDec);
double calcEquationOfTime(double t);
double calcSunDeclination(double t);
double calcSunRtAscension(double t);
double calcObliquityCorrection(double t);
double calcMeanObliquityOfEcliptic(double t);
double calcSunApparentLong(double t);
double calcSunTrueLong(double t);
double calcSunEqOfCenter(double t);
double calcEccentricityEarthOrbit(double t);
double calcGeomMeanAnomalySun(double t);
double calcGeomMeanLongSun(double t);
double degToRad(double angleDeg);
double radToDeg(double angleRad);
double calcDoyFromJD(double jd);
double calcDateFromJD(double jd);
double calcTimeJulianCent(double jd);