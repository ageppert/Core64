/*
PURPOSE: Insert a sub-system API between the OLED Screen libraries and the top level applications.
         The problem is that I want to access the library functions directly...
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

// To access the instance of display that is defined in the OLED_Screen subsystem, this is required:
#include <Adafruit_SSD1306.h>
extern Adafruit_SSD1306 display;

void OLEDScreenSetup();
void OLEDScreenUpdate();
void OLEDScreenClear();
void OLEDTopLevelModeSet(uint8_t state);
void OLED_Show_Matrix_Mono_Hex();			// TO DO: Pass in a pointer to this function with data to be displayed.
void OLED_Display_Stability_Work_Around();

#endif
