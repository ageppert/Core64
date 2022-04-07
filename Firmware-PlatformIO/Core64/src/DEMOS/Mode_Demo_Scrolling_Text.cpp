#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "Mode_Demo_Scrolling_Text.h"
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

void ModeDemoScrollingText() {
    if (TopLevelModeChangedGet()) {
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
    OLEDTopLevelModeSet(TopLevelModeGet());
    OLEDScreenUpdate();
    #if defined BOARD_CORE64_TEENSY_32
      #ifdef NEON_PIXEL_ARRAY
        Neon_Pixel_Array_Matrix_Mono_Display();
        CopyCoreMemoryToMonochromeNeonPixelArrayMemory();
      #endif
    #elif defined BOARD_CORE64C_RASPI_PICO
      // Nothing here
    #endif
}