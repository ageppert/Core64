#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "Drawing.h"

#include <stdint.h>
#include <stdbool.h>

#include "Mode_Manager.h"

#include "Config/HardwareIOMap.h"
//#include "SubSystems/Heart_Beat.h"
#include "SubSystems/Serial_Port.h"
#include "Hal/LED_Array_HAL.h"
#include "Hal/Neon_Pixel_HAL.h"
#include "SubSystems/OLED_Screen.h"
#include "SubSystems/Analog_Input_Test.h"
#include "Hal/Buttons_HAL.h"
#include "Hal/Core_HAL.h"
//#include "Hal/EEPROM_HAL.h"
//#include "SubSystems/I2C_Manager.h"
//#include "SubSystems/SD_Card_Manager.h"
//#include "SubSystems/Ambient_Light_Sensor.h"
#include "Libraries/CommandLine/CommandLine.h"
#include "Drivers/Core_Driver.h"
#include "SubSystems/Test_Functions.h"
#include "Hal/Debug_Pins_HAL.h"

#include "SubSystems/Command_Line_Handler.h"

static bool     Button1Released = true;
static bool     Button2Released = true;
static bool     Button3Released = true;
static bool     Button4Released = true;
static uint32_t Button1HoldTime = 0;
static uint32_t Button2HoldTime = 0;
static uint32_t Button3HoldTime = 0;
static uint32_t Button4HoldTime = 0;

static bool      DrawFirstTimeUsed       = true;   // Keep track of he first time draw mode is used, to choose whether the default image should be displayed.
static bool      DrawDelayTimerStarted   = false;   // Upon entry, delay a second or two to avoid accidentally placing a pixel.
static bool      DrawDelayTimerFinished  = false;  // When the Timer is finished, proceed to allow drawing.
static uint32_t  DrawDelayTimer          = 0;
const  uint32_t  DrawDelayTimeOut        = 1000;   // milliseconds to wait
static uint32_t  DrawLastTimeUsed        = 0;
const  uint32_t  DrawLastTimeUsedTimeout = 500;   // How long to wait (milliseconds) and reset DrawDelayTimerStarted = true

void Draw() {
      if ( (millis() - DrawLastTimeUsed) >= DrawLastTimeUsedTimeout) {
        DrawDelayTimerStarted = true;
        DrawLastTimeUsed = millis();
      }
      else {
        DrawLastTimeUsed = millis();
      }


      if (DrawDelayTimerStarted) {
        Serial.println();
        Serial.println("  Drawing Mode");
        Serial.println("    + = Show Color Pallette (not yet implemented)");
        Serial.println("    - = Hide Color Pallette (not yet implemented)");
        Serial.println("    S = Screen Clear");
        Serial.print(PROMPT);
        DrawDelayTimer = millis();
        DrawDelayTimerStarted = false;
        DrawDelayTimerFinished = false;
      }
      // LED_Array_Monochrome_Set_Color(0,255,255);

      // If this was the first time into this state, set default screen to be 0xDEADBEEF and 0xC0D3C4FE
      if (DrawFirstTimeUsed == true) {
        LED_Array_Monochrome_Set_Color(0,255,255);      // Hue 0 = RED
        // LED_Array_Monochrome_Set_Color(160,255,255);      // Hue 35 = peach orange, 96=green, 135 auqa, 160=blue
        LED_Array_Binary_Write_Default();
        LED_Array_Binary_To_Matrix_Mono();
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Binary_Write_Default();
          Neon_Pixel_Array_Binary_To_Matrix_Mono();
        #endif
        OLEDScreenClear();
        DrawFirstTimeUsed = false;
      }     
      
      TopLevelThreeSoftButtonGlobalEnableSet (false);            // Ensures this flag stays disabled in this mode. Preventing global use of +, -, S BUTTONS.
      Core_Mem_Scan_For_Magnet();                                // Monitor cores for changes.

      if (DrawDelayTimerFinished == true) {
        // Which cores changed state?
        // Add selected color to that pixel in the LED Array.
        for (uint8_t y=0; y<8; y++) {
          for (uint8_t x=0; x<8; x++)  {
            if (CoreArrayMemory [y][x]) { 
              LED_Array_Matrix_Mono_Write(y, x, 1);
              #ifdef NEON_PIXEL_ARRAY
                Neon_Pixel_Array_Matrix_Mono_Write(y, x, 1);
              #endif
            }
          }
        }
      }
      else {
        if ( (millis() - DrawDelayTimer ) >= DrawDelayTimeOut) {
          DrawDelayTimerFinished = true;
        }
      }

      // Quick touch of 'S' clears the screen.
      if ( (Button4Released) && (ButtonState(4,0) > 50 ) ) { 
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Memory_Clear();
        #endif
        Button4Released = false;
        LED_Array_Memory_Clear();
        Serial.println("  LED Matrix Cleared.");   
      }
      if (ButtonState(4,0) == 0) {
        Button4Released = true;
      }

      LED_Array_Matrix_Mono_Display();                  // Show the updated LED array.
      LED_Array_Matrix_Mono_to_Binary();                // Convert whatever is in the LED Matrix Array to a 64-bit binary value...
      OLEDTopLevelModeSet(TopLevelModeGet());
      OLED_Show_Matrix_Mono_Hex();                      // ...and display it on the OLED.
      #ifdef NEON_PIXEL_ARRAY
        Neon_Pixel_Array_Matrix_Mono_Display();
      #endif
}