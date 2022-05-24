""" adapted found from:
https://microcontrollerslab.com/neo-6m-gps-module-raspberry-pi-pico-micropython/

used to debug the uart and print the GPS module output to the repl.
"""
from machine import Pin, UART
import utime, time

gpsModule = UART(1, baudrate=9600, tx=Pin(4), rx=Pin(5))
print(gpsModule)

buff = bytearray(255)

TIMEOUT = False
FIX_STATUS = False

latitude = ""
longitude = ""
satellites = ""
GPStime = ""


def getGPS(gpsModule):
    global FIX_STATUS, TIMEOUT, latitude, longitude, satellites, GPStime
    
    timeout = time.time() + 1
    while True:
        gpsModule.readline()
        buff = str(gpsModule.readline())
        parts = buff.split(',')
        if ("VTG" in parts[0]):
            print('*found VTG*')
            print(parts)
            print('**')
        if ("GGA" in parts[0] and len(parts) == 15):
            print('found gga')
            if(parts[1] and parts[2] and parts[3] and parts[4] and parts[5] and parts[6] and parts[7]):
                # print(buff)
                latitude = convertToDegree(parts[2])
                if (parts[3] == 'S'):
                    latitude = -float(latitude)
                longitude = convertToDegree(parts[4])
                if (parts[5] == 'W'):
                    longitude = -float(longitude)
                satellites = parts[7]
                GPStime = parts[1][0:2] + ":" + parts[1][2:4] + ":" + parts[1][4:6]
                FIX_STATUS = True
                break
        if (time.time() > timeout):
            TIMEOUT = True
            break
        utime.sleep_ms(500)
        
def convertToDegree(RawDegrees):

    RawAsFloat = float(RawDegrees)
    firstdigits = int(RawAsFloat/100) 
    nexttwodigits = RawAsFloat - float(firstdigits*100)
    
    Converted = float(firstdigits + nexttwodigits/60.0)
    Converted = '{0:.6f}'.format(Converted) 
    return str(Converted)

while True:

    getGPS(gpsModule)

    if(FIX_STATUS == True):
        print("Printing GPS data...\n")
        print(f"Latitude: {latitude}")
        print(f"Longitude: {longitude}")
        print(f"Satellites: {satellites}")
        print(f"Time: {GPStime}")
        print("----------------------")
        FIX_STATUS = False

    if(TIMEOUT == True):
        print(f"No GPS data is found.")
        TIMEOUT = False