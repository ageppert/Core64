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

#include "DEMOS/Demos_Sub_Menu.h"
#include "DEMOS/Demo_Modes.h"
#include "GAMES/Games_Sub_Menu.h"
#include "GAMES/Game_Snake.h"
#include "GAMES/Game_Pong.h"
#include "APPS/Apps_Sub_Menu.h"
#include "APPS/Drawing.h"
#include "UTILITIES/Utilities_Sub_Menu.h"
#include "SPECIAL/Special_Sub_Menu.h"
#include "SETTINGS/Settings_Sub_Menu.h"

#define DebugDelayBetweenStartUpStates 250

// All of these items in this array list must be updated in the corresponding "enum TopLevelMode" of Mode_Manager.h to match 1:1.
// This array is use for convenience, to allow printing the mode to the serial port or screen.
  const char* TOP_LEVEL_MODE_NAME_ARRAY[] =
  {
      " MODE_START_POWER_ON",
      "  MODE_START_ALIVE",
      "  MODE_START_EEPROM",
      "  MODE_START_POWER_CHECK",
      "  MODE_START_CONFIG_SPECIFIC",
      "  MODE_START_SEQUENCE_COMPLETE",
      " MODE_DGAUSS_MENU                     ",
      "  MODE_DEMO_SUB_MENU                  ",
      "   MODE_DEMO_SCROLLING_TEXT           ",
      "   MODE_DEMO_LED_TEST_ONE_MATRIX_MONO ",
      "   MODE_DEMO_LED_TEST_ONE_MATRIX_COLOR",
      "   MODE_DEMO_END_OF_LIST              ",
      "  MODE_GAME_SUB_MENU                  ",
      "   MODE_GAME_SNAKE                    ",
      "   MODE_GAME_PONG                     ",
      "   MODE_GAME_END_OF_LIST              ",
      "  MODE_APP_SUB_MENU                   ",
      "   MODE_APP_DRAW                      ",
      "   MODE_APP_END_OF_LIST               ",
      "  MODE_UTIL_SUB_MENU                  ",
      "   MODE_UTIL_FLUX_DETECTOR            ",
      "   MODE_UTIL_END_OF_LIST              ",
      "  MODE_SPECIAL_SUB_MENU               ",
      "   MODE_LED_TEST_ALL_BINARY           ",
      "   MODE_LED_TEST_ONE_STRING           ",
      "   MODE_TEST_EEPROM                   ",
      "   MODE_LED_TEST_ALL_COLOR            ",
      "   MODE_CORE_TOGGLE_BIT               ",
      "   MODE_CORE_TEST_ONE                 ",
      "   MODE_CORE_TEST_MANY                ",
      "   MODE_HALL_TEST                     ",
      "   MODE_GPIO_TEST                     ",
      "   MODE_SPECIAL_LOOPBACK_TEST         ",
      "   MODE_SPECIAL_HARD_REBOOT           ",
      "   MODE_SPECIAL_END_OF_LIST           ",
      "  MODE_SETTINGS_SUB_MENU              ",
      "   MODE_SETTINGS_END_OF_LIST          ",
      " MODE_MANUFACTURING_MENU              ",
      "  MODE_MANUFACTURING_EEPROM_FACTORY_WRITE",
      "  MODE_MANUFACTURING_END_OF_LIST      ",
      " MODE_LAST                            " 
  };
  // Make sure the Enum and Array are the same size.
  // https://stackoverflow.com/questions/34669164/ensuring-array-is-filled-to-size-at-compile-time
  _Static_assert(sizeof(TOP_LEVEL_MODE_NAME_ARRAY)/sizeof(*TOP_LEVEL_MODE_NAME_ARRAY) == (MODE_LAST+1), "Top_Level_Mode_Name_Array item missing or extra, does not match TopLevelMode Enum!");

static uint16_t TopLevelMode = MODE_START_POWER_ON;
static uint16_t TopLevelModeDefault = MODE_DEMO_SCROLLING_TEXT;
static uint16_t TopLevelModePrevious;
static bool     TopLevelModeChanged = false;
static bool     TopLevelThreeSoftButtonGlobalEnable = true;
static bool     TopLevelSetSoftButtonGlobalEnable = true;

static uint32_t PowerOnTimems = 0;
static uint32_t PowerOnSequenceMinimumDurationms = 3500;

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

static uint32_t MenuTimeoutTimerms  = 0;
static uint32_t MenuTimeoutDeltams  = 0;
static bool MenuTimeoutFirstTimeRun = 0;

// Every function which sets a new top level mode shall trigger an update through the serial port.
void TopLevelModeSetToDefault               ()  { TopLevelMode = TopLevelModeDefault; TopLevelModeSetChanged(false); TopLevelModeSetChanged(true); }
void TopLevelModeSet          (uint16_t value)  { TopLevelMode = value; TopLevelModeSetChanged(false); TopLevelModeSetChanged(true); }
void TopLevelModeSetInc                     ()  { TopLevelMode++; TopLevelModeSetChanged(false); TopLevelModeSetChanged(true); }
void TopLevelModeSetDec                     ()  { TopLevelMode--; TopLevelModeSetChanged(false); TopLevelModeSetChanged(true); }               
// These functions do not change modes so they don't need the serial port update.
void TopLevelModePreviousSet  (uint16_t value)  { TopLevelModePrevious = value; }
void TopLevelModeDefaultSet   (uint16_t value)  { TopLevelModeDefault = value; }
uint16_t TopLevelModePreviousGet            ()  { return (TopLevelModePrevious); }
uint16_t TopLevelModeDefaultGet             ()  { return (TopLevelModeDefault); }
uint16_t TopLevelModeGet                    ()  { return (TopLevelMode); }

void TopLevelModeChangeSerialPortDisplay () {
  Serial.println();
  Serial.print("  TopLevelMode changed from ");
  Serial.print(TopLevelModePreviousGet());
  Serial.print(TOP_LEVEL_MODE_NAME_ARRAY[TopLevelModePreviousGet()]);
  Serial.print(" to ");
  Serial.print(TopLevelModeGet());
  Serial.print(" ");
  Serial.print(TOP_LEVEL_MODE_NAME_ARRAY[TopLevelModeGet()]);
  Serial.println(".");
}

void TopLevelModeSetChanged (bool value) {          // Flag that a mode change has occurred.  User application has one time to use this before it is reset.
  if ( (TopLevelModeChanged == false) && (value == true) ) {
    TopLevelModeChangeSerialPortDisplay();            // Display the change in the serial port.
    TopLevelThreeSoftButtonGlobalEnableSet (true);    // Ensures this flag set to enabled after a mode change, so modes that need it disabled will need to force it to be disabled. 
    CommandLineEnableSet(true);                       // Ensures the CommandLine is activated on mode change.
  }
  TopLevelModeChanged = value;
}

bool TopLevelModeChangedGet () {  
  if (TopLevelModeChanged == true) {
    TopLevelModeSetChanged (false); 
    return (true);
    }
    else {
      return (false);
    }
}

void TopLevelThreeSoftButtonGlobalEnableSet (bool value) {
  TopLevelThreeSoftButtonGlobalEnable = value;
  TopLevelSetSoftButtonGlobalEnable = value;
}
bool TopLevelThreeSoftButtonGlobalEnableGet ()           {   return (TopLevelThreeSoftButtonGlobalEnable); }

void TopLevelSetSoftButtonGlobalEnableSet (bool value) {   TopLevelSetSoftButtonGlobalEnable = value; }
bool TopLevelSetSoftButtonGlobalEnableGet ()           {   return (TopLevelSetSoftButtonGlobalEnable); }

void TopLevelModeManagerCheckButtons () {
  // GLOBAL MODE SWITCHING - Check for soft button touch activation.
  // If button is pressed, it must be released and pressed again for subsequent action.
  // M is MENU. Press to enter, and exit the DGAUSS Top Level Menu. The M is always active, in contrast to the +,-,S buttons which can be disabled.
  // OPTIONAL BUTTON USE
  // These three may be disabled within a mode for special use in that mode. Otherwise, they can function globally as:
  // + goes to the next mode in a sub-menu sequence.
  // - goes to the next mode in a sub-menu sequence.
  // S jumps to Settings Sub-Menu. 

  if (StreamTopLevelModeEnable) {
    Serial.print(TopLevelModeGet());
    Serial.print(" ");
    Serial.print(TOP_LEVEL_MODE_NAME_ARRAY[TopLevelModeGet()]);
  }
    Button1HoldTime = ButtonState(1,0);
    Button2HoldTime = ButtonState(2,0);
    Button3HoldTime = ButtonState(3,0);
    Button4HoldTime = ButtonState(4,0);

  // Checking the "M" soft button to enter or exit the DGAUSS menu to the previous mode before entering.
  if ( (Button1Released == true) && (Button1HoldTime >= 100) ) {
    ButtonState(1,1); // Force a "release" after press by clearing the button hold down timer
    Button1Released = false;
    if (TopLevelModeGet()!=MODE_DGAUSS_MENU) {                // Enter DGAUSS menu
      TopLevelModePreviousSet(TopLevelModeGet());
      TopLevelModeSet(MODE_DGAUSS_MENU);
    }
    else {                                                    // Exit DGAUSS menu
      if (TopLevelModePreviousGet() == MODE_DGAUSS_MENU) {    // In this case, if previous was already the DGAUSS menu, 
        TopLevelModeSet(TopLevelModeDefault);                 // just go to default demo mode. 
      }
      else {
        uint32_t temporary = TopLevelModePreviousGet();       // Otherwise, go the previous mode, after storing it temporarily.
        TopLevelModePreviousSet(MODE_DGAUSS_MENU);            // Set the previous mode to DGAUSS menu.
        TopLevelModeSet(temporary);                           // Now go to the previous mode.
      }
    }
  }
  else {
    if (Button1HoldTime == 0) {
      Button1Released = true;
    }
  }

  if (TopLevelThreeSoftButtonGlobalEnableGet()) {
    // Checking the "-" soft button.
    if ( (Button2Released == true) && (Button2HoldTime >= 100) ) {
      ButtonState(2,1); // Force a "release" after press by clearing the button hold down timer
      Button2Released = false;
      TopLevelModePrevious = TopLevelMode;
      TopLevelModeSetDec();
    }
    else {
      if (Button2HoldTime == 0) {
        Button2Released = true;
      }
    }

    // Checking the "+" soft button.
    if ( (Button3Released == true) && (Button3HoldTime >= 100) ) {
      ButtonState(3,1); // Force a "release" after press by clearing the button hold down timer
      Button3Released = false;
      TopLevelModePrevious = TopLevelMode;
      TopLevelModeSetInc();
    }
    else {
      if (Button3HoldTime == 0) {
        Button3Released = true;
      }
    }
  }

  if (TopLevelSetSoftButtonGlobalEnableGet()) {
    // Checking the "S" soft button.
    if ( (Button4Released == true) && (Button4HoldTime >= 100) ) {
      ButtonState(4,1); // Force a "release" after press by clearing the button hold down timer
      Button4Released = false;
      TopLevelModePrevious = TopLevelMode;
      TopLevelModePreviousSet(TopLevelModeGet());
      TopLevelModeSet(MODE_SETTINGS_SUB_MENU);
    }
    else {
      if (Button4HoldTime == 0) {
        Button4Released = true;
      }
    }
  }
}

void MenuTimeOutCheckReset () {
  MenuTimeoutDeltams = 0;
  MenuTimeoutFirstTimeRun = true;
}

bool MenuTimeOutCheck (uint32_t MenuTimeoutLimitms) {
  static uint32_t NowTimems;
  static uint32_t StartTimems;
  NowTimems = millis();
  if(MenuTimeoutFirstTimeRun) { StartTimems = NowTimems; MenuTimeoutFirstTimeRun = false;}
  MenuTimeoutDeltams = NowTimems-StartTimems;
  if (MenuTimeoutDeltams >= MenuTimeoutLimitms) {
    MenuTimeOutCheckReset();
    Serial.println();
    Serial.println("  Menu or mode timeout.");
    return (true);
  }
  else {
    return (false);
  }
}

void TopLevelModeManagerRun () {
  switch(TopLevelMode) {
    // *************************************************************************************************************************************************** //
    case MODE_START_POWER_ON:                    // Perform the bare minimum required to start and support serial status and debugging.
    // *************************************************************************************************************************************************** //
      PowerOnTimems = millis();
      Debug_Pins_Setup();
      SerialPortSetup();
      TopLevelModePreviousSet (TopLevelModeGet());
      Serial.print("  TopLevelMode is: ");
      Serial.print(TopLevelModeGet());
      Serial.print(" ");
      Serial.print(TOP_LEVEL_MODE_NAME_ARRAY[TopLevelModeGet()]);
      Serial.println(".");
      Serial.println("  Power-on sequence has begun.");
      CommandLineSetup();
      #if defined BOARD_CORE64_TEENSY_32
        #ifdef NEON_PIXEL_ARRAY
          // Don't perform a heart beat because it will mess up the Neon Pixels since the Heart Beat LED is shared with the SPI CLK pin.
        #else
          HeartBeatSetup();
        #endif
      #elif defined BOARD_CORE64C_RASPI_PICO
        HeartBeatSetup();
      #endif
      Serial.println("  Heartbeat started.");
      delay(DebugDelayBetweenStartUpStates);
      TopLevelModeSetInc();
      break;

    // *************************************************************************************************************************************************** //
    case MODE_START_ALIVE:                       // Show signs of life as soon as possible after power on, assuming default hardware.
    // *************************************************************************************************************************************************** //
      handleSplash("");                                       // Splash screen serial text
      handleThanks("");                                       // Thank yous!
      Serial.println("  Starting default expected hardware:");
      I2CManagerSetup();                                      // Required to scan the I2C Bus.
      I2CManagerBusScan();                                    // Determine hardware available on the I2C bus. Especially the hall sensor buttons.
      Serial.println("    Starting LED Matrix.");
      LED_Array_Init();                                       // Assuming this hardware is available... it's a blind output only.
      LED_Array_Start_Up_Symbol_Loop_Begin();                 // Begin the start-up symbol sequence, manually called in each subsequent step of the start-up sequence.
      Serial.println("    Starting OLED Display.");
      OLEDScreenSetup();
      TopLevelModePreviousSet (TopLevelModeGet());
      TopLevelModeSetInc();
      delay(DebugDelayBetweenStartUpStates);
      break;

    // *************************************************************************************************************************************************** //
    case MODE_START_EEPROM:                     // Check the EEPROM for Hardware Version and expected peripherals/configuration/settings.
    // *************************************************************************************************************************************************** //
      LED_Array_Start_Up_Symbol_Loop_Continue();
      OLEDTopLevelModeSet(TopLevelModeGet());
      OLEDScreenUpdate();
      Serial.println("  EEPROM read has begun...");
      if (ReadHardwareVersion()) {Serial.println("  EEPROM Present.");}
      else {Serial.println("  EEPROM not found. Default firmware values used.");}
      Serial.println("  ...completed EEPROM read.");
      Serial.println("  Starting Hall Sensor Buttons");
      Buttons_Setup(); // This breaks Pico MBED with error code.
      TopLevelModePreviousSet (TopLevelModeGet());
      TopLevelModeSetInc();
      delay(DebugDelayBetweenStartUpStates);
      break;

    // *************************************************************************************************************************************************** //
    case MODE_START_POWER_CHECK:                     // Check voltages and display along with version/config info
    // *************************************************************************************************************************************************** //
      LED_Array_Start_Up_Symbol_Loop_Continue();
      OLEDTopLevelModeSet(TopLevelModeGet());
      OLEDScreenUpdate();
      Serial.println("  Power check has begun...");
      AnalogSetup();
      AnalogUpdate();
      Serial.print("  Switched Voltage: ");
      Serial.println(GetBatteryVoltageV(),2);
      Serial.println("  ...completed power check.");
      handleInfo("");               // Print some info about the system (this also checks hardware version, born-on, and serial number)
      TopLevelModePreviousSet (TopLevelModeGet());
      TopLevelModeSetInc();
      delay(DebugDelayBetweenStartUpStates);
      break;

    // *************************************************************************************************************************************************** //
    case MODE_START_CONFIG_SPECIFIC:                     // Enable and adjust based on EEPROM configuration parameters
    // *************************************************************************************************************************************************** //
      LED_Array_Start_Up_Symbol_Loop_Continue();                // Continue the start-up symbol sequence.
      OLEDTopLevelModeSet(TopLevelModeGet());
      Serial.println("  Configuration specific setup has begun...");
      // TODO brightness default set from EEPROM
      CoreSetup();
      OLEDScreenUpdate();
      SDInfo();
      #if defined BOARD_CORE64_TEENSY_32
        AmbientLightSetup();
        Neon_Pixel_Array_Init();
        SDCardSetup();
      #elif defined BOARD_CORE64C_RASPI_PICO
        // TODO: Handle the difference in the hardware inside the functions above and remove this #if sequence
      #endif
      Serial.println("  ...Completed configuration specific setup.");
      TopLevelModePreviousSet (TopLevelModeGet());
      TopLevelModeSetInc();
      delay(DebugDelayBetweenStartUpStates);
      break;

    // *************************************************************************************************************************************************** //
    case MODE_START_SEQUENCE_COMPLETE:                  // Print the HELP menu, PROMPT, and jump into default demo mode
    // *************************************************************************************************************************************************** //
      LED_Array_Start_Up_Symbol_Loop_Continue();                // Continue the start-up symbol sequence.
      OLEDTopLevelModeSet(TopLevelModeGet());
      OLEDScreenUpdate();
      if (TopLevelModeChangedGet()){
        Serial.println("  ...start-up sequence has been completed.");
        handleHelp("");               // Print the help menu
        I2CManagerBusScan(); // Temporary to make testing core boards faster
        Serial.print(PROMPT);
      }
      if (millis() >= (PowerOnTimems + PowerOnSequenceMinimumDurationms)) {
        TopLevelModePreviousSet (TopLevelModeGet());
        TopLevelModeSet(TopLevelModeDefault);
      delay(DebugDelayBetweenStartUpStates);
      }
      break;

// *************************************************************************************************************************************************** //
// ***************************************************** DGAUSS MENU ********************************************************************************* //
// *************************************************************************************************************************************************** //
    case MODE_DGAUSS_MENU:
      if (TopLevelModeChangedGet()){
        MenuTimeOutCheckReset();
        Serial.println();
        Serial.println("  Entered DGAUSS MENU on Core Memory Array / LED Matrix");
        Serial.println("    *** SUB_MENUS acccessible on magnetic core touch screen only.");
        Serial.println("    d = Demos");
        Serial.println("    G = Games");
        Serial.println("    A = Apps");
        Serial.println("    u = Utils");
        Serial.println("    s = Special");
        Serial.println("    s = Settings");
        Serial.println("    To access modes directly from serial command line, type 'mode' and press RETURN.");
        WriteColorFontSymbolToLedScreenMemoryMatrixColor(0);
        LED_Array_Matrix_Color_Display();
        #if defined BOARD_CORE64_TEENSY_32
          #ifdef NEON_PIXEL_ARRAY
            CopyColorFontSymbolToNeonPixelArrayMemory(0);
            Neon_Pixel_Array_Matrix_Mono_Display();
          #endif
        #elif defined BOARD_CORE64C_RASPI_PICO
          // Nothing here
        #endif
        Serial.print(PROMPT);
      }
      if (MenuTimeOutCheck(30000)) { TopLevelModeSetToDefault(); }
  
      Core_Mem_Scan_For_Magnet();
      // Was the D touched with magnetic stylus?
      for (uint8_t y=0; y<5; y++) {
        for (uint8_t x=0; x<2; x++) {
          if ( (CoreArrayMemory [y][x]) || (CoreArrayMemory [7][5]) ) { 
            Serial.println();
            Serial.println("  Demo sub-menu selected.");   
            TopLevelModePreviousSet (TopLevelModeGet());
            TopLevelModeSet (MODE_DEMO_SUB_MENU);
          }
        }
      }      
      // Was G touched with magnetic stylus?
      for (uint8_t y=0; y<4; y++) {
        for (uint8_t x=2; x<5; x++) {
          if (CoreArrayMemory [y][x]) { 
            Serial.println();
            Serial.println("  Game sub-menu selected.");   
            TopLevelModePreviousSet (TopLevelModeGet());
            TopLevelModeSet (MODE_GAME_SUB_MENU);
          }
        }
      }
      // Was A touched with magnetic stylus?
      for (uint8_t y=0; y<4; y++) {
        for (uint8_t x=5; x<8; x++) {
          if (CoreArrayMemory [y][x]) { 
            Serial.println();
            Serial.println("  Application sub-menu selected.");   
            TopLevelModePreviousSet (TopLevelModeGet());
            TopLevelModeSet (MODE_APP_SUB_MENU);
          }
        }
      }
      // Was U touched with magnetic stylus?
      for (uint8_t y=5; y<8; y++) {
        for (uint8_t x=0; x<4; x++) {
          if (CoreArrayMemory [y][x]) { 
            Serial.println();
            Serial.println("  Utilities sub-menu selected.");   
            TopLevelModePreviousSet (TopLevelModeGet());
            TopLevelModeSet (MODE_UTIL_SUB_MENU);
          }
        }
      }
      // Was S(pecial) touched with magnetic stylus?
      for (uint8_t y=5; y<8; y++) {
        for (uint8_t x=3; x<5; x++) {
          if ( (CoreArrayMemory [y][x]) || (CoreArrayMemory [5][5]) ) { 
            Serial.println();
            Serial.println("  Special sub-menu selected.");   
            TopLevelModePreviousSet (TopLevelModeGet());
            TopLevelModeSet (MODE_SPECIAL_SUB_MENU);
            TopLevelModeSetChanged(true);
          }
        }
      }
      // Was S(settings) touched with magnetic stylus?
      for (uint8_t y=5; y<8; y++) {
        for (uint8_t x=6; x<8; x++) {
          if ( (CoreArrayMemory [y][x]) || (CoreArrayMemory [7][5]) ) { 
            Serial.println();
            Serial.println("  Settings sub-menu selected.");   
            TopLevelModePreviousSet (TopLevelModeGet());
            TopLevelModeSet (MODE_SETTINGS_SUB_MENU);
          }
        }
      }

      OLEDTopLevelModeSet(TopLevelModeGet());
      OLEDScreenUpdate();
      break;

// *************************************************************************************************************************************************** //
// ***************************************************** DEMO **************************************************************************************** //
// *************************************************************************************************************************************************** //
    case MODE_DEMO_SUB_MENU:                    DemosSubMenu();                           break;
    case MODE_DEMO_SCROLLING_TEXT:              DemoScrollingText();                      break;
    case MODE_DEMO_LED_TEST_ONE_MATRIX_MONO:    DemoLedTestOneMatrixMono();               break;
    case MODE_DEMO_LED_TEST_ONE_MATRIX_COLOR:   DemoLedTestOneMatrixColor();              break;
    case MODE_DEMO_END_OF_LIST:                 DemoEndofList();                          break;

// *************************************************************************************************************************************************** //
// ***************************************************** GAME **************************************************************************************** //
// *************************************************************************************************************************************************** //
    case MODE_GAME_SUB_MENU:                    GamesSubMenu();                           break;
    case MODE_GAME_SNAKE:                       GameSnake();                              break;
    case MODE_GAME_PONG:                        GamePlayPong();                           break;
    case MODE_GAME_END_OF_LIST:                 TopLevelModeSet(MODE_GAME_SUB_MENU);      break;

// *************************************************************************************************************************************************** //
// ***************************************************** APP ***************************************************************************************** //
// *************************************************************************************************************************************************** //
    case MODE_APP_SUB_MENU:                     AppsSubMenu();                            break;
    case MODE_APP_DRAW:                         Draw();                                   break;
    case MODE_APP_END_OF_LIST:                  TopLevelModeSet(MODE_APP_SUB_MENU);       break;

// *************************************************************************************************************************************************** //
// ***************************************************** UTIL **************************************************************************************** //
// *************************************************************************************************************************************************** //
    case MODE_UTIL_SUB_MENU:  UtilitiesSubMenu();  break;

      case MODE_UTIL_FLUX_DETECTOR:                         // Read 64 cores 10ms (110us 3x core write, with 40us delay 64 times), update LEDs 2ms
        LED_Array_Monochrome_Set_Color(50,255,255);
        LED_Array_Memory_Clear();
//        #if defined BOARD_CORE64_TEENSY_32
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
//        #elif defined BOARD_CORE64C_RASPI_PICO
//          // TODO: Port Core HAL and Driver to handle Teensy and Pico.
//        #endif
        OLEDTopLevelModeSet(TopLevelModeGet());
        OLEDScreenUpdate();
        break;

      case MODE_UTIL_END_OF_LIST: TopLevelModeSet(MODE_UTIL_SUB_MENU);    break;

// *************************************************************************************************************************************************** //
// ***************************************************** SPECIAL ************************************************************************************* //
// *************************************************************************************************************************************************** //
    case MODE_SPECIAL_SUB_MENU:   SpecialSubMenu();      break;

      case MODE_LED_TEST_ALL_BINARY: // Counts from lower right and left/up in binary, using Binary LUT.
        TopLevelThreeSoftButtonGlobalEnableSet (true);
        LED_Array_Test_Count_Binary();
        OLEDTopLevelModeSet(TopLevelModeGet());
        OLEDScreenUpdate();
        break;

      case MODE_LED_TEST_ONE_STRING: // Illuminates one pixel, sequentially from left to right, top to bottom using 1D string addressing.
        TopLevelThreeSoftButtonGlobalEnableSet (true);
        LED_Array_Test_Pixel_String();
        OLEDTopLevelModeSet(TopLevelModeGet());
        OLEDScreenUpdate();
        break;

      case MODE_TEST_EEPROM: // 
        // value = EEPROM_Hardware_Version_Read(a);  // Teensy internal emulated EEPROM
        TopLevelThreeSoftButtonGlobalEnableSet (true);
        Serial.println();
        EepromByteValue = EEPROMExtDefaultReadByte(Eeprom_Byte_Mem_Address);
        Serial.print(Eeprom_Byte_Mem_Address);
        Serial.print("\t");
        Serial.print(EepromByteValue);
        Eeprom_Byte_Mem_Address = Eeprom_Byte_Mem_Address + 1;
        if (Eeprom_Byte_Mem_Address == 128) {
          Eeprom_Byte_Mem_Address = 0;
        }
        // delay(10);
        break;
        
      case MODE_LED_TEST_ALL_COLOR: // FastLED Demo of all color
        TopLevelThreeSoftButtonGlobalEnableSet (true);
        LED_Array_Test_Rainbow_Demo();
        OLEDTopLevelModeSet(TopLevelModeGet());
        OLEDScreenUpdate();
        break;
        
      case MODE_CORE_TOGGLE_BIT:     // Just toggle a single bit on and off. Or just pulse on.
        TopLevelThreeSoftButtonGlobalEnableSet (true);
        coreToTest=0;
        LED_Array_Monochrome_Set_Color(50,255,255);
        #if defined BOARD_CORE64_TEENSY_32        
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
        #elif defined BOARD_CORE64C_RASPI_PICO
          // TODO: Port Core HAL and Driver to handle Teensy and Pico.
              // TODO: Remove this test once shift registers are working. To test if the Shift Registers are working, toggle the matrix drive transistors, with matrix enable OFF.
              Core_Mem_All_Drive_IO_Toggle();

              // ClearRowZeroAndColZero ();
              Core_Mem_Bit_Write(0,0);
              LED_Array_String_Write(0,0);
              LED_Array_String_Display();
              delay(5);
              // SetRowZeroAndColZero ();
              Core_Mem_Bit_Write(0,1);
              LED_Array_String_Write(0,1);
              LED_Array_String_Display();
        #endif
        OLEDTopLevelModeSet(TopLevelModeGet());
        OLEDScreenUpdate();
        delay(100);  // This delay makes it easier to trigger on the first debug pulse consistently.
        break;

      case MODE_CORE_TEST_ONE:
        TopLevelThreeSoftButtonGlobalEnableSet (true);
        coreToTest=0;
        LED_Array_Monochrome_Set_Color(100,255,255);
        LED_Array_Memory_Clear();
        #if defined BOARD_CORE64_TEENSY_32     
          //LED_Array_String_Write(coreToTest,1);               // Default to pixel on
          //  TracingPulses(1);
          // Core_Mem_Bit_Write(coreToTest,0);                     // default to bit set
          Core_Mem_Bit_Write(coreToTest,1);                     // default to bit set
          //  TracingPulses(2);
          if (Core_Mem_Bit_Read(coreToTest)==true) {LED_Array_String_Write(coreToTest, 1);}
          else { LED_Array_String_Write(coreToTest, 0); }
          //  TracingPulses(1);
          // delay(10);
        #elif defined BOARD_CORE64C_RASPI_PICO
          // TracingPulses_Debug_Pin_1(1);
          Core_Mem_Bit_Write(coreToTest,1);                     // default to bit set
          // TracingPulses_Debug_Pin_1(2);
          if (Core_Mem_Bit_Read(coreToTest)==true) {LED_Array_String_Write(coreToTest, 1);}
          else { LED_Array_String_Write(coreToTest, 0); }
          // TracingPulses_Debug_Pin_1(3);
        #endif
        LED_Array_String_Display();
        OLEDTopLevelModeSet(TopLevelModeGet());
        OLEDScreenUpdate();
        delay(100);  // This delay makes it easier to trigger on the first debug pulse consistently.
        break;

      case MODE_CORE_TEST_MANY:
        TopLevelThreeSoftButtonGlobalEnableSet (true);
        coreToTest=0;
        #if defined BOARD_CORE64_TEENSY_32     
          if (TopLevelModeChangedGet()) {LED_Array_Memory_Clear();}
          for (uint8_t bit = coreToTest; bit<(64); bit++)
            {
            LED_Array_Monochrome_Set_Color(100,255,255);
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
        #elif defined BOARD_CORE64C_RASPI_PICO
          if (TopLevelModeChangedGet()) {LED_Array_Memory_Clear();}
          for (uint8_t bit = coreToTest; bit<(64); bit++)
            {
            LED_Array_Monochrome_Set_Color(100,255,255);
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
        #endif
        OLEDTopLevelModeSet(TopLevelModeGet());
        OLEDScreenUpdate();
        break;

    case MODE_HALL_TEST:
      TopLevelThreeSoftButtonGlobalEnableSet (false);
      LED_Array_Monochrome_Set_Color(25,255,255);
      LED_Array_Memory_Clear();
      if(Button1HoldTime) { LED_Array_String_Write(56,1); Serial.println(Button1HoldTime); }
      if(Button2HoldTime) { LED_Array_String_Write(58,1); Serial.println(Button2HoldTime); }
      if(Button3HoldTime) { LED_Array_String_Write(60,1); Serial.println(Button3HoldTime); }
      if(Button4HoldTime) { LED_Array_String_Write(62,1); Serial.println(Button4HoldTime); }
      LED_Array_String_Display();
      OLEDTopLevelModeSet(TopLevelModeGet());
      OLEDScreenUpdate();
      break;

    case MODE_GPIO_TEST:
      TopLevelThreeSoftButtonGlobalEnableSet (true);
      LED_Array_Monochrome_Set_Color(25,255,255);
      LED_Array_Memory_Clear();
      LED_Array_String_Display();
      OLEDTopLevelModeSet(TopLevelModeGet());
      OLEDScreenUpdate();
      DebugAllGpioToggleTest();
      break;

    case MODE_SPECIAL_LOOPBACK_TEST:
      TopLevelThreeSoftButtonGlobalEnableSet (true);
      LED_Array_Monochrome_Set_Color(125,255,255);
      LED_Array_Memory_Clear();
      LED_Array_Matrix_Mono_Display();
      OLEDTopLevelModeSet(TopLevelModeGet());
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
      delay(5000);
      TopLevelModeSet(MODE_SPECIAL_HARD_REBOOT);
      break;

    case MODE_SPECIAL_HARD_REBOOT:
      LED_Array_Memory_Clear();
      LED_Array_Matrix_Mono_Display();
      LED_Array_Monochrome_Set_Color(125,255,255);
      OLEDTopLevelModeSet(TopLevelModeGet());
      OLEDScreenUpdate();
      Serial.println();
      Serial.println("  Hard rebooting in 3 seconds.");      
      delay(3000);
      handleReboot(" ");
      break;

    case MODE_SPECIAL_END_OF_LIST:      TopLevelModeSet(MODE_SPECIAL_SUB_MENU);       break;

// *************************************************************************************************************************************************** //
// ***************************************************** SETTINGS ************************************************************************************ //
// *************************************************************************************************************************************************** //
    case MODE_SETTINGS_SUB_MENU:        SettingsSubMenu();                            break;
    case MODE_SETTINGS_END_OF_LIST:     TopLevelModeSet(MODE_SETTINGS_SUB_MENU);      break;

// *************************************************************************************************************************************************** //
// ***************************************************** MANUFACTURING ******************************************************************************* //
// *************************************************************************************************************************************************** //
    case MODE_MANUFACTURING_MENU:
      if (TopLevelModeChangedGet()) {
        MenuTimeOutCheckReset();
        Serial.println();
        Serial.println("  Entered MODE_MANUFACTURING_MENU.");   
        Serial.println("  Nothing to see yet.");
        Serial.print(PROMPT);
        TopLevelThreeSoftButtonGlobalEnableSet(true);
        WriteColorFontSymbolToLedScreenMemoryMatrixColor(11);   // TODO: Change to a mfg symbol.
        LED_Array_Matrix_Color_Display();
        }
      if (MenuTimeOutCheck(3000)) { TopLevelModeSetToDefault(); }
      TopLevelModeManagerCheckButtons();
      OLEDTopLevelModeSet(TopLevelModeGet());
      OLEDScreenUpdate();
      break;

    case MODE_MANUFACTURING_EEPROM_FACTORY_WRITE:
      if (TopLevelModeChangedGet()) {
        MenuTimeOutCheckReset();
        Serial.println();
        Serial.println("  Entered MODE_MANUFACTURING_EEPROM_FACTORY_WRITE.");   
        Serial.println("  Commandline Interface Disabled. CLI will be re-enabled after exiting this mode.");
        Serial.println("  Will time out in 10 seconds, waiting for pre-formatted payload. Enter it now.");
        CommandLineEnableSet(false);        // Disable Commandline Interface for now.
        /*
          string 4x128 bytes BoardIDEEPROMDataRawSerialIncoming
          bool               BoardIDEEPROMDataRawSerialIncomingFull
          array  128 Bytes   BoardIDEEPROMDataArrayParsedIncoming
          bool               BoardIDEEPROMDataArrayParsedIncomingValid
          array  128 Bytes   BoardIDEEPROMDataArrayCopyOfEEPROM   // <- this needs to be defined in EEPROM subsystem
          bool               BoardIDEEPROMDataArrayCopyOfEEPROMValid
        */
        // Receive serial input and file it into a circular buffer 4x128 Bytes, waiting for termination with CR.
        // Example: 1,2,3,4,5,...,255
        
        // Do not include CR in the buffer. The CR is a terminating sequence indicator.
        
        // Parse Buffer at commas into a 1 BYTE x 256 array BoardIDEEPROMIncomingData
        
        // Test 'checksum' XOR Bytes at Byte 31 and 63.

        // Fail, reject and try again.

        // Pass, write to EEPROM. The read back out to compare and verify.

        TopLevelThreeSoftButtonGlobalEnableSet(true);
        WriteColorFontSymbolToLedScreenMemoryMatrixColor(11);   // TODO: Change to a mfg symbol.
        LED_Array_Matrix_Color_Display();
        }
      if (MenuTimeOutCheck(10000)) { TopLevelModeSetToDefault(); }
      TopLevelModeManagerCheckButtons();
      OLEDTopLevelModeSet(TopLevelModeGet());
      OLEDScreenUpdate();
      break;

    case MODE_MANUFACTURING_END_OF_LIST: TopLevelModeSet(MODE_MANUFACTURING_MENU);      break;

// *************************************************************************************************************************************************** //
// ***************************************************** LAST AND DEFAULT **************************************************************************** //
// *************************************************************************************************************************************************** //
    case MODE_LAST:
      LED_Array_Memory_Clear();
      LED_Array_Matrix_Mono_Display();
      LED_Array_Monochrome_Set_Color(125,255,255);
      OLEDTopLevelModeSet(TopLevelModeGet());
      OLEDScreenUpdate();
      Serial.println();
      Serial.println("  TopLevelMode reached the end of the mode list. Soft restart back to MODE_START_POWER_ON.");      
      TopLevelModeSetToDefault();   
      break;

    default:
      Serial.println();
      Serial.print("  ");      
      Serial.print(TopLevelModeGet());
      Serial.println(" is an undefined or unimplemented TopLevelMode. Soft restart back to MODE_START_POWER_ON.");
      TopLevelMode = MODE_START_POWER_ON;   
      break;
  } // Closure of switch(TopLevelMode)

// *************************************************************************************************************************************************** //
// ***************************************************** CLEAR "MODE CHANGED" FLAG ******************************************************************* //
// *************************************************************************************************************************************************** //
  // if (TopLevelModeChangedGet()) { TopLevelModeSetChanged (false); } // Just in case it wasn't used by this point.

// *************************************************************************************************************************************************** //
// *****************************************************                               CHECK FOR SOFT BUTTON PRESSES ********************************* //
// *************************************************************************************************************************************************** //
  TopLevelModeManagerCheckButtons();

} // Closure of TopLevelModeManagerRun ()

