#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "App_Paint.h"

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

// VARIABLES, COMMON TO ALL APPS
  enum AppState {
    APP_STATE_INTRO_SCREEN ,    // 0
    APP_STATE_SET_UP ,          // 1
    APP_STATE_PAINTING ,        // 2
    APP_STATE_CLEAR ,           // 3
    APP_STATE_LAST              // Last one, return to Startup 0. This is different than "default" which is handled by switch/case statement.
  };
  volatile uint8_t  AppState;
  volatile uint32_t nowTime;
  volatile uint32_t AppUpdateLastRunTime;
  volatile uint32_t AppUpdatePeriod;

// VARIABLES, CUSTOM TO THIS APP
  enum PaletteState {
   PALETTE_STATE_OFF ,          // 0
   PALETTE_STATE_TOP ,          // 1
   PALETTE_STATE_BOTTOM ,       // 2
   PALETTE_STATE_LAST           // Last one, return to Startup 0. This is different than "default" which is handled by switch/case statement.
  };
  volatile uint8_t  PaletteState;

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

void AppRunPaint() {
  // First time entry into this application, display symbol and how-to information.
  if (TopLevelModeChangedGet()) {                          
    Serial.println();
    Serial.println("   Painting App Sub-Menu");
    Serial.println("    M = Main DGAUSS Menu");
    Serial.println("    + = Show color pallette, alternating on top or bottom of screen");
    Serial.println("    - = Hide color pallette");
    Serial.println("    S = Screen clear");
    Serial.print(PROMPT);
    TopLevelThreeSoftButtonGlobalEnableSet(true); // Make sure MENU + and - soft buttons are enabled to goto next APP.
    TopLevelSetSoftButtonGlobalEnableSet(false);  // Disable the S button as SETTINGS so it can be used in this game as Select.


// UNFINISHED BELOW THIS LINE

    WriteGamePongSymbol(0);
    LED_Array_Matrix_Color_Display();
    
    MenuTimeOutCheckReset();
    GameState = GAME_STATE_INTRO_SCREEN;
    GameUpdatePeriod = 33;
    GameOverTimerAutoReset = 3000;
    MenuTimeOutCheckReset();
  }
    OLEDTopLevelModeSet(TopLevelModeGet());
    OLEDScreenUpdate();


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // PONG GAME STATE MACHINE
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  nowTime = millis();
  if ((nowTime - GameUpdateLastRunTime) >= GameUpdatePeriod)
  {
    if (DebugLevel == 4) { Serial.print("Pong Game State = "); Serial.println(GameState); }
    // Service a game state.
    switch(AppState)
    {
      case APP_STATE_INTRO_SCREEN:
        // Check for touch of "S" to start the game
        if (ButtonState(4,0) >= 100) { MenuTimeOutCheckReset(); GameState = GAME_STATE_SET_UP; }
        // Timeout to next game option in the sequence
        if (MenuTimeOutCheck(10000))  { TopLevelModeSetInc(); }
        break;

      case GAME_STATE_SET_UP:
        
        break;

      case GAME_STATE_PLAY:       
        if (GameLogic()) { MenuTimeOutCheckReset(); }
        if (MenuTimeOutCheck(60000)) { TopLevelModeSetToDefault(); } // Is it time to timeout? Then timeout.
        if (Winner) { GameOverTimer = nowTime; GameState = 3; }
        ConvertGameFieldToLEDMatrixScreenMemory();
        // Check for touch of "S" to start the game over.
        if (ButtonState(4,0) >= 200) { MenuTimeOutCheckReset(); GameState = GAME_STATE_SET_UP; }
        // GameDebugSerialPrintMap();
        break;

      case GAME_STATE_ROUND_WIN:
        if (Winner == 1) { WriteGamePongSymbol(1); GamePlayerOneScore++;}
        if (Winner == 2) { WriteGamePongSymbol(2); GamePlayerOneScore++;}
        // TODO: Display both players scores after each round. If one player reaches 9, go to GAME_STATE_LOOSE
        if ((nowTime - GameOverTimer) > GameOverTimerAutoReset) {
          MenuTimeOutCheckReset();
          GameState = GAME_STATE_SET_UP;
        }
        break;

      case GAME_STATE_FINISHED: // 
        // TODO: Declare a winner. 

      case GAME_STATE_END: // 
        // TODO: Wait for "S" or timeout 20 seconds to begin another match.

      default:
        break;
    }

    // Service non-state dependent stuff
    // GameScreenRefresh();
    AppUpdateLastRunTime = nowTime;
  }



  // Wait for a screen touch or S button touch to begin drawing. Timeout to next APP in 10 seconds. 


      if ( (millis() - DrawLastTimeUsed) >= DrawLastTimeUsedTimeout) {
        DrawDelayTimerStarted = true;
        DrawLastTimeUsed = millis();
      }
      else {
        DrawLastTimeUsed = millis();
      }


      if (DrawDelayTimerStarted) {
//        Serial.println();
//        Serial.println("  Drawing Mode");
//        Serial.println("    + = Show Color Pallette (not yet implemented)");
//        Serial.println("    - = Hide Color Pallette (not yet implemented)");
//        Serial.println("    S = Screen Clear");
//        Serial.print(PROMPT);
        DrawDelayTimer = millis();
        DrawDelayTimerStarted = false;
        DrawDelayTimerFinished = false;
      }

      // If this was the first time into this state, set default color and fill 64 bits with 0xDEADBEEF and 0xC0D3C4FE
      if (DrawFirstTimeUsed == true) {
        LED_Array_Monochrome_Set_Color(0,255,255);      // Hue 0 = RED, Hue 35 = peach orange, 96=green, 135 aqua, 160=blue
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

      if (DrawDelayTimerFinished == true) {
        // Which cores changed state?
        Core_Mem_Scan_For_Magnet();                                // Monitor cores for changes.
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
        LED_Array_Memory_Clear();
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Memory_Clear();
        #endif
        Button4Released = false;
        Serial.println("  LED Matrix Cleared.");   
      }
      if (ButtonState(4,0) == 0) {
        Button4Released = true;
      }

      LED_Array_Matrix_Mono_Display();                  // Show the updated LED array.
      LED_Array_Matrix_Mono_to_Binary();                // Convert whatever is in the LED Matrix Array to a 64-bit binary value...
      #ifdef NEON_PIXEL_ARRAY
        Neon_Pixel_Array_Matrix_Mono_Display();         // Show the updated Neon Pixel array too.
      #endif
      OLEDTopLevelModeSet(TopLevelModeGet());           // Make sure the OLED display has the current mode text.
      OLED_Show_Matrix_Mono_Hex();                      // Display the HEX equivalent of the 64 bits on the OLED.


}