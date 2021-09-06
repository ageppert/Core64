#include <stdint.h>
#include <stdbool.h>

/*
#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
*/

#include "HardwareIOMap.h"

#include "Core_Driver.h"
#include "I2C_Manager.h"
#include "EEPROM_HAL.h"

#if defined BOARD_CORE64_TEENSY_32

  enum CorePatternArrangement
  {
       CORE_ARRANGEMENT_UNDEFINED = 0,
  /*   If the outer corners of the core plane cores are arranged like this
       / \
       \ /
       Use CORE_ARRANGEMENT_NORMAL (Default)                                       
  */
       CORE_ARRANGEMENT_NORMAL = 1,
  /*   If the outer corners of the core plane cores are arranged like this:
       \ /
       / \
       Use CORE_ARRANGEMENT_OPPOSITE
  */
       CORE_ARRANGEMENT_OPPOSITE = 2,
  };
  static uint8_t CorePatternArrangement = CORE_ARRANGEMENT_NORMAL;  // Default Core Pattern Arrangement. Updateable from serial port and EEPROM.

  // Array from 1-20 with MCU pin # associated to verbose transitor drive line name. Ex: PIN_MATRIX_DRIVE_Q1P
  // Array position number 0 is not used in the matrix pin numbering
  // Matrix Drive Line array position :         0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33
  // Look out for these!              :                                                               *  *  *
  // V0.3 MCU PIN #                   :       N/A, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17, -, -,20,21,22
  // V0.4 MCU PIN #                   :       N/A, -, 1, 2, 3, 4, 5, 6, 7, 8, -, -, -, -, -, -, 9,10, -, -, -, -, -, -,11,12,13,14,15,16,17,20,21,22
  static uint8_t MatrixDrivePinNumber[34] = {
    0,
    0,
    PIN_MATRIX_DRIVE_Q1P ,
    PIN_MATRIX_DRIVE_Q1N ,
    PIN_MATRIX_DRIVE_Q2P ,
    PIN_MATRIX_DRIVE_Q2N ,
    PIN_MATRIX_DRIVE_Q3P ,
    PIN_MATRIX_DRIVE_Q3N ,
    PIN_MATRIX_DRIVE_Q4P ,
    PIN_MATRIX_DRIVE_Q4N ,
    0,
    0,
    0,
    0,
    0,
    0,
    PIN_MATRIX_DRIVE_Q5P ,
    PIN_MATRIX_DRIVE_Q5N ,
    0,
    0,
    0,
    0,
    0,
    0,
    PIN_MATRIX_DRIVE_Q6P ,
    PIN_MATRIX_DRIVE_Q6N ,
    PIN_MATRIX_DRIVE_Q7P ,
    PIN_MATRIX_DRIVE_Q7N ,
    PIN_MATRIX_DRIVE_Q8P ,
    PIN_MATRIX_DRIVE_Q8N ,
    PIN_MATRIX_DRIVE_Q9P ,
    PIN_MATRIX_DRIVE_Q9N ,
    PIN_MATRIX_DRIVE_Q10P,
    PIN_MATRIX_DRIVE_Q10N
  };

  // All QxN transistors are Active High.
  // All QxP transistors are Active Low.

  // V0.4 hardware (direct MCU pin control)
  // Look up the drive line number by knowing where it is in the array by it's own number.
  // In other words, the pin # has to be the same as the array position.
  // Array needs to be as big as the largest used pin number.
  // Ex:
  //                             array position:  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33
  // V0.4 MCU PIN #              usable pins   :  x, x, 1, 2, 3, 4, 5, 6, 7, 8, x, x, x, x, x, x, 9,10, x, x, x, x, x, x,11,12,13,14,15,16,17,20,21,22
  // MCU output pins are set to these states to correspond to activation of the transistor needed to achieve on/off state.
  const bool MatrixDrivePinInactiveState[34] =  { 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}; // logic level to turn off transistor
  const bool MatrixDrivePinActiveState[34]   =  { 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}; // logic level to turn on transistor


  // MCU output pin is set to these states to correspond to activation of the transistor needed to achieve active/inactive state.
  #define WRITE_ENABLE_ACTIVE   1 // logic level to turn on transistor
  #define WRITE_ENABLE_INACTIVE 0 // logic level to turn off transistor

  static uint8_t CorePlane = 1;           // Default Core Plane if it is not specified by the user application.
  const bool CorePlaneAddr[9][3] = {
    { 0,0,0 },  // not used
    { 0,0,0 },  // Plane 1 (MSB to LSB)
    { 0,0,1 },  // Plane 2
    { 0,1,0 },  // Plane 3
    { 0,1,1 },  // Plane 4
    { 1,0,0 },  // Plane 5
    { 1,0,1 },  // Plane 6
    { 1,1,0 },  // Plane 7
    { 1,1,1 }   // Plane 8     
  };

  // Given a Core Memory Matrix Row 0 to 7 the array below specifies which 2 pins connected to transistors are required to set the row.
  // CMM front (user) view is with Row 0 on top, 7 on bottom.
  // Each row of the array corresponds to rows 0 to 7 of the CMM.
  // Each row is sequence of 2 transitors, first one connects to top four rows and second one connects to the bottom four rows.
  /* 
  The original assumption of current going left to right in a row does not work because the cores
  are not all placed in the same orientation. The cores alternate back and forth in a row, and in 
  columns, to make the circuit simpler. A new row set and clear array are required which take into 
  account that every other bit needs to have the current direction reversed in order to compensate 
  if all of the cores are to be physically addressed in an orderly sequence. 
  */
  // V0.1.x and V0.2.x and V0.4.x hardware (direct MCU pin control)

  static uint8_t CMMDSetRowByBit[][2] = {

    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 32    ROW 4 P/N swapped from ROW 0
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 33    ROW 4
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 34    ROW 4
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 35    ROW 4
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 36    ROW 4
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 37    ROW 4
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 38    ROW 4
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 39    ROW 4

    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 40    ROW 5 P/N swapped from ROW 1
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 41    ROW 5
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 42    ROW 5
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 43    ROW 5
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 44    ROW 5
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 45    ROW 5
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 46    ROW 5
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 47    ROW 5

    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 48    ROW 6 P/N swapped from ROW 2
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 49    ROW 6 
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 50    ROW 6 
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 51    ROW 6 
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 52    ROW 6 
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 53    ROW 6 
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 54    ROW 6 
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 55    ROW 6 

    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 56    ROW 7 P/N swapped from ROW 3
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 57    ROW 7
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 58    ROW 7
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 59    ROW 7
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 60    ROW 7
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 61    ROW 7
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 62    ROW 7
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },   // Bit 63    ROW 7

    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit  0    ROW 0
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit  1    ROW 0
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit  2    ROW 0
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit  3    ROW 0
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit  4    ROW 0
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit  5    ROW 0
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit  6    ROW 0
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit  7    ROW 0

    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit  8    ROW 1
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit  9    ROW 1
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 10    ROW 1
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 11    ROW 1
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 12    ROW 1
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 13    ROW 1
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 14    ROW 1
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 15    ROW 1

    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 16    ROW 2
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 17    ROW 2
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 18    ROW 2
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 19    ROW 2
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 20    ROW 2
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 21    ROW 2
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 22    ROW 2
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 23    ROW 2

    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 24    ROW 3
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 25    ROW 3
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 26    ROW 3
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 27    ROW 3
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 28    ROW 3
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 29    ROW 3
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 30    ROW 3
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P }  // Bit 31    ROW 3
  }; 

  static uint8_t CMMDClearRowByBit[][2] = {

    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 32    ROW 4 P/N swapped from ROW 0
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 33    ROW 4
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 34    ROW 4
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 35    ROW 4
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 36    ROW 4
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 37    ROW 4
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 38    ROW 4
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 39    ROW 4

    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 40    ROW 5 P/N swapped from ROW 1
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 41    ROW 5
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 42    ROW 5
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 43    ROW 5
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 44    ROW 5
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 45    ROW 5
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 46    ROW 5
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 47    ROW 5

    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 48    ROW 6 P/N swapped from ROW 2
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 49    ROW 6
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 50    ROW 6
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 51    ROW 6
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 52    ROW 6
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 53    ROW 6
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 54    ROW 6
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 55    ROW 6

    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 56    ROW 7 P/N swapped from ROW 3
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 57    ROW 7
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 58    ROW 7
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 59    ROW 7
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 60    ROW 7
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 61    ROW 7
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 62    ROW 7
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },   // Bit 63    ROW 7

    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 0     ROW 0
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 1     ROW 0
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 2     ROW 0
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 3     ROW 0
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 4     ROW 0
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 5     ROW 0
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 6     ROW 0
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 7     ROW 0      

    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit  8    ROW 1
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit  9    ROW 1
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 10    ROW 1
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 11    ROW 1
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 12    ROW 1
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 13    ROW 1
    { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 14    ROW 1
    { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 15    ROW 1

    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 16    ROW 2
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 17    ROW 2
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 18    ROW 2
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 19    ROW 2
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 20    ROW 2
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 21    ROW 2
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 22    ROW 2
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 23    ROW 2

    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 24    ROW 3
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 25    ROW 3
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 26    ROW 3
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 27    ROW 3
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 28    ROW 3
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 29    ROW 3
    { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 30    ROW 3
    { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N }  // Bit 31    ROW 3
  };

  // CMMD = Core Memory Matrix Drive
  // Given a Core Memory Matrix Column 0 to 7 the array below specifies which 2 pins connected to transistors are required to set the column.
  // CMM front (user) view is with Column 0 on left, 7 on right.
  // Each row of the array corresponds to columns 0 to 7 of the CMM.
  // Each row is sequence of 2 transitors, first one is at the top and second one is at the bottom.

  // Set is given the arbitrary definition of current flow upward in that column.
  // Top of column connected to VMEM and bottom of column connected to GNDPWR.
  uint8_t CMMDSetCol[8][2] = {
    { PIN_MATRIX_DRIVE_Q3P, PIN_MATRIX_DRIVE_Q1N },  // Column 0
    { PIN_MATRIX_DRIVE_Q4P, PIN_MATRIX_DRIVE_Q1N },  // Column 1
    { PIN_MATRIX_DRIVE_Q5P, PIN_MATRIX_DRIVE_Q1N },  // Column 2
    { PIN_MATRIX_DRIVE_Q6P, PIN_MATRIX_DRIVE_Q1N },  // Column 3
    { PIN_MATRIX_DRIVE_Q3P, PIN_MATRIX_DRIVE_Q2N },  // Column 4
    { PIN_MATRIX_DRIVE_Q4P, PIN_MATRIX_DRIVE_Q2N },  // Column 5
    { PIN_MATRIX_DRIVE_Q5P, PIN_MATRIX_DRIVE_Q2N },  // Column 6
    { PIN_MATRIX_DRIVE_Q6P, PIN_MATRIX_DRIVE_Q2N }   // Column 7      
  };

  // Clear is given the arbitrary definition of current flow downward in that column.
  // Top of column connected to GNDPWR and bottom of column connected to VMEM.
  uint8_t CMMDClearCol[8][2] = {
    { PIN_MATRIX_DRIVE_Q3N, PIN_MATRIX_DRIVE_Q1P },  // Column 0
    { PIN_MATRIX_DRIVE_Q4N, PIN_MATRIX_DRIVE_Q1P },  // Column 1
    { PIN_MATRIX_DRIVE_Q5N, PIN_MATRIX_DRIVE_Q1P },  // Column 2
    { PIN_MATRIX_DRIVE_Q6N, PIN_MATRIX_DRIVE_Q1P },  // Column 3
    { PIN_MATRIX_DRIVE_Q3N, PIN_MATRIX_DRIVE_Q2P },  // Column 4
    { PIN_MATRIX_DRIVE_Q4N, PIN_MATRIX_DRIVE_Q2P },  // Column 5
    { PIN_MATRIX_DRIVE_Q5N, PIN_MATRIX_DRIVE_Q2P },  // Column 6
    { PIN_MATRIX_DRIVE_Q6N, PIN_MATRIX_DRIVE_Q2P }   // Column 7      
  };

  void Core_Driver_Setup() {
      pinMode(Pin_Sense_Pulse, INPUT_PULLUP);
      pinMode(Pin_Sense_Reset, OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q1P,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q1N,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q2P,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q2N,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q3P,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q3N,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q4P,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q4N,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q5P,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q5N,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q6P,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q6N,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q7P,  OUTPUT); // Shared pin 13. Onboard LED, Hearbeat. Return to previous state when finished using.
      pinMode(PIN_MATRIX_DRIVE_Q7N,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q8P,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q8N,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q9P,  OUTPUT); // Shared pin 17. LED Array. Return to previous state when finished using.
      pinMode(PIN_MATRIX_DRIVE_Q9N,  OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q10P, OUTPUT);
      pinMode(PIN_MATRIX_DRIVE_Q10N, OUTPUT);
      pinMode(PIN_WRITE_ENABLE, OUTPUT);
      pinMode(Pin_SAO_G1_SPARE_1_CP_ADDR_0, OUTPUT);
      pinMode(Pin_SAO_G2_SPARE_2_CP_ADDR_1, OUTPUT);
      #ifdef Pin_SPARE_3_Assigned_To_Spare_3_Output
        pinMode(Pin_SPARE_3_CP_ADDR_2, OUTPUT);
      #endif
      #ifdef Pin_SPARE_5_Assigned_To_Spare_5_Output
        pinMode(Pin_SPI_Reset_Spare_5, OUTPUT);
      #endif
      #ifdef Pin_Spare_4_IR_IN_Assigned_To_Spare_4_Output
        pinMode(Pin_Spare_4_IR_IN, OUTPUT);
      #endif

      #ifdef Pin_SAO_G1_SPARE_1_CP_ADDR_0_Assigned_To_CP_ADDR_0_Output
       pinMode(Pin_SAO_G1_SPARE_1_CP_ADDR_0, OUTPUT);
       #define CORE_PLANE_SELECT_ACTIVE
      #endif
      #ifdef Pin_SAO_G2_SPARE_2_CP_ADDR_1_Assigned_To_CP_ADDR_1_Output
       pinMode(Pin_SAO_G2_SPARE_2_CP_ADDR_1, OUTPUT);
       #define CORE_PLANE_SELECT_ACTIVE
      #endif
      #ifdef Pin_SPARE_3_CP_ADDR_2_Assigned_To_CP_ADDR_2_Output
        pinMode(Pin_SPARE_3_CP_ADDR_2, OUTPUT);
       #define CORE_PLANE_SELECT_ACTIVE
      #endif

      Serial.println();
      Serial.println("Core Pattern Arrangement. 1=Normal. 2=Opposite.");
      Serial.print("     Firmware default: ");
      Serial.println(CorePatternArrangement);
      CorePatternArrangement = EEPROMExtReadCorePatternArrangement();
      Serial.print("     EEPROM setting: ");
      Serial.print(CorePatternArrangement);
      Serial.println(" will be used.");
   
      // If the Core Pattern Arrangement is opposite, swap the pin matrix drive values between bits 0-31 with 32-63
      if(CorePatternArrangement == CORE_ARRANGEMENT_OPPOSITE) {
        uint8_t temp[1];
        for (uint8_t i = 0; i <= 31; i++) {
          temp[0] = CMMDSetRowByBit[i][0];
          temp[1] = CMMDSetRowByBit[i][1];
          CMMDSetRowByBit[i][0] = CMMDSetRowByBit[(32+i)][0];
          CMMDSetRowByBit[i][1] = CMMDSetRowByBit[(32+i)][1];
          CMMDSetRowByBit[(32+i)][0] = temp[0];
          CMMDSetRowByBit[(32+i)][1] = temp[1];

          temp[0] = CMMDClearRowByBit[i][0];
          temp[1] = CMMDClearRowByBit[i][1];
          CMMDClearRowByBit[i][0] = CMMDClearRowByBit[(32+i)][0];
          CMMDClearRowByBit[i][1] = CMMDClearRowByBit[(32+i)][1];
          CMMDClearRowByBit[(32+i)][0] = temp[0];
          CMMDClearRowByBit[(32+i)][1] = temp[1];
        }
      }
  }

  void Core_Plane_Select(uint8_t plane) {
    CorePlane = plane;
  }

  void Core_Plane_Set_Addr(uint8_t plane) {
    digitalWriteFast(Pin_SAO_G1_SPARE_1_CP_ADDR_0,  CorePlaneAddr [plane] [2] );
    digitalWriteFast(Pin_SAO_G2_SPARE_2_CP_ADDR_1,  CorePlaneAddr [plane] [1] );
    #ifdef Pin_SPARE_3_CP_ADDR_2
      digitalWriteFast(Pin_SPARE_3_CP_ADDR_2,         CorePlaneAddr [plane] [0] );
    #endif
  }

  void MatrixEnableTransistorInactive() { 
    digitalWriteFast(PIN_WRITE_ENABLE, WRITE_ENABLE_INACTIVE);
    #ifdef CORE_PLANE_SELECT_ACTIVE
      // No need to clear the Core Plane Addr
    #else
      // digitalWriteFast(Pin_SAO_G1_SPARE_1_CP_ADDR_0, 0); // Assume and activate Core Plane 1 for all testing now.
    #endif
  }

  void MatrixEnableTransistorActive()   { 
    #ifdef CORE_PLANE_SELECT_ACTIVE
      Core_Plane_Set_Addr(CorePlane);
    #else
      // digitalWriteFast(Pin_SAO_G1_SPARE_1_CP_ADDR_0, 1); // Assume and activate Core Plane 1 for all testing now.
    #endif
    digitalWriteFast(PIN_WRITE_ENABLE, WRITE_ENABLE_ACTIVE);
  }

  void MatrixDriveTransistorsInactive() {
    // Set all the matrix lines to the safe state, all transistors inactive.
    for (uint8_t i = 2; i <= 9; i++) {
      digitalWriteFast(MatrixDrivePinNumber[i], MatrixDrivePinInactiveState[i]);
    }
    for (uint8_t i = 16; i <= 17; i++) {
      digitalWriteFast(MatrixDrivePinNumber[i], MatrixDrivePinInactiveState[i]);
    }
    for (uint8_t i = 24; i <= 33; i++) {
      digitalWriteFast(MatrixDrivePinNumber[i], MatrixDrivePinInactiveState[i]);
    }
  }

  void ReturnMatrixQ9NtoLowForLEDArray() {
    // if (HardwareVersionMinor == 2) { digitalWriteFast(PIN_MATRIX_DRIVE_Q9P, 0); }
  }

  // Configure four transistors to activate the specified core.
  void SetRowAndCol (uint8_t row, uint8_t col) {
    // decode bit # from row and col data to resolve the correct row drive polarity
    uint8_t bit = col + (row*8);
      digitalWriteFast( (CMMDSetRowByBit[bit] [0] ), MatrixDrivePinActiveState[ CMMDSetRowByBit[bit] [0] ] );
      delayMicroseconds(1); 
    TracingPulses(1);
      digitalWriteFast( (CMMDSetRowByBit[bit] [1] ), MatrixDrivePinActiveState[ CMMDSetRowByBit[bit] [1] ] );
      // Use col to select the proper place in the look up table
      // columns are easier to decode with the simpler CMMDSetCol look-up table.
      delayMicroseconds(1); 
    TracingPulses(1);
      digitalWriteFast( (CMMDSetCol[col] [0] ), MatrixDrivePinActiveState[ CMMDSetCol[col] [0] ] );
      delayMicroseconds(1); 
    TracingPulses(1);
      digitalWriteFast( (CMMDSetCol[col] [1] ), MatrixDrivePinActiveState[ CMMDSetCol[col] [1] ] );
      delayMicroseconds(1); 
    TracingPulses(1);
  }

  // Use col to selection the proper place in the look up table
  void ClearRowAndCol (uint8_t row, uint8_t col) {
    // decode bit # from row and col data to resolve the correct row drive polarity
    uint8_t bit = col + (row*8);
      digitalWriteFast( (CMMDClearRowByBit[bit] [0] ), MatrixDrivePinActiveState[ CMMDClearRowByBit[bit] [0] ] ); // for bit 0, pin 
      delayMicroseconds(1); 
    TracingPulses(1);
      digitalWriteFast( (CMMDClearRowByBit[bit] [1] ), MatrixDrivePinActiveState[ CMMDClearRowByBit[bit] [1] ] ); // for bit 0, pin 
      delayMicroseconds(1); 
    TracingPulses(1);
      // columns are easier to decode with the simpler CMMDSetCol look-up table.
      digitalWriteFast( (CMMDClearCol[col] [0] ), MatrixDrivePinActiveState[ CMMDClearCol[col] [0] ] ); // for bit 0, pin 
      delayMicroseconds(1); 
    TracingPulses(1);
      digitalWriteFast( (CMMDClearCol[col] [1] ), MatrixDrivePinActiveState[ CMMDClearCol[col] [1] ] ); // for bit 0, pin    
      delayMicroseconds(1); 
    TracingPulses(1);
  }

  void ClearRowZeroAndColZero () {
    digitalWriteFast( (PIN_MATRIX_DRIVE_Q7P), 0 ); // for bit 0, row 0 YL0 to VMEM
    digitalWriteFast( (PIN_MATRIX_DRIVE_Q9N), 1 ); // for bit 0, row 0 YL4 to GND
    digitalWriteFast( (PIN_MATRIX_DRIVE_Q3N), 1 ); // for bit 0, col 0 XT0 to GND
    digitalWriteFast( (PIN_MATRIX_DRIVE_Q1P), 0 ); // for bit 0, col 0 XB0 to VMEM
  }

  void SetRowZeroAndColZero () {
    digitalWriteFast( (PIN_MATRIX_DRIVE_Q7N), 1 ); // for bit 0, row 0 YL0 to GND
    digitalWriteFast( (PIN_MATRIX_DRIVE_Q9P), 0 ); // for bit 0, row 0 YL4 to VMEM
    digitalWriteFast( (PIN_MATRIX_DRIVE_Q3P), 0 ); // for bit 0, col 0 XT0 to VMEM
    digitalWriteFast( (PIN_MATRIX_DRIVE_Q1N), 1 ); // for bit 0, col 0 XB0 to GND
  }

  void CoreSenseReset() {
      digitalWriteFast( Pin_Sense_Reset, 1);
      // Teensy 3.2 either runs too fast or optimizes out the pulses if this delay is not included.
      // The delay is also useful to see the pulse on the scope.
      delayMicroseconds(1); 
      digitalWriteFast( Pin_Sense_Reset, 0);    
  }

  bool SenseWirePulse() {
    bool temp = 0;
    temp = digitalReadFast(Pin_Sense_Pulse);
    // TracingPulses(temp);
    return temp;
  }

  void DebugIOESpare1_On() {
    digitalWriteFast(Pin_SAO_G1_SPARE_1_CP_ADDR_0, 1);
  }

  void DebugIOESpare1_Off() {
    digitalWriteFast(Pin_SAO_G1_SPARE_1_CP_ADDR_0, 0);
  }

  void DebugIOESpare2_On() {
    digitalWriteFast(Pin_SAO_G2_SPARE_2_CP_ADDR_1, 1);
  }

  void DebugIOESpare2_Off() {
    digitalWriteFast(Pin_SAO_G2_SPARE_2_CP_ADDR_1, 0);
  }

  void DebugPin14_On() {
    #ifdef Pin_SPARE_3_Assigned_To_Spare_3_Output
    digitalWriteFast(Pin_SPARE_3_CP_ADDR_2, 1);
    #endif
  }

  void DebugPin10_On() {
    #ifdef Pin_Spare_4_IR_IN_Assigned_To_Spare_4_Output
    digitalWriteFast(Pin_Spare_4_IR_IN, 1);
    #endif
  }

  void DebugPin10_Off() {
    #ifdef Pin_Spare_4_IR_IN_Assigned_To_Spare_4_Output
    digitalWriteFast(Pin_Spare_4_IR_IN, 0);
    #endif
  }

  void DebugPin14_Off() {
    #ifdef Pin_SPARE_3_Assigned_To_Spare_3_Output
    digitalWriteFast(Pin_SPARE_3_CP_ADDR_2, 0);
    #endif
  }

  void DebugPin15_On() {
    #ifdef Pin_SPARE_5_Assigned_To_Spare_5_Output
    digitalWriteFast(Pin_SPI_Reset_Spare_5, 1);
    #endif
  }

  void DebugPin15_Off() {
    #ifdef Pin_SPARE_5_Assigned_To_Spare_5_Output
    digitalWriteFast(Pin_SPI_Reset_Spare_5, 0);
    #endif
  }

  void TracingPulses(uint8_t numberOfPulses) {
    for (uint8_t i = 1; i <= numberOfPulses; i++) {
      DebugPin10_On();
      DebugPin14_On();
      DebugPin15_On();
      // Teensy 3.2 either runs too fast or optimizes out multiple pulses if this delay is not included.
      // The delay is also useful to see the # of traces pulses on the scope or they are too narrow.
      delayMicroseconds(1); 
      DebugPin10_Off();
      DebugPin14_Off();
      DebugPin15_Off();
    }
  }

#elif defined BOARD_CORE64C_RASPI_PICO

#endif
