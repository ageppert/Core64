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

#include "HardwareIOMap.h"
#include "Heart_Beat.h"
#include "Serial_Debug.h"
#include "LED_Array_HAL.h"
#include "Neon_Pixel_HAL.h"
#include "OLED_Screen.h"
#include "Analog_Input_Test.h"
#include "Buttons_HAL.h"
#include "Core_HAL.h"
#include "EEPROM_HAL.h"
#include "I2C_Manager.h"
#include "SD_Card_Manager.h"
#include "Ambient_Light_Sensor.h"
#include "src/CommandLine/CommandLine.h"
#include "Core_Driver.h"

// Command Line Stuff:
  #if defined BOARD_CORE64_TEENSY_32
    #define PROMPT "Core64> "
  #elif defined BOARD_CORE64C_RASPI_PICO
    #define PROMPT "Core64c: "
  #else
    #define PROMPT "unknown: "
  #endif
  int StreamEnable = 0;
  CommandLine commandLine(Serial, PROMPT);        // CommandLine instance.

// #define DEBUG 1

static uint32_t SerialNumber = 0;          // Default value is 0 and should be non-zero if the Serial Number is valid.
static bool TopLevelModeChanged = false;
enum TopLevelMode                  // Top Level Mode State Machine
{
  MODE_STARTUP = 0,                //   0 Start-up
  MODE_SCROLLING_TEXT,             //   1 Scrolling text at power on
  MODE_CORE_FLUX_DETECTOR,         //   2 Testing all cores and displaying core state
  MODE_MONOCHROME_DRAW,            //   3 Test LED Driver with binary values
  MODE_LED_TEST_ALL_BINARY,        //   4 Test LED Driver with binary values
  MODE_LED_TEST_ONE_STRING,        //   5 Testing LED Driver
  MODE_LED_TEST_ONE_MATRIX_MONO,   //   6 Testing LED Driver with matrix array and monochrome color
  MODE_LED_TEST_ONE_MATRIX_COLOR,  //   7 Testing LED Driver with matrix array and multi-color symbols
  MODE_TEST_EEPROM,                //   8
  MODE_LED_TEST_ALL_COLOR,         //   9 Test LED Driver with all pixels and all colors
  MODE_CORE_TOGGLE_BIT,            //  10 Test one core with one function
  MODE_CORE_TEST_ONE,              //  11 Testing core #coreToTest and displaying core state
  MODE_CORE_TEST_MANY,             //  12 Testing multiple cores and displaying core state
  MODE_HALL_TEST,                  //  13 Testing hall switch and sensor response
  MODE_LAST,                       //  14 last one, return to 0.
} ;
static uint8_t TopLevelModeDefault = MODE_STARTUP;
static uint8_t TopLevelMode = TopLevelModeDefault;
uint8_t value = 0;
uint8_t a = 0;

/*                      
                          *********************
                          ***     Setup     ***
                          *********************
*/
void setup() {
  HeartBeatSetup();
  #if defined BOARD_CORE64_TEENSY_32
    LED_Array_Init();
    SerialDebugSetup();
      Serial.begin(SERIAL_PORT_SPEED);  // Need to move this serial stuff into the Serial_Debug.c file out of here!
      LED_Array_Test_Pixel_Matrix_Color();
    EEPROM_Setup();
    OLEDScreenSetup();
    I2CManagerSetup();
      delay(3000);
    I2CManagerBusScan();
    // TO DO: Most of this setup should occur after the hardware version is determined, so setup is configured appropriately
    AnalogSetup();
    Buttons_Setup();
    CoreSetup();
    SDCardSetup();
    AmbientLightSetup();
    Neon_Pixel_Array_Init();
  #elif defined BOARD_CORE64C_RASPI_PICO
    EEPROM_Setup();
    I2CManagerSetup();
      delay(3000);
    I2CManagerBusScan();
    OLEDScreenSetup();
    Buttons_Setup();
  #endif
    CommandLineSetup();
}

/*                      
                          *********************
                          ***   MAIN LOOP   ***
                          *********************
*/

void loop() {
  static uint8_t  ColorFontSymbolToDisplay = 2;
  static bool     Button1Released = true;
  static bool     Button2Released = true;
  static bool     Button3Released = true;
  static bool     Button4Released = true;
  static uint32_t Button1HoldTime = 0;
  static uint32_t Button2HoldTime = 0;
  static uint32_t Button3HoldTime = 0;
  static uint32_t Button4HoldTime = 0;
  static uint8_t  coreToTest = 0;

  /*                      *********************
                          *** Housekeepting ***
                          *********************
  */
  HeartBeat(); 
  commandLine.update();
  #if defined BOARD_CORE64_TEENSY_32
    AnalogUpdate();
    AmbientLightUpdate();
    SDCardVoltageLog(1000);
    if (StreamEnable)
    {
      Serial.println(TopLevelMode);
    }
    #ifdef DEBUG
      Serial.println("  DEBUG enabled."); // Need to abstract this debug stuff
    #endif
  #elif defined BOARD_CORE64C_RASPI_PICO
    
  #endif
    /*                      ************************
                            *** User Interaction ***
                            ************************
    */

    // MODE SWITCHING
    // If button is pressed, go to next mode.
    // Must be released and pressed again for subsequent action.

    if (TopLevelModeChanged) {
      Serial.println("");
      Serial.print("  Mode changed to: ");
      Serial.println(TopLevelMode);
      Serial.print(PROMPT);         // Print the first prompt to show the system is ready for input
      TopLevelModeChanged = false;
    }

    Button1HoldTime = ButtonState(1,0);
    Button2HoldTime = ButtonState(2,0);
    Button3HoldTime = ButtonState(3,0);
    Button4HoldTime = ButtonState(4,0);
    if ( (Button1Released == true) && (Button1HoldTime >= 500) ){
      ButtonState(1,1); // Force a "release" after press by clearing the button hold down timer
      Button1Released = false;
      ColorFontSymbolToDisplay++;
      if(ColorFontSymbolToDisplay>3) { ColorFontSymbolToDisplay = 0; }
      TopLevelMode++;
      TopLevelModeChanged = true; // User application has one time to use this before it is reset.
    }
    else {
      if (Button1HoldTime == 0) {
        Button1Released = true;
        TopLevelModeChanged = false;
      }
    }

    switch(TopLevelMode)
    {
    case MODE_STARTUP:
      // delay(1500);               // Wait a little bit for the serial port to connect if this is connected to a computer terminal
      handleSplash("");             // Splash screen
      handleInfo("");               // Print some info about the system (this also checks hardware version, born-on, and serial number)
      handleHelp("");               // Print the help menu
      Serial.print(PROMPT);         // Print the first prompt to show the system is ready for input
      TopLevelMode++;
      TopLevelModeChanged = true;  // User application has one time to use this before it is reset.
      break;

    case MODE_SCROLLING_TEXT:
      if (TopLevelModeChanged) {
        LED_Array_Monochrome_Set_Color(135,255,255);  // 135,255,255 = OLED aqua
      }
      ScrollTextToCoreMemory();   // This writes directly to the RAM core memory array and bypasses reading it.
      #if defined SCROLLING_TEXT_BYPASS_CORE_MEMORY
      #else
        Core_Mem_Array_Write();     // Transfer from RAM Core Memory Array to physical core memory
        Core_Mem_Array_Read();      // Transfer from physical core memory to RAM Core Memory Array
      #endif
      CopyCoreMemoryToMonochromeLEDArrayMemory();
      LED_Array_Matrix_Mono_Display();
      
      #if defined BOARD_CORE64_TEENSY_32
        delay(25);
        OLEDSetTopLevelMode(TopLevelMode);
        OLEDScreenUpdate();
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Matrix_Mono_Display();
          CopyCoreMemoryToMonochromeNeonPixelArrayMemory();
        #endif
      #elif defined BOARD_CORE64C_RASPI_PICO
        OLEDSetTopLevelMode(TopLevelMode);
        OLEDScreenUpdate();    
      #endif
      break;

#if defined BOARD_CORE64_TEENSY_32
    case MODE_CORE_FLUX_DETECTOR:                         // Read 64 cores 10ms (110us 3x core write, with 40us delay 64 times), update LEDs 2ms
      LED_Array_Monochrome_Set_Color(50,255,255);
      LED_Array_Memory_Clear();
      for (coreToTest = 0; coreToTest < 64 ; coreToTest++) {   
        Core_Mem_Bit_Write(coreToTest,1);                     // default to bit set
        if (Core_Mem_Bit_Read(coreToTest)==true) {
          LED_Array_String_Write(coreToTest, 1);
          #ifdef NEON_PIXEL_ARRAY
            Neon_Pixel_Array_String_Write(coreToTest, 1);
          #endif
          }
        else { 
          LED_Array_String_Write(coreToTest, 0); 
          #ifdef NEON_PIXEL_ARRAY
            Neon_Pixel_Array_String_Write(coreToTest, 0);
          #endif
          }
        delayMicroseconds(40); // This 40us delay is required or LED array, first 3-4 pixels in the electronic string, get weird! RF?!??
      }
      LED_Array_String_Display();
      #ifdef NEON_PIXEL_ARRAY
        Neon_Pixel_Array_Matrix_String_Display();
      #endif
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      break;

    case MODE_MONOCHROME_DRAW:       // Simple drawing mode
      LED_Array_Monochrome_Set_Color(0,255,255);
      // Monitor cores for changes. 
        Core_Mem_Monitor();
      // Which cores changed state?
      // Add selected color to that pixel in the LED Array.
      for (uint8_t y=0; y<8; y++)
      {
        for (uint8_t x=0; x<8; x++)
        {
          if (CoreArrayMemory [y][x]) { 
            LED_Array_Matrix_Mono_Write(y, x, 1);
            #ifdef NEON_PIXEL_ARRAY
              Neon_Pixel_Array_Matrix_Mono_Write(y, x, 1);
            #endif
          }
        }
      }
      // Quick touch of the hall sensor clears the screen.
      if ( (Button1Released) && (ButtonState(1,0) > 50 ) ) { 
        LED_Array_Memory_Clear(); 
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Memory_Clear();
        #endif
      }
      // If this was the first time into this state, set default screen to be 0xDEADBEEF and 0xC0D3C4FE
      if (TopLevelModeChanged)
      {
        //LED_Array_Monochrome_Set_Color(0,255,255);      // Hue 0 = RED
        LED_Array_Monochrome_Set_Color(160,255,255);      // Hue 35 = peach orange, 96=green, 135 auqa, 160=blue
        LED_Array_Binary_Write_Default();
        LED_Array_Binary_To_Matrix_Mono();
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Binary_Write_Default();
          Neon_Pixel_Array_Binary_To_Matrix_Mono();
        #endif
        OLEDScreenClear();
      }
      LED_Array_Matrix_Mono_Display();                  // Show the updated LED array.
      LED_Array_Matrix_Mono_to_Binary();                // Convert whatever is in the LED Matrix Array to a 64-bit binary value...
      OLEDSetTopLevelMode(TopLevelMode);
      OLED_Show_Matrix_Mono_Hex();                      // ...and display it on the OLED.
      #ifdef NEON_PIXEL_ARRAY
        Neon_Pixel_Array_Matrix_Mono_Display();
      #endif
      break;

    case MODE_LED_TEST_ALL_BINARY: // Counts from lower right and left/up in binary.
      LED_Array_Test_Count_Binary();
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      // Uncomment next line to skip out of this test state immediately
      // TopLevelMode = MODE_LAST;
      break;

    case MODE_LED_TEST_ONE_STRING: // Turns on 1 pixel, sequentially, from left to right, top to bottom using 1D string addressing
      LED_Array_Test_Pixel_String();
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      break;

    case MODE_LED_TEST_ONE_MATRIX_MONO: // Turns on 1 pixel, sequentially, from left to right, top to bottom using 2D matrix addressing
      LED_Array_Test_Pixel_Matrix_Mono();
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      break;

    case MODE_LED_TEST_ONE_MATRIX_COLOR: // Multi-color symbols
      LED_Array_Test_Pixel_Matrix_Color();
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      break;

    case MODE_TEST_EEPROM: // 
      // value = EEPROM_Hardware_Version_Read(a);  // Teensy internal emulated EEPROM
      value = EEPROMExtDefaultReadByte(a);
      Serial.print(a);
      Serial.print("\t");
      Serial.print(value);
      Serial.println();
      a = a + 1;
      if (a == 128) {
        a = 0;
      }
      delay(100);
      break;
      
    case MODE_LED_TEST_ALL_COLOR: // FastLED Demo of all color
      LED_Array_Test_Rainbow_Demo();
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      break;

    case MODE_CORE_TOGGLE_BIT:     // Just toggle a single bit on and off. Or just pulse on.
      coreToTest=0;
      LED_Array_Monochrome_Set_Color(50,255,255);
      for (uint8_t bit = coreToTest; bit<(coreToTest+1); bit++)
        {
          // IOESpare1_On();
          Core_Mem_Bit_Write(bit,0);
          LED_Array_String_Write(bit,0);
          LED_Array_String_Display();
          // IOESpare1_Off();
          // delay(5);

          // IOESpare1_On();
          Core_Mem_Bit_Write(bit,1);
          LED_Array_String_Write(bit,1);
          LED_Array_String_Display();
          // IOESpare1_Off();
          // delay(50);
        }

      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      break;

    case MODE_CORE_TEST_ONE:
      coreToTest=0;
      LED_Array_Monochrome_Set_Color(100,255,255);
      LED_Array_Memory_Clear();
      //LED_Array_String_Write(coreToTest,1);               // Default to pixel on
      //  TracingPulses(1);
      // Core_Mem_Bit_Write(coreToTest,0);                     // default to bit set
      Core_Mem_Bit_Write(coreToTest,1);                     // default to bit set
      //  TracingPulses(2);
      if (Core_Mem_Bit_Read(coreToTest)==true) {LED_Array_String_Write(coreToTest, 1);}
      else { LED_Array_String_Write(coreToTest, 0); }
      //  TracingPulses(1);
      // delay(10);
      LED_Array_String_Display();
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      delay(10);
      break;

    case MODE_CORE_TEST_MANY:
      coreToTest=0;
      for (uint8_t bit = coreToTest; bit<(64); bit++)
        {
        LED_Array_Monochrome_Set_Color(100,255,255);
        LED_Array_Memory_Clear();
        //LED_Array_String_Write(coreToTest,1);               // Default to pixel on
        //  TracingPulses(1);
        // Core_Mem_Bit_Write(coreToTest,0);                     // default to bit set
        Core_Mem_Bit_Write(bit,1);                     // default to bit set
        //  TracingPulses(2);
        if (Core_Mem_Bit_Read(bit)==true) {LED_Array_String_Write(bit, 1);}
        else { LED_Array_String_Write(bit, 0); }
        //  TracingPulses(1);
        // delay(10);
        LED_Array_String_Display();
        }
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      break;
  #elif defined BOARD_CORE64C_RASPI_PICO
    
  #endif

    case MODE_HALL_TEST:
      LED_Array_Monochrome_Set_Color(25,255,255);
      LED_Array_Memory_Clear();

      // IOESpare1_On();
      if(Button1HoldTime) { LED_Array_String_Write(56,1); Serial.println(Button1HoldTime); }
      if(Button2HoldTime) { LED_Array_String_Write(58,1); Serial.println(Button2HoldTime); }
      if(Button3HoldTime) { LED_Array_String_Write(60,1); Serial.println(Button3HoldTime); }
      if(Button4HoldTime) { LED_Array_String_Write(62,1); Serial.println(Button4HoldTime); }
      // IOESpare1_Off();

      LED_Array_String_Display();
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
    
      break;

    case MODE_LAST:
      LED_Array_Memory_Clear();
      LED_Array_Matrix_Mono_Display();
      LED_Array_Monochrome_Set_Color(125,255,255);
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      TopLevelMode = MODE_STARTUP;   
      break;

    default:
      Serial.println("Invalid TopLevelMode");
      TopLevelMode = MODE_STARTUP;   
      break;
    }
}

void coreTesting() {
  static uint8_t BitToTest = 63;
  /*
  static uint8_t c = 0;
  for (uint8_t i = 1; i <=2; i++)
  {
    CoreWriteBit(c,0); CoreWriteBit(c,1);
  }
  c++;
  if (c == 64) {c=0;}
  */
  // Read testing
  delay(1000);
  Core_Mem_Bit_Write(BitToTest,1);
  LED_Array_String_Write(BitToTest,1);
  LED_Array_String_Display();
  delay(1000);
  Core_Mem_Bit_Write(BitToTest,0);
  LED_Array_String_Write(BitToTest,0);
  LED_Array_String_Display();
  // CoreArrayMemory [0][3] = CoreReadBit(3);
  // Whole Array Testing
  //  for (uint8_t i = 0; i <= 63; i++ ) { CoreWriteBit(i,1); }
  //  for (uint8_t i = 0; i <= 63; i++ ) { CoreWriteBit(i,0); }
  // Column Testing
  // for (uint8_t i = 1; i <= 63; i=i+8 ) { CoreWriteBit(i,0); CoreWriteBit(i,1); }
  //for (uint8_t i = 0; i <= 7; i++ ) { CoreWriteBit(i,1); }
  // Row Testing
  // for (uint8_t i = 0; i <= 63; i=i+8 ) { CoreWriteBit(i,1); }
  // for (uint8_t i = 0; i <= 63; i=i+8 ) { CoreWriteBit(i,0); }
  /*
  CoreWriteBit(0,1);
  CoreWriteBit(0,1);
  CoreWriteBit(0,0);
  CoreWriteBit(7,1);
  CoreWriteBit(7,1);
  CoreWriteBit(0,0);
  //CoreReadBit(0);
  //CoreClearAll();
  */
}
