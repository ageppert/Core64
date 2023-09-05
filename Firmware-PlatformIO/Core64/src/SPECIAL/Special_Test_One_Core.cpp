// INCLUDES FOR THIS MODE
#include "Special_Test_One_Core.h"

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

void SpecialTestOneCore() {
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
  enum ModeState {
    STATE_INTRO_SCREEN_WAIT_FOR_SELECT,   // 0
    STATE_SET_UP ,                        // 1
    STATE_TEST_ONE_CORE,                    // 2
    };
  static volatile uint8_t  ModeState;

  static volatile uint32_t nowTimems;
  static volatile uint32_t UpdateLastRunTime;
  static volatile uint32_t UpdatePeriod            = 33;       // in ms (33ms = 30fps)

  static volatile uint8_t  coreToTest;
  static volatile uint8_t  coreMinToTest;
  static volatile uint8_t  coreMaxToTest;

  if (TopLevelModeChangedGet()) {                     // Fresh entry into this mode.
    Serial.println();
    Serial.println("  Special Test One Pixel Mode");
    Serial.println("    + = nothing");
    Serial.println("    - = nothing");
    Serial.println("    S = nothing");
    Serial.println("  Used for demo with oscilloscope connected as follows:");
    Serial.println("  Analog Channel 1 = Sense Wire A end");
    Serial.println("  Analog Channel 2 = Sense Wire B end");
    Serial.println("  Digital Channel 1 = Core Matrix Enable");
    Serial.println("  Digital Channel 2 = Sense Output A");
    Serial.println("  Digital Channel 3 = Sense Output B");
    Serial.print(PROMPT);
    TopLevelThreeSoftButtonGlobalEnableSet(true); // Make sure + and - soft buttons are enabled to move to next mode if desired.
    TopLevelSetSoftButtonGlobalEnableSet(false);  // Disable the S button as SET, so it can be used to select.
    WriteUtilFluxSymbol(1);
    LED_Array_Color_Display(1);
    MenuTimeOutCheckReset();
    ModeState = STATE_INTRO_SCREEN_WAIT_FOR_SELECT;
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  STATE MACHINE
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  nowTimems = millis();
  if ((nowTimems - UpdateLastRunTime) >= UpdatePeriod) {
    if (DebugLevel == 4) { Serial.print("Special Test One Pixel State = "); Serial.println(ModeState); }
    // Service the mode state.
    switch(ModeState)
    {  
      case STATE_INTRO_SCREEN_WAIT_FOR_SELECT:
        // Check for touch of "S" to select paint mode and stay here
        if (ButtonState(4,0) >= 100) { MenuTimeOutCheckReset(); Button4Released = false; ModeState = STATE_SET_UP; }
        // Timeout to next sub-menu option in the sequence
        // DO NOT TIME OUT
        // if (MenuTimeOutCheck(3000))  { TopLevelModeSetInc(); }
        break;

      case STATE_SET_UP:
        LED_Array_Monochrome_Set_Color(35,255,255);      // Hue 0 RED, 35 peach orange, 96 green, 135 aqua, 160 blue
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Memory_Clear();
        #endif
        LED_Array_Memory_Clear(); // Clear LED Array Memory
        TopLevelThreeSoftButtonGlobalEnableSet(false); // Disable + and - soft buttons from global mode switching use. Prevents accidental mode change when waving magnets around.
        ModeState = STATE_TEST_ONE_CORE;
        break;

      case STATE_TEST_ONE_CORE:   // Read 64 cores 10ms (110us 3x core write, with 40us delay 64 times), update LEDs 2ms
        LED_Array_Monochrome_Set_Color(35,255,255);
        LED_Array_Memory_Clear();
        coreMinToTest = 0;
        coreMaxToTest = 0;
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
        OLEDTopLevelModeSet(TopLevelModeGet());
        OLEDScreenUpdate();

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