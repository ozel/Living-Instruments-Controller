#include "clips.h"

#define DEBUG 0

//#define INST1 1

#ifdef INST1

#define CLIP_NUM 5
//                photo,led,  min,max,raw,out, status,dimm
struct clip clip1 = { 8, 26,  1023, 0,  0,  0,      0,  0};
struct clip clip2 = { 9, 28,  1023, 0,  0,  0,      0,  0};
struct clip clip3 = {10, 30,  1023, 0,  0,  0,      0,  0};
struct clip clip4 = {11, 32,  1023, 0,  0,  0,      0,  0};
struct clip clip5 = {12, 34,  1023, 0,  0,  0,      0,  0};
struct clip *photoClips[] = {&clip1, &clip2,&clip3,&clip4,&clip5};

#else

#define CLIP_NUM 6
//simple UNO board, no status & dimm LEDs
//                photo,led,  min,max,raw,out, status,dimm
struct clip clip1 = { 0,  8,  1023, 0,  0,  0,      0,  0}; //pressure sensor
struct clip clip2 = { 1,  9,  1023, 0,  0,  0,      0,  0}; //pressure sensor
struct clip clip3 = { 2, 10,  1023, 0,  0,  0,      0,  0};
struct clip clip4 = { 3, 11,  1023, 0,  0,  0,      0,  0};
struct clip clip5 = { 4, 12,  1023, 0,  0,  0,      0,  0};
struct clip clip6 = { 5, 13,  1023, 0,  0,  0,      0,  0};
struct clip *photoClips[] = {&clip1, &clip2,&clip3,&clip4,&clip5,&clip6};


#endif

void setup() //define launch routine parameters

{
  Serial.begin(115200);  //Begin serial communcation

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  //init default clip variables
  //clips_init(photoClips);
  if(DEBUG) Serial.println("Living Instrument says Hello World!");

  //configure clip pins
  if(DEBUG) Serial.println("configure clip pins");
  clips_conf_pins(photoClips);
  
  delay(100);
  //calibrate clip output
  if(DEBUG) Serial.println("calibrate clips");
  clips_calibrate(photoClips);
  
  if(DEBUG) Serial.println("------------");

  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
}

void loop() //define loop parameters
{

  clips_read(photoClips);
  if (photoClips[0]->output > (255/10))
  {
    digitalWrite(13, HIGH);
    //delay(100); //short delay for faster response to light.
  } else if (photoClips[0]->output  < 10)
  {
    digitalWrite(13, LOW);
  }
}
