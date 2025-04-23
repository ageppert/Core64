// INCLUDES FOR THIS MODE
#include "App_Midi.h"

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
#include "SubSystems/Midi.h"

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
  STATE_MIDI                            // 2
  };
static volatile uint8_t  ModeState;

static volatile uint32_t nowTimems;
static volatile uint32_t UpdateLastRunTime;
static volatile uint32_t UpdatePeriod              = 33;       // in ms (33ms = 30fps)
static volatile bool       ModeFirstTimeUsed       = true;     // Keep track of the first time this mode is used.

// Duplicate of screen image for local use
static volatile uint8_t LedScreenMemoryLocalArrayHue [8][8];
static volatile uint8_t LedScreenMemoryLocalArraySat [8][8];

uint8_t MidiVisibleExtentX = 8;
uint8_t MidiVisibleExtentY = 8;
uint8_t MidiHue = 170;
uint8_t MidiSat = 255;
bool    ModeSustainEnable  = true;

void AppMidi() {
  if (TopLevelModeChangedGet()) {                     // Fresh entry into this mode.
    Serial.println();
    Serial.println("  Midi Mode");
    Serial.println("    + = Next Midi Instrument");
    Serial.println("    - = Previous Midi Instrument");
    Serial.println("    S = Sustain Enable/Disable");
    Serial.print(PROMPT);
    TopLevelThreeSoftButtonGlobalEnableSet(true); // Make sure + and - soft buttons are enabled to move to next mode if desired.
    TopLevelSetSoftButtonGlobalEnableSet(false);  // Disable the S button as SET, so it can be used to select.
    WriteAppMidiSymbol(0);
    LED_Array_Color_Display(0);
    MenuTimeOutCheckReset();
    ModeState = STATE_INTRO_SCREEN_WAIT_FOR_SELECT;
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { 
      MidiVisibleExtentX = 4;
      MidiVisibleExtentY = 4;
    }
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
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  STATE MACHINE
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  nowTimems = millis();
  if ((nowTimems - UpdateLastRunTime) >= UpdatePeriod) {
    if (DebugLevel == 4) { Serial.print("App Midi State = "); Serial.println(ModeState); }
    // Service the mode state.
    switch(ModeState)
    {  
      case STATE_INTRO_SCREEN_WAIT_FOR_SELECT:
      #if defined SAO_MIDI
        // Check for touch of "S" to select mdi mode and stay here
        if (ButtonState(4,0) >= 100) { MenuTimeOutCheckReset(); Button4Released = false; ModeState = STATE_SET_UP; }
      #else
        Serial.print("Midi mode not available in this firmware compilation.");
      #endif
        // Timeout to next sub-menu option in the sequence
        if (MenuTimeOutCheck(3000))  { TopLevelModeSetInc(); }
        break;

      case STATE_SET_UP:
      {
        // If this was the first time into this state since power on set screen up
        if (ModeFirstTimeUsed == true) {
          MidiHue = 170;
          MidiSat = 255;
          WriteAppMidiSymbol(0);
          LED_Array_Color_Display(0);                  // Show the updated LED array.
          #ifdef NEON_PIXEL_ARRAY
            Neon_Pixel_Array_Binary_Write_Default();
            Neon_Pixel_Array_Binary_To_Matrix_Mono();
          #endif
          OLEDScreenClear();
          ModeFirstTimeUsed = false;
        }
        display.clearDisplay();
        display.setTextSize(1);      // Normal 1:1 pixel scale
        display.setCursor(0,0);     // Start at top-left corner
        display.print(F("Mode: "));
        display.println(TopLevelModeGet(),DEC);
        display.println(TOP_LEVEL_MODE_NAME_ARRAY[TopLevelModeGet()]);
        display.println(F(""));
        display.println(F("M   = Menu"));
        display.println(F("-/+ = More Midi Sub-Modes"));
        display.println(F("S   = Sustain En/Release"));
        OLED_Display_Stability_Work_Around();
        TopLevelThreeSoftButtonGlobalEnableSet(false); // Disable + and - soft buttons from global mode switching use.
        ModeState = STATE_MIDI;
      }
        break;

      case STATE_MIDI:
      {
        Core_Mem_Scan_For_Magnet();                 // Find the cores currently affected by magnetism.

        // Update the LED Array, similar to flux detect mode, even with MIDI stuff enabled.
        for (uint8_t y=0; y<MidiVisibleExtentX; y++) {
          for (uint8_t x=0; x<MidiVisibleExtentX; x++)  {
            MidiHue = AppMidiSymbolsHue[1][y][x];
            if (CoreArrayMemory [y][x]) { 
              MidiSat = 255;
              LED_Array_Matrix_Color_Hue_Sat_Write(y, x, MidiHue, MidiSat);
              #ifdef NEON_PIXEL_ARRAY
                Neon_Pixel_Array_Matrix_Mono_Write(y, x, 1);
              #endif
            }
            else {
              MidiSat = 155;
              LED_Array_Matrix_Color_Hue_Sat_Write(y, x, MidiHue, MidiSat);
              #ifdef NEON_PIXEL_ARRAY
                Neon_Pixel_Array_Matrix_Mono_Write(y, x, 0);
              #endif
            }
          }
        }
        #if defined SAO_MIDI
          uint8_t pixel;
          uint8_t rowcolmax = 8;
          // Look for changes in the core matrix and update the MIDI Notes accordingly
          for (uint8_t y=0; y<MidiVisibleExtentX; y++) {
            for (uint8_t x=0; x<MidiVisibleExtentX; x++)  {
              pixel = (y*rowcolmax)+x;
              if (CoreArrayMemoryChanged [y][x]) {
                if (CoreArrayMemory [y][x]) {
                  noteOn(0x90, pixel, 1);                  // Note on channel 1 (0x90), some note value (note), middle velocity (0x45):
                }
                else {
                  if (!ModeSustainEnable) { noteOff(0x90, pixel); }
                }  
              }
            }
          }
        #endif

        // Touch of 'S' toggles sustain mode on and off.
        if ( (Button4Released) && (ButtonState(4,0) > 500 ) ) { 
          Button4Released = false;
          Serial.print("  Sustain mode = ");
          if (ModeSustainEnable) { ModeSustainEnable = false; Serial.println("DISABLED");}
          else { ModeSustainEnable = true; Serial.println("ENABLED");}
        }
        if (ButtonState(4,0) == 0) {
          Button4Released = true;
        }

        // Touch of '+' moves to next Midi sub-mode.
        if ( (Button3Released) && (ButtonState(3,0) > 200 ) ) { 
          // TODO: move to next mode when there are some more
        }
        if (ButtonState(3,0) == 0) {
          Button3Released = true;
        }

        // Touch of '-' moves to previuos Midi sub-mode.
        if ( (Button2Released) && (ButtonState(2,0) > 100 ) ) { 
          // TODO: move to previous mode when there are some more
          // All MIDI CHANNELS ON CORE64 MATRIX OFF
          for (uint8_t i=0; i<64; i++) { noteOff(0x90, i); }
        }
        if (ButtonState(2,0) == 0) {
          Button2Released = true;
        }

        LED_Array_Color_Display(0);                       // Show the updated LED array.
        LED_Array_Matrix_Mono_to_Binary();                // Convert whatever is in the LED Matrix Array to a 64-bit binary value...
        // OLEDTopLevelModeSet(TopLevelModeGet());
        // OLEDScreenUpdate();
        // OLED_Show_Matrix_Mono_Hex();                      // ...and display it on the OLED.
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Matrix_Mono_Display();
        #endif
      }
      break;

      default:
        break;
    }
    // Service non-state dependent stuff
    UpdateLastRunTime = nowTimems;
  }

} // MIDI APP
