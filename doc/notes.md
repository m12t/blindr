Blindr... smart window blinds.

a real project named after the satirical YouTube video by Joma Tech `if Apple made window blinds...`

The project uses a microcontroller and a GPS module to open and close window blinds gradually around sunrise and sunset, while still allowing for manual window operation via a momentary 3 position toggle switch.


hardware:
- ✅ microcontroller (Raspberry Pi Pico)
- ✅ stepper motor (STP-42D201-37; see `/stepper.md`)
- stepper motor driver (trinamic TMC2130 or 2209)
	- form factor: module
	- chip: TMC2130
	- the TMC2130 permits 256 for virtually silent operation
	- stall detection for sensorless position finding (open and close limits on the blinds)
	- this requires a decoupling capacitor (100 uF) to the motor power supply
	- must be able to supply 1.7A peak for the stepper
- GPS module (for getting position data AND time data)
	- this eliminates the need for an RTC module because GPS provides time data
	- this also allows this code to be used by anyone on Earth as-is
	- Intuitively, blinds go on windows which look outside, so signal strength should be strong
- 3 position momentary toggle switch for manually controlling the blinds
- 5V power supply (used by Pico)
- stepper power supply
- 3D printed housing for the unit


GPS Module requirements:
- cost effective
	- high precision is not needed
- low power consumption
	- low update rate is acceptible since the Pico has an onboard RTC that can be trusted in between updates
	- best if it can be shut down and turned on as needed by the pico. Is it as simple as powering it off the pico and pulling the GPS module to ground?
