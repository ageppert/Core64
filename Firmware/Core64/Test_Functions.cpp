#include <stdint.h>
#include <stdbool.h>

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include "HardwareIOMap.h"

/* Testing V0.6 Logic Boards in the first super-simple test fixture.
// Result values
        0 = PASS
    1-200 = FAIL as number of test failures
  201-254 = Specific error code
      255 = Unhandled exception
*/
uint8_t LoopBackTest() {
  uint8_t TestFailCounter = 255 ; // Result = Unhandled
  #if defined BOARD_CORE64_TEENSY_32
    if ( (HardwareVersionMinor == 6) ) {
      TestFailCounter = 0;
      // SPI_SDO and SPI_SDI test
      pinMode(Pin_SPI_SDO, OUTPUT);
      pinMode(Pin_SPI_SDI, INPUT);
      digitalWrite(Pin_SPI_SDO,0);
      if (digitalRead(Pin_SPI_SDI)!=0) {TestFailCounter ++;}
      digitalWrite(Pin_SPI_SDO,1);
      if (digitalRead(Pin_SPI_SDI)!=1) {TestFailCounter ++;}
      pinMode(Pin_SPI_SDI, OUTPUT);
      pinMode(Pin_SPI_SDO, INPUT);
      digitalWrite(Pin_SPI_SDI,0);
      if (digitalRead(Pin_SPI_SDO)!=0) {TestFailCounter ++;}
      digitalWrite(Pin_SPI_SDI,1);
      if (digitalRead(Pin_SPI_SDO)!=1) {TestFailCounter ++;}

      // IR_OUT and SPARE_4_IR_IN test
      pinMode(Pin_IR_OUT, OUTPUT);
      pinMode(Pin_Spare_4_IR_IN, INPUT);
      digitalWrite(Pin_IR_OUT,0);
      if (digitalRead(Pin_Spare_4_IR_IN)!=0) {TestFailCounter ++;}
      digitalWrite(Pin_IR_OUT,1);
      if (digitalRead(Pin_Spare_4_IR_IN)!=1) {TestFailCounter ++;}
      pinMode(Pin_Spare_4_IR_IN, OUTPUT);
      pinMode(Pin_IR_OUT, INPUT);
      digitalWrite(Pin_Spare_4_IR_IN,0);
      if (digitalRead(Pin_IR_OUT)!=0) {TestFailCounter ++;}
      digitalWrite(Pin_Spare_4_IR_IN,1);
      if (digitalRead(Pin_IR_OUT)!=1) {TestFailCounter ++;}

      // SPARE GPIO 1 and 2 test.
      pinMode(Pin_SAO_G1_SPARE_1_CP_ADDR_0, OUTPUT);
      pinMode(Pin_SAO_G2_SPARE_2_CP_ADDR_1, INPUT);
      digitalWrite(Pin_SAO_G1_SPARE_1_CP_ADDR_0,0);
      if (digitalRead(Pin_SAO_G2_SPARE_2_CP_ADDR_1)!=0) {TestFailCounter ++;}
      digitalWrite(Pin_SAO_G1_SPARE_1_CP_ADDR_0,1);
      if (digitalRead(Pin_SAO_G2_SPARE_2_CP_ADDR_1)!=1) {TestFailCounter ++;}
      pinMode(Pin_SAO_G2_SPARE_2_CP_ADDR_1, OUTPUT);
      pinMode(Pin_SAO_G1_SPARE_1_CP_ADDR_0, INPUT);
      digitalWrite(Pin_SAO_G2_SPARE_2_CP_ADDR_1,0);
      if (digitalRead(Pin_SAO_G1_SPARE_1_CP_ADDR_0)!=0) {TestFailCounter ++;}
      digitalWrite(Pin_SAO_G2_SPARE_2_CP_ADDR_1,1);
      if (digitalRead(Pin_SAO_G1_SPARE_1_CP_ADDR_0)!=1) {TestFailCounter ++;}
    }
    if ( (HardwareVersionMinor == 5) ) {
      TestFailCounter = 253; // Result = Not implemented for Core64 V0.5.0
    }

  #elif defined BOARD_CORE64C_RASPI_PICO
    TestFailCounter = 254; // Result = Not implemented for Core64c
  #endif

  return TestFailCounter;
}
