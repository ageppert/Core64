#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "Snake.h"

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


void Snake() {
  if (TopLevelModeChangedGet()) {
    MenuTimeOutCheckReset();
    Serial.println();
    Serial.println("   Snake Game Sub-Menu");
    Serial.println("    M = Main DGAUSS Menu");
    Serial.println("    + = Random Start Next Maze");
    Serial.println("    - = undefined");
    Serial.println("    S = undefined");
    Serial.print(PROMPT);
    TopLevelThreeSoftButtonGlobalEnableSet(1);
    WriteColorFontSymbolToLedScreenMemoryMatrixColor(11); // TODO: update to use a snake logo
    LED_Array_Monochrome_Set_Color(0,255,255);
    LED_Array_Matrix_Color_Display();
    }
  MenuTimeOutCheckAndExitToModeDefault();
  // TopLevelThreeSoftButtonGlobalEnableSet (false);            // Ensures this flag stays disabled in this mode. Preventing global use of +, -, S BUTTONS.
  OLEDTopLevelModeSet(TopLevelModeGet());
  OLEDScreenUpdate();

  // Insert the Game Logic Here

}