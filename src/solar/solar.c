#include "solar.h"

// the below was translated from the code running at:
// https://gml.noaa.gov/grad/solcalc/


double radToDeg(double angleRad) {
	return (180.0 * angleRad / PI);
}

double degToRad(double angleDeg) {
	return (PI * angleDeg / 180.0);
}

double calcGeomMeanLongSun(double t) {
	double L0 = 280.46646 + t * (36000.76983 + t*(0.0003032));
	while(L0 > 360.0) {
		L0 -= 360.0;
	}
	while(L0 < 0.0) {
		L0 += 360.0;
	}
	return L0;		// in degrees
}

double calcGeomMeanAnomalySun(double t) {
	double M = 357.52911 + t * (35999.05029 - 0.0001537 * t);
	return M;		// in degrees
}

double calcEccentricityEarthOrbit(double t) {
	double e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
	return e;		// unitless
}

double calcSunEqOfCenter(double t) {
	double m = calcGeomMeanAnomalySun(t);
	double mrad = degToRad(m);
	double sinm = sin(mrad);
	double sin2m = sin(mrad+mrad);
	double sin3m = sin(mrad+mrad+mrad);
	double C = sinm * (1.914602 - t * (0.004817 + 0.000014 * t)) + sin2m * (0.019993 - 0.000101 * t) + sin3m * 0.000289;
	return C;		// in degrees
}

double calcSunTrueLong(double t) {
	double l0 = calcGeomMeanLongSun(t);
	double c = calcSunEqOfCenter(t);
	double O = l0 + c;
	return O;		// in degrees
}

double calcSunApparentLong(double t) {
	double o = calcSunTrueLong(t);
	double omega = 125.04 - 1934.136 * t;
	double lambda = o - 0.00569 - 0.00478 * sin(degToRad(omega));
	return lambda;		// in degrees
}

double calcMeanObliquityOfEcliptic(double t) {
	double seconds = 21.448 - t*(46.8150 + t*(0.00059 - t*(0.001813)));
	double e0 = 23.0 + (26.0 + (seconds/60.0))/60.0;
	return e0;		// in degrees
}

double calcObliquityCorrection(double t) {
	double e0 = calcMeanObliquityOfEcliptic(t);
	double omega = 125.04 - 1934.136 * t;
	double e = e0 + 0.00256 * cos(degToRad(omega));
	return e;		// in degrees
}

double calcSunRtAscension(double t) {
	double e = calcObliquityCorrection(t);
	double lambda = calcSunApparentLong(t);
	double tananum = (cos(degToRad(e)) * sin(degToRad(lambda)));
	double tanadenom = (cos(degToRad(lambda)));
	double alpha = radToDeg(atan2(tananum, tanadenom));
	return alpha;		// in degrees
}

double calcSunDeclination(double t) {
	double e = calcObliquityCorrection(t);
	double lambda = calcSunApparentLong(t);
	double sint = sin(degToRad(e)) * sin(degToRad(lambda));
	double theta = radToDeg(asin(sint));
	return theta;		// in degrees
}

double calcEquationOfTime(double t) {
	double epsilon = calcObliquityCorrection(t);
	double l0 = calcGeomMeanLongSun(t);
	double e = calcEccentricityEarthOrbit(t);
	double m = calcGeomMeanAnomalySun(t);

	double y = tan(degToRad(epsilon)/2.0);
	y *= y;

	double sin2l0 = sin(2.0 * degToRad(l0));
	double sinm   = sin(degToRad(m));
	double cos2l0 = cos(2.0 * degToRad(l0));
	double sin4l0 = sin(4.0 * degToRad(l0));
	double sin2m  = sin(2.0 * degToRad(m));

	double Etime = y * sin2l0 - 2.0 * e * sinm + 4.0 * e * y * sinm * cos2l0 - 0.5 * y * y * sin4l0 - 1.25 * e * e * sin2m;
	return radToDeg(Etime)*4.0;	// in minutes of time
}

double getJD(int16_t year, int8_t month, int8_t day) {
	if (month <= 2) {
		year -= 1;
		month += 12;
	}
	double A = floor(year/100);
	double B = 2 - A + floor(A/4);
	double JD = floor(365.25*(year + 4716)) + floor(30.6001*(month+1)) + day + B - 1524.5;
	return JD;
}

double calcTimeJulianCent(double jd) {
	double T = (jd - 2451545.0)/36525.0;
	return T;
}

double calcSunriseSetUTC(double rise, double JD, double latitude, double longitude) {
	double t = calcTimeJulianCent(JD);
	double eqTime = calcEquationOfTime(t);
	double solarDec = calcSunDeclination(t);
	double hourAngle = calcHourAngleSunrise(latitude, solarDec);
	if (!rise) hourAngle = -hourAngle;
	double delta = longitude + radToDeg(hourAngle);
	double timeUTC = 720 - (4.0 * delta) - eqTime;	// in minutes

	return timeUTC;
}

double calcHourAngleSunrise(double lat, double solarDec) {
	double latRad = degToRad(lat);
	double sdRad  = degToRad(solarDec);
	double HAarg = (cos(degToRad(90.833))/(cos(latRad)*cos(sdRad))-tan(latRad) * tan(sdRad));
	double HA = acos(HAarg);
	return HA;		// in radians (for sunset, use -HA)
}


double calcSunriseSet(double rise, double JD, double latitude, double longitude, double timezone) {
    // rise = 1 for sunrise, 0 for sunset
	double timeUTC = calcSunriseSetUTC(rise, JD, latitude, longitude);
	double newTimeUTC = calcSunriseSetUTC(rise, JD + timeUTC/1440.0, latitude, longitude); 
    double timeLocal;
	if (newTimeUTC) {
		timeLocal = newTimeUTC + (timezone * 60.0);
		double riseT = calcTimeJulianCent(JD + newTimeUTC/1440.0);
		double jday = JD;
		if ( (timeLocal < 0.0) || (timeLocal >= 1440.0) ) {
			double increment = ((timeLocal < 0) ? 1 : -1);
			while ((timeLocal < 0.0)||(timeLocal >= 1440.0)) {
				timeLocal += increment * 1440.0;
				jday -= increment;
			}
		}

	} else { // no sunrise/set found
        return 0.0;
    }
    return timeLocal;
}

void calculate_solar_events(int8_t *rise_hour, int8_t *rise_minute,
                            int8_t *set_hour, int8_t *set_minute,
                            int16_t year, int8_t month, int8_t day,
                            int utc_offset, double latitude, double longitude) {
    // the entry point and only function from this lib that needs to be called
    // from outside of /solar. This function accepts 4 pointers which are then
    // populated with the calculated times.
    double jday = getJD(year, month, day);
    printf("JDAY: %f\n", jday);      // rbf
	double rise = calcSunriseSet(1, jday, latitude, longitude, utc_offset);
	double set  = calcSunriseSet(0, jday, latitude, longitude, utc_offset);

    int8_t rh = floor(rise / 60);
    int8_t rm = rise - 60*rh;
    int8_t sh = floor(set / 60);
    int8_t sm = set - 60*sh;

	printf("rise: %d/%d/%d  %d:%d:00\n", month, day, year, rh, rm);  // rbf
    printf("set:  %d/%d/%d  %d:%d:00\n", month, day, year, sh, sm);  // rbf

    *rise_hour = rh;
    *rise_minute = rm;
    *set_hour = sh;
    *set_minute = sm;
}
