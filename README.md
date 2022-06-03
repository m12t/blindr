# blindr
An embedded systems project that automates the articulation of window blinds such that they automatically open at sunrise and close at sunset each day.

## About the Project
The goal for this project is to have window blinds that automatically open and close around sunrise and sunset in an effort to instill healthy a sleep pattern. The code is written in C using the C/C++ SDK for the Raspberry Pi Pico microcontroller board.

The name `blindr` was a joke on the joke from this hilarious video by Joma Tech on [if Apple made window blinds...](https://youtu.be/Hv6EMd8dlQk).


## Features
1. Fully motorized articulation of blind slats
1. A generic code architecture that can be used by anyone, anywhere on Earth, without modification (_granted you use the same components found below_)
1. No bluetooth or WiFi needed -- a fully isolated and contained system
1. The ability to source latitude and longitude (needed for the solar calculations) and precise datetime data from a GNSS module
1. The ability to enable and disable automation while still being able to "manually" control the blinds via a 3 position toggle switch


## High Level Code Overview
Being an embedded system that will run 24/7, reliability and low power consumption were identified as mission-critical parameters from the project's inception.
Some examples of this in action:
1. The stepper and stepper driver board are put to sleep (no current flows to the motor or driver charge pump) whenever it's not actively running
1. The main processing cores sleep indefinitely only to be woken up by toggle switch interrupts and RTC-based alarms for solar events
1. Once the datetime data (from a NMEA ZDA sentence) and position data (from a NMEA GGA sentence) are captured, the GNSS module can be disconnected and used for other projects. Note, however, it is needed on each startup to set the Pico uC's onboard RTC


## User Interface
The only direct input the user has with the system is through the 3 position toggle switch. Once powered on, the Pico will wait for toggle pin input that will be used to detect the boundaries of the blinds (fully closed up and fully closed down). It's critical that the first input in either direction (the order doesn't matter) goes all the way to the desired boundary. Once low and high boundaries are set, the toggle pin can be used to control the position of the blinds "manually". The code keeps track of the position of the blinds at all times and will not let the blinds turn past the given boundaries. To disable automation and keep the blinds shut indefinitely, leave the toggle switch flipped up or down which will move to the specified boundary, sleep the stepper, and wait for the toggle to be reset to the neutral position to enable automation again. There is currently no way to leave the blinds open and disable automation without unplugging the unit. This functionality could be encoded in a switch pattern such as `[up] [down] [up]` within a specified time frame, but leaving the blinds indefinitely open wasn't nearly as useful as being able to leave them indefinitely closed.


## Hardware
I have some CAD and STL files for the stepper motor housing and a driveshaft I drafted up that will work if you happen to have the exact model of stepper (Shinano Kenshi STP-42D201-37 with 14 tooth gear) and blinds (Home Depot Home Decorators Collection) as me. I 3D printed the mount and driveshaft in PLA, though you will likely have to design your own or find another method of getting your stepper to actually turn your blinds. This repo focuses on the code, however, which _is_ compatable with any bipolar stepper and any mounting system you might use.


# BOM (2022 prices)
1. [$4]  1x Raspberry Pi Pico
1. [$11] 1x gnss module (I used a quadcopter module - TBS M8.2 which uses a Ublox M8030-KT, though any UBX protocol module _should_ work)
1. [$21.50] 1x Big Easy Driver (A4988/A4983) capable of supplying 2A, though your particular stepper might require more or less current
1. [$15] A NEMA 17 stepper motor or similar. Smaller may work, but it needs to be bipolar. For example, a 28BYJ-48 would not work without modification
1. [FREE] old laptop charger (+8 to +30V filtered DC) which directly and indirectly powers all of the components. If you plan on building these blinds, I bet you or someone you know has an unused charger that could be repurposed to the task (**this also reduces E-waste in the process!**). As a last resort, check your local bartering site and do some haggling ;)
1. [$2] 3 position toggle switch on-off-on. Note, a momentary switch (on)-off-(on) will prevent disabling of automation with the current code since it will always trigger a rising edge when the switch resets to the center position, re-enabling automation
1. [$3] ~10 feet of thin gauge (28-30 AWG for signal wires, see your stepper's datasheet for recommended stepper wire gauge) stranded core wire
1. [$5] (optional) connectors to allow components to be easily reused for other tasks (I use Molex Picoblade, though there are certainly easier options to crimp out there)
Total: **<$60** 
I reckon that's not bad considering COTS automated blinds typically run 3-4x that and this is far more extensible and the components can be reused for other tasks!


## Contributions
1. Contributions are welcome!
1. New feature ideas, issues, PRs, etc.
