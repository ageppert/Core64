#include <Arduino.h>  // Required in order to build in PlatformIO

/*
    ____                __   _  _   
   / ___|___  _ __ ___ / /_ | || |  
  | |   / _ \| '__/ _ \ '_ \| || |_ 
  | |__| (_) | | |  __/ (_) |__   _|
   \____\___/|_|  \___|\___/   |_|  
                                  
  Core64 Interactive Core Memory - Project website: www.Core64.io
  2019-2021 Concept and Design by Andy Geppert of www.MachineIdeas.com
  Hardware version and details in HardwareIOMap.h
  This source code: https://www.github.com/ageppert/Core64
  
  DEVELOPMENT ENVIRONMENT
    Arduino IDE 1.8.9                                       https://www.arduino.cc/en/Main/Software
    Core64 requires TEENSYDUINO LOADER 1.53                 https://www.pjrc.com/teensy/td_download.html
      Select ALL additional libraries during installation of Teensyduino Loader and associate with the Arduino 1.8.9 installation.
    Core64c does not use the TEENSYDUINO LOADER AT ALL

  LIBRARY DEPENDENCIES
    USER MUST INSTALL MANUALLY IN ARDUINO
      Arduino > Tools > Manage Libraries > Install
      The libraries should end up being in your "Libraries" folder in your default Arduino Sketchbook location.
      Adafruit_SSD1306                              2.2.0   by Adafruit for Monochrome OLED 128x64 and 128x32
      Adafruit_GFX_Library                          1.10.6  by Adafruit
      Adafruit_BusIO                                1.3.1   by Adafruit, testeing 1.9.3 with RP2040 support
    TEENSYDUINO LOADER 1.53 INSTALLED THESE
      Wire                                          1.0     in Arduino1.8.9.app/Contents/Java/hardware/teensy/avr/libraries/Wire/
      FastLED                                       3.3.3   in Arduino1.8.9.app/Contents/Java/hardware/teensy/avr/libraries/FastLED by Daniel Garcia
      SPI                                           1.0     in Arduino1.8.9.app/Contents/Java/hardware/teensy/avr/libraries/SPI/
    INCLUDED IN THIS PROJECT'S SRC DIRECTORY
      Si7210
      LTR329
      CommandLine                                   2.1.0   by Bas Stottelaar (MIT License) https://github.com/basilfx/Arduino-CommandLine 
    OPTIONAL STUFF IF YOU ADD THIS HARDWARE
      Adafruit ILI9341 Library                      1.5.6   by Adafruit
      Adafruit_SSD1327                              1.0.0   by Adafruit for Monochrome OLED 128x128
      Adafruit_SSD1351                              1.2.7   by Adafruit for Color OLED 1.27" and 1.5" in the Adafruit shop
      SdFat - Adafruit Fork                         1.2.3   by Bill Greiman (fork of SdFat)
      SparkFun Ambient Light Sensor Arduino Library 1.0.3   by Ellas Santistevan
      TeensyView                                    1.1.0   by Sparkfun for monochrome OLED 128x32 in the Sparkfun store
 */

#include <stdint.h>
#include <stdbool.h>

#include "Config/HardwareIOMap.h"
#include "SubSystems/Heart_Beat.h"
#include "SubSystems/Serial_Port.h"
#include "Hal/LED_Array_HAL.h"
#include "Hal/Neon_Pixel_HAL.h"
#include "SubSystems/OLED_Screen.h"
#include "SubSystems/Analog_Input_Test.h"
#include "Hal/Buttons_HAL.h"
#include "Hal/Core_HAL.h"
#include "Hal/EEPROM_HAL.h"
#include "SubSystems/I2C_Manager.h"
#include "SubSystems/SD_Card_Manager.h"
#include "SubSystems/Ambient_Light_Sensor.h"
#include "Libraries/CommandLine/CommandLine.h"
#include "Drivers/Core_Driver.h"
#include "SubSystems/Test_Functions.h"
#include "Hal/Debug_Pins_HAL.h"

#include "SubSystems/Command_Line_Handler.h"
#include "Mode_Manager.h"

extern uint8_t DebugLevel;  // See Serial_Debug.cpp to set default
static uint32_t MainLoopStartTime = 0 ;
static uint32_t MainLoopEndTime = 0 ;
static uint32_t MainLoopDurationLast = 0 ;
static uint32_t MainLoopDurationShortest = 123456 ;
static uint32_t MainLoopDurationLongest = 0 ;
const  uint32_t MainLoopDebugTimeUpdatePeriod = 1000; 
static uint32_t MainLoopDebugTimeUpdateLast = 0; 

void setup() {
  SerialPortSetup();
  TopLevelModeSetToDefault();
  HeartBeatSetup();
  #if defined BOARD_CORE64_TEENSY_32
    LED_Array_Init();
    LED_Array_Test_Pixel_Matrix_Color();
    OLEDScreenSetup();
    I2CManagerSetup();
    //  delay(3000);
    I2CManagerBusScan();
    // TO DO: Most of this setup should occur after the hardware version is determined, so setup is configured appropriately
    Buttons_Setup();
    CoreSetup();
    SDCardSetup();
    AmbientLightSetup();
    Neon_Pixel_Array_Init();
  #elif defined BOARD_CORE64C_RASPI_PICO
    I2CManagerSetup();
    //  delay(3000);
    I2CManagerBusScan();
    OLEDScreenSetup();
    Buttons_Setup();
  #endif
    ReadHardwareVersion();
    CommandLineSetup();
    AnalogSetup();
    AnalogUpdate();
  Serial.println("Serial Monitor should be connected by now.");
  if(DebugLevel==1) { Serial.println("END OF SETUP"); }
}

void loop() {
  MainLoopStartTime = millis(); 
  /*                      *********************
                          *** Housekeepting ***
                          *********************
  */
  if(DebugLevel==1) { Serial.println("DEBUG: START OF HOUSEKEEPING"); }
  HeartBeat(); 
  AnalogUpdate();
  CommandLineUpdate();
  #if defined BOARD_CORE64_TEENSY_32
    AmbientLightUpdate();
    // SDCardVoltageLog(1000);
  #elif defined BOARD_CORE64C_RASPI_PICO
    // not yet implemented
  #endif

  if(DebugLevel==1) { Serial.println("DEBUG: START OF TOP LEVEL MODE FUNCTIONS"); }

    // Serial.print("  Switched Voltage: ");
    // Serial.println(GetBatteryVoltageV(),2);
  /*                      ************************
                          *** User Interaction ***
                          ************************
  */
  TopLevelModeManagerRun();
  if(DebugLevel==1) { Serial.println("DEBUG: END OF TOP LEVEL MODE FUNCTIONS"); }
  MainLoopEndTime = millis();
  MainLoopDurationLast = MainLoopEndTime - MainLoopStartTime;
  if (MainLoopDurationShortest > MainLoopDurationLast ) { MainLoopDurationShortest = MainLoopDurationLast; }
  if (MainLoopDurationLongest < MainLoopDurationLast ) { MainLoopDurationLongest = MainLoopDurationLast; }
  if(DebugLevel==3) { 
    if ( (MainLoopEndTime - MainLoopDebugTimeUpdateLast) >= MainLoopDebugTimeUpdatePeriod) {
      MainLoopDebugTimeUpdateLast = MainLoopEndTime; 
      Serial.print("DEBUG: MAIN LOOP DURATION (Min, Max): "); 
      Serial.print(MainLoopDurationLast); 
      Serial.print(" (");
      Serial.print(MainLoopDurationShortest);
      Serial.print(",");
      Serial.print(MainLoopDurationLongest);
      Serial.println(")");
    }
  }
}
