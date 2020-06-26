/*  Example of playing sampled sounds,
    using Mozzi sonification library.
  
    Demonstrates one-shot samples scheduled
    with EventDelay(), and fast random numbers with 
    xorshift96() and rand(), a more friendly wrapper for xorshift96().
  
    Circuit: Audio output on digital pin 9 on a Uno or similar, or
    DAC/A14 on Teensy 3.0/3.1, or 
    check the README or http://sensorium.github.com/Mozzi/
  
    Mozzi help/discussion/announcements:
    https://groups.google.com/forum/#!forum/mozzi-users
  
    Tim Barrass 2012, CC by-nc-sa.

    This sketch is meant to work with two photoresistors (LDR) set up as
    voltage dividers and attached to Analog pins 0 and 3 of the
    Arduino. One LDR triggers a sample to play. The other adjusts the
    pitch of the sample.
*/

//#include <ADC.h>  // Teensy 3.0/3.1 uncomment this line and install http://github.com/pedvide/ADC
#include <MozziGuts.h>
#include <Sample.h> // Sample template
#include <samples/bamboo/bamboo_00_2048_int8.h> // wavetable data
#include <samples/bamboo/bamboo_01_2048_int8.h> // wavetable data
#include <samples/bamboo/bamboo_02_2048_int8.h> // wavetable data
#include <EventDelay.h>
#include <mozzi_rand.h>


const char LDR1_PIN = 0;  // set the analog input pin for one LDR
const char LDR2_PIN = 3;  // set the analog input pin for the other LDR 
const int threshold = 650; //this is the triggering level. Adjust for best results

#define CONTROL_RATE 64

// use: Sample <table_size, update_rate> SampleName (wavetable)
Sample <BAMBOO_00_2048_NUM_CELLS, AUDIO_RATE>aBamboo0(BAMBOO_00_2048_DATA);
float recorded_pitch0 = (float) BAMBOO_00_2048_SAMPLERATE / (float) BAMBOO_00_2048_NUM_CELLS;
Sample <BAMBOO_01_2048_NUM_CELLS, AUDIO_RATE>aBamboo1(BAMBOO_01_2048_DATA);
float recorded_pitch1 = (float) BAMBOO_01_2048_SAMPLERATE / (float) BAMBOO_01_2048_NUM_CELLS;
Sample <BAMBOO_02_2048_NUM_CELLS, AUDIO_RATE>aBamboo2(BAMBOO_02_2048_DATA);
float recorded_pitch2 = (float) BAMBOO_02_2048_SAMPLERATE / (float) BAMBOO_02_2048_NUM_CELLS;

// for scheduling audio gain changes
EventDelay kTriggerDelay;

boolean triggered = false;

void setup(){
  startMozzi(CONTROL_RATE);
  aBamboo0.setFreq((float) BAMBOO_00_2048_SAMPLERATE / (float) BAMBOO_00_2048_NUM_CELLS); // play at the speed it was recorded at
  aBamboo1.setFreq((float) BAMBOO_01_2048_SAMPLERATE / (float) BAMBOO_01_2048_NUM_CELLS);
  aBamboo2.setFreq((float) BAMBOO_02_2048_SAMPLERATE / (float) BAMBOO_02_2048_NUM_CELLS);
  kTriggerDelay.set(10); // countdown ms, within resolution of CONTROL_RATE
}


byte randomGain(){
  //return lowByte(xorshift96())<<1;
  return rand(200) + 55;
}

// referencing members from a struct is meant to be a bit faster than seperately
// ....haven't actually tested it here...
struct gainstruct{
  byte gain0;
  byte gain1;
  byte gain2;
}
gains;


void updateControl(){

  int knob_value = mozziAnalogRead(LDR1_PIN); // value is 0-1023
  int piezo_value = mozziAnalogRead(LDR2_PIN); // value is 0-1023

  float pitch0 = (recorded_pitch0 * (float) (knob_value) / 512.f) + 0.1f;
  float pitch1 = (recorded_pitch1 * (float) (knob_value) / 512.f) + 0.1f;
  float pitch2 = (recorded_pitch2 * (float) (knob_value) / 512.f) + 0.1f;
  aBamboo0.setFreq(pitch0/2);
  aBamboo1.setFreq(pitch1/3);
  aBamboo2.setFreq(pitch2/4);

  if (piezo_value>threshold) {
    if (!triggered){
    if(kTriggerDelay.ready()){
    switch(rand(0, 3)) {
    case 0:
      gains.gain0 = randomGain();
      aBamboo0.start();
      triggered = true;
      break;
    case 1:
      gains.gain1 = randomGain();
      aBamboo1.start();
      triggered = true;
      break;
    case 2:
      gains.gain2 = randomGain();
      aBamboo2.start();
      triggered = true;
      break;
    }
    kTriggerDelay.start();
  }
      
      
    }
  }else{
    triggered = false;
  }  
  

}


int updateAudio(){
  int asig= (int) 
    ((long) aBamboo0.next()*gains.gain0 +
      aBamboo1.next()*gains.gain1 +
      aBamboo2.next()*gains.gain2)>>4;
  //clip to keep audio loud but still in range
  if (asig > 243) asig = 243;
  if (asig < -244) asig = -244;
  return asig;
}


void loop(){
  audioHook();
}
