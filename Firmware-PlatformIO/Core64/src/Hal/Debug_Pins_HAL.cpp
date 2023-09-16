#include <stdint.h>
#include <stdbool.h>

#if (ARDUINO >= 100)
   #include <Arduino.h>
#else
   #include <WProgram.h>
#endif

#include "Hal/Debug_Pins_HAL.h"
#include "Config/HardwareIOMap.h"

#if defined  MCU_TYPE_MK20DX256_TEENSY_32
   extern void Debug_Pins_Setup() {
      pinMode(Pin_SAO_G1_SPARE_1_CP_ADDR_0, OUTPUT);
      Debug_Pin_1(0);
   }

   extern void Debug_Pin_1(bool High_nLow) {
   digitalWrite(Pin_SAO_G1_SPARE_1_CP_ADDR_0, High_nLow);
   }

   void DebugAllGpioToggleTest() {
      return;
   }

#elif defined MCU_TYPE_RP2040
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
        pinMode(   Pin_HS1_or_SPARE5_or_CP5, OUTPUT);
      #endif
      #ifdef    Pin_HS2_or_SPARE6_or_CP6
        pinMode(   Pin_HS2_or_SPARE6_or_CP6, OUTPUT);
      #endif
      #ifdef    Pin_HS3_or_SPARE7_or_CP7
        pinMode(   Pin_HS3_or_SPARE7_or_CP7, OUTPUT);
      #endif
      #ifdef    Pin_HS4_or_SPARE8_or_CP8
        pinMode(   Pin_HS4_or_SPARE8_or_CP8, OUTPUT);
      #endif

      #ifdef    Pin_SPI_CS1
        pinMode(   Pin_SPI_CS1, OUTPUT);
      #endif
      #ifdef    Pin_SPI_RST
        pinMode(   Pin_SPI_RST, OUTPUT);
      #endif
      #ifdef    Pin_SPI_CD 
        pinMode(   Pin_SPI_CD , OUTPUT);
      #endif
      #ifdef    Pin_SPI_SDO
        pinMode(   Pin_SPI_SDO, OUTPUT);
      #endif
      #ifdef    Pin_SPI_SDI
        pinMode(   Pin_SPI_SDI, OUTPUT);
      #endif
      #ifdef    Pin_SPI_CLK
        pinMode(   Pin_SPI_CLK, OUTPUT);
      #endif
   }

   void Debug_Pin_1(bool High_nLow) {
      if ( (LogicBoardTypeGet() == eLBT_CORE16_PICO ) && (HardwareVersionMajor == 0) && (HardwareVersionMinor == 1) ) {
         #ifdef C16P_HWV0_1_0_PIN_SAO1_G1_OR_CM_Q1N
            digitalWrite(C16P_HWV0_1_0_PIN_SAO1_G1_OR_CM_Q1N, High_nLow);
         #endif         
      }
      if ( (LogicBoardTypeGet() == eLBT_CORE16_PICO ) && (HardwareVersionMajor >= 0) && (HardwareVersionMinor >= 2) ) {
         #ifdef C16P_HWV0_2_0_PIN_SAO1_G1_OR_CM_Q1N
            digitalWrite(C16P_HWV0_2_0_PIN_SAO1_G1_OR_CM_Q1N, High_nLow);
         #endif         
      }
      else {
         #ifdef Pin_SAO_G1_or_SPARE1_or_CP1
            digitalWrite(Pin_SAO_G1_or_SPARE1_or_CP1, High_nLow);
         #endif
      }
   }

   void Debug_Pin_2(bool High_nLow) {
      if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
         #ifdef C16P_PIN_SAO2_G2_OR_SPI_CS1
            digitalWrite(C16P_PIN_SAO2_G2_OR_SPI_CS1, High_nLow);
         #endif         
      }
      else {
         #ifdef Pin_SAO_G2_or_SPARE2_or_CP2
            digitalWrite(Pin_SAO_G2_or_SPARE2_or_CP2, High_nLow);
         #endif
      }
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

   void Debug_Pin_6(bool High_nLow) {
      #ifdef Pin_HS2_or_SPARE6_or_CP6
         digitalWrite(Pin_HS2_or_SPARE6_or_CP6, High_nLow);
      #endif
   }

   void Debug_Pin_7(bool High_nLow) {
      #ifdef Pin_HS3_or_SPARE7_or_CP7
         digitalWrite(Pin_HS3_or_SPARE7_or_CP7, High_nLow);
      #endif
   }

   void Debug_Pin_8(bool High_nLow) {
      #ifdef Pin_HS4_or_SPARE8_or_CP8
         digitalWrite(Pin_HS4_or_SPARE8_or_CP8, High_nLow);
      #endif
   }
   void Debug_Pin_SPI_CS1(bool High_nLow) {
      #ifdef Pin_SPI_CS1
         digitalWrite(Pin_SPI_CS1, High_nLow);
      #endif
   }

   void Debug_Pin_SPI_RST(bool High_nLow) {
      #ifdef Pin_SPI_RST
         digitalWrite(Pin_SPI_RST, High_nLow);
      #endif
   }

   void Debug_Pin_SPI_CD(bool High_nLow) {
      #ifdef Pin_SPI_CD
         digitalWrite(Pin_SPI_CD, High_nLow);
      #endif
   }

   void Debug_Pin_SPI_SDO(bool High_nLow) {
      #ifdef Pin_SPI_SDO
         digitalWrite(Pin_SPI_SDO, High_nLow);
      #endif
   }

   void Debug_Pin_SPI_SDI(bool High_nLow) {
      #ifdef Pin_SPI_SDI
         digitalWrite(Pin_SPI_SDI, High_nLow);
      #endif
   }

   void Debug_Pin_SPI_CLK(bool High_nLow) {
      #ifdef Pin_SPI_CLK
         digitalWrite(Pin_SPI_CLK, High_nLow);
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

   void DebugAllGpioToggleTest() {
      Debug_Pins_Setup(); // Make sure all of these pins to be toggled are configured as digital outputs.
 
      Debug_Pin_1(1);
      Debug_Pin_2(1);
      Debug_Pin_3(1);
      Debug_Pin_4(1);
      Debug_Pin_5(1);
      Debug_Pin_6(1);
      Debug_Pin_7(1);
      Debug_Pin_8(1);

      Debug_Pin_1(0);
      Debug_Pin_2(0);
      Debug_Pin_3(0);
      Debug_Pin_4(0);
      Debug_Pin_5(0);
      Debug_Pin_6(0);
      Debug_Pin_7(0);
      Debug_Pin_8(0);

      Debug_Pin_SPI_CS1(1);
      Debug_Pin_SPI_RST(1); 
      Debug_Pin_SPI_CD (1);
      Debug_Pin_SPI_SDO(1); 
      Debug_Pin_SPI_SDI(1); 
      Debug_Pin_SPI_CLK(1);

      Debug_Pin_SPI_CS1(0);
      Debug_Pin_SPI_RST(0); 
      Debug_Pin_SPI_CD (0);
      Debug_Pin_SPI_SDO(0); 
      Debug_Pin_SPI_SDI(0); 
      Debug_Pin_SPI_CLK(0); 
   }
#endif
