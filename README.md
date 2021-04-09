Elektor Nixie Clock firmware
============================

What is it?
-----------

This is an alternative firmware for the Elektor Six Digit Nixie Clock
(150189-71). It replaces the original firmware running on the clock’s
PIC18F4420.

Building and installing
-----------------------

You will need the MPLAB X IDE:
https://www.microchip.com/en-us/development-tools-tools-and-software/mplab-x-ide#tabs

As well as the MPLAB XC8 compiler:
https://www.microchip.com/en-us/development-tools-tools-and-software/mplab-xc-compilers

(Note that using the higher optimization levels of the compiler requires a
license, but that is not necessary to build this project.)

An appropriate PIC programmer (such as the PICkit 3) will also be required.

Open the folder containing this file in MPLAB X IDE as a project.

Connect the programmer to the board (with the PICkit 3, Pin 1/MCLR, indicated
by a little triangle, needs to be connected to the board on the opposite side
of the “K1” text).

Click Make and program device in MPLAB X IDE. (If the clock is not powered, you
may need to change the programmer’s settings to enable its +5V output).

To change the DST settings, edit the settings.h file.