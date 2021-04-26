/*
PURPOSE: Set-up and share serial debug messages with non-blocking code
SETUP:
- Use the built-in serial port, set up in Serial_Debug.c
*/
 
#ifndef SERIAL_DEBUG_H
#define SERIAL_DEBUG_H

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SERIAL_PORT_SPEED   15200

void SerialDebugSetup();
void SerialDebugProcess();

#ifdef __cplusplus
} // extern "C"
#endif

#endif
