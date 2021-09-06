#include <stdint.h>
#include <stdbool.h>
#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include "HardwareIOMap.h"
#include <Wire.h>   // Default is SCL0 and SDA0 on pins 19/18 of Teensy LC
// #define not needed, as Wire.h library takes care of this pin configuration.
// #define Pin_I2C_Bus_Data       18
// #define Pin_I2C_Bus_Clock      19
#include "Analog_Input_Test.h"
#include "LED_Array_HAL.h"

#include <Adafruit_GFX.h>
#include <Fonts/FreeMono9pt7b.h>  // https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define CLK_DURING   2000000   // I2C frequency during OLED write, like Wire.setClock(). Default was 400 kHz if not specified.
#define CLK_AFTER    2000000   // I2C frequency during OLED write, like Wire.setClock(). Default was 100 kHz if not specified.

#if defined OLED_64X128
  #include <Adafruit_SSD1306.h>
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, CLK_DURING, CLK_AFTER);
#elif defined OLED_128X128
  #include <Adafruit_SSD1327.h>
  #define SCREEN_WIDTH  128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 128 // OLED display height, in pixels
  Adafruit_SSD1327 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, CLK_DURING, CLK_AFTER);
#else
  #include <Adafruit_SSD1306.h>
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, CLK_DURING, CLK_AFTER);
#endif

uint8_t TopLevelModeLocal = 0;

// Call this routine to update the OLED display.
// Refreshing the OLED display is otherwise not stable, possibly due to some library compression stuff.
void OLED_Display_Stability_Work_Around() {   
  display.invertDisplay(true);        // Inverting and
  display.invertDisplay(false);       // Reverting the screen memory seems to be a good workaround.
  display.display();
}

void OLEDScreenSplash() {
// Short
  /*
  display.clearDisplay();
  display.setCursor(0, 0);     // Start at top-left corner
  display.print(F("State:"));
  display.println(TopLevelModeLocal,DEC);
  display.display();
  */
// Long
  display.setTextSize(1);
  display.clearDisplay();
  display.setCursor(0, 10);     // Start at top-left corner
  display.println(F(" Core64.io"));
  display.print(F("HV:"));
  display.print(HardwareVersionMajor);
  display.print(F("."));
  display.print(HardwareVersionMinor);
  display.print(F("."));
  display.println(HardwareVersionPatch);
  display.println(FIRMWAREVERSION);
  display.print(F("S:"));
  display.print(TopLevelModeLocal,DEC);  
  display.print(F(" "));
  display.print(F("V:"));
  display.println(GetBatteryVoltageV(),2);
  // display.println(F("mV"));
  OLED_Display_Stability_Work_Around();
}

void OLEDScreenSetup() {
#if defined OLED_64X128
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64 
    Serial.println(F("SSD1306 allocation failed"));
  }
  else
  {
    Serial.println(F("SSD1306 allocation did not fail"));
  }
  display.setTextColor(WHITE); // Draw white text
#elif defined OLED_128X128
  if(!display.begin(0x3C, 1)) { // Address 0x3C for 128x128 
    Serial.println(F("SSD1327 allocation failed"));
  }
  else
  {
    Serial.println(F("SSD1327 allocation did not fail"));
  }
  display.setTextColor(SSD1327_WHITE); // Draw white text
#else
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64 (default)
    Serial.println(F("SSD1306 allocation failed"));
  }
  else
  {
    Serial.println(F("SSD1306 allocation did not fail"));
  }
  display.setTextColor(WHITE); // Draw white text
#endif

  display.clearDisplay();
  display.display();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setFont(&FreeMono9pt7b);
/*  
  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.println(F(" Core64.io"));
  display.println(F("   Andy   "));
  display.println(F("  Geppert "));
  display.display();
*/
  OLEDScreenSplash();
}

void OLEDScreenUpdate() {
  static unsigned long UpdatePeriodms = 100;  
  static unsigned long NowTime = 0;
  static unsigned long UpdateTimer = 0;
  NowTime = millis();
  if ((NowTime - UpdateTimer) >= UpdatePeriodms)
  {
    UpdateTimer = NowTime;
    OLEDScreenSplash();                             // TO DO: This refresh causes the aqua colored Hackaday logo (and others) to blink. Is it signal interference?
  }
}

void OLEDScreenClear() {
    display.clearDisplay();
    display.setCursor(0, 0);
    OLED_Display_Stability_Work_Around();
}

void OLEDSetTopLevelMode(uint8_t state) {
  TopLevelModeLocal = state;
}

void OLED_Show_Matrix_Mono_Hex() {
  uint64_t Full64BitValue;
  uint8_t  HexValue;
  static unsigned long UpdatePeriodms = 50;  
  static unsigned long NowTime = 0;
  static unsigned long UpdateTimer = 0;
  NowTime = millis();
  if ((NowTime - UpdateTimer) >= UpdatePeriodms)
  {
    UpdateTimer = NowTime;
    display.clearDisplay();
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setCursor(0,9);     // Start at top-left corner
    display.println(F("Hex View: "));
    Full64BitValue = LED_Array_Binary_Read();
    display.print(F(" "));
    for(int8_t i = 60; i >= 0; i=i-4)
    {
      HexValue = (Full64BitValue >> i);           // Get the 4 LSb to display in hex, but also 4 MSb that are not wanted.
      HexValue = (HexValue & B00001111);          // Mask out the 4 MSb and keep only 4 LSb
      if (!HexValue) {display.print(F("0"));}
      else {display.print(HexValue,HEX);}
      if (i==32) {display.println(); display.print(F(" "));}
    }
    display.println();
    display.print(F("S:"));
    display.print(TopLevelModeLocal,DEC);  
    display.print(F(" "));
    display.print(F("V:"));
    display.println(GetBatteryVoltageV(),2);
    OLED_Display_Stability_Work_Around();
  }
}















