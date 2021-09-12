# joyRGB
Joycon RGB LED kit

See the PDF for info on operating this kit

Features:
- Use the front buttons to control the LED kit
- Full RGB
- Use HEX/HTML Color codes to set up your own presets
- Ability to change color of buttons on the fly
- Rainbow mode
- Auto power on/off
- Brightness control

Install instructions:
- Part 1 - https://www.youtube.com/watch?v=M1x_yPTUGak
- Part 2 - https://www.youtube.com/watch?v=-7K7XvtfKoE
- Part 3 - https://www.youtube.com/watch?v=3CIYKLGDYLE

This Joycon RGB LED project allows the use of individually addressable LEDs to light up the buttons. It does require
trimming the shell, but the end result is a very clean install with little to no light bleed.

While I use hollowed out resin-cast buttons on installs, clear buttons from ExtremeRate will probably also work with some modification.

The rubber pad is replaced with a 3D printed bracket (STL file in this repo) for rigidity underneath the flex PCB. 
The gerber file should be manufactured fairly thin. There's a file for the hardener layer as well to support the pads underneath the
Attiny85 microcontroller.

See the bill of materials for all parts required.

The 10k ohm resistor is used in front of the transistor to control the 'Latch' line on the microcontroller for auto on-off functionality.
330 ohm resistor goes on the LED data line to smooth the signal.
