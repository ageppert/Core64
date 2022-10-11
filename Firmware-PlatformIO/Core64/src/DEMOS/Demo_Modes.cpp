#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "Demo_Modes.h"
#include "Mode_Manager.h"

#include <stdint.h>
#include <stdbool.h>

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
//#include "SubSystems/Test_Functions.h"
#include "Hal/Debug_Pins_HAL.h"

#include "SubSystems/Command_Line_Handler.h"

volatile uint32_t DemoTimeoutDeltams = 0;
volatile uint32_t DemoTimeoutFirstTimeRun = true;
volatile uint32_t DemoTimeoutLimitms = 13500;         // How long to run the demo mode in ms, default value.

void DemoTimeOutCheckReset () {
  DemoTimeoutDeltams = 0;
  DemoTimeoutFirstTimeRun = true;
}

void DemoTimeOutCheckAndMoveToNext () {
  static uint32_t NowTimems;
  static uint32_t StartTimems;
  NowTimems = millis();
  if(DemoTimeoutFirstTimeRun) { StartTimems = NowTimems; DemoTimeoutFirstTimeRun = false;}
  DemoTimeoutDeltams = NowTimems-StartTimems;
  if (DemoTimeoutDeltams >= DemoTimeoutLimitms) {
    DemoTimeOutCheckReset();
    Serial.println();
    Serial.println("  Demo timeout. Moving to next demo.");
    TopLevelModeSetInc();
  }
}

void DemoScrollingText() {
  if (TopLevelModeChangedGet()) {
    LED_Array_Monochrome_Set_Color(135,255,255);  // 135,255,255 = OLED aqua 
    ScrollTextToCoreMemoryCompleteFlagClear();
    CoreClearAll();
    LED_Array_Memory_Clear();
    LED_Array_Matrix_Mono_Display();
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
  OLEDTopLevelModeSet(TopLevelModeGet());
  OLEDScreenUpdate();
  #if defined BOARD_CORE64_TEENSY_32
    #ifdef NEON_PIXEL_ARRAY
      CopyCoreMemoryToMonochromeNeonPixelArrayMemory();
      Neon_Pixel_Array_Matrix_Mono_Display();
    #endif
  #elif defined BOARD_CORE64C_RASPI_PICO
    // Nothing here
  #endif
  if (ScrollTextToCoreMemoryCompleteFlagCheck()) { 
    Serial.println("  Text scroll complete. Moving to next demo.");
    TopLevelModeSetInc();
  }
}

// Addressing rows and columns sequentially, 2-D Knight Rider Style
void DemoLedTestOneMatrixMono() {
  if (TopLevelModeChangedGet()) { DemoTimeOutCheckReset(); }
  LED_Array_Test_Pixel_Matrix_Mono();
  OLEDTopLevelModeSet(TopLevelModeGet());
  OLEDScreenUpdate();
  DemoTimeOutCheckAndMoveToNext();
  Serial.println("  This one isn't very interesting, so skip it.");
  TopLevelModeSetInc();
}

// Multi-color symbols
void DemoLedTestOneMatrixColor() {
  if (TopLevelModeChangedGet()) { DemoTimeOutCheckReset(); }
  LED_Array_Test_Pixel_Matrix_Color();
  OLEDTopLevelModeSet(TopLevelModeGet());
  OLEDScreenUpdate();
  DemoTimeOutCheckAndMoveToNext();
}

// End of Demos, go back to the beginning (default) demo mode
void DemoEndofList() {
  TopLevelModeSetToDefault();
  OLEDTopLevelModeSet(TopLevelModeGet());
  OLEDScreenUpdate();
}
