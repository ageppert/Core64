#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "Special_Sub_Menu.h"

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

void SpecialSubMenu() {
  if (TopLevelModeChangedGet()) {
    MenuTimeOutCheckReset();
    Serial.println();
    Serial.println("   Special Sub-Menu");
    Serial.println("    M = Main DGAUSS Menu");
    Serial.println("    + = Next Special Function");
    Serial.println("    - = Previous Special Function");
    Serial.println("    S = Select Special Function");
    Serial.print(PROMPT);
    TopLevelSetSoftButtonGlobalEnableSet(false);
    WriteColorFontSymbolToLedScreenMemoryMatrixColor(5);
    LED_Array_Matrix_Color_Display();
    }
  if (MenuTimeOutCheck(3000)) { TopLevelModeSetInc(); }
  TopLevelModeManagerCheckButtons();
  OLEDTopLevelModeSet(TopLevelModeGet());
  OLEDScreenUpdate();
  // TODO: List the items in the sub-menu.
}