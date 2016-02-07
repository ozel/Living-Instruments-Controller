# Living-Instruments-Controller

Arduino Mega and Uno based controller for the Living Instruments project of Hackuarium:
http://wiki.hackuarium.ch/w/Living_Instruments

Developed with Arduino 1.6.6
Requires the Arduino USB host shield and USB-MIDI host libraries:
https://github.com/felis/USB_Host_Shield_2.0
https://github.com/YuuichiAkagawa/USBH_MIDI 

The Uno variant sends 6 analog values scaled to a calibrated min./max. range (#define INST TUBES_UNO)
The Mega variant send the values only if the keys are pressed on an USB-MIDI keyboard. (#define INST BUBBLES)
Lower Octave: values on/off as keys pressed, maijor tones C-H
Higher Octave: every key down toggles the transmission of sensor values between on and off, major tones C-H
