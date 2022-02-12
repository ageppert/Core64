#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "Mode_Manager.h"

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

enum TopLevelMode                  // Top Level Mode State Machine
{
  MODE_STARTUP                    =   0,                  // Start-up, then go to APP #1 as the default
  MODE_SCROLLING_TEXT             =   1,                  // Scrolling text at power on
  MODE_CORE_FLUX_DETECTOR         =   2,                  // Testing all cores and displaying core state
  MODE_MONOCHROME_DRAW            =   3,                  // Test LED Driver with binary values
  
  MODE_LED_TEST_ALL_BINARY        =   4,                  // Test LED Driver with binary values
  MODE_LED_TEST_ONE_STRING        =   5,                  // Testing LED Driver
  MODE_LED_TEST_ONE_MATRIX_MONO   =   6,                  // Testing LED Driver with matrix array and monochrome color
  MODE_LED_TEST_ONE_MATRIX_COLOR  =   7,                  // Testing LED Driver with matrix array and multi-color symbols
  MODE_TEST_EEPROM                =   8,                  // 
  MODE_LED_TEST_ALL_COLOR         =   9,                  // Test LED Driver with all pixels and all colors
  
  MODE_GAUSS_MENU                 =  10,                  // GAUSS MENU (TOP LEVEL MENU)

  MODE_CORE_TOGGLE_BIT            =  11,                  // Test one core with one function
  MODE_CORE_TEST_ONE              =  12,                  // Testing core #coreToTest and displaying core state
  MODE_CORE_TEST_MANY             =  13,                  // Testing multiple cores and displaying core state
  MODE_HALL_TEST                  =  14,                  // Testing hall switch and sensor response
  MODE_LOOPBACK_TEST              = 501,                  // For the manufacturing test fixture, test unused IO pins

  MODE_LAST                       = 999,                  // Last one, return to Startup 0.
  MODE_HARD_REBOOT               = 1000                   // Hard Reboot.
} ;
static uint16_t TopLevelModeDefault = MODE_STARTUP;
static uint16_t TopLevelMode;
static uint16_t TopLevelModePrevious;
static bool     TopLevelModeChanged = false;

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

uint8_t EepromByteValue = 0;
uint8_t Eeprom_Byte_Mem_Address = 0;
extern int StreamTopLevelModeEnable;

void SetTopLevelModeDefault() { TopLevelMode = TopLevelModeDefault; }

void SetTopLevelMode (uint16_t value) {   TopLevelMode = value; }
uint16_t GetTopLevelMode ()           {   return (TopLevelMode); }
void SetTopLevelModeInc ()            {   TopLevelMode++; }

void SetTopLevelModePrevious (uint16_t value)   { TopLevelModePrevious = value; }
uint16_t GetTopLevelModePrevious ()             { return (TopLevelModePrevious); }

void SetTopLevelModeChanged (bool value) {   TopLevelModeChanged = value; }
bool GetTopLevelModeChanged ()           {   return (TopLevelModeChanged); }

void TopLevelModeRun () {
    // MODE SWITCHING
    // If button is pressed, go to next mode.
    // Must be released and pressed again for subsequent action.

    if (StreamTopLevelModeEnable) { Serial.println(GetTopLevelMode()); }

    #if defined BOARD_CORE64_TEENSY_32
      Button1HoldTime = ButtonState(1,0);
      Button2HoldTime = ButtonState(2,0);
      Button3HoldTime = ButtonState(3,0);
      Button4HoldTime = ButtonState(4,0);
    #elif defined BOARD_CORE64C_RASPI_PICO
      // Not yet implemented
    #endif

    if ( (Button1Released == true) && (Button1HoldTime >= 500) ) {
      ButtonState(1,1); // Force a "release" after press by clearing the button hold down timer
      Button1Released = false;
      ColorFontSymbolToDisplay++;
      if(ColorFontSymbolToDisplay>3) { ColorFontSymbolToDisplay = 0; }
      TopLevelModePrevious = TopLevelMode;
      SetTopLevelModeInc();
      SetTopLevelModeChanged (true); // User application has one time to use this before it is reset.
    }
    else {
      if (Button1HoldTime == 0) {
        Button1Released = true;
        // SetTopLevelModeChanged (false);
      }
    }

    if (GetTopLevelModeChanged()) {
      Serial.println("");
      Serial.print("  TopLevelMode changed to ");
      Serial.print(GetTopLevelMode());
      Serial.print(" from ");
      Serial.print(TopLevelModePrevious);
      Serial.println(".");
      Serial.print(PROMPT);         // Print the first prompt to show the system is ready for input
    }

    switch(TopLevelMode)
    {
    case MODE_STARTUP:
      delay(1500);                  // Wait a little bit for the serial port to connect if this is connected to a computer terminal
      handleSplash("");             // Splash screen
      handleInfo("");               // Print some info about the system (this also checks hardware version, born-on, and serial number)
      handleHelp("");               // Print the help menu
      Serial.print(PROMPT);         // Print the first prompt to show the system is ready for input
      SetTopLevelModeInc();
      SetTopLevelModeChanged(true);  // User application has one time to use this before it is reset.
      break;

    case MODE_SCROLLING_TEXT:
      if (GetTopLevelModeChanged()) {
        LED_Array_Monochrome_Set_Color(135,255,255);  // 135,255,255 = OLED aqua
      }
      ScrollTextToCoreMemory();   // This writes directly to the RAM core memory array and bypasses reading it.
      #if defined SCROLLING_TEXT_BYPASS_CORE_MEMORY
        // Nothing here
      #else
        Core_Mem_Array_Write();     // Transfer from RAM Core Memory Array to physical core memory
        Core_Mem_Array_Read();      // Transfer from physical core memory to RAM Core Memory Array
      #endif
      CopyCoreMemoryToMonochromeLEDArrayMemory();
      LED_Array_Matrix_Mono_Display();
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      #if defined BOARD_CORE64_TEENSY_32
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Matrix_Mono_Display();
          CopyCoreMemoryToMonochromeNeonPixelArrayMemory();
        #endif
      #elif defined BOARD_CORE64C_RASPI_PICO
        // Nothing here
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
        Serial.println("  LED Matrix Cleared.");      
      }
      // If this was the first time into this state, set default screen to be 0xDEADBEEF and 0xC0D3C4FE
      if (GetTopLevelModeChanged())
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
      EepromByteValue = EEPROMExtDefaultReadByte(Eeprom_Byte_Mem_Address);
      Serial.print(Eeprom_Byte_Mem_Address);
      Serial.print("\t");
      Serial.print(EepromByteValue);
      Serial.println();
      Eeprom_Byte_Mem_Address = Eeprom_Byte_Mem_Address + 1;
      if (Eeprom_Byte_Mem_Address == 128) {
        Eeprom_Byte_Mem_Address = 0;
      }
      delay(100);
      break;
      
    case MODE_LED_TEST_ALL_COLOR: // FastLED Demo of all color
      LED_Array_Test_Rainbow_Demo();
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      break;
      
    case MODE_GAUSS_MENU:
      WriteColorFontSymbolToLedScreenMemoryMatrixColor(4);
      LED_Array_Matrix_Color_Display(); 
      break;

    case MODE_CORE_TOGGLE_BIT:     // Just toggle a single bit on and off. Or just pulse on.
      coreToTest=0;
      LED_Array_Monochrome_Set_Color(50,255,255);
      for (uint8_t bit = coreToTest; bit<(coreToTest+1); bit++)
        {
          // IOESpare1_On();
          Core_Mem_Bit_Write_With_V_MON(bit,0);
          LED_Array_String_Write(bit,0);
          LED_Array_String_Display();
          // IOESpare1_Off();
          // delay(5);

          // IOESpare1_On();
          Core_Mem_Bit_Write_With_V_MON(bit,1);
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

    case MODE_LOOPBACK_TEST:
      LED_Array_Monochrome_Set_Color(125,255,255);
      LED_Array_Memory_Clear();
      LED_Array_Matrix_Mono_Display();
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      if (LoopBackTest() == 0) {
        Serial.print("  LoopBackTest passed. Results code: ");
        Serial.println(LoopBackTest());
        Serial.println();
      }
      else {
        Serial.print("  LoopBackTest FAILED. Results code: ");
        Serial.println(LoopBackTest());
      }
      Serial.println("  Result values");
      Serial.println("        0 = PASS");
      Serial.println("    1-200 = FAIL as number of test failures");
      Serial.println("      253 = Not implemented for Core64 Vx.5.x.");
      Serial.println("      254 = Not implemented for Core64c.");
      Serial.println("      255 = Unhandled exception");
      Serial.println("  Hard rebooting to return all IO to default states. 8 seconds to go...");      
      delay(8000);
      handleReboot(" ");
      break;

    case MODE_LAST:
      LED_Array_Memory_Clear();
      LED_Array_Matrix_Mono_Display();
      LED_Array_Monochrome_Set_Color(125,255,255);
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      Serial.println("  TopLevelMode reached the end of the mode list. Soft restart back to MODE_STARTUP.");      
      TopLevelMode = MODE_STARTUP;   
      break;

    case MODE_HARD_REBOOT:
      LED_Array_Memory_Clear();
      LED_Array_Matrix_Mono_Display();
      LED_Array_Monochrome_Set_Color(125,255,255);
      OLEDSetTopLevelMode(TopLevelMode);
      OLEDScreenUpdate();
      Serial.println("  Hard rebooting in 3 seconds.");      
      delay(3000);
      handleReboot(" ");
      break;

    default:
      Serial.print("  ");      
      Serial.print(GetTopLevelMode());
      Serial.println(" is an undefined TopLevelMode. Soft restart back to MODE_STARTUP.");
      TopLevelMode = MODE_STARTUP;   
      break;
    }

    if (GetTopLevelModeChanged()) {
      SetTopLevelModeChanged (false);
    }

}