#include "clips.h"
#include <EEPROM.h>
#include <MIDI.h>
#include <AnalogSmooth.h>

AnalogSmooth avgC2 = AnalogSmooth(30);
AnalogSmooth avgC3 = AnalogSmooth(15);
AnalogSmooth avgC4 = AnalogSmooth(30);
AnalogSmooth avgC5 = AnalogSmooth(15);


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

int lastC2 = 0;
int lastC3 = 0;
int lastC4 = 0;
int lastC5 = 0;

int read_sensor (int pin) {
  if (SIMULATE == 1) {
    return random(0, MAX_SENSOR_VALUE + 1);
  } else {
    return analogRead(pin);
  }
}

void clips_conf_pins (struct clip* C[]) {
  int j = 0;
  for (int i = 0; i < CLIP_NUM; i++) {
    EEPROM.get(  j  * sizeof(int), C[i]->minLight);
    EEPROM.get((j + 1)*sizeof(int), C[i]->maxLight);
    j += 2;
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
  //#if (INST == TUBES)
  //  int offset = 2; //skip preassure sensors
  //#else
  int offset = 0;
  //#endif

  for (int i = offset; i < CLIP_NUM; i++) {
    //Enable Status and Dimmable leds to know which clip is calibrating
    digitalWrite(C[i]->statusLedPin, HIGH);
    analogWrite(C[i]->dimLedPin, HIGH);
    digitalWrite(C[i]->ledPin, HIGH); //enbale LED opposite of sensor
    // reset min/max.
    C[i]->minLight = 1023;
    C[i]->maxLight = 0;
    for (int j = 0; j < CALIB_ITERATIONS ; j++) {
      C[i]->rawValue = read_sensor(C[i]->photoPin);
      if ( C[i]->rawValue > C[i]->maxLight) {
        C[i]->maxLight = C[i]->rawValue;
        if (DEBUG) {
          //Serial.print(C[i]->rawValue);
          //Serial.println(" new max");
        }
      }
      if (C[i]->rawValue < C[i]->minLight) {
        C[i]->minLight = C[i]->rawValue;
        if (DEBUG) {
          //Serial.print(C[i]->rawValue);
          //Serial.println(" new min");
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
    digitalWrite(C[i]->ledPin, LOW); //disable LED opposite of sensor
  }
  if (DEBUG) Serial.print("saving calibration...");
  int idx = 0;
  for (int i = 0; i < CLIP_NUM; i++) {
    EEPROM.put(  idx  * sizeof(int), C[i]->minLight);
    EEPROM.put((idx + 1)*sizeof(int), C[i]->maxLight);
    idx += 2;
  }
  if (DEBUG) Serial.println("done.");
}

void clips_read(struct clip* C[]) {
  //Serial.print(millis());
  //if (DEBUG) Serial.print('>');
  int newData = 0;
  for (int i = 0; i < CLIP_NUM; i++) {
    if (C[i]->active == true) {
      //enable clip LED
      digitalWrite(C[i]->ledPin, HIGH);
      digitalWrite(C[i]->statusLedPin, HIGH);
    }
#if (INST == TUBES)
    if (C[2]->active || C[3]->active) {
      digitalWrite(C[2]->statusLedPin, HIGH);
      digitalWrite(C[3]->statusLedPin, HIGH);
    }
    else if (C[4]->active || C[5]->active) {
      digitalWrite(C[4]->statusLedPin, HIGH);
      digitalWrite(C[5]->statusLedPin, HIGH);
    }
#endif

    C[i]->rawValue = read_sensor(C[i]->photoPin);
    C[i]->output = map(C[i]->rawValue, C[i]->minLight, C[i]->maxLight, 0, MAX_SENSOR_VALUE);
    //clip over or undershots, outside of calibration range
    C[i]->output = constrain(C[i]->output, 0, MAX_SENSOR_VALUE);

    //Serial.print(' ');
    if (C[i]->active == true) {
      if (C[i]->output > 0) {
        newData++;
      }

      //#if (INST == TUBES)
      //      //invert for LED clips (nr. 3 to 6);
      //      if (i >= 2) {
      //        C[i]->output = MAX_SENSOR_VALUE - C[i]->output;
      //      }
      //#endif

      //set status LEDs
      // This sets the threshold for turning the dimming LED (in percentage of the max value).
      if (C[i]->output > (C[i]->maxLight / (DIMM_THRESHHOLD))) {
        //set dimm LEDs
        analogWrite(C[i]->dimLedPin, map(C[i]->output, 0, MAX_SENSOR_VALUE, DIMM_MIN_LEVEL, DIMM_MAX_LEVEL));
      } else {
        analogWrite(C[i]->dimLedPin, 0);
      }
      //report sensor value
      //Serial.print(C[i]->output);
    } else {
      //switch off LEDs
      digitalWrite(C[i]->ledPin, LOW);
      digitalWrite(C[i]->statusLedPin, LOW);
      //analogWrite(C[i]->dimLedPin, 0);

      //report 0 value
      //Serial.print(0);
    }
  }
  if (newData >= 0) {
    //only prints if at least one clips' value is higher than 0
    for (int i = 0; i < CLIP_NUM; i++) {
      Serial.print(C[i]->output);
      Serial.print(' ');
    }
    Serial.println();

    if (OUTPUT_MIDI) {

      //clips 0,2,3
      
      if (C[2]->output <= (avgC2.smooth(C[2]->output) - 3)) { //|| C[2]->output > (lastC2 + 3) ){
        //dark fluid = gate on
        MIDI.sendRealTime(midi::Start);
      }

      if (C[2]->output >= (avgC2.smooth(C[2]->output) + 3)) { //|| C[3]->output > (lastC3 + 3) ){
        //transparent = gate off
        MIDI.sendRealTime(midi::Stop);
      }

      if (C[3]->output <= (avgC3.smooth(C[3]->output) - 7)) { //|| C[2]->output > (lastC2 + 3) ){
        MIDI.sendRealTime(midi::SystemReset);
      }

      if (C[3]->output >= (avgC3.smooth(C[3]->output) + 7)) { //|| C[3]->output > (lastC3 + 3) ){
        MIDI.sendRealTime(midi::SystemReset);
      }

      MIDI.sendAfterTouch(C[0]->output, 1);

      //clips 1,4,5
      if (C[4]->output <= (avgC4.smooth(C[4]->output) - 3)) {
        MIDI.sendNoteOn(48, 127 - C[1]->output, 1);
      }

      if (C[4]->output >= (avgC4.smooth(C[4]->output) + 3) || !C[4]->active) {
        MIDI.sendNoteOff(48, C[1]->output, 1);
      }

      if (C[5]->output <= (avgC5.smooth(C[5]->output) - 7)) {
        MIDI.sendNoteOn(55, 127 - C[1]->output, 1);
      }

      if (C[5]->output >= (avgC5.smooth(C[5]->output) + 7) || !C[5]->active) {
        MIDI.sendNoteOff(55, C[1]->output, 1);
      }

      lastC2 = C[2]->output;
      lastC3 = C[3]->output;
      lastC4 = C[4]->output;
      lastC5 = C[5]->output;

      MIDI.sendControlChange(1, C[1]->output, 1); //CC1 = mod wheeel
      //MIDI.sendRealTime(midi::SystemReset);
      //MIDI.sendRealTime(midi::Start);
      //MIDI.sendRealTime(midi::Stop);
      //MIDI.sendRealTime(midi::Clock);
    }
  }
}
