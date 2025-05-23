/*
PURPOSE: Set-up and update the NeoPixel Style LED Array with non-blocking code through abstract memory buffers.
SETUP: Configure in FastLED_Config.h
USAGE: Init, Clear, Write to one of the buffers in monochrome or color mode, update the LED array.
*/
 
#ifndef LED_ARRAY_HAL_H
#define LED_ARRAY_HAL_H

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <stdint.h>

/* Colorwheel in HSB color space for addressable LEDs:
      0 Red     R
     42 Yellow  RG
     85 Green    G
    127 Cyan     GB
    170 Blue      B
    220 Purple  R B 
    254 RED     R
    255 WHITE - special exception added by Andy
*/

// Updated 2019-01-11 for HAL over Driver architecture
// The API for the LED Array HAL
void LED_Array_Init();				// Called once from Setup. Set up the LED array with the driver using chipset, data pin, color order, correction, brightness from FastLED_Config.h, and clear the array.
void LED_Array_Memory_Clear(); 		// Clears all of the LED Array memory buffers.
uint8_t LED_Array_Get_Pixel_Value(uint8_t y = 0, uint8_t x = 0); // Return the value of the pixel from the local LED Array memory buffer.
void LED_Array_Monochrome_Set_Color(uint8_t hue, uint8_t saturation, uint8_t value); // Override the default monochrome color for next pixel to be written to the any monochrome memory buffer.
void LED_Array_Monochrome_Increment_Color(uint8_t HueIncrement); // Increment the color hue by this amount. 0-255

void LED_Array_Start_Up_Symbol_Loop_Begin();        // Cycles through a specific symbol list for the Start-Up mode. Starts the mode at first index position.
void LED_Array_Start_Up_Symbol_Loop_Continue();     // Cycles through a specific symbol list for the Start-Up mode. Continues the mode where ever index was.

void LED_Array_Test_Count_Binary(); // Test all 64 LEDs. Counting from 0 to 2^64 from bottom up.
void LED_Array_Test_Pixel_String(); // Test all 64 LEDs. Turns on 1 pixel, sequentially, from left to right, top to bottom using 1D string addressing
// Tests four functions: clear, monochrome color change, incrementally write one pixel 1d, display 1d buffer
void LED_Array_Test_Pixel_Matrix_Mono(); // Test all 64 LEDs. Turns on all LEDs in a row/col, sequentially, from upper left to lower right using 2D matrix addressing
// Tests four functions: clear, monochrome color change, incrementally write one pixel 2d, display 2d buffer
void LED_Array_Test_Pixel_Matrix_Color(); // Test all 64 LEDs. Using multi-color symbols.
// Tests four functions: clear, multi-color symbols, , display 2d buffer
void LED_Array_Test_Rainbow_Demo();	// Full color demo.
void LED_Array_Test_All_RGB(uint8_t color);

void LED_Array_Binary_Write(uint64_t BinaryValue);
uint64_t LED_Array_Binary_Read();
void LED_Array_String_Write(uint8_t bit, bool value);
void LED_Array_Matrix_Mono_Write(uint8_t y, uint8_t x, bool value);
bool LED_Array_Matrix_Mono_Read(uint8_t y, uint8_t x);
void LED_Array_Matrix_Color_Hue_Write(uint8_t y, uint8_t x, uint8_t hue);
void LED_Array_Matrix_Color_Hue_Sat_Write(uint8_t y, uint8_t x, uint8_t hue, uint8_t sat);

void LED_Array_Binary_Display();
void LED_Array_String_Display();
void LED_Array_Matrix_Mono_Display();          // Can be replaced with LED_Array_Color_Display(1)
void LED_Array_Color_Display(bool HuenHueSat); // Display from Hue only with 1, and Hue and Sat with 0

void LED_Array_Binary_Write_Default(); // Set the Binary memory to it's default value
void LED_Array_Binary_To_Matrix_Mono(); // convert the contents of the 64 Bit Binary screen memory to 8x8 Matrix Monochrome memory
void LED_Array_Matrix_Mono_to_Binary(); // convert the contents of 8x8 Matrix Monochrome memory to the 64 Bit Binary Screen Memory

void LED_Array_Auto_Brightness();	// If ambient light sensor is available, use to adjust LED Array brightness
void LED_Array_Set_Brightness(uint8_t brightness); // Set brightness directly.
// TO DO: Clean up the naming convention of these sub-functions
void WriteColorFontSymbolToLedScreenMemoryMatrixHue(uint8_t SymbolNumber);
void WriteAppPaintSymbol(uint8_t SymbolNumber);
void WriteAppMidiSymbol(uint8_t SymbolNumber);
void WriteUtilFluxSymbol(uint8_t SymbolNumber);
void WriteAppPaintPalette(bool TopNBottom);         // Top = 1, Bottom = 0
void WriteGameSnakeSymbol(uint8_t SymbolNumber);
void WriteGamePongSymbol(uint8_t SymbolNumber);

// Older stuff that needs cleanup
void CopyCoreMemoryToMonochromeLEDArrayMemory();

#endif
