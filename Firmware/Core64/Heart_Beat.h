/*
PURPOSE: Heart beat built-in LED with non-blocking code
SETUP:
- Use the built-in LED, and assign the proper pin number in Heart_Beat.c for the device in use.
*/
 
#ifndef HEART_BEAT_H
#define HEART_BEAT_H

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void HeartBeatSetup();
void HeartBeat();

#ifdef __cplusplus
} // extern "C"
#endif

#endif
