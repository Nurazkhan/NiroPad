# NiroPad
Simlple macropad to make your computer usage even faster.
[![View PCB on KiCanvas](https://hack.club/pcb-badge)](https://kicanvas.org/?repo=https://github.com/Nurazkhan/NiroPad/tree/main/pcb/NiroPad)
![NiroPad](<assets/3D enclosure.png>)

I made this macropad with the help of hackClub. You can find useful references/examples in [Hackpad]([url](https://stardance.hackclub.com/missions/hackpad)) section.
## Features
* 128x32px for cool animations.
* 9 Cherry MX style Keys for best experience.
* 1 EC11 encoder to control with accuracy.
* Bright 18 RGB LEDS to make it GLOW. (9 for underglow, 4 under encoder, 4 near display, 1 under display)
* 3D printed perfect case with personal mentions.
* QMK Firmware

## CAD Model
CAD consists of a total of 3 parts: Case, Cover, Knob.
The case and the cover for it fit together with 4*M3 screws and heatset inserts.
The knob is attached to the encoder by friction.
The case has space for keys and displays, so it fits perfectly.
![NiroPad](<assets/3DenclosurewithoutCase.png>)
Made in OnShape. by Nurazkhan (me).

## PCB
![NiroPad](<assets/SchemePCB.png>)
Schematic was made in KiCad.
![NiroPad](<assets/PCBroutes.png>)
PCB routing was also made in KiCad Software. Footprints for keys were imported from recommended resources; you can also find [it]([url](https://github.com/sszczep/kicad-libraries)).

## Firmware Overview
The NiroPad Works on QMK firmware.
Main functions are:
* 9 keys for the number section. (My laptop doesn't have a separate keyboard section for numbers).
* Encoder to control volume.
* Dynamic LED control based on CAPS LOCK state.
* OLED showing the mode of the keyboard.
In the future, have to add more functions (changing modes from number sections to shortcuts, or media control, etc.
Also, planning to add animations to the OLED display.


## BOM
 * 9 Cherry MX style switches
 * 9 1N4148 diodes
 * 1 Custom PCB
 * 0.91 SSD1306 OLED display 128x32px
 * Xiao Speeed RPI2040 microcontroller
 * 3d printer enclosure (top and bottom) and knob for encoder
 * EC11 Encoder
 * 4 M3*16mm screws
 * 4 M3*5m*4mm heatset inserts
 

The process of designing and creation you can see in the journals folder.

Thank you!
#hackClub
