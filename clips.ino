#include "clips.h"
#include <EEPROM.h>

//void clips_init (struct clip* C[]) {
//  for (int i = 0; i < CLIP_NUM; i++) {
//    C[i]->photoPin = 0;
//    C[i]->ledPin = 13; //just not 0 to avoid messing with the serial pins
//    C[i]->minLight = 1023;
//    C[i]->maxLight = 0;
//    C[i]->rawValue = 0;
//    C[i]->output = 0;
//    C[i]->active = false;
//  }
//}

int read_sensor (int pin) {
    if (SIMULATE == 1) {
        return random(0, MAX_SENSOR_VALUE+1);
    } else {
        return analogRead(pin);
    }
}

void clips_conf_pins (struct clip* C[]) {
  int j = 0;
  for (int i = 0; i < CLIP_NUM; i++) {
    EEPROM.get(  j  *sizeof(int),C[i]->minLight);
    EEPROM.get((j+1)*sizeof(int),C[i]->maxLight);
    j+=2;
    if (DEBUG) {
      Serial.print("pins: ");
      Serial.print(C[i]->ledPin);
      Serial.print(' ');
      Serial.print(C[i]->photoPin);
      Serial.print(" calib: ");
      Serial.print(C[i]->minLight);
      Serial.print(' ');
      Serial.println(C[i]->maxLight);
    }
    pinMode(C[i]->ledPin, OUTPUT);
    digitalWrite(C[i]->ledPin, HIGH);
    if (C[i]->statusLedPin) pinMode(C[i]->statusLedPin, OUTPUT);
    if (C[i]->statusLedPin) digitalWrite(C[i]->statusLedPin, LOW);
    if (C[i]->dimLedPin) pinMode(C[i]->dimLedPin, OUTPUT);
    if (C[i]->dimLedPin) analogWrite(C[i]->dimLedPin, 0); // set dimled to 0
    //if (DEBUG) analogWrite(C[i]->dimLedPin, 125);
  }
}

void clips_calibrate(struct clip* C[]) {
  if (DEBUG) Serial.println("calibrate clips");  
  //several iterations for calibration
  #if (INST == TUBES)
    int offset = 2; //skip preassure sensors
  #else
    int offset = 0;
  #endif

  for (int i = offset; i < CLIP_NUM; i++) {
    //Enable Status and Dimmable leds to know which clip is calibrating
    digitalWrite(C[i]->statusLedPin, HIGH);
    analogWrite(C[i]->dimLedPin, HIGH);
    // reset min/max.
    C[i]->minLight = 1023;
    C[i]->maxLight = 0;
    for (int j = 0; j < CALIB_ITERATIONS ; j++) {
      C[i]->rawValue = read_sensor(C[i]->photoPin);
      if ( C[i]->rawValue > C[i]->maxLight) {
        C[i]->maxLight = C[i]->rawValue;
        if (DEBUG) {
          Serial.print(C[i]->rawValue);
          Serial.println(" new max");
        }
      }
      if (C[i]->rawValue < C[i]->minLight) {
        C[i]->minLight = C[i]->rawValue;
        if (DEBUG) { 
          Serial.print(C[i]->rawValue);
          Serial.println(" new min");
        }
      }
      delay(5);
      if (DEBUG && j % 100 == 0) Serial.print('.');
    }
    if (DEBUG) {
      Serial.print("calib clip #");
      Serial.print(i);
      Serial.print(' ');
      Serial.print(C[i]->minLight );
      Serial.print(' ');
      Serial.println(C[i]->maxLight );
    }
    //Disable Status and Dimmable leds to know which clip is calibrating
    digitalWrite(C[i]->statusLedPin, LOW);
    analogWrite(C[i]->dimLedPin, LOW);
  }
  if (DEBUG) Serial.print("saving calibration...");
  int idx = 0;
  for (int i = 0; i < CLIP_NUM; i++) {
    EEPROM.put(  idx  *sizeof(int),C[i]->minLight);
    EEPROM.put((idx+1)*sizeof(int),C[i]->maxLight);
    idx+=2;
  }
  if (DEBUG) Serial.println("done.");
}

void clips_read(struct clip* C[]) {
  //Serial.print(millis());
  if (DEBUG) Serial.print('>');
  for (int i = 0; i < CLIP_NUM; i++) {
    if (C[i]->active == true) {
      //enable clip LED
      digitalWrite(C[i]->ledPin, HIGH);
      digitalWrite(C[i]->statusLedPin, HIGH);
    }
    
//    if (SIMULATE) {
//        C[i]->output = random(0, MAX_SENSOR_VALUE);
//    } else {
      C[i]->rawValue = read_sensor(C[i]->photoPin);
      C[i]->output = map(C[i]->rawValue, C[i]->minLight, C[i]->maxLight, 0, MAX_SENSOR_VALUE);
      //clip over or undershots, outside of calibration range
      C[i]->output = constrain(C[i]->output, 0, MAX_SENSOR_VALUE);
//    }

    Serial.print(' ');
    if (C[i]->active == true) {
      //set status LEDs
      // This sets the threshold for turning the dimming LED (in percentage of the max value).

#if (INST == TUBES)
      //invert for LED clips (nr. 3 to 6);
      if (i >= 2) {
      C[i]->output = MAX_SENSOR_VALUE - C[i]->output;
      }
#endif

      if (C[i]->output > (C[i]->maxLight / (DIMM_THRESHHOLD))) {
        //set dimm LEDs
        analogWrite(C[i]->dimLedPin, map(C[i]->output, 0, MAX_SENSOR_VALUE, DIMM_MIN_LEVEL, DIMM_MAX_LEVEL));
      } else {
        analogWrite(C[i]->dimLedPin, 0);
      }
      //report sensor value
      Serial.print(C[i]->output);
    } else {
      //switch off LEDs
      digitalWrite(C[i]->ledPin, LOW);
      digitalWrite(C[i]->statusLedPin, LOW);
      //analogWrite(C[i]->dimLedPin, 0);

      //report 0 value
      Serial.print(0);
    }
  }
  Serial.println();
}

