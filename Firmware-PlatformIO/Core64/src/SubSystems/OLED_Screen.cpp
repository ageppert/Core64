#include <stdint.h>
#include <stdbool.h>

/*
#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
*/

#include "SubSystems/OLED_Screen.h"
#include "Config/HardwareIOMap.h"
#include "Config/Firmware_Version.h"
#include "Mode_Manager.h"

  #include <Wire.h>   // Default is SCL0 and SDA0 on pins 19/18 of Teensy LC
  // #define not needed, as Wire.h library takes care of this pin configuration.
  // #define Pin_I2C_Bus_Data       18
  // #define Pin_I2C_Bus_Clock      19
  #include "SubSystems/Analog_Input_Test.h"
  #include "Hal/LED_Array_HAL.h"

  #include <Adafruit_GFX.h>
  //#include <Fonts/FreeMono9pt7b.h>  // https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
  //#include <Fonts/FreeMonoBold7pt7b.h>

  // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
  #define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
  #define CLK_DURING   1000000   // I2C frequency during OLED write, like Wire.setClock(). Default was 400 kHz if not specified.
  #define CLK_AFTER    1000000   // I2C frequency during OLED write, like Wire.setClock(). Default was 100 kHz if not specified.

  #if defined OLED_64X128
    #include <Adafruit_SSD1306.h>
    #define SCREEN_WIDTH 128 // OLED display width, in pixels
    #define SCREEN_HEIGHT 64 // OLED display height, in pixels
    #if defined  MCU_TYPE_MK20DX256_TEENSY_32
      Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, CLK_DURING, CLK_AFTER);
    #elif defined MCU_TYPE_RP2040
      MbedI2C I2C1_OLED(p10,p11);
      Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C1_OLED, OLED_RESET, CLK_DURING, CLK_AFTER);
    #endif

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

  #if defined OLED_SPI_128x128_COLOR
    #include <Adafruit_GFX.h>
    #include <Adafruit_SSD1351.h>
    // #include <SPI.h>
    // Screen dimensions
    #define OLED_SPI_COLOR_SCREEN_WIDTH  128
    #define OLED_SPI_COLOR_SCREEN_HEIGHT 128 // Change this to 96 for 1.27" OLED.
    // Color definitions
    #define	OLED_SPI_COLOR_BLACK           0x0000
    #define	OLED_SPI_COLOR_BLUE            0x001F
    #define	OLED_SPI_COLOR_RED             0xF800
    #define	OLED_SPI_COLOR_GREEN           0x07E0
    #define OLED_SPI_COLOR_CYAN            0x07FF
    #define OLED_SPI_COLOR_MAGENTA         0xF81F
    #define OLED_SPI_COLOR_YELLOW          0xFFE0  
    #define OLED_SPI_COLOR_WHITE           0xFFFF
    // Option 1: use any pins but a little slower
    Adafruit_SSD1351 SpiOledColor = Adafruit_SSD1351(OLED_SPI_COLOR_SCREEN_WIDTH, OLED_SPI_COLOR_SCREEN_HEIGHT, Pin_SPI_CS2, Pin_SPI_DC, Pin_SPI_SDO, Pin_SPI_CLK, Pin_SPI_RST);  
    // Option 2: must use the hardware SPI pins 
    // This is much faster - also required if you want
    // to use the microSD card (see the image drawing example)
    // Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);
  #endif

  static uint8_t TopLevelModeLocal = 0;
  static bool OledScreenSetupComplete = false;

  // Call this routine to update the OLED display.
  // Refreshing the OLED display is otherwise not stable, possibly due to some library compression stuff.
  void OLED_Display_Stability_Work_Around() {   
    display.invertDisplay(true);        // Inverting and
    display.invertDisplay(false);       // Reverting the screen memory seems to be a good workaround.
    display.display();
  }

  void OLED_SPI_COLOR_SPLASH() {
    #if defined OLED_SPI_128x128_COLOR
      SpiOledColor.fillRect(0, 0, 128, 128, OLED_SPI_COLOR_BLACK);
      SpiOledColor.setCursor(0,0);
      SpiOledColor.setTextSize(1);
      SpiOledColor.println();
      SpiOledColor.print(" ");
      SpiOledColor.setTextSize(4);
      SpiOledColor.setTextColor(OLED_SPI_COLOR_GREEN);
      SpiOledColor.println(" VCF");
      SpiOledColor.setTextSize(2);
      SpiOledColor.println();
      SpiOledColor.setTextColor(OLED_SPI_COLOR_YELLOW);
      SpiOledColor.println(" Make Core");
      SpiOledColor.println();
      SpiOledColor.println(" Memories!");
      SpiOledColor.setTextSize(1);
      SpiOledColor.setTextColor(OLED_SPI_COLOR_WHITE);
      SpiOledColor.println();
      SpiOledColor.println();
      SpiOledColor.println("    www.Core64.io");
    #endif
  }

  void OLEDScreenSplash() {
    OLEDTopLevelModeSet(TopLevelModeGet());
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
    display.setCursor(0, 0);     // Start at top-left corner
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { display.println(F("   www.Core16.io")); }
    else { display.println(F("   www.Core64.io")); }
    display.print(F("HWV: "));
    display.print(HardwareVersionMajor);
    display.print(F("."));
    display.print(HardwareVersionMinor);
    display.print(F("."));
    display.print(HardwareVersionPatch);
    #if defined  MCU_TYPE_MK20DX256_TEENSY_32
      display.println(F(" (Teensy)"));
    #elif defined MCU_TYPE_RP2040
      display.println(F(" (Pico)"));    
    #endif
    display.print(F("FWV: "));
    display.print(FirmwareVersionMajor);
    display.print(".");
    display.print(FirmwareVersionMinor);
    display.print(".");
    display.println(FirmwareVersionPatch);
    display.print(F("Volts: "));
    display.println(GetBatteryVoltageV(),2);
    display.print("");
    display.print(F("  Mode: "));
    display.println(TopLevelModeLocal,DEC);
    display.println(TOP_LEVEL_MODE_NAME_ARRAY[TopLevelModeLocal]);
    OLED_Display_Stability_Work_Around();
  }

  void OLEDScreenSetup() {
    Serial.print("  OLED Screen Setup started.");

    #if defined  MCU_TYPE_MK20DX256_TEENSY_32
      // Wire.begin(); // Nothing to do here with the Arduino Core for I2C.
    #elif defined MCU_TYPE_RP2040
      // "Begin" is needed before the driver tries to talk to the hardware for Pico with MBED.
      I2C1_OLED.begin();                     // testing this as a replacement to wire.xxxxx calls for Pico MBED
      Serial.println("      I2C1_OLED.begin() was called.");
    #endif

      #if defined OLED_64X128
        if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64 
          Serial.println(F("    SSD1306 allocation failed."));
        }
        else
        {
          Serial.println(F("    SSD1306 allocation did not fail."));
          OledScreenSetupComplete = true;
        }
        display.setTextColor(WHITE); // Draw white text
      #elif defined OLED_128X128
        if(!display.begin(0x3C, 1)) { // Address 0x3C for 128x128 
          Serial.println(F("    SSD1327 allocation failed."));
        }
        else
        {
          Serial.println(F("    SSD1327 allocation did not fail."));
        }
        display.setTextColor(SSD1327_WHITE); // Draw white text
      #else
        if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64 (default)
          Serial.println(F("    SSD1306 allocation failed."));
        }
        else
        {
          Serial.println(F("    SSD1306 allocation did not fail."));
        }
        display.setTextColor(WHITE); // Draw white text
      #endif
      
      #if defined OLED_SPI_128x128_COLOR
        SpiOledColor.begin();
        OLED_SPI_COLOR_SPLASH();
      #endif

      display.clearDisplay();
      display.display();
      display.setTextSize(1);      // Normal 1:1 pixel scale
      display.setCursor(0, 0);     // Start at top-left corner
      display.cp437(true);         // Use full 256 char 'Code Page 437' font
      //display.setFont(&FreeMono9pt7b);
      display.setFont(NULL);
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
      Serial.print("  OLED Screen Setup completed.");
  }

  void OLEDScreenUpdate() {
    static unsigned long UpdatePeriodms = 100;  
    static unsigned long NowTime = 0;
    static unsigned long UpdateTimer = 0;
    if (OledScreenSetupComplete) {
      NowTime = millis();
      if ((NowTime - UpdateTimer) >= UpdatePeriodms)
      {
        UpdateTimer = NowTime;
        OLEDScreenSplash();     // TO DO: This refresh causes the aqua colored Hackaday logo (and others) to blink. Is it signal interference?
      }
    }
    else {
      Serial.print("  OLED Screen has not been initialized yet. Cannot update.");
    }
  }

  void OLEDScreenClear() {
      display.clearDisplay();
      display.setCursor(0, 0);
      OLED_Display_Stability_Work_Around();
  }

  void OLEDTopLevelModeSet(uint8_t state) {
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
        HexValue = (HexValue & 0b00001111);          // Mask out the 4 MSb and keep only 4 LSb
        if (!HexValue) {display.print(F("0"));}
        else {display.print(HexValue,HEX);}
        if (i==32) {display.println(); display.print(F(" "));}
      }
      display.println();
      display.print(F("M:"));
      display.print(TopLevelModeLocal,DEC);  
      display.print(F(" "));
      display.print(F("V:"));
      display.println(GetBatteryVoltageV(),2);
      OLED_Display_Stability_Work_Around();
    }
  }
