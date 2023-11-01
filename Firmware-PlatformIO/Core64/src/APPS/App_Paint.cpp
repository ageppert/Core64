// INCLUDES FOR THIS MODE
#include "App_Paint.h"

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
  STATE_PAINT                           // 2
  };
static volatile uint8_t  ModeState;

enum PalettePosition {
  PALETTE_NONE,      // 0
  PALETTE_BOTTOM ,   // 1
  PALETTE_TOP        // 2
};
static volatile uint8_t  PalettePosition;
static volatile uint8_t           BrushHue;
static volatile uint8_t           BrushSat;

static volatile uint32_t nowTimems;
static volatile uint32_t UpdateLastRunTime;
static volatile uint32_t UpdatePeriod            = 33;       // in ms (33ms = 30fps)
static volatile bool       ModeFirstTimeUsed       = true;     // Keep track of the first time this mode is used.
static volatile uint8_t    StepHue                 = 2;        // how many hue steps between paint blending updates
static volatile uint8_t    StepSat                 = 3;        // how many saturation steps between paint blending updates

// Duplicate of screen image for local use
static volatile uint8_t LedScreenMemoryLocalArrayHue [8][8];
static volatile uint8_t LedScreenMemoryLocalArraySat [8][8];

// Temporary storage of the screen that is overwritten with the palette displayed
static volatile uint8_t TempHue[2][8] = {
  {  0,  0,  0,  0,  0,  0,  0,  0},
  {  0,  0,  0,  0,  0,  0,  0,  0}
};
static volatile uint8_t TempSat[2][8] = {
  {  0,  0,  0,  0,  0,  0,  0,  0},
  {  0,  0,  0,  0,  0,  0,  0,  0}
};

uint8_t VisibleExtentX = 8;
uint8_t VisibleExtentY = 8;

// discern incoming color and existing color, and how to handle the differences in order to blend the colors if saturation is below 255. 
void Paint_Color_Mix_Hue_Sat(uint8_t y, uint8_t x, uint8_t InBrushHue, uint8_t InBrushSat){
  uint8_t OutBrushHue = 0;
  uint8_t OutBrushSat = 0;
  /*
  Serial.print("  Hue/Sat");
  Serial.print(" Existing = ");
  Serial.print(LedScreenMemoryLocalArrayHue[y][x]);
  Serial.print(" / ");
  Serial.print(LedScreenMemoryLocalArraySat[y][x]);  
  Serial.print(" Incoming = ");
  Serial.print(InBrushHue);
  Serial.print(" / ");
  Serial.print(InBrushSat);  
  */
  // For incoming brush colors that are fully saturated on/off (SAT=0 or 255, top row color palette, including black/blank, and the WHITE in bottom row)...
  // ...or an exception if the pixel is presently off, directly pass the incoming hue/sat through and display it.
  if ((InBrushSat == 0) || (InBrushSat == 255) || (LedScreenMemoryLocalArrayHue[y][x] == 0)) { 
    OutBrushHue = InBrushHue;
    OutBrushSat = InBrushSat;
  }
  // Otherwise the incoming brush color must have SAT between 1 and 254 (bottom row muted colors palette) and the incoming color shall be mixed with the existing pixel color. 
	else { // if ((InBrushSat > 0) || (InBrushSat < 255)) {
  // Each successive pass through this function first shifts the color toward the incoming hue while maintaining the existing saturation level.
    // Move the pixel hue downward if needed
    //       3     <     5 
    if (InBrushHue < LedScreenMemoryLocalArrayHue[y][x])  {
      // Prevent hue from overshooting or wrapping around. If present pixel hue is within one StepHue, jump to the desired hue.
      //            5                        -  3   = 2 > 3 false 
      if ( (LedScreenMemoryLocalArrayHue[y][x]-StepHue) > InBrushHue ) { // Will a step down in hue still be more than one step above the target hue?
        OutBrushHue = LedScreenMemoryLocalArrayHue[y][x] - StepHue;      // Yes, take a step down.
      }
      else {                                                             // No, jump to the target hue.
        OutBrushHue = InBrushHue; 
      }
      OutBrushSat = LedScreenMemoryLocalArraySat[y][x];                  // And no change to saturation.
    }
    //        250       >     245 
    else if (InBrushHue > LedScreenMemoryLocalArrayHue[y][x])  { 
      // Prevent hue from overshooting or wrapping around. If present pixel hue is within one StepHue, jump to the desired hue.
      //            245                        +  3   = 248 < 250 false 
      if ( (LedScreenMemoryLocalArrayHue[y][x]+StepHue) < InBrushHue ) { // Will a step down in hue still be more than one step above the target hue?
        OutBrushHue = LedScreenMemoryLocalArrayHue[y][x] + StepHue;      // Yes, take a step down.
      }
      else {                                                             // No, jump to the target hue.
        OutBrushHue = InBrushHue; 
      }
      OutBrushSat = LedScreenMemoryLocalArraySat[y][x];                  // And no change to saturation.
    }
    // After the color has been shifted, then increase saturation on successive calls, keeping the existing hue.
    else if (LedScreenMemoryLocalArraySat[y][x] < (255 - StepSat) ) { 
      OutBrushSat = LedScreenMemoryLocalArraySat[y][x] + StepSat; 
      OutBrushHue = LedScreenMemoryLocalArrayHue[y][x]; 
    }
    // If the saturation peaks at 255, don't roll around. Stay at 255.
    else if (LedScreenMemoryLocalArraySat [y][x] > (255 - StepSat - 1) ) { 
      OutBrushSat = 255; 
      OutBrushHue = LedScreenMemoryLocalArrayHue[y][x]; 
    }
  }
  /*
  Serial.print(" Outgoing = ");
  Serial.print(OutBrushHue);
  Serial.print(" / ");
  Serial.println(OutBrushSat);
  */  
  LED_Array_Matrix_Color_Hue_Sat_Write(y, x, OutBrushHue, OutBrushSat);
  LedScreenMemoryLocalArrayHue[y][x] = OutBrushHue;
  LedScreenMemoryLocalArraySat[y][x] = OutBrushSat;
}

void MoveBufferToScreen(bool TopNBottom) {
  uint8_t offset = 0;
  if (TopNBottom) { offset = 0; }     // The top part of the screen.
  else { offset = VisibleExtentY-2; } // The bottom part of the screen.
  for (uint8_t y=0; y<2; y++) {
    for (uint8_t x=0; x<VisibleExtentX; x++)  {
      LED_Array_Matrix_Color_Hue_Sat_Write(y+offset, x, TempHue[y][x], TempSat[y][x]);
    }
  }
}

void CopyLedLocalToScreen() {
  for (uint8_t y=0; y<VisibleExtentY; y++) {
    for (uint8_t x=0; x<VisibleExtentX; x++)  {
      LED_Array_Matrix_Color_Hue_Sat_Write(y, x, LedScreenMemoryLocalArrayHue[y][x], LedScreenMemoryLocalArraySat[y][x]);
    }
  }
}

void CopySymboltoLocal(uint8_t SymbolNumber) {
  for (uint8_t y=0; y<VisibleExtentY; y++) {
    for (uint8_t x=0; x<VisibleExtentX; x++)  {
      if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
        LedScreenMemoryLocalArrayHue[y][x] = AppPaintSymbols16bitHue[SymbolNumber][y][x]; 
        LedScreenMemoryLocalArraySat[y][x] = AppPaintSymbols16bitSat[SymbolNumber][y][x];
      }
      else {
        LedScreenMemoryLocalArrayHue[y][x] = AppPaintSymbolsHue[SymbolNumber][y][x]; 
        LedScreenMemoryLocalArraySat[y][x] = AppPaintSymbolsSat[SymbolNumber][y][x];
      }
    }
  }
}

void MoveScreenToBuffer(bool TopNBottom) {
  uint8_t offset = 0;
  if (TopNBottom) { offset = 0; }     // The top part of the screen.
  else { offset = VisibleExtentY-2; } // The bottom part of the screen.
  for (uint8_t y=0; y<2; y++) {
    for (uint8_t x=0; x<VisibleExtentX; x++)  {
      TempHue[y][x] = LedScreenMemoryLocalArrayHue[y+offset][x];
      TempSat[y][x] = LedScreenMemoryLocalArraySat[y+offset][x];
    }
  }
}

void AppPaint() {
  if (TopLevelModeChangedGet()) {                     // Fresh entry into this mode.
    Serial.println();
    Serial.println("  Paint Mode");
    Serial.println("    + = Show Color Pallette");
    Serial.println("    - = Hide Color Pallette");
    Serial.println("    S = Screen Clear");
    Serial.print(PROMPT);
    TopLevelThreeSoftButtonGlobalEnableSet(true); // Make sure + and - soft buttons are enabled to move to next mode if desired.
    TopLevelSetSoftButtonGlobalEnableSet(false);  // Disable the S button as SET, so it can be used to select.
    WriteAppPaintSymbol(0);
    LED_Array_Color_Display(0);
    MenuTimeOutCheckReset();
    ModeState = STATE_INTRO_SCREEN_WAIT_FOR_SELECT;
    PalettePosition = PALETTE_NONE;
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { 
      VisibleExtentX = 4;
      VisibleExtentY = 4;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  STATE MACHINE
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  nowTimems = millis();
  if ((nowTimems - UpdateLastRunTime) >= UpdatePeriod) {
    if (DebugLevel == 4) { Serial.print("App Paint State = "); Serial.println(ModeState); }
    // Service the mode state.
    switch(ModeState)
    {  
      case STATE_INTRO_SCREEN_WAIT_FOR_SELECT:
        // Check for touch of "S" to select paint mode and stay here
        if (ButtonState(4,0) >= 100) { MenuTimeOutCheckReset(); Button4Released = false; ModeState = STATE_SET_UP; }
        // Timeout to next sub-menu option in the sequence
        if (MenuTimeOutCheck(3000))  { TopLevelModeSetInc(); }
        break;

      case STATE_SET_UP:
        // If this was the first time into this state since power on set default screen to be 0xDEADBEEF and 0xC0D3C4FE
        if (ModeFirstTimeUsed == true) {
          // LED_Array_Monochrome_Set_Color(0,255,255);      // Hue 0 RED, 35 peach orange, 96 green, 135 aqua, 160 blue
          BrushHue = 25 ;
          BrushSat = 255;
          WriteAppPaintSymbol(3);
          CopySymboltoLocal(3);
          LED_Array_Binary_Write_Default();
          LED_Array_Binary_To_Matrix_Mono();
          LED_Array_Color_Display(0);                  // Show the updated LED array.
          #ifdef NEON_PIXEL_ARRAY
            Neon_Pixel_Array_Binary_Write_Default();
            Neon_Pixel_Array_Binary_To_Matrix_Mono();
          #endif
          OLEDScreenClear();
          ModeFirstTimeUsed = false;
        }
        else {
          CopyLedLocalToScreen();
        }
        TopLevelThreeSoftButtonGlobalEnableSet(false); // Disable + and - soft buttons from global mode switching use.
        ModeState = STATE_PAINT;
        break;

      case STATE_PAINT:
        // Find the cores currently affected by magnetism.
        Core_Mem_Scan_For_Magnet();

        // Depending on the status of the color palette, draw only, or select colors
        if (PalettePosition == PALETTE_NONE) {
          for (uint8_t y=0; y<VisibleExtentY; y++) {
            for (uint8_t x=0; x<VisibleExtentX; x++)  {
              if (CoreArrayMemory [y][x]) { 
                Paint_Color_Mix_Hue_Sat(y, x, BrushHue, BrushSat);
                #ifdef NEON_PIXEL_ARRAY
                  Neon_Pixel_Array_Matrix_Mono_Write(y, x, 1);
                #endif
              }
            }
          }
        }
        else if (PalettePosition == PALETTE_BOTTOM) {
          // Draw zone
          for (uint8_t y=0; y<(VisibleExtentY-2); y++) {
            for (uint8_t x=0; x<VisibleExtentX; x++)  {
              if (CoreArrayMemory [y][x]) { 
                Paint_Color_Mix_Hue_Sat(y, x, BrushHue, BrushSat);
                #ifdef NEON_PIXEL_ARRAY
                  Neon_Pixel_Array_Matrix_Mono_Write(y, x, 1);
                #endif
              }
            }
          }
          // Color select zone
          for (uint8_t x=0; x<VisibleExtentX; x++)  {
            if (CoreArrayMemory [(VisibleExtentY-2)][x]) { 
              if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { 
                BrushHue = AppPaintSymbols16bitHue[1][(VisibleExtentY-2)][x];
                BrushSat = AppPaintSymbols16bitSat[1][(VisibleExtentY-2)][x];
              }
              else {
                BrushHue = AppPaintSymbolsHue[1][(VisibleExtentY-2)][x];
                BrushSat = AppPaintSymbolsSat[1][(VisibleExtentY-2)][x];
              }
            }
          }
          if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { 
            for (uint8_t x=1; x<(VisibleExtentX); x++)  {     // Core16 avoid x=0
              if (CoreArrayMemory [(VisibleExtentY-1)][x]) { 
                BrushHue = AppPaintSymbols16bitHue[1][(VisibleExtentY-1)][x];
                BrushSat = AppPaintSymbols16bitSat[1][(VisibleExtentY-1)][x];
              }
            }
          }
          else {
            for (uint8_t x=1; x<(VisibleExtentX-1); x++)  { // Core16 avoid x=0 and 7
              if (CoreArrayMemory [(VisibleExtentY-1)][x]) {
                BrushHue = AppPaintSymbolsHue[1][(VisibleExtentY-1)][x];
                BrushSat = AppPaintSymbolsSat[1][(VisibleExtentY-1)][x];
              }
            }
          }
        }
        else if (PalettePosition == PALETTE_TOP) {
          // Draw zone
          for (uint8_t y=2; y<VisibleExtentY; y++) {
            for (uint8_t x=0; x<VisibleExtentX; x++)  {
              if (CoreArrayMemory [y][x]) { 
                Paint_Color_Mix_Hue_Sat(y, x, BrushHue, BrushSat);
                #ifdef NEON_PIXEL_ARRAY
                  Neon_Pixel_Array_Matrix_Mono_Write(y, x, 1);
                #endif
              }
            }
          }
          // Color select zone
          for (uint8_t x=0; x<VisibleExtentX; x++)  {
            if (CoreArrayMemory [0][x]) {
              if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { 
                BrushHue = AppPaintSymbols16bitHue[2][0][x];
                BrushSat = AppPaintSymbols16bitSat[2][0][x];
              }
              else {
                BrushHue = AppPaintSymbolsHue[2][0][x];
                BrushSat = AppPaintSymbolsSat[2][0][x];
              }
            }
          }

          if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { 
            for (uint8_t x=1; x<(VisibleExtentX); x++)  { // Core16 avoid x=0
              if (CoreArrayMemory [1][x]) { 
                BrushHue = AppPaintSymbols16bitHue[2][1][x];
                BrushSat = AppPaintSymbols16bitSat[2][1][x];
              }
            }
          }
          else {
            for (uint8_t x=1; x<(VisibleExtentX-1); x++)  { // Core64 avoid X=0 and 7
              if (CoreArrayMemory [1][x]) { 
                BrushHue = AppPaintSymbolsHue[2][1][x];
                BrushSat = AppPaintSymbolsSat[2][1][x];
              }
            }
          }
        }
        
        // Touch of 'S' clears the screen.
        if ( (Button4Released) && (ButtonState(4,0) > 500 ) ) { 
          #ifdef NEON_PIXEL_ARRAY
            Neon_Pixel_Array_Memory_Clear();
          #endif
          Button4Released = false;
          LED_Array_Memory_Clear(); // Clear LED Array Memory
          // and clear the local LED array memory
          for (uint8_t y=0; y<VisibleExtentY; y++) {
            for (uint8_t x=0; x<VisibleExtentX; x++)  {
              LedScreenMemoryLocalArrayHue[y][x] = 0;
              LedScreenMemoryLocalArraySat[y][x] = 0;
              // and clear the temp LED array memory
              if (y < 2) { 
                TempHue[y][x] = 0;
                TempSat[y][x] = 0;
              }
            }
          }
          Serial.println("  LED Array Cleared.");
        }
        if (ButtonState(4,0) == 0) {
          Button4Released = true;
        }

        // Touch of '+' brings up the color palette and cycles it through top and bottom positions.
        if ( (Button3Released) && (ButtonState(3,0) > 200 ) ) { 
          Button3Released = false;
          // If color palette is already up, swap top/bottom placement.
          if (PalettePosition == PALETTE_BOTTOM) {
            MoveBufferToScreen(0); // Buffer -> Screen Bottom : Write the temp image buffer over the palette.
            MoveScreenToBuffer(1); // Screen Top -> Buffer : Copy the top two rows of the existing image to temp image buffer.
            Serial.println("  Color Palette moved to top.");
            PalettePosition = PALETTE_TOP;
          }
          else if (PalettePosition == PALETTE_TOP) {
            MoveBufferToScreen(1); // Buffer -> Screen Top : Write the temp image buffer over the palette.
            MoveScreenToBuffer(0); // Screen Bottom -> Buffer : Copy the bottom two rows of the existing image to temp image buffer.
            Serial.println("  Color Palette moved to bottom.");
            PalettePosition = PALETTE_BOTTOM;
          }
          // If color palette is not already displayed, display it at the bottom.
          else if (PalettePosition == PALETTE_NONE) {
            MoveScreenToBuffer(0); // Screen Bottom -> Buffer : Copy the bottom two rows of the existing image to temp image buffer.
            Serial.println("  Color Palette displayed at bottom.");
            PalettePosition = PALETTE_BOTTOM;
          }
        }
        if (ButtonState(3,0) == 0) {
          Button3Released = true;
        }

        // Touch of '-' hides the color palette.
        if ( (Button2Released) && (ButtonState(2,0) > 100 ) ) { 
          Button2Released = false;
          Serial.println("  Color Palette hidden.");
          if (PalettePosition == PALETTE_TOP) {
            MoveBufferToScreen(1); // Buffer -> Screen Top
          }
          if (PalettePosition == PALETTE_BOTTOM) {
            MoveBufferToScreen(0); // Buffer -> Screen Bottom
          }
          PalettePosition = PALETTE_NONE;
        }
        if (ButtonState(2,0) == 0) {
          Button2Released = true;
        }

        // If the Color Palette is up, display it.
        if (PalettePosition == PALETTE_BOTTOM) {
          // show it on the bottom
          Serial.println("  Color Palette showing on bottom.");
          WriteAppPaintPalette(0);
          // Overwrite far right two pixels with currently selected color
          LED_Array_Matrix_Color_Hue_Sat_Write((VisibleExtentY-1), 0, BrushHue, BrushSat);
          if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { 
            // Except for Core16, just overwrite the one pixel since there isn't as much room to work with.
          }
          else {
            LED_Array_Matrix_Color_Hue_Sat_Write((VisibleExtentY-1), (VisibleExtentX-1), BrushHue, BrushSat);
          }
        }
        if (PalettePosition == PALETTE_TOP) {
          // show it on the top
          Serial.println("  Color Palette showing on top.");
          WriteAppPaintPalette(1);
          // Overwrite far right two pixels with currently selected color
          LED_Array_Matrix_Color_Hue_Sat_Write(1, 0, BrushHue, BrushSat);
          if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { 
            // Except for Core16, just overwrite the one pixel since there isn't as much room to work with.
          }
          else {
           LED_Array_Matrix_Color_Hue_Sat_Write(1, (VisibleExtentX-1), BrushHue, BrushSat);
          }
        }


        LED_Array_Color_Display(0);                  // Show the updated LED array.
        LED_Array_Matrix_Mono_to_Binary();                // Convert whatever is in the LED Matrix Array to a 64-bit binary value...
        OLEDTopLevelModeSet(TopLevelModeGet());
        // OLEDScreenUpdate();
        OLED_Show_Matrix_Mono_Hex();                      // ...and display it on the OLED.
        #ifdef NEON_PIXEL_ARRAY
          Neon_Pixel_Array_Matrix_Mono_Display();
        #endif
        break;

      default:
        break;
    }
    // Service non-state dependent stuff
    UpdateLastRunTime = nowTimems;
  }
} // PAINT APP
