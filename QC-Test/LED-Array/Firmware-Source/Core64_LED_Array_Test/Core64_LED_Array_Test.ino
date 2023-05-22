/* 
 * Title: Core64 LED Array Test
 * Description: Using output on #define PIN, cycle 64 LEDs continuously through RED 3s, GREEN 3s, BLUE 3s, WHITE (R+G+B) 3s. 
 * Assumptions: Should work for any Arduino-compatible microcontroller board. 
 * Library Required: Adafruit NeoPixel 1.11.0
 * As written, assumes/requires installation of Raspberry Pi board manager (Tools > Boards > Board Manager)
 * and configuration: Tools > Board > Raspberry Pi RP2040 Boards(2.5.0) > Raspberry Pi Pico
 * LED Array powered by 5V and driven from Raspberry Pi Pico pin 22.

 * Adapted from:
*/

// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// Released under the GPLv3 license to match the rest of the
// Adafruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        22 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 64 // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 2000 // Time (in milliseconds) to pause between pixels

void setup() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}

void loop() {
  pixels.clear(); // Set all pixel colors to 'off'

  // The first NeoPixel in a strand is #0, second is 1, all the way up
  // to the count of pixels minus one.
  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
  // Here we're using a moderately bright green color:
  
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(70, 0, 0));
  }
  pixels.show();
  delay(DELAYVAL);

  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 50, 0));
  }
  pixels.show();
  delay(DELAYVAL);

  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 70));
  }
  pixels.show();
  delay(DELAYVAL);

  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(40, 40, 40));
  }
  pixels.show();
  delay(DELAYVAL);

}
