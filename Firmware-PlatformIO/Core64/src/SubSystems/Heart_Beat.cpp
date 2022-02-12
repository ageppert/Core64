#include <stdint.h>
#include <stdbool.h>

//#if (ARDUINO >= 100)
#include <Arduino.h>          // Not sure why, but this is required for Arduino IDE to recognize OUTPUT in the pinMode of setup.
//#else
//#include <WProgram.h>
//#endif

#include "SubSystems/Heart_Beat.h"
#include "Config/HardwareIOMap.h"

void HeartBeatSetup() {
  pinMode(Pin_Built_In_LED, OUTPUT);
  #if defined BOARD_CORE64_TEENSY_32
    digitalWriteFast(Pin_Built_In_LED, 1);
  #elif defined BOARD_CORE64C_RASPI_PICO
    digitalWrite(Pin_Built_In_LED, 1);
  #endif
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
  #if defined BOARD_CORE64_TEENSY_32
    digitalWriteFast(Pin_Built_In_LED, LED_HEARTBEAT_STATE);
  #elif defined BOARD_CORE64C_RASPI_PICO
    digitalWrite(Pin_Built_In_LED, LED_HEARTBEAT_STATE);
  #endif
}
