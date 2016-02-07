#ifndef __CLIPS_H
#define __CLIPS_H


struct clip {
  int photoPin; 
  int ledPin;
  int minLight;
  int maxLight;
  int rawValue;
  int output;   //rawValue calibrated between max/min range
  // follwing stuff is only for visuals:
  int statusLedPin;  //high power 1 watt LED
  int dimLedPin;     //high power 1 watt LED, dimmed according to the signal
};
#endif

