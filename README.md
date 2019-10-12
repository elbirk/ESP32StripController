# ESP32StripController
Visual Pinball Stripcontroller on ESP32 WROOM-32

Modified version of TeensyStripController for ESP32 WROOM-32
You need to replace directoutput.dll before it works wit DOF R3++.

Original version found on.
https://github.com/DirectOutput/TeensyStripController

Uploaded ALFA version.

Yet to be fixed:
DOF cannot change Striplength, you have to alter the code. This is because I to late discovered that FASTLED library cannot change the strip length dynamically. So either I need to find a new libary or make a hack for fastled

Version that do not use special directoutput.dll


