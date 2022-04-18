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
      #ifdef Pin_SAO_G1_or_SPARE1_or_CP1
        pinMode(Pin_SAO_G1_or_SPARE1_or_CP1, OUTPUT);
      #endif
      #ifdef Pin_SAO_G2_or_SPARE2_or_CP2
        pinMode(Pin_SAO_G2_or_SPARE2_or_CP2, OUTPUT);
      #endif
      #ifdef           Pin_SPARE3_or_CP3
        pinMode(          Pin_SPARE3_or_CP3, OUTPUT);
      #endif
      #ifdef           Pin_SPARE4_or_CP4
        pinMode(          Pin_SPARE4_or_CP4, OUTPUT);
      #endif
      #ifdef    Pin_HS1_or_SPARE5_or_CP5
        pinMode(   Pin_HS2_or_SPARE6_or_CP6, OUTPUT);
      #endif
      #ifdef    Pin_HS2_or_SPARE6_or_CP6
        pinMode(   Pin_HS3_or_SPARE7_or_CP7, OUTPUT);
      #endif
      #ifdef    Pin_HS3_or_SPARE7_or_CP7
        pinMode(   Pin_HS4_or_SPARE8_or_CP8, OUTPUT);
      #endif
      #ifdef    Pin_HS4_or_SPARE8_or_CP8
        pinMode(   Pin_HS4_or_SPARE8_or_CP8, OUTPUT);
      #endif
   }

   void Debug_Pin_1(bool High_nLow) {
      digitalWrite(Pin_SAO_G1_or_SPARE1_or_CP1, High_nLow);
   }

   void Debug_Pin_2(bool High_nLow) {
      #ifdef Pin_SAO_G2_or_SPARE2_or_CP2
         digitalWrite(Pin_SAO_G2_or_SPARE2_or_CP2, High_nLow);
      #endif
   }

   void Debug_Pin_3(bool High_nLow) {
      #ifdef Pin_SPARE3_or_CP3
         digitalWrite(Pin_SPARE3_or_CP3, High_nLow);
      #endif
   }

   void Debug_Pin_4(bool High_nLow) {
      #ifdef Pin_SPARE4_or_CP4
         digitalWrite(Pin_SPARE4_or_CP4, High_nLow);
      #endif
   }

   void Debug_Pin_5(bool High_nLow) {
      #ifdef Pin_HS1_or_SPARE5_or_CP5
         digitalWrite(Pin_HS1_or_SPARE5_or_CP5, High_nLow);
      #endif
   }

  void TracingPulses_Debug_Pin_1(uint8_t numberOfPulses) {
    for (uint8_t i = 1; i <= numberOfPulses; i++) {
      Debug_Pin_1(1);
      // Teensy 3.2 either runs too fast or optimizes out multiple pulses if this delay is not included.
      // The delay is also useful to see the # of traces pulses on the scope or they are too narrow.
      delayMicroseconds(1); 
      Debug_Pin_1(0);
    }
  }

#endif
