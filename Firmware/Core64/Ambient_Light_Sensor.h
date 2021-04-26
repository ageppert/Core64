/*
PURPOSE: Test the AMBIENT_LIGHT_SENSOR
SETUP: See source file
*/
 
#ifndef AMBIENT_LIGHT_SENSOR_H
#define AMBIENT_LIGHT_SENSOR_H

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <stdint.h>

//#ifdef __cplusplus
//extern "C" {
//#endif

void AmbientLightSetup();
uint8_t GetAmbientLightLevel8BIT();
void ReadAmbientLightLevel();
void AmbientLightUpdate();
bool AmbientLightAvaible();

//#ifdef __cplusplus
//} // extern "C"
//#endif

#endif
