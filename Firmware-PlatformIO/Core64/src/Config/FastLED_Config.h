/*
PURPOSE: Configure the NeoPixel Style LED Array for the FastLED library
Pimoroni Unicorn Hat: https://shop.pimoroni.com/products/unicorn-hat 
*/
 
#ifndef FASTLED_CONFIG_H
	#define FASTLED_CONFIG_H

	#if (ARDUINO >= 100)
	#include <Arduino.h>
	#else
	#include <WProgram.h>
	#endif

	#include <stdint.h>
	#include "Config/HardwareIOMap.h"

	#define COLOR_ORDER GRB
	#ifdef CORE64_LED_MATRIX
		#define CHIPSET     WS2813			// Used in Core64 LED MATRIX
	#else
		#define CHIPSET     WS2812B			// Used in Pimoroni Unicorn Hat. Limited to a data rate of about 800Kbps, hard coded in FastLED library. 
	#endif
	#define C16P_BRIGHTNESS_DEFAULT		15	// Default and only brightness because there is no diffuser or brightness sensor on Core16. 
	#define BRIGHTNESS       		   185	// Initial brightness level
	#define BRIGHTNESS_MIN	 			 5	// Lowest useable brightness
	#define BRIGHTNESS_MAX			   255	// Highest useable brightness

	// 100 = GREEN
	// 135,255,255 = OLED aqua
	#define DEFAULTLEDArrayMonochromeColorH		135
	#define DEFAULTLEDArrayMonochromeColorS		255
	#define DEFAULTLEDArrayMonochromeColorV 	255


	uint8_t kMatrixWidth = 8;
	uint8_t kMatrixHeight = 8;
	#ifdef CORE64_LED_MATRIX
		const bool    kMatrixSerpentineLayout = false;	
	#else
		const bool    kMatrixSerpentineLayout = true;
	#endif

	uint16_t NumLedsC64 = (kMatrixWidth * kMatrixHeight);
	#define NUM_LEDS_C16 16														// Needed here because it is referenced even when using FastLED with Teensy-based Core64.
	#define NUM_LEDS_C64 64
	CRGB leds_plus_safety_pixel[ NUM_LEDS_C64 + 1];
	CRGB* const leds( leds_plus_safety_pixel + 1);

#endif
