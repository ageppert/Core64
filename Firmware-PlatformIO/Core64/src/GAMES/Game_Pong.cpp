#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "Game_Pong.h"

// INCLUDES, COMMON TO ALL GAMES
  #include <stdint.h>
  #include <stdbool.h>
  #include "Mode_Manager.h"
  #include "Config/HardwareIOMap.h"
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
  #include "Hal/Buttons_HAL.h"
  #include "SubSystems/Command_Line_Handler.h"

// VARIABLES, COMMON TO ALL GAMES
  enum GameState {
    GAME_STATE_INTRO_SCREEN ,    // 0
    GAME_STATE_SET_UP ,          // 1
    GAME_STATE_PLAY ,            // 2
    GAME_STATE_WIN ,             //
    GAME_STATE_LOOSE ,           //
    GAME_STATE_LAST              // Last one, return to Startup 0. Note, this is different than "default" which is handled by switch/case statement.
  };
  volatile uint8_t  GameState;
  volatile uint32_t nowTime;
  volatile uint32_t GameUpdateLastRunTime;
  volatile uint32_t GameUpdatePeriod;
  volatile uint32_t GameOverTimer;
  volatile uint32_t GameOverTimerAutoReset;  
  volatile bool     GameMovementDetected;
  volatile bool     GameOver;                   
  volatile uint8_t  Winner;                     // 0 = neither, 1 = player one (left), 2 = player two (right)

// VARIABLES, CUSTOM TO EACH GAME
  // All arrays are addresssed Y,X to match visual layout in IDE to as-see-on-screen.
  // Majority of these values are set-up in GameStartMap()
  // Game Field Space. Center 8x8 is visible. Top/bottom rows are for bounce wall. Left/Right columnes are for end zone.
  volatile uint8_t  GameField         [10][10]; // Upper left corner is 0,0.
  volatile uint8_t  GameViewOffsetY =        1; // Upper left corner of the viewing window within GameField (limited <2)
  volatile uint8_t  GameViewOffsetX =        1; // Upper left corner of the viewing window within GameField (limited <2)
  // Game Element Positions and Movement
  volatile uint8_t  BallPositionYNow          ; // 0 to 9, 0 to 9 pixels
  volatile uint8_t  BallPositionXNow          ; // 0 to 9, 0 to 9 pixels
  volatile uint8_t  BallPositionYNext         ; // 0 to 9, 0 to 9 pixels
  volatile uint8_t  BallPositionXNext         ; // 0 to 9, 0 to 9 pixels
  volatile int8_t   BallDirectionY            ; // -1 or +1 pixels (+1, +1 is toward lower right)
  volatile int8_t   BallDirectionX            ; // -1 or +1 pixels (-1, -1 is toward upper left)
  volatile uint16_t BallVelocity              ; // lower number is faster (ms between position updates)
  volatile uint32_t BallUpdateLastRunTime;    ; // time of last update (ms)
  volatile uint8_t  Paddle1CenterYNow         ; // 2-7, limited to field of view
  volatile uint8_t  Paddle1CenterXNow         ; // 1-3, yes... not just against the left edge!
  volatile uint8_t  Paddle2CenterYNow         ; // 2-7, limited to field of view
  volatile uint8_t  Paddle2CenterXNow         ; // 6-8, yes... not just against the right edge!
  volatile uint8_t  Paddle1CenterYNext        ; // 2-7, limited to field of view
  volatile uint8_t  Paddle1CenterXNext        ; // 1-3, yes... not just against the left edge!
  volatile uint8_t  Paddle2CenterYNext        ; // 2-7, limited to field of view
  volatile uint8_t  Paddle2CenterXNext        ; // 6-8, yes... not just against the right edge!
  // Stylus Positi
  volatile int8_t   Stylus1Y                  ; // 1-8
  volatile int8_t   Stylus1X                  ; // 1-3
  volatile int8_t   Stylus2Y                  ; // 1-8
  volatile int8_t   Stylus2X                  ; // 6-8
  bool              StylusMovementDetected    ; // If false, skip the player paddle update.
  /* Not sure this is needed? 
  volatile unsigned long CorePhysicalStateChanged = 0;
  */ 
  // Game Pixel Types
  uint8_t GPT_OPEN      =   0 ;        //
  uint8_t GPT_BALL      =   1 ;        //
  uint8_t GPT_WALL      =   2 ;        //
  uint8_t GPT_END_ZONE  =   3 ;        //
  uint8_t GPT_P1L_TOP   =  11 ;        //
  uint8_t GPT_P1L_MID   =  12 ;        //
  uint8_t GPT_P1L_BOT   =  13 ;        //
  uint8_t GPT_P2R_TOP   =  21 ;        //
  uint8_t GPT_P2R_MID   =  22 ;        //
  uint8_t GPT_P2R_BOT   =  23 ;        //
  // Game Pixel Colors
  uint8_t GPC_OPEN      =   0 ;        // no color
  uint8_t GPC_BALL      = 255 ;        // white
  uint8_t GPC_WALL      = 254 ;        // red
  uint8_t GPC_END_ZONE  =  85 ;        // green
  uint8_t GPC_P1L_TOP   = 255 ;        // white
  uint8_t GPC_P1L_MID   = 255 ;        // white
  uint8_t GPC_P1L_BOT   = 255 ;        // white
  uint8_t GPC_P2R_TOP   = 255 ;        // white
  uint8_t GPC_P2R_MID   = 255 ;        // white
  uint8_t GPC_P2R_BOT   = 255 ;        // white
  /* Sample set-up visualized
    X      0    1    2    3    4    5    6    7    8    9
  Y     --------------- PLAY FIELD BOUNDARY ---------------
  0     |  3 |  2 |  2 |  2 |  2 |  2 |  2 |  2 |  2 |  3 |
        |    *********** VIEWABLE BOUNDARY ***********    |
  1     |  3 * 11 |  0 |  0 |  0 |  0 |  0 |  0 |  0 *  3 |
  2     |  3 * 12 |  0 |  0 |  0 |  0 |  0 |  0 |  0 *  3 |
  3     |  3 * 13 |  0 |  0 |  0 |  0 |  0 |  0 |  0 *  3 |
  4     |  3 *  0 |  0 |  0 |  1 |  0 |  0 |  0 |  0 *  3 |
  5     |  3 *  0 |  0 |  0 |  0 |  0 |  0 |  0 | 21 *  3 |
  6     |  3 *  0 |  0 |  0 |  0 |  0 |  0 |  0 | 22 *  3 |
  7     |  3 *  0 |  0 |  0 |  0 |  0 |  0 |  0 | 23 *  3 |
  8     |  3 *  0 |  0 |  0 |  0 |  0 |  0 |  0 |  0 *  3 |
        |    *****************************************    |
  9     |  3 |  2 |  2 |  2 |  2 |  2 |  2 |  2 |  2 |  3 |
        ---------------------------------------------------
  */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PONG GAME FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GameSetUpBounceWalls() {
  for (uint8_t x=1; x<=8; x++) {
    GameField[0][x] = GPT_WALL;
    GameField[9][x] = GPT_WALL;
  }
}

void GameSetUpEndZones() {
  for (uint8_t y=0; y<=9; y++) {
    GameField[y][0] = GPT_END_ZONE;
    GameField[y][9] = GPT_END_ZONE;
  }
}

void GameSetUpClearPlayingField() {
  for (uint8_t x=1; x<=8; x++) {
    for (uint8_t y=1; y<=8; y++) {
      GameField[y][x] = GPT_OPEN;
    }
  }
}

bool StylusFind() {
  bool StylusFound = false;
  // Look for stylus on left 3 columns of game play area
  for (uint8_t x=0; x<=2; x++){
    for (uint8_t y=0; y<=7; y++){
      if (CoreArrayMemory[y][x]){
        StylusFound = true;
        Paddle1CenterXNext = x+1; // "+1" converts Core Array position to gamefield position
        Paddle1CenterYNext = y+1; // "+1" converts Core Array position to gamefield position
        x = 8;
        y = 8; // Force exit from loop as soon as a stylus is sensed on this side.
      }
    }
  }
  // Look for stylus on right 3 columns of game play area
  for (uint8_t x=5; x<=7; x++){
    for (uint8_t y=0; y<=7; y++){
      if (CoreArrayMemory[y][x]){
        StylusFound = true;
        Paddle2CenterXNext = x+1; // "+1" converts Core Array position to gamefield position
        Paddle2CenterYNext = y+1; // "+1" converts Core Array position to gamefield position
        x = 8;
        y = 8; // Force exit from loop as soon as a stylus is sensed on this side.
      }
    }
  }
  return StylusFound;
}

void Paddle1Update() {
  // Did the paddle move? If yes, update paddle. Else, do nothing and return.
  if ( (Paddle1CenterXNext != Paddle1CenterXNow) || (Paddle1CenterYNext != Paddle1CenterYNow) ) {
    // Erase old paddle position
    GameField [Paddle1CenterYNow-1][Paddle1CenterXNow] = GPC_OPEN;
    GameField [Paddle1CenterYNow  ][Paddle1CenterXNow] = GPC_OPEN;
    GameField [Paddle1CenterYNow+1][Paddle1CenterXNow] = GPC_OPEN;
    // Constrain paddle center position to stay fully on screen, in 3 leftmost columns.
    if (Paddle1CenterXNext<1) {Paddle1CenterXNext=1;}
    if (Paddle1CenterXNext>3) {Paddle1CenterXNext=3;}
    if (Paddle1CenterYNext<2) {Paddle1CenterYNext=2;}
    if (Paddle1CenterYNext>7) {Paddle1CenterYNext=7;}
    // Update paddle to new position
    Paddle1CenterXNow = Paddle1CenterXNext;
    Paddle1CenterYNow = Paddle1CenterYNext;
  }
  GameField [Paddle1CenterYNow-1][Paddle1CenterXNow] = GPT_P1L_TOP;
  GameField [Paddle1CenterYNow  ][Paddle1CenterXNow] = GPT_P1L_MID;
  GameField [Paddle1CenterYNow+1][Paddle1CenterXNow] = GPT_P1L_BOT;
}

void Paddle2Update(){
  // Did the paddle move? If yes, update paddle. Else, do nothing and return.
  if ( (Paddle2CenterXNext != Paddle2CenterXNow) || (Paddle2CenterYNext != Paddle2CenterYNow) ) {
    // Erase old paddle position
    GameField [Paddle2CenterYNow-1][Paddle2CenterXNow] = GPC_OPEN;
    GameField [Paddle2CenterYNow  ][Paddle2CenterXNow] = GPC_OPEN;
    GameField [Paddle2CenterYNow+1][Paddle2CenterXNow] = GPC_OPEN;
    // Constrain paddle center position to stay fully on screen, in 3 rightmost columns.
    if (Paddle2CenterXNext<6) {Paddle2CenterXNext=6;}
    if (Paddle2CenterXNext>8) {Paddle2CenterXNext=8;}
    if (Paddle2CenterYNext<2) {Paddle2CenterYNext=2;}
    if (Paddle2CenterYNext>7) {Paddle2CenterYNext=7;}
    // Update paddle to new position
    Paddle2CenterXNow = Paddle2CenterXNext;
    Paddle2CenterYNow = Paddle2CenterYNext;
  }
  GameField [Paddle2CenterYNow-1][Paddle2CenterXNow] = GPT_P2R_TOP;
  GameField [Paddle2CenterYNow  ][Paddle2CenterXNow] = GPT_P2R_MID;
  GameField [Paddle2CenterYNow+1][Paddle2CenterXNow] = GPT_P2R_BOT;
}

void BallUpdate() {
  // Is it time to update the ball yet?
  if ((nowTime - BallUpdateLastRunTime) >= BallVelocity)
  {
    // Ball direction always needs an X component. Can't be 0 or there no game play.
    if (BallDirectionX == 0) {BallDirectionX = 1;}
    
    // Calculate new position of ball
    BallPositionYNext = BallPositionYNow + BallDirectionY;
    BallPositionXNext = BallPositionXNow + BallDirectionX;

    // Test: Is ball entering the end zone for a point?
    if ( GameField [BallPositionYNext][BallPositionXNext] == GPT_END_ZONE) {
      // Test: Collision with left end zone?
      if (BallPositionXNext == 0) { Winner = 2; }
      // Test: Collision with right end zone?
      if (BallPositionXNext == 9) { Winner = 1; }
    } 

    // Test: Is ball colliding with wall and needs to bounce?
    if ( GameField [BallPositionYNext][BallPositionXNext] == GPT_WALL) {
      // Test: Collision with top wall?
      if (BallPositionYNext == 0) { BallDirectionY = 1; }
      // Test: Collision with bottom wall?
      if (BallPositionYNext == 9) { BallDirectionY = -1; }
      // Over
      BallPositionYNext = BallPositionYNow + BallDirectionY;
    }

    // Test: Is ball colliding with paddle 1 and needs to bounce based on paddle contact point?
    if ( GameField [BallPositionYNext][BallPositionXNext] == GPT_P1L_TOP) {
      BallDirectionY = -1;
      BallDirectionX =  1;
      BallPositionYNext = BallPositionYNow + BallDirectionY;
      BallPositionXNext = BallPositionXNow + BallDirectionX;
    }
    if ( GameField [BallPositionYNext][BallPositionXNext] == GPT_P1L_MID) {
      BallDirectionY =  0;
      BallDirectionX =  1;
      BallPositionYNext = BallPositionYNow + BallDirectionY;
      BallPositionXNext = BallPositionXNow + BallDirectionX;
    }
    if ( GameField [BallPositionYNext][BallPositionXNext] == GPT_P1L_BOT) {
      BallDirectionY =  1;
      BallDirectionX =  1;
      BallPositionYNext = BallPositionYNow + BallDirectionY;
      BallPositionXNext = BallPositionXNow + BallDirectionX;
    }
    // Test: Is ball colliding with paddle 2 and needs to bounce based on paddle contact point?
    if ( GameField [BallPositionYNext][BallPositionXNext] == GPT_P2R_TOP) {
      BallDirectionY = -1;
      BallDirectionX = -1;
      BallPositionYNext = BallPositionYNow + BallDirectionY;
      BallPositionXNext = BallPositionXNow + BallDirectionX;
    }
    if ( GameField [BallPositionYNext][BallPositionXNext] == GPT_P2R_MID) {
      BallDirectionY =  0;
      BallDirectionX = -1;
      BallPositionYNext = BallPositionYNow + BallDirectionY;
      BallPositionXNext = BallPositionXNow + BallDirectionX;
    }
    if ( GameField [BallPositionYNext][BallPositionXNext] == GPT_P2R_BOT) {
      BallDirectionY =  1;
      BallDirectionX = -1;
      BallPositionYNext = BallPositionYNow + BallDirectionY;
      BallPositionXNext = BallPositionXNow + BallDirectionX;
    }
    
    // Test Again: Is ball colliding with wall and needs to bounce?
    /*
    if ( GameField [BallPositionYNext][BallPositionXNext] == GPT_WALL) {
      // Test: Collision with top wall?
      if (BallPositionYNext == 0) { BallDirectionY = 1; }
      // Test: Collision with bottom wall?
      if (BallPositionYNext == 9) { BallDirectionY = -1; }
      BallPositionYNext = BallPositionYNow + BallDirectionY;
      BallPositionXNext = BallPositionXNow + BallDirectionX;
    }
    */

    // Clear current ball position
    GameField [BallPositionYNow][BallPositionXNow] = GPT_OPEN;
    // Update and plot new ball position
    // Test: Is ball out of game field? Don't let that happen and don't over-write boundaries.
    if (BallPositionYNext==0) {BallPositionYNext = 1;}
    if (BallPositionYNext >8) {BallPositionYNext = 8;}
    if (BallPositionXNext==0) {BallPositionYNext = 1;}
    if (BallPositionXNext >8) {BallPositionXNext = 8;}
    GameField [BallPositionYNext][BallPositionXNext] = GPT_BALL;
    BallPositionYNow = BallPositionYNext ;
    BallPositionXNow = BallPositionXNext ;
    BallUpdateLastRunTime = nowTime;
  }
}

void GameStartMap() {
  GameViewOffsetX = 1; // 1 = normal view of playing field
  GameViewOffsetY = 1; // 1 = normal view of playing field
  // Random start Y position for paddles, against left/right walls
  Paddle1CenterYNow = (uint8_t) random (2,8); // 2;
  Paddle1CenterXNow = 1;
  Paddle2CenterYNow = (uint8_t) random (2,8); // 7;
  Paddle2CenterXNow = 8;
  BallPositionYNow  = (uint8_t) random (2,7); // 2;
  BallPositionXNow  = (uint8_t) random (4,5); // 4;
  BallDirectionY    = (uint8_t) random (-1,1); // 1;
  BallDirectionX    = (uint8_t) random (-1,1); // 1;
  BallVelocity      = 300; // ms between movements, lower number is faster movement, down to the game refresh rate.              
  GameSetUpBounceWalls();
  GameSetUpEndZones();
  GameSetUpClearPlayingField();
  // Render the first paddle and ball positions to the screen.
  Paddle1Update();
  Paddle2Update();
  BallUpdate();
}

bool GameLogic() { // Returns 1 if activity of stylus movement is detected. Else 0 for no activity.
  GameMovementDetected = false;
  Core_Mem_Scan_For_Magnet();                   // Update the the CoreArrayMemory to see where magnets are.
  if (StylusFind()) {                           // Determine stylus positions for player 1 and 2
    GameMovementDetected = true;
    Paddle1Update();                            // Render paddle 1 update 
    Paddle2Update();                            // Render paddle 2 update
    }  
  BallUpdate();                                 // Render ball update
  return GameMovementDetected;
}

void ConvertGameFieldToScreenMemory() {
  uint8_t ScreenX = 0;
  uint8_t ScreenY = 0;
  uint8_t Color   = 0;
  if(DebugLevel==4) {
    Serial.println();
    Serial.println("ConvertGameFieldToScreenMemory() and the Game Field Colors are:");
  }
  if (GameViewOffsetX > 2) { GameViewOffsetX = 2;}
  if (GameViewOffsetY > 2) { GameViewOffsetY = 2;}
  for (uint8_t y=GameViewOffsetY; y<=(GameViewOffsetY+7); y++) {    // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
    for (uint8_t x=GameViewOffsetX; x<=(GameViewOffsetX+7); x++) {
      if (GameField[y][x] == GPT_OPEN    ) { Color = GPC_OPEN    ; }
      if (GameField[y][x] == GPT_BALL    ) { Color = GPC_BALL    ; }
      if (GameField[y][x] == GPT_WALL    ) { Color = GPC_WALL    ; }
      if (GameField[y][x] == GPT_END_ZONE) { Color = GPC_END_ZONE; }
      if (GameField[y][x] == GPT_P1L_TOP ) { Color = GPC_P1L_TOP ; }
      if (GameField[y][x] == GPT_P1L_MID ) { Color = GPC_P1L_MID ; }
      if (GameField[y][x] == GPT_P1L_BOT ) { Color = GPC_P1L_BOT ; }
      if (GameField[y][x] == GPT_P2R_TOP ) { Color = GPC_P2R_TOP ; }
      if (GameField[y][x] == GPT_P2R_MID ) { Color = GPC_P2R_MID ; }
      if (GameField[y][x] == GPT_P2R_BOT ) { Color = GPC_P2R_BOT ; }
      LED_Array_Matrix_Color_Write(ScreenY, ScreenX, Color);
      if(DebugLevel==4) {
        Serial.print(Color);
        Serial.print(" ,");
      }
      ScreenX++;
    }
    ScreenX = 0;
    if(DebugLevel==4) {
      Serial.println();
    }
    ScreenY++;
  }
}

void GameSerialPrintMap() {
  Serial.println();
  Serial.println("Game Field Pixel Type:");
  for (uint8_t y=0; y<=9; y++) {    // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
    for (uint8_t x=0; x<=9; x++) {
      Serial.print(GameField[y][x]);
      Serial.print(" ,");
    }
    Serial.println();
  }
}


void GamePlayPong() {
  if (TopLevelModeChangedGet()) {                           // First time entry into this mode.
    Serial.println();
    Serial.println("   Pong Game Sub-Menu");
    Serial.println("    M = Main DGAUSS Menu");
    Serial.println("    + = Next Game");
    Serial.println("    - = Previous Game");
    Serial.println("    S = Select / Start Game Again");
    Serial.print(PROMPT);
    TopLevelThreeSoftButtonGlobalEnableSet(true); // Make sure MENU + and - soft buttons are enabled.
    TopLevelSetSoftButtonGlobalEnableSet(false);  // Disable the S button as SET, so it can be used in this game as Select.
    WriteGamePongSymbol(0);
    MenuTimeOutCheckReset();
    LED_Array_Matrix_Color_Display();
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
    switch(GameState)
    {
      case GAME_STATE_INTRO_SCREEN: // 0 Game Intro/Splash Screen
        // Check for touch of "S" to start the game
        if (ButtonState(4,0) >= 100) { MenuTimeOutCheckReset(); GameState = GAME_STATE_SET_UP; }
        // Timeout to next game option in the sequence
        if (MenuTimeOutCheck(5000))  { TopLevelModeSetInc(); }
        break;

      case GAME_STATE_SET_UP: // 1 Setup a new game
        GameOver = false;
        Winner = 0;
        GameStartMap();
        ConvertGameFieldToScreenMemory();
        GameState = GAME_STATE_PLAY;
        break;

      case GAME_STATE_PLAY: // 2 Play       
        if (GameLogic()) { MenuTimeOutCheckReset(); }
        if (MenuTimeOutCheck(60000)) { TopLevelModeSetToDefault(); } // Is it time to timeout? Then timeout.
        if (Winner) { GameOverTimer = nowTime; GameState = 3; }
        ConvertGameFieldToScreenMemory();
        // Check for touch of "S" to start the game over.
        if (ButtonState(4,0) >= 200) { MenuTimeOutCheckReset(); GameState = GAME_STATE_SET_UP; }
        break;

      case 3: // Winner
        if (Winner == 1) { WriteGamePongSymbol(1); }
        if (Winner == 2) { WriteGamePongSymbol(2); }
        if ((nowTime - GameOverTimer) > GameOverTimerAutoReset) {
          MenuTimeOutCheckReset();
          GameState = GAME_STATE_SET_UP;
        }
        break;

    /*
      case 4: // Winner = Green Screen
        for (uint8_t x=0; x<=7; x++) {
          for (uint8_t y=0; y<=7; y++) {
            SnakeGameMemory[y][x] = -2;
          }
        }
        if ((nowTime - GameOverTimer) > GameOverTimerAutoReset) {
          MenuTimeOutCheckReset();
          GameState = 1;
        }
        // WriteColorFontSymbolToLedScreenMemoryMatrixColor(13);
        WriteGameSnakeSymbol(1);
        break;
    */

      default:
        break;
    }
    GameSerialPrintMap(); // For debugging
    LED_Array_Matrix_Color_Display();
    GameUpdateLastRunTime = nowTime;
  }

} // Pong
