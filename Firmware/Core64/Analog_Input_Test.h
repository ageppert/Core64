/*
PURPOSE: Test the ANANLOG input pins that are planned for reading battery voltage and/or current with non-blocking code
SETUP:
- Configure in Analog_Input_Test.c
*/
 
#ifndef ANALOG_INPUT_TEST_H
#define ANALOG_INPUT_TEST_H

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <stdint.h>

//#ifdef __cplusplus
//extern "C" {
//#endif
uint16_t GetBatteryVoltagemV();
float 	 GetBatteryVoltageV();
float 	 GetBus5V0VoltageV();
float 	 GetBus3V3VoltageV();
float    GetCoreTC0V();
float    GetCoreBC0V();
float    GetCoreLR0V();
float    GetCoreRR0V();
void     AnalogSetup();
void     AnalogUpdate();
void     AnalogUpdateCoresOnly();

//#ifdef __cplusplus
//} // extern "C"
//#endif

#endif
