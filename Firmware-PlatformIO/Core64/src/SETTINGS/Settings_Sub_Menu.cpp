#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "Settings_Sub_Menu.h"

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

void SettingsSubMenu() {
  if (TopLevelModeChangedGet()) {
    MenuTimeOutCheckReset();
    Serial.println();
    Serial.println("   Settings Sub-Menu");
    Serial.println("    M = Main DGAUSS Menu");
    Serial.println("    + = Next Setting");
    Serial.println("    - = Previous Setting");
    Serial.println("    S = Select Setting To Change");
    Serial.print(PROMPT);
    TopLevelSetSoftButtonGlobalEnableSet(false);
    WriteColorFontSymbolToLedScreenMemoryMatrixColor(6);
    LED_Array_Matrix_Color_Display();
    #if defined  MCU_TYPE_MK20DX256_TEENSY_32
      #ifdef NEON_PIXEL_ARRAY
        CopyColorFontSymbolToNeonPixelArrayMemory(6);
        Neon_Pixel_Array_Matrix_Mono_Display();
      #endif
    #elif defined MCU_TYPE_RP2040
      // Nothing here
    #endif
    }
  if (MenuTimeOutCheck(10000)) { TopLevelModeSetToDefault(); }
  TopLevelModeManagerCheckButtons();
  OLEDTopLevelModeSet(TopLevelModeGet());
  OLEDScreenUpdate();
  // TODO: List the items in the sub-menu.
}