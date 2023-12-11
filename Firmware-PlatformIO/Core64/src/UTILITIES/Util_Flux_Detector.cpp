// INCLUDES FOR THIS MODE
#include "Util_Flux_Detector.h"

// INCLUDES COMMON TO ALL MODES
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
#include <stdint.h>
#include <stdbool.h>
#include "Mode_Manager.h"
#include "Config/HardwareIOMap.h"
#include "Config/CharacterMap.h"
#include "SubSystems/Serial_Port.h"
#include "Hal/LED_Array_HAL.h"
#include "Hal/Neon_Pixel_HAL.h"
#include "SubSystems/OLED_Screen.h"
#include "SubSystems/Analog_Input_Test.h"
#include "Hal/Buttons_HAL.h"
#include "Hal/Core_HAL.h"
#include "Libraries/CommandLine/CommandLine.h"
#include "Drivers/Core_Driver.h"
#include "SubSystems/Test_Functions.h"
#include "Hal/Debug_Pins_HAL.h"
#include "SubSystems/Command_Line_Handler.h"

void UtilFluxDetector() {
  // VARIABLES COMMON TO ALL MODES
  static volatile bool      Button1Released         = true;
  static volatile bool      Button2Released         = true;
  static volatile bool      Button3Released         = true;
  static volatile bool      Button4Released         = true;
  static volatile uint32_t  Button1HoldTime         = 0;
  static volatile uint32_t  Button2HoldTime         = 0;
  static volatile uint32_t  Button3HoldTime         = 0;
  static volatile uint32_t  Button4HoldTime         = 0;

  // VARIABLES TO CUSTOMIZE FOR EACH STATE IN THIS MODE
  enum e_ModeState {
    STATE_INTRO_SCREEN_WAIT_FOR_SELECT,   // 0
    STATE_SET_UP ,                        // 1
    STATE_FLUX_DETECT,                    // 2
    };
  static volatile uint8_t  e_ModeState;

  static volatile uint32_t nowTimems;
  static volatile uint32_t UpdateLastRunTime;
  static volatile uint32_t UpdatePeriod            = 33;       // in ms (33ms = 30fps)

  static volatile uint8_t  coreToTest;
  static volatile uint8_t  coreMinToTest;
  static volatile uint8_t  coreMaxToTest;

  if (TopLevelModeChangedGet()) {                     // Fresh entry into this mode.
    Serial.println();
    Serial.println("  Util / Flux Detector");
    Serial.println("    + = Next sub-menu");
    Serial.println("    - = Previous sub-menu");
    Serial.println("    S = Select this mode");    
    Serial.print(PROMPT);
    TopLevelThreeSoftButtonGlobalEnableSet(true); // Make sure + and - soft buttons are enabled to move to next mode if desired.
    TopLevelSetSoftButtonGlobalEnableSet(false);  // Disable the S button as SET, so it can be used to select.
    WriteUtilFluxSymbol(0);
    LED_Array_Color_Display(1);
    #ifdef NEON_PIXEL_ARRAY
      CopyLedArrayMemoryToNeonPixelArrayMemory();
      Neon_Pixel_Array_Matrix_String_Display();
    #endif
    display.clearDisplay();
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setCursor(0,0);     // Start at top-left corner
    display.print(F("Mode: "));
    display.println(TopLevelModeGet(),DEC);
    display.println(TOP_LEVEL_MODE_NAME_ARRAY[TopLevelModeGet()]);
    display.println(F(""));
    display.println(F("M   = Menu"));
    display.println(F("-/+ = Previous/Next"));
    display.println(F("S   = Select"));
    OLED_Display_Stability_Work_Around();

    MenuTimeOutCheckReset();
    e_ModeState = STATE_INTRO_SCREEN_WAIT_FOR_SELECT;
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  STATE MACHINE
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  nowTimems = millis();
  if ((nowTimems - UpdateLastRunTime) >= UpdatePeriod) {
    if (DebugLevel == 4) { Serial.print("Util, Flux Detector State = "); Serial.println(e_ModeState); }
    // Service the mode state.
    switch(e_ModeState)
    {  
      case STATE_INTRO_SCREEN_WAIT_FOR_SELECT:
        // Check for touch of "S" to select this mode and stay here
        if (ButtonState(4,0) >= 100) { MenuTimeOutCheckReset(); Button4Released = false; e_ModeState = STATE_SET_UP; }
        // Timeout to next sub-menu option in the sequence
        if (MenuTimeOutCheck(3000))  { TopLevelModeSetInc(); }
        break;

      case STATE_SET_UP:
        Serial.println();
        Serial.println("  Util, Flux Detector Mode Selected");
        Serial.println("    All buttons disabled.");
        Serial.print(PROMPT);
        TopLevelThreeSoftButtonGlobalEnableSet(false); // Disable + and - soft buttons from global mode switching use. Prevents accidental mode change when waving magnets around.
        TopLevelSetMenuButtonGlobalEnableSet(false); // Disable Menu to prevent accidental triggering in flux detector mode.
        LED_Array_Monochrome_Set_Color(35,255,255);      // Hue 0 RED, 35 peach orange, 96 green, 135 aqua, 160 blue
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Memory_Clear();
        #endif
        LED_Array_Memory_Clear(); // Clear LED Array Memory
        CoreToStartTestSet(0);
        CoreToEndTestSet(63);
        display.clearDisplay();
        display.setTextSize(1);      // Normal 1:1 pixel scale
        display.setCursor(0,0);     // Start at top-left corner
        display.print(F("Mode: "));
        display.println(TopLevelModeGet(),DEC);
        display.println(TOP_LEVEL_MODE_NAME_ARRAY[TopLevelModeGet()]);
        display.println(F(""));
        display.println(F("All buttons disabled."));
        display.println(F("Power cycle to exit"));
        display.println(F("flux detector mode."));
        OLED_Display_Stability_Work_Around();
        e_ModeState = STATE_FLUX_DETECT;
        break;

      case STATE_FLUX_DETECT:   // Read 64 cores 10ms (110us 3x core write, with 40us delay 64 times), update LEDs 2ms
        LED_Array_Monochrome_Set_Color(50,255,255);
        LED_Array_Memory_Clear();
        coreMinToTest = CoreToStartTestGet();
        coreMaxToTest = CoreToEndTestGet();
        for (coreToTest = coreMinToTest; coreToTest <= coreMaxToTest ; coreToTest++) {   
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
        // OLEDTopLevelModeSet(TopLevelModeGet());
        // OLEDScreenUpdate();
        // OLED_Show_Matrix_Mono_Hex();                      // ...and display it on the OLED.

/*
        LED_Array_Color_Display(0);                  // Show the updated LED array.
        LED_Array_Matrix_Mono_to_Binary();                // Convert whatever is in the LED Matrix Array to a 64-bit binary value...
        OLEDTopLevelModeSet(TopLevelModeGet());
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Matrix_Mono_Display();
        #endif
*/
        break;

      default:
        break;
    }
    // Service non-state dependent stuff
    UpdateLastRunTime = nowTimems;
  }
} // FLUX DETECTOR END