#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "HardwareIOMap.h"

#include <stdint.h>
#include <stdbool.h>

#include "Hal/EEPROM_HAL.h"

static volatile uint8_t eLogicBoardType = eLBT_UNKNOWN; // default to unknown logic board Type

// All of these items in this array list must be updated in the corresponding "enum eLogicBoardType" of HardwareIOMap.h to match 1:1.
// This array is use for convenience, to allow printing the mode to the serial port or screen.
  const char* LogicBoardTypeArrayText[] =
  {
      "eLBT_CORE64_T32",    // 0
      "eLBT_CORE64C_PICO",  // 1
      "eLBT_CORE64_PICO",   // 2
      "eLBT_CORE16_PICO",   // 3
      "eLBT_UNKNOWN"        // 4
  };
  // Make sure the Enum and Array are the same size.
  // https://stackoverflow.com/questions/34669164/ensuring-array-is-filled-to-size-at-compile-time
  _Static_assert(sizeof(LogicBoardTypeArrayText)/sizeof(*LogicBoardTypeArrayText) == (eLBT_UNKNOWN+1), "Logic Board Type Array Test item missing or extra, does not match eLogicBoardType Enum!");


uint8_t LogicBoardTypeGet () {
  return eLogicBoardType;
}

void LogicBoardTypeSet (uint8_t incoming) {
  eLogicBoardType = incoming;
}