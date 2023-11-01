#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "Game_Snake.h"

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
#include "Hal/Buttons_HAL.h"

#include "SubSystems/Command_Line_Handler.h"

static bool     Button1Released = true;
static bool     Button2Released = true;
static bool     Button3Released = true;
static bool     Button4Released = true;
static uint32_t Button1HoldTime = 0;
static uint32_t Button2HoldTime = 0;
static uint32_t Button3HoldTime = 0;
static uint32_t Button4HoldTime = 0;

static uint8_t  GameState = 0;
static uint32_t nowTime = 0;
static uint32_t SnakeGameUpdateLastRunTime = 0;
static uint32_t SnakeGameUpdatePeriod = 33;
static uint32_t GameOverTimer = 0;
static uint32_t GameOverTimerAutoReset = 3000;
static bool GameOver = false;
static bool Winner = false;
volatile unsigned long CorePhysicalStateChanged = 0;
volatile unsigned long ButtonPressDuration_ms = 0;
volatile unsigned int SnakeHeadX; // lower left, remember the logical array position is backwards how it is written in source code.
volatile unsigned int SnakeHeadY; 
volatile signed int SnakeLength;
volatile signed int OldSnakeLength;
volatile unsigned int TestX = 0;
volatile unsigned int TestY = 0;
bool MovementDetected = false;

/*  The game space memory 8x8 playable area, using signed 8-bit value
     0 = nothing in that pixel
     1 = snake body
     2 = snake head
    -1 = poison
    -2 = food
*/ 
int8_t SnakeGameMemory [8][8];
uint8_t ColorBlank     =   0; // blank
uint8_t ColorSnakeBody = 220; // purple
uint8_t ColorSnakeHead = 127; // blue
uint8_t ColorPoison    = 254; // red
uint8_t ColorFood      =  85; // green

uint8_t SnakeVisibleExtentX = 8;
uint8_t SnakeVisibleExtentY = 8;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SNAKE GAME FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RandomStartMap() {
  for (uint8_t x=0; x<SnakeVisibleExtentX; x++) {
    for (uint8_t y=0; y<SnakeVisibleExtentY; y++) {
      int RandomPixel = random(0, 6); // wider range than need to get some extra blank space
      if (RandomPixel == 1) { RandomPixel = RandomPixel * (-1); }
      if (RandomPixel == 2) { RandomPixel = RandomPixel * (-1); }
      if (RandomPixel > 2) { RandomPixel = 0; } // Clear pixels play area
      SnakeGameMemory[y][x] = RandomPixel;
    }
  }
  SnakeHeadX = random(0, (SnakeVisibleExtentX-1));
  SnakeHeadY = random(0, (SnakeVisibleExtentY-1));
  SnakeGameMemory[SnakeHeadY][SnakeHeadX] = 1; // The snake starts here!
}

void IncreaseSnakeLength() {
  SnakeLength++;
}

void DecreaseSnakeLength() {
  SnakeLength = SnakeLength - 2;
  if (SnakeLength < 1) { GameOver = true; }
}

void RemoveSnakeTail() {
  // scan for positive numbers greater than SnakeLength and remove them from screen memory locations
  for (uint8_t x=0; x<SnakeVisibleExtentX; x++) {    // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
    for (uint8_t y=0; y<SnakeVisibleExtentY; y++) {
      if (SnakeGameMemory[y][x] > SnakeLength ) { SnakeGameMemory[y][x] = 0; } // 
    }
  }  
}

void AreYouAWinner() {
  // Assume a win
  Winner = true;
  // scan for -2 numbers, if any are found left, you're not a winner yet.
  for (uint8_t x=0; x<SnakeVisibleExtentX; x++) {    // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen
    for (uint8_t y=0; y<SnakeVisibleExtentY; y++) {
      if (SnakeGameMemory[y][x] == ((signed int)-2) ) { Winner = false; }
    }
  }  
}

bool SnakeGameLogic() { // Returns 1 if activity of stylus movement is detected. Else 0 for no activity.
  // Y Keep track of previous state in the screen memory (already in screen memory array)
  // Y Keep track of updated state affected by stylus (already in CorePhysicalStateChanged when this routine runs)
  // Y Keep track of the head in X and Y position to make it easier to detect a stylus move
  // Y Translate the core change from 8x8 bit array 
  // Y Test, scan stylus memory, and if there is a stylus, update screen memory
    //  for (uint8_t x=0; x<=7; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
    //  {
    //    for (uint8_t y=0; y<=3; y++)
    //    {
    //      if (CoreArrayMemory[y][x] == 1) { SnakeGameMemory[y][x] = 1; }
    //    }
    //  }

  // Look for movement near the snake head since that is all that is valid. Check the four cardinal directions, as long as they are one screen.
  // Test right for movement and a blank pixel
    OldSnakeLength = SnakeLength;
    MovementDetected = false;
    TestX = SnakeHeadX + 1;
    TestY = SnakeHeadY;
    if ((TestX < SnakeVisibleExtentX) && (TestX >= 0) && (MovementDetected == false))
    {
      if (CoreArrayMemory[TestY][TestX] == 1)
      {
        if (SnakeGameMemory[TestY][TestX] == 0) // blank pixel found so move in
        { 
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
        if (SnakeGameMemory[TestY][TestX] == 1) // crashed into snake means you die!
        {
          //GameOver = true;
        }
        if (SnakeGameMemory[TestY][TestX] == -2) // food grows snake 1, and moves in
        {
          IncreaseSnakeLength();
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
        if (SnakeGameMemory[TestY][TestX] == -1) // poison shortens snake 2, and moves in
        {
          DecreaseSnakeLength();
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
      }
    }
 
  // Test down for movement and a blank pixel
    TestX = SnakeHeadX;
    TestY = SnakeHeadY + 1;
    if ((TestY < SnakeVisibleExtentY) && (TestY >= 0) && (MovementDetected == false))
    { 
      if (CoreArrayMemory[TestY][TestX] == 1)
      {
        if (SnakeGameMemory[TestY][TestX] == 0) // blank pixel found so move in
        { 
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
        if (SnakeGameMemory[TestY][TestX] == 1) // crashed into snake means you die!
        {
          //GameOver = true;
        }
        if (SnakeGameMemory[TestY][TestX] == -2) // food grows snake 1, and moves in
        {
          IncreaseSnakeLength();
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
        if (SnakeGameMemory[TestY][TestX] == -1) // poison shortens snake 2, and moves in
        {
          DecreaseSnakeLength();
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
      }
    }

  // Test left for movement and a blank pixel
    TestX = SnakeHeadX - 1;
    TestY = SnakeHeadY;
    if ((TestX < SnakeVisibleExtentX) && (TestX >= 0) && (MovementDetected == false))
    { 
      if (CoreArrayMemory[TestY][TestX] == 1)
      {
        if (SnakeGameMemory[TestY][TestX] == 0) // blank pixel found so move in
        { 
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
        if (SnakeGameMemory[TestY][TestX] == 1) // crashed into snake means you die!
        {
          //GameOver = true;
        }
        if (SnakeGameMemory[TestY][TestX] == -2) // food grows snake 1, and moves in
        {
          IncreaseSnakeLength();
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
        if (SnakeGameMemory[TestY][TestX] == -1) // poison shortens snake 2, and moves in
        {
          DecreaseSnakeLength();
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
      }
    }
 
  // Test UP for movement and a blank pixel
    TestX = SnakeHeadX;
    TestY = SnakeHeadY - 1;
    if ((TestY < SnakeVisibleExtentY) && (TestY >= 0) && (MovementDetected == false))
    { 
      if (CoreArrayMemory[TestY][TestX] == 1)
      {
        if (SnakeGameMemory[TestY][TestX] == 0) // blank pixel found so move in
        { 
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
        if (SnakeGameMemory[TestY][TestX] == 1) // crashed into snake means you die!
        {
          //GameOver = true;
        }
        if (SnakeGameMemory[TestY][TestX] == -2) // food grows snake 1, and moves in
        {
          IncreaseSnakeLength();
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
        if (SnakeGameMemory[TestY][TestX] == -1) // poison shortens snake 2, and moves in
        {
          DecreaseSnakeLength();
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
      }
    }
    
    if (MovementDetected == true)
    {
      if (SnakeLength > OldSnakeLength) // snake got longer, no need to remove anything.
      {
          // scan for positive numbers up to SnakeLength and add one to those screen memory locations
          for (uint8_t x=0; x<SnakeVisibleExtentX; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
          {
            for (uint8_t y=0; y<SnakeVisibleExtentY; y++)
            {
              if (SnakeGameMemory[y][x] > (signed int)0) { SnakeGameMemory[y][x]=SnakeGameMemory[y][x]+(signed int)1; }
            }
          }  
          // Nothing to erase
      }
      else if (SnakeLength < OldSnakeLength) // snake got shorter, remove any above snake length
      {
          for (uint8_t x=0; x<SnakeVisibleExtentX; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
          {
            for (uint8_t y=0; y<SnakeVisibleExtentY; y++)
            {
              if (SnakeGameMemory[y][x] >= (signed int)SnakeLength) { SnakeGameMemory[y][x]=0; }
            }
          }
          // scan for positive numbers up to SnakeLength and add one to those screen memory locations
          for (uint8_t x=0; x<SnakeVisibleExtentX; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
          {
            for (uint8_t y=0; y<SnakeVisibleExtentY; y++)
            {
              if (SnakeGameMemory[y][x] > 0) { SnakeGameMemory[y][x]=SnakeGameMemory[y][x]+(signed int)1; }
            }
          }
          //RemoveSnakeTail();
      }
      else if (SnakeLength == OldSnakeLength) // no change, just remove the old end, then increment existing body
      {
          for (uint8_t x=0; x<SnakeVisibleExtentX; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
          {
            for (uint8_t y=0; y<SnakeVisibleExtentY; y++)
            {
              if (SnakeGameMemory[y][x] >= (signed int)SnakeLength) { SnakeGameMemory[y][x]=0; }
            }
          }
          // scan for positive numbers up to SnakeLength and add one to those screen memory locations
          for (uint8_t x=0; x<SnakeVisibleExtentX; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
          {
            for (uint8_t y=0; y<SnakeVisibleExtentY; y++)
            {
              if (SnakeGameMemory[y][x] > 0) { SnakeGameMemory[y][x]=SnakeGameMemory[y][x]+(signed int)1; }
            }
          }
      }
      SnakeGameMemory[SnakeHeadY][SnakeHeadX] = 1; // new snake head position
      OldSnakeLength = SnakeLength;
    }
    AreYouAWinner();
    return MovementDetected;
}

void ConvertSnakeGameMemoryToScreenMemory() {
  for (uint8_t x=0; x<SnakeVisibleExtentX; x++) {    // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
    for (uint8_t y=0; y<SnakeVisibleExtentY; y++) {
      if (SnakeGameMemory[y][x] == 0) { LED_Array_Matrix_Color_Hue_Write(y, x, ColorBlank); }
      if (SnakeGameMemory[y][x] == 1) { LED_Array_Matrix_Color_Hue_Write(y, x, ColorSnakeBody); }
      if (SnakeGameMemory[y][x] == 2) { LED_Array_Matrix_Color_Hue_Write(y, x, ColorSnakeHead); }
      if (SnakeGameMemory[y][x] == -1) { LED_Array_Matrix_Color_Hue_Write(y, x, ColorPoison); }
      if (SnakeGameMemory[y][x] == -2) { LED_Array_Matrix_Color_Hue_Write(y, x, ColorFood); }
    }
  }
}

void GameSnake() {
  if (TopLevelModeChangedGet()) {                           // First time entry into this mode.
    Serial.println();
    Serial.println("   Snake Game Sub-Menu");
    Serial.println("    M = Main DGAUSS Menu");
    Serial.println("    + = Next Game");
    Serial.println("    - = Previous Game");
    Serial.println("    S = Select / Start Game Again");
    Serial.print(PROMPT);
    TopLevelThreeSoftButtonGlobalEnableSet(true); // Make sure MENU + and - soft buttons are enabled.
    TopLevelSetSoftButtonGlobalEnableSet(false);  // Disable the S button as SET, so it can be used in this game as Select.
    WriteGameSnakeSymbol(0);
    MenuTimeOutCheckReset();
    LED_Array_Color_Display(1);
    GameState = 0;
    MenuTimeOutCheckReset();
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { 
      SnakeVisibleExtentX = 4;
      SnakeVisibleExtentY = 4;
    }
  }
    OLEDTopLevelModeSet(TopLevelModeGet());
    OLEDScreenUpdate();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // SNAKE GAME STATE MACHINE
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  nowTime = millis();
  if ((nowTime - SnakeGameUpdateLastRunTime) >= SnakeGameUpdatePeriod)
  {
    Core_Mem_Scan_For_Magnet(); // Update the position of the stylus in the CoreArrayMemory
    if(DebugLevel==4) { Serial.print("Snake Game State = "); Serial.println(GameState); }
    switch(GameState)
    {
      case 0: // Game Splash Screen, wait for select button to start the game.
        // CheckStartButton();
        if (ButtonState(4,0) >= 100) {
          MenuTimeOutCheckReset();
          GameState = 1;
        }
        if (MenuTimeOutCheck(5000)) { TopLevelModeSetInc(); }
        break;
      case 1: // Setup a new game
        Winner = false;
        GameOver = false;
        SnakeLength = 1; 
        RandomStartMap(); // internally clears upper left so snake can start there
        ConvertSnakeGameMemoryToScreenMemory(); // Convert SnakeGameMemory to LED Matrix and refresh it.
        MenuTimeOutCheckReset();
        GameState = 2;
        break;
      case 2: // Play
        if (SnakeGameLogic()) {          // Returns 1 if there was activity detected by stylus.
          MenuTimeOutCheckReset();      // Prevent mode timeout
        }
        if (MenuTimeOutCheck(30000)) { TopLevelModeSetToDefault(); } // Is it time to timeout? Then timeout.
        ConvertSnakeGameMemoryToScreenMemory(); // Convert SnakeGameMemory to LED Matrix and refresh it.
        if (GameOver) { GameOverTimer = nowTime; GameState = 3; }
        if (Winner)  { GameOverTimer = nowTime; GameState = 4; }
        // CheckStartButton();
        if (ButtonState(4,0) >= 200) {
          MenuTimeOutCheckReset();
          GameState = 1;
        }
        break;
      case 3: // Game Over = Red Screen
        for (uint8_t x=0; x<SnakeVisibleExtentX; x++) {
          for (uint8_t y=0; y<SnakeVisibleExtentY; y++) {
            SnakeGameMemory[y][x] = -1;
          }
        }
        if ((nowTime - GameOverTimer) > GameOverTimerAutoReset) {
          MenuTimeOutCheckReset();
          GameState = 1;
        }
        // WriteColorFontSymbolToLedScreenMemoryMatrixHue(14);
        WriteGameSnakeSymbol(2);
        break;
      case 4: // Winner = Green Screen
        for (uint8_t x=0; x<SnakeVisibleExtentX; x++) {
          for (uint8_t y=0; y<SnakeVisibleExtentY; y++) {
            SnakeGameMemory[y][x] = -2;
          }
        }
        if ((nowTime - GameOverTimer) > GameOverTimerAutoReset) {
          MenuTimeOutCheckReset();
          GameState = 1;
        }
        // WriteColorFontSymbolToLedScreenMemoryMatrixHue(13);
        WriteGameSnakeSymbol(1);
        break;
      default:
        break;
    }
    LED_Array_Color_Display(1);
    SnakeGameUpdateLastRunTime = nowTime;
  }

} // Snake
