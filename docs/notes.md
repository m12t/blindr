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
