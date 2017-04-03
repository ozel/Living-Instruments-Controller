#include <usbh_midi.h>
#include <usbhub.h>
#include <EEPROM.h>

#include "clips.h"

// available instrument variants
#define BUBBLES   1 // Arduino Mega
#define TUBES     2 // Arduino Mega
#define TUBES_UNO 3 // for testing with an Arduino UNO, no USB MIDI keyboard support with this one 

//CHOOSE INSTRUMENT CONTROLLER CONFIGURATION HERE:
#define INST      BUBBLES //TUBES

// for using this with the MAX/MSP patch, all four variables below here must be set to 0!
#define DEBUG     0 //enable debug prints
#define SIMULATE  0 //generates random data

#define CLEAR_EEPROM 0          // in case of a new Arduino board, clear the EEPROM. this allows proper storage of calibration data
#define CALIB_AFTER_RESET 0     // enable or disable calibration procedure directly after reset

// define number of iterations for calibration procedure
#if (SIMULATE)
  #define CALIB_ITERATIONS  5   // only used in simulation mode, should be bigger than 2...
#else
  #define CALIB_ITERATIONS  3000
#endif

//configuration for the dimmable high-power LEDS
// DIMM_THRESHHOLD should be between 2 (lowest sensitivity) and 10 (Highest sensitivity).
#define DIMM_THRESHHOLD   10  //threshhold (in % of max. sensor value); below that, no dimming of high power LED (see clips.ino)
//target range to scale the mapped sensor value to the dimming range for the high power LEDs 
#define DIMM_MIN_LEVEL    40 //if signal above threshold, thats the min. dimmed setting   - 0   = completly off
#define DIMM_MAX_LEVEL    150 //if signal above threshold, thats the max. dimmed setting  - 255 = completly on

//max. value that every sensor output range is mapped to (minimum = 0)
#define MAX_SENSOR_VALUE  255

// don't change anything below here, unless you know what you are doing ;-)

#if (INST == BUBBLES)

#define CLIP_NUM 5
//                photo,led,  min,max,raw,out, active, status,dimm
struct clip clip1 = { 8, 26,  1023, 0,  0,  0,  true,     14,  2};
struct clip clip2 = { 9, 28,  1023, 0,  0,  0,  true,     15,  3};
struct clip clip3 = {10, 30,  1023, 0,  0,  0,  true,     16,  4};
struct clip clip4 = {11, 32,  1023, 0,  0,  0,  true,     17,  5};
struct clip clip5 = {12, 34,  1023, 0,  0,  0,  true,     18,  6};
// struct clip clip6 = {13, 36,  1023, 0,  0,  0,  false,     19,  7}; //TODO: pin 7 might interfere with the USB host shield
struct clip *photoClips[CLIP_NUM] = {&clip1, &clip2, &clip3, &clip4, &clip5};

#elif (INST == TUBES)

#define CLIP_NUM 6
//                photo,led,  min,max,  raw,out, active, status,dimm
struct clip clip1 = { 8, 26,  511, 1023,  0,  0,  true,     14,  2}; //pressure sensor, min/max calibration disabled
struct clip clip2 = { 9, 28,  511, 1023,  0,  0,  true,     15,  3}; //pressure sensor, min/max calibration disabled
struct clip clip3 = {10, 30,  1023,   0,  0,  0,  true,     16,  4};
struct clip clip4 = {11, 32,  1023,   0,  0,  0,  true,     17,  5};
struct clip clip5 = {12, 34,  1023,   0,  0,  0,  true,     18,  6};
struct clip clip6 = {13, 36,  1023,   0,  0,  0,  true,     19,  7}; //TODO: pin 7 might interfere with the USB host shield
struct clip *photoClips[CLIP_NUM] = {&clip1, &clip2, &clip3, &clip4, &clip5, &clip6};

#elif (INST == TUBES_UNO)

#define CLIP_NUM 6
//simple UNO board, no status & dimm LEDs, NO USB HOST shield -> collides with pins 10-13
//                photo,led,  min,   max,raw,out,active,status,dimm
struct clip clip1 = { 0,  8,   511, 1023,  0,  0, true,     0,  0}; //pressure sensor, min/max calibration disabled
struct clip clip2 = { 1,  9,   511, 1023,  0,  0, true,     0,  0}; //pressure sensor, min/max calibration disabled
struct clip clip3 = { 2, 10,  1023,    0,  0,  0, true,     0,  0};
struct clip clip4 = { 3, 11,  1023,    0,  0,  0, true,     0,  0};
struct clip clip5 = { 4, 12,  1023,    0,  0,  0, true,     0,  0};
struct clip clip6 = { 5, 13,  1023,    0,  0,  0, true,     0,  0};
struct clip *photoClips[CLIP_NUM] = {&clip1, &clip2, &clip3, &clip4, &clip5, &clip6};

#endif

USB  Usb;
//USBHub Hub(&Usb);
USBH_MIDI  Midi(&Usb);

void MIDI_poll();
uint16_t pid, vid;
//int debug_led = 13;


void setup() //define launch routine parameters

{
  Serial.begin(9600);  //Begin serial communcation

//  pinMode(debug_led, OUTPUT);
//  digitalWrite(debug_led, LOW);
  
  if(INST != TUBES_UNO) {
    //setup USB host shield
    vid = pid = 0;
    if (Usb.Init() == -1) {
      while (1); //halt
    }
    delay( 200 );
  }

  randomSeed(analogRead(5));
  
  if (DEBUG) Serial.println("Living Instrument says Hello World!");
  
  int testread = 0;
  EEPROM.get(0, testread);
  if(testread == -1 || CLEAR_EEPROM){
    if (DEBUG) Serial.println("clearing EEPROM");
    for (int i = 0 ; i < 6*2*sizeof(int) ; i++) {
      EEPROM.write(i, 0);
    }
  } 

  //configure clip pins
  if (DEBUG) Serial.println("configure clip pins");
  clips_conf_pins(photoClips);
  
  delay(100);
  if (CALIB_AFTER_RESET) { 
    //calibrate clip output
    clips_calibrate(photoClips);
  }

  if (DEBUG) Serial.println("------------");

//  digitalWrite(debug_led, HIGH);
//  delay(500);
//  digitalWrite(debug_led, LOW);
//  delay(500);
//  digitalWrite(debug_led, HIGH);
//  delay(500);
//  digitalWrite(debug_led, LOW);
}

void loop() //define loop parameters
{

  Usb.Task();
  if ( Usb.getUsbTaskState() == USB_STATE_RUNNING )
  {
    MIDI_poll();
  }

  clips_read(photoClips);
  //delay(5);
  // TODO: usbh_midi example have here a special delay that always waits for 1 ms maximum
  //       we may need that here, too
  
}

// Poll USB MIDI Controler and send to serial
// activate clips based on note press
void MIDI_poll()
{
  char buf[20];
  uint8_t bufMidi[64];
  uint16_t  rcvd;

  if (Midi.vid != vid || Midi.pid != pid) {
    sprintf(buf, "VID:%04X, PID:%04X", Midi.vid, Midi.pid);
    if (DEBUG) Serial.println(buf);
    vid = Midi.vid;
    pid = Midi.pid;
  }
  //rcvd = Midi.RecvData(  bufMidi);
  //if (rcvd > 0 ) {
  while((rcvd = Midi.RecvData(  bufMidi)) > 0) {
  //if(Midi.RecvData( &rcvd,  bufMidi) == 0){
    sprintf(buf, "%08X: ", millis());
    if (DEBUG) Serial.print(buf);
    if (DEBUG) Serial.print(' ');
    if (DEBUG) Serial.print(rcvd);
    if (DEBUG) Serial.print(':');
    for (int i = 0; i < 3; i++) {
      sprintf(buf, " %02X", bufMidi[i]);
      if (DEBUG) Serial.print(buf);
    }
    if ( bufMidi[0] == 0x90) {
      //note on
      if (DEBUG) Serial.print(" note on");
      switch (bufMidi[1]) {
        //lower octave
        case 0x30:
          photoClips[0]->active = true;
          break;
        case 0x32:
          photoClips[1]->active = true;
          break;
        case 0x34:
          photoClips[2]->active = true;
          break;
        case 0x35:
          photoClips[3]->active = true;
          break;
        case 0x37:
          photoClips[4]->active = true;
          break;
#if (CLIP_NUM == 6)
        case 0x39:
          photoClips[5]->active = true;
          break;
#endif
        //higher octave, toggles mute
        case 0x3C:
          photoClips[0]->active = !photoClips[0]->active;
          break;
        case 0x3E:
          photoClips[1]->active = !photoClips[1]->active;
          break;
        case 0x40:
          photoClips[2]->active = !photoClips[2]->active;
          break;
        case 0x41:
          photoClips[3]->active = !photoClips[3]->active;
          break;
        case 0x43:
          photoClips[4]->active = !photoClips[4]->active;
          break;
#if (CLIP_NUM == 6)
        case 0x45:
          photoClips[5]->active = !photoClips[5]->active;
          break;
#endif
        case 0x54: // two Cs up from the toggle mute's base C
          //calibrate clip output
          clips_calibrate(photoClips);
          break;
      }
    } else if ( bufMidi[0] == 0x80) {
      //note off
      if (DEBUG) Serial.print(" note off");
      switch (bufMidi[1]) {
        //lower octave
        case 0x30:
          photoClips[0]->active = false;
          break;
        case 0x32:
          photoClips[1]->active = false;
          break;
        case 0x34:
          photoClips[2]->active = false;
          break;
        case 0x35:
          photoClips[3]->active = false;
          break;
        case 0x37:
          photoClips[4]->active = false;
          break;
#if (CLIP_NUM == 6)
        case 0x39:
          photoClips[5]->active = false;
          break;
#endif
      }
    }
    if (DEBUG) Serial.println();
  }
}
