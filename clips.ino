#include "clips.h"

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

void clips_conf_pins (struct clip* C[]) {
  for (int i = 0; i < CLIP_NUM; i++) {
    if (DEBUG) {
      Serial.print("pins ");
      Serial.print(C[i]->ledPin);
      Serial.print(' ');
      Serial.println(C[i]->photoPin);
    }
    pinMode(C[i]->ledPin, OUTPUT);
    digitalWrite(C[i]->ledPin, HIGH);
    if (C[i]->statusLedPin) pinMode(C[i]->statusLedPin, OUTPUT);
    if (C[i]->statusLedPin) digitalWrite(C[i]->statusLedPin, LOW);
    if (C[i]->dimLedPin) pinMode(C[i]->dimLedPin, OUTPUT);
    if (C[i]->dimLedPin) analogWrite(C[i]->dimLedPin, 0);
  }
}

void clips_calibrate(struct clip* C[]) {
  //1000 iterations
  for (int i = 0; i < CLIP_NUM; i++) {
    for (int j = 0; j < 1000 ; j++) {
      C[i]->rawValue = analogRead(C[i]->photoPin);
      if ( C[i]->rawValue > C[i]->maxLight) {
        C[i]->maxLight = C[i]->rawValue;
      }
      if (C[i]->rawValue < C[i]->minLight) {
        C[i]->minLight = C[i]->rawValue;
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
  }
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
    C[i]->rawValue = analogRead(C[i]->photoPin);
    C[i]->output = map(C[i]->rawValue, C[i]->minLight, C[i]->maxLight, 0, MAX_SENSOR_VALUE);
    //clip over or undershots, outside of calibration range
    C[i]->output = constrain(C[i]->output, 0, MAX_SENSOR_VALUE);

    Serial.print(' ');
    if (C[i]->active == true) {
      //set status LEDs
      if (C[i]->output > (C[i]->maxLight / (DIMM_THRESHHOLD))) {
        //set dimm LEDs
        analogWrite(C[i]->dimLedPin, map(C[i]->output, 0, MAX_SENSOR_VALUE, DIMM_MIN_LEVEL, DIMM_MAX_LEVEL));
      } else {
        analogWrite(C[i]->dimLedPin, 0);
      }
      //report sensor value
      if (SIMULATE) {
        Serial.print(random(0, MAX_SENSOR_VALUE));
      } else {
        Serial.print(C[i]->output);
      }
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

