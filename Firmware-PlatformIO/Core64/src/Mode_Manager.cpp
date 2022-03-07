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

#include "APPS/Drawing.h"

// All of these items in this array list must be updated in the corresponding "enum TopLevelMode" of Mode_Manager.h to match 1:1.
// This array is use for convenience, to allow printing the mode to the serial port or screen.
const char* TOP_LEVEL_MODE_NAME_ARRAY[] =
{
    "MODE_START_POWER_ON",
    "   MODE_START_INIT",
    "   MODE_START_EEPROM",
    "   MODE_START_SEQUENCE_COMPLETE",
    "MODE_DGAUSS_MENU                           ",
    "    MODE_DEMO_SUB_MENU                     ",
    "        MODE_DEMO_SCROLLING_TEXT           ",
    "        MODE_DEMO_LED_TEST_ONE_MATRIX_MONO ",
    "        MODE_DEMO_LED_TEST_ONE_MATRIX_COLOR",
    "        MODE_DEMO_END_OF_LIST              ",
    "    MODE_GAME_SUB_MENU                     ",
    "    MODE_APP_SUB_MENU                      ",
    "        MODE_APP_DRAW                      ",
    "    MODE_UTIL_SUB_MENU                     ",
    "        MODE_UTIL_FLUX_DETECTOR            ",
    "    MODE_SPECIAL_SUB_MENU                  ",
    "        MODE_LED_TEST_ALL_BINARY           ",
    "        MODE_LED_TEST_ONE_STRING           ",
    "        MODE_TEST_EEPROM                   ",
    "        MODE_LED_TEST_ALL_COLOR            ",
    "        MODE_CORE_TOGGLE_BIT               ",
    "        MODE_CORE_TEST_ONE                 ",
    "        MODE_CORE_TEST_MANY                ",
    "        MODE_HALL_TEST                     ",
    "        MODE_SPECIAL_LOOPBACK_TEST         ",
    "        MODE_SPECIAL_HARD_REBOOT                   ",
    "    MODE_SETTINGS_SUB_MENU                 ",
    "MODE_MANUFACTURING_MENU                    ",
    "MODE_LAST                                  " 
};

// https://stackoverflow.com/questions/34669164/ensuring-array-is-filled-to-size-at-compile-time
_Static_assert(sizeof(TOP_LEVEL_MODE_NAME_ARRAY)/sizeof(*TOP_LEVEL_MODE_NAME_ARRAY) == (MODE_LAST+1), "Top_Level_Mode_Name_Array item missing or extra, does not match TopLevelMode Enum!");

// OR the poorman's static assertion which creates a compiler error with array size of -1.
// typedef char CHECK_COLOR_NAMES[sizeof(MODE_NAMES) / sizeof(MODE_NAMES[0]) == MODE_LAST ? 1 : -1];

// TODO: Also set a flag that the firmware can read (at power on) and display a warning in serial/screen.

static uint16_t TopLevelModeDefault = MODE_DEMO_SCROLLING_TEXT;
static uint16_t TopLevelMode = MODE_START_POWER_ON;
static uint16_t TopLevelModePrevious;
static bool     TopLevelModeChanged = false;
static bool     TopLevelThreeSoftButtonGlobalEnable = true;

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

static uint32_t MenuTimeoutTimerms  = 0;
const uint32_t MenuTimeoutLimitms   = 30000; // 30 second timeout in menus.
static uint32_t MenuTimeoutDeltams  = 0;
static bool MenuTimeoutFirstTimeRun =0;

void TopLevelModeSetToDefault() { TopLevelMode = TopLevelModeDefault; }

void TopLevelModeSet (uint16_t value) {   TopLevelMode = value; TopLevelModeSetChanged(false); TopLevelModeSetChanged(true); }
uint16_t TopLevelModeGet ()           {   return (TopLevelMode); }
void TopLevelModeSetInc ()            {   TopLevelMode++; TopLevelModeSetChanged(true); }
void TopLevelModeSetDec ()            {   TopLevelMode--; TopLevelModeSetChanged(true); }               

void TopLevelModeSetPrevious (uint16_t value)   { TopLevelModePrevious = value; }
uint16_t TopLevelModeGetPrevious ()             { return (TopLevelModePrevious); }

void TopLevelModeChangeSerialPortDisplay () {
  Serial.println();
  Serial.print("  TopLevelMode changed from ");
  Serial.print(TopLevelModeGetPrevious());
  Serial.print(TOP_LEVEL_MODE_NAME_ARRAY[TopLevelModeGetPrevious()]);
  Serial.print(" to ");
  Serial.print(TopLevelModeGet());
  Serial.print(" ");
  Serial.print(TOP_LEVEL_MODE_NAME_ARRAY[TopLevelModeGet()]);
  Serial.println(".");
  // Serial.print(PROMPT);         // Print the first prompt to show the system is ready for input
}

void TopLevelModeSetChanged (bool value) {          // Flag that a mode change has occurred.  User application has one time to use this before it is reset.
  if ( (TopLevelModeChanged == false) && (value == true) ) {
    TopLevelModeChangeSerialPortDisplay();            // Display the change in the serial port.
    TopLevelThreeSoftButtonGlobalEnableSet (true);    // Ensures this flag set to enabled after a mode change, so modes that need it disabled will need to force it to be disabled. 
  }
  TopLevelModeChanged = value;
}

bool TopLevelModeGetChanged ()           {   return (TopLevelModeChanged); }

void TopLevelThreeSoftButtonGlobalEnableSet (bool value) {   TopLevelThreeSoftButtonGlobalEnable = value; }
bool TopLevelThreeSoftButtonGlobalEnableGet ()           {   return (TopLevelThreeSoftButtonGlobalEnable); }

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

  #if defined BOARD_CORE64_TEENSY_32
    Button1HoldTime = ButtonState(1,0);
    Button2HoldTime = ButtonState(2,0);
    Button3HoldTime = ButtonState(3,0);
    Button4HoldTime = ButtonState(4,0);
  #elif defined BOARD_CORE64C_RASPI_PICO
    // Not yet implemented
  #endif

  // Checking the "M" soft button to enter or exit the DGAUSS menu.
  if ( (Button1Released == true) && (Button1HoldTime >= 100) ) {
    ButtonState(1,1); // Force a "release" after press by clearing the button hold down timer
    Button1Released = false;
    ColorFontSymbolToDisplay++;
    if(ColorFontSymbolToDisplay>3) { ColorFontSymbolToDisplay = 0; }
    if (TopLevelModeGet()!=MODE_DGAUSS_MENU) {                // Enter DGAUSS menu
      TopLevelModeSetPrevious(TopLevelModeGet());
      TopLevelModeSet(MODE_DGAUSS_MENU);
    }
    else {                                                    // Exit DGAUSS menu
      if (TopLevelModeGetPrevious() == MODE_DGAUSS_MENU) {    // In this case, if previous was already the DGAUSS menu, 
        TopLevelModeSet(TopLevelModeDefault);                 // just go to default demo mode. 
      }
      else {
        uint32_t temporary = TopLevelModeGetPrevious();            // Otherwise, go the previous mode, after storing it temporarily.
        TopLevelModeSetPrevious(MODE_DGAUSS_MENU);            // Set the previous mode to DGAUSS menu.
        TopLevelModeSet(temporary);                                // Now go to the previous mode.
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

    // Checking the "S" soft button.
    if ( (Button4Released == true) && (Button4HoldTime >= 100) ) {
      ButtonState(4,1); // Force a "release" after press by clearing the button hold down timer
      Button4Released = false;
      TopLevelModePrevious = TopLevelMode;
      TopLevelModeSetPrevious(TopLevelModeGet());
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

void MenuTimeOutCheckAndExitToModeDefault () {
  static uint32_t NowTimems;
  static uint32_t StartTimems;
  NowTimems = millis();
  if(MenuTimeoutFirstTimeRun) { StartTimems = NowTimems; MenuTimeoutFirstTimeRun = false;}
  MenuTimeoutDeltams = NowTimems-StartTimems;
  if (MenuTimeoutDeltams >= MenuTimeoutLimitms) {
    MenuTimeOutCheckReset();
    Serial.println();
    Serial.println("  Menu timeout. Returning to TopLevelModeDefault.");
    TopLevelModeSetPrevious(TopLevelModeGet());
    TopLevelModeSet (TopLevelModeDefault);
  }
}

void TopLevelModeManagerRun () {
// *************************************************************************************************************************************************** //
// *****************************************************                               CHECK FOR SOFT BUTTON PRESSES ********************************* //
// *************************************************************************************************************************************************** //
  // if (TopLevelModeGetChanged()) { TopLevelModeSetChanged (false); }
  TopLevelModeManagerCheckButtons();
  switch(TopLevelMode) {

// *************************************************************************************************************************************************** //
    case MODE_START_POWER_ON:                   // *************************************************************************************************** //
// *************************************************************************************************************************************************** //
      TopLevelModeSetChanged(false);
      delay(1500);                  // Wait a little bit for the serial port to connect if this is connected to a computer terminal
      Serial.println();
      Serial.println("  Power-on start up sequence has begun...");
      handleSplash("");             // Splash screen
      handleInfo("");               // Print some info about the system (this also checks hardware version, born-on, and serial number)
      handleHelp("");               // Print the help menu
      TopLevelModeSetPrevious (TopLevelModeGet());
      TopLevelModeSetInc();
      break;

// *************************************************************************************************************************************************** //
    case MODE_START_INIT:                       // *************************************************************************************************** //
// *************************************************************************************************************************************************** //
      Serial.println("  Init sequence has begun...");
      TopLevelModeSetPrevious (TopLevelModeGet());
      TopLevelModeSetInc();
      break;

// *************************************************************************************************************************************************** //
    case MODE_START_EEPROM:                     // *************************************************************************************************** //
// *************************************************************************************************************************************************** //
      Serial.println("  EEEPROM read has begun...");
      TopLevelModeSetPrevious (TopLevelModeGet());
      TopLevelModeSetInc();
      break;

// *************************************************************************************************************************************************** //
// ***************************************************** PROMPT AND JUMP TO DEFAULT MODE **************************************************************** //
// *************************************************************************************************************************************************** //
    case MODE_START_SEQUENCE_COMPLETE:
      Serial.println("  Start-up sequence has been completed.");
      Serial.print(PROMPT);
      TopLevelModeSetPrevious (TopLevelModeGet());
      TopLevelModeSet(TopLevelModeDefault);
      break;

// *************************************************************************************************************************************************** //
// ***************************************************** DGAUSS MENU ********************************************************************************* //
// *************************************************************************************************************************************************** //
    case MODE_DGAUSS_MENU:
      if (TopLevelModeGetChanged()){
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
        WriteColorFontSymbolToLedScreenMemoryMatrixColor(4);
        LED_Array_Matrix_Color_Display();
        Serial.print(PROMPT);
      }
      MenuTimeOutCheckAndExitToModeDefault();
      Core_Mem_Scan_For_Magnet();
      // Was the D touched with magnetic stylus?
      for (uint8_t y=0; y<5; y++) {
        for (uint8_t x=0; x<2; x++) {
          if ( (CoreArrayMemory [y][x]) || (CoreArrayMemory [7][5]) ) { 
            Serial.println();
            Serial.println("  Demo sub-menu selected.");   
            TopLevelModeSetPrevious (TopLevelModeGet());
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
            TopLevelModeSetPrevious (TopLevelModeGet());
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
            TopLevelModeSetPrevious (TopLevelModeGet());
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
            TopLevelModeSetPrevious (TopLevelModeGet());
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
            TopLevelModeSetPrevious (TopLevelModeGet());
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
            TopLevelModeSetPrevious (TopLevelModeGet());
            TopLevelModeSet (MODE_SETTINGS_SUB_MENU);
          }
        }
      }
      break;

// *************************************************************************************************************************************************** //
// ***************************************************** DEMO **************************************************************************************** //
// *************************************************************************************************************************************************** //
    case MODE_DEMO_SUB_MENU:
            Serial.println();
            Serial.println("  Automatically moving to MODE_DEMO_SCROLLING_TEXT.");   
            TopLevelModeSetPrevious (TopLevelModeGet());
            TopLevelModeSet (MODE_DEMO_SCROLLING_TEXT);
      break;

    case MODE_DEMO_SCROLLING_TEXT:
      if (TopLevelModeGetChanged()) {
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
      OLEDTopLevelModeSet(TopLevelMode);
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

    case MODE_DEMO_LED_TEST_ONE_MATRIX_MONO: // Turns on 1 pixel, sequentially, from left to right, top to bottom using 2D matrix addressing
      LED_Array_Test_Pixel_Matrix_Mono();
      OLEDTopLevelModeSet(TopLevelMode);
      OLEDScreenUpdate();
      break;

    case MODE_DEMO_LED_TEST_ONE_MATRIX_COLOR: // Multi-color symbols
      LED_Array_Test_Pixel_Matrix_Color();
      OLEDTopLevelModeSet(TopLevelMode);
      OLEDScreenUpdate();
      break;

    case MODE_DEMO_END_OF_LIST:
      TopLevelModeSet(TopLevelModeDefault);
      OLEDTopLevelModeSet(TopLevelMode);
      OLEDScreenUpdate();
      break;

// *************************************************************************************************************************************************** //
// ***************************************************** GAME **************************************************************************************** //
// *************************************************************************************************************************************************** //
    case MODE_GAME_SUB_MENU:
            Serial.println();
            Serial.println("  Entered MODE_GAME_SUB_MENU.");   
            Serial.println("  Nothing to see yet. Back to Demo mode.");   
            TopLevelModeSetPrevious (TopLevelModeGet());
            TopLevelModeSet (MODE_DEMO_SCROLLING_TEXT);
      break;

// *************************************************************************************************************************************************** //
// ***************************************************** APP ***************************************************************************************** //
// *************************************************************************************************************************************************** //
    case MODE_APP_SUB_MENU:
            Serial.println();
            Serial.println("  Automatically moving to MODE_APP_DRAW.");   
            TopLevelModeSetPrevious (TopLevelModeGet());
            TopLevelModeSet (MODE_APP_DRAW);
      break;

    case MODE_APP_DRAW: Draw(); break;

// *************************************************************************************************************************************************** //
// ***************************************************** UTIL **************************************************************************************** //
// *************************************************************************************************************************************************** //
    case MODE_UTIL_SUB_MENU:
            Serial.println();
            Serial.println("  Automatically moving to MODE_UTIL_FLUX_DETECTOR.");   
            TopLevelModeSetPrevious (TopLevelModeGet());
            TopLevelModeSet (MODE_UTIL_FLUX_DETECTOR);
      break;

    #if defined BOARD_CORE64_TEENSY_32
        case MODE_UTIL_FLUX_DETECTOR:                         // Read 64 cores 10ms (110us 3x core write, with 40us delay 64 times), update LEDs 2ms
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
          OLEDTopLevelModeSet(TopLevelMode);
          OLEDScreenUpdate();
          break;
      #elif defined BOARD_CORE64C_RASPI_PICO
        
      #endif

// *************************************************************************************************************************************************** //
// ***************************************************** SPECIAL ************************************************************************************* //
// *************************************************************************************************************************************************** //
    case MODE_SPECIAL_SUB_MENU:
            Serial.println();
            Serial.println("  Automatically moving to first item in MODE_SPECIAL_SUB_MENU.");   
            TopLevelModeSetPrevious (TopLevelModeGet());
            TopLevelModeSetInc();
      break;

    #if defined BOARD_CORE64_TEENSY_32
      case MODE_LED_TEST_ALL_BINARY: // Counts from lower right and left/up in binary, using Binary LUT.
        LED_Array_Test_Count_Binary();
        OLEDTopLevelModeSet(TopLevelMode);
        OLEDScreenUpdate();
        break;

      case MODE_LED_TEST_ONE_STRING: // Illuminates one pixel, sequentially from left to right, top to bottom using 1D string addressing.
        LED_Array_Test_Pixel_String();
        OLEDTopLevelModeSet(TopLevelMode);
        OLEDScreenUpdate();
        break;

      case MODE_TEST_EEPROM: // 
        // value = EEPROM_Hardware_Version_Read(a);  // Teensy internal emulated EEPROM
        Serial.println();
        EepromByteValue = EEPROMExtDefaultReadByte(Eeprom_Byte_Mem_Address);
        Serial.print(Eeprom_Byte_Mem_Address);
        Serial.print("\t");
        Serial.print(EepromByteValue);
        Eeprom_Byte_Mem_Address = Eeprom_Byte_Mem_Address + 1;
        if (Eeprom_Byte_Mem_Address == 128) {
          Eeprom_Byte_Mem_Address = 0;
        }
        // delay(100);
        break;
        
      case MODE_LED_TEST_ALL_COLOR: // FastLED Demo of all color
        LED_Array_Test_Rainbow_Demo();
        OLEDTopLevelModeSet(TopLevelMode);
        OLEDScreenUpdate();
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

        OLEDTopLevelModeSet(TopLevelMode);
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
        OLEDTopLevelModeSet(TopLevelMode);
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
        OLEDTopLevelModeSet(TopLevelMode);
        OLEDScreenUpdate();
        break;
    #elif defined BOARD_CORE64C_RASPI_PICO
      
    #endif

    case MODE_HALL_TEST:
      TopLevelThreeSoftButtonGlobalEnableSet (false);
      LED_Array_Monochrome_Set_Color(25,255,255);
      LED_Array_Memory_Clear();

      // IOESpare1_On();
      if(Button1HoldTime) { LED_Array_String_Write(56,1); Serial.println(Button1HoldTime); }
      if(Button2HoldTime) { LED_Array_String_Write(58,1); Serial.println(Button2HoldTime); }
      if(Button3HoldTime) { LED_Array_String_Write(60,1); Serial.println(Button3HoldTime); }
      if(Button4HoldTime) { LED_Array_String_Write(62,1); Serial.println(Button4HoldTime); }
      // IOESpare1_Off();

      LED_Array_String_Display();
      OLEDTopLevelModeSet(TopLevelMode);
      OLEDScreenUpdate();
    
      break;

    case MODE_SPECIAL_LOOPBACK_TEST:
      LED_Array_Monochrome_Set_Color(125,255,255);
      LED_Array_Memory_Clear();
      LED_Array_Matrix_Mono_Display();
      OLEDTopLevelModeSet(TopLevelMode);
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
      OLEDTopLevelModeSet(TopLevelMode);
      OLEDScreenUpdate();
      Serial.println();
      Serial.println("  Hard rebooting in 3 seconds.");      
      delay(3000);
      handleReboot(" ");
      break;

// *************************************************************************************************************************************************** //
// ***************************************************** SETTINGS ************************************************************************************ //
// *************************************************************************************************************************************************** //
    case MODE_SETTINGS_SUB_MENU:
            Serial.println();
            Serial.println("  Entered MODE_SETTINGS_SUB_MENU.");   
            Serial.println("  Nothing to see yet. Back to Demo mode.");   
            TopLevelModeSetPrevious (TopLevelModeGet());
            TopLevelModeSet (MODE_DEMO_SCROLLING_TEXT);
      break;

// *************************************************************************************************************************************************** //
// ***************************************************** MANUFACTURING ******************************************************************************* //
// *************************************************************************************************************************************************** //
    case MODE_MANUFACTURING_MENU:
      Serial.println();
      Serial.println("  Entered MODE_MANUFACTURING_MENU.");   
      Serial.println("  Nothing to see yet. Back to Demo mode.");   
      TopLevelModeSetPrevious (TopLevelModeGet());
      TopLevelModeSetToDefault();
      break;

// *************************************************************************************************************************************************** //
// ***************************************************** LAST AND DEFAULT **************************************************************************** //
// *************************************************************************************************************************************************** //
    case MODE_LAST:
      LED_Array_Memory_Clear();
      LED_Array_Matrix_Mono_Display();
      LED_Array_Monochrome_Set_Color(125,255,255);
      OLEDTopLevelModeSet(TopLevelMode);
      OLEDScreenUpdate();
      Serial.println();
      Serial.println("  TopLevelMode reached the end of the mode list. Jumping back to default mode.");      
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
  if (TopLevelModeGetChanged()) { TopLevelModeSetChanged (false); }
} // Closure of TopLevelModeManagerRun ()

