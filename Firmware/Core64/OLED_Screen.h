/*
PURPOSE: Set-up and update the NeoPixel Style LED Array with non-blocking code
SETUP:
- Configure in LED_Array.c
*/
 
#ifndef OLED_SCREEN_H
#define OLED_SCREEN_H

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <stdint.h>

//#ifdef __cplusplus
//extern "C" {
//#endif

void OLEDScreenSetup();
void OLEDScreenUpdate();
void OLEDScreenClear();
void OLEDSetTopLevelState(uint8_t state);
void OLED_Show_Matrix_Mono_Hex();			// TO DO: Pass in a pointer to this function with data to be displayed.

//#ifdef __cplusplus
//} // extern "C"
//#endif

#endif
