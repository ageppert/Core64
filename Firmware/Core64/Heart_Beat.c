#include <stdint.h>
#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include "HardwareIOMap.H"

void HeartBeatSetup() {
  pinMode(Pin_Built_In_LED, OUTPUT);
  digitalWriteFast(Pin_Built_In_LED, 1);
}

// Purpose: Blink an LED so the user knows the system is alive.
void HeartBeat() {
  static unsigned long HeartBeatSequence[] = {150,150,150,550}; // On, off, on, off 
  static unsigned HeartBeatSequencePosition = 0;
  static unsigned long NowTime = 0;
  static uint8_t LED_HEARTBEAT_STATE = 1;
  static unsigned long ledHeartBeatTimer = 0;
  NowTime = millis();
  if ((NowTime - ledHeartBeatTimer) >= HeartBeatSequence[HeartBeatSequencePosition])
  {
    LED_HEARTBEAT_STATE = !LED_HEARTBEAT_STATE;
    ledHeartBeatTimer = NowTime;
    HeartBeatSequencePosition++;
    if(HeartBeatSequencePosition>3) {HeartBeatSequencePosition = 0;}
  }
  digitalWriteFast(Pin_Built_In_LED, LED_HEARTBEAT_STATE);
}
