#include <stdint.h>
#include <stdbool.h>

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include "Hal/Debug_Pins_HAL.h"
#include "Config/HardwareIOMap.h"

#if defined BOARD_CORE64_TEENSY_32
   extern void Debug_Pins_Setup() {
      pinMode(Pin_SAO_G1_SPARE_1_CP_ADDR_0, OUTPUT);
      Debug_Pin_1(0);
   }

   extern void Debug_Pin_1(bool High_nLow) {
   digitalWrite(Pin_SAO_G1_SPARE_1_CP_ADDR_0, High_nLow);
   }
#elif defined BOARD_CORE64C_RASPI_PICO
   extern void Debug_Pins_Setup() {
      pinMode(Pin_SAO_G1_SPARE_1_CP_ADDR_1, OUTPUT);
      Debug_Pin_1(0);
   }

   extern void Debug_Pin_1(bool High_nLow) {
   digitalWrite(Pin_SAO_G1_SPARE_1_CP_ADDR_1, High_nLow);
   }
#endif
