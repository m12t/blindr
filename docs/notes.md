Blindr... smart window blinds.

a real project named after the satirical YouTube video by Joma Tech `if Apple made window blinds...`

The project uses a microcontroller and a GPS module to open and close window blinds gradually around sunrise and sunset, while still allowing for manual window operation via a momentary 3 position toggle switch.


hardware:
- microcontroller (Raspberry Pi Pico)
- stepper motor (TBD)
- stepper motor driver (n/a, controlled with code from the Pico)
- GPS module (for getting position data AND time data)
	- this eliminates the need for an RTC module because GPS provides time data
	- this also allows this code to be used by anyone on Earth as-is
	- Intuitively, blinds go on windows which look outside, so signal strength should be strong
- 3 position momentary toggle switch for manually controlling the blinds
- 5V power supply (used by Pico and stepper motor)
- 3D printed housing for the unit


for articulating the blind position, use revolutions at first. 8 turns goes from shut with slats up to shut with slats down. 4.5 turns _up_ from down gets to middle, 4 turns _down_ from top gets to middle. Other options are something like a mems gyro + accelerometer like the MPU-6050 mounted on one of the blinds to gauge angle for more precise control.

GPS Module requirements:
- cost effective
	- high precision is not needed
- low power consumption
	- low update rate is acceptible since the Pico has an onboard RTC that can be trusted in between updates.
