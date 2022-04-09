#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>


// int[] schedule(int, int);
float getHa(float, float);
float getGamma(int, int);
float getDecl(float);
float getSunrise(float);
float getSunset(float);

int main() {
	int date, hour;  // out of 365, 366 for leap years
	double gamma, ha, decl;
	printf("%f\n", M_PI);	
	const double LAT = 44.97753;
	// use struct for date and hour? parsing from GPS module
	date = 90;
	hour = 10;
	gamma = getGamma(date, hour);
	printf("gamma: %f\n", gamma);
	decl = getDecl(gamma);
	printf("decl: %f\n", decl);
	ha = getHa(LAT, decl);
	printf("ha: %f\n", ha);
	float sunrise = getSunrise(ha);
	float sunset = getSunset(ha);

}

float getGamma(int date, int hour) {
	float gamma;
	gamma = 2*M_PI/365 * (date - 1 + (hour - 12)/24);
	return gamma;
}

float getHa(float lat, float decl) {
	float ha;
	ha = acos(cos(90.833)/(cos(lat)*cos(decl)) - tan(lat)*tan(decl));
	return ha;
}

float getDecl(float gamma) {
	float decl;
	decl = 0.006918 - 0.399912 * cos(gamma) + 0.070257 * sin(gamma) - 0.006758 * cos(2 * gamma) + 0.000907 * sin(2 * gamma) - 0.002697 * cos(3 * gamma) + 0.00148 * sin(3 * gamma);
	return decl;
}

float getSunrise(float ha) {
	float sunrise;
	sunrise = 720 - 4*
}

float getSunset(float ha) {

}


