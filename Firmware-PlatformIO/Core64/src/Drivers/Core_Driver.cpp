#include <stdint.h>
#include <stdbool.h>

/*
#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
*/

#include "Config/HardwareIOMap.h"

#include "Drivers/Core_Driver.h"
#include "SubSystems/I2C_Manager.h"
#include "Hal/EEPROM_HAL.h"
#include "SubSystems/Serial_Port.h"
#include "Hal/Debug_Pins_HAL.h"

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

static uint8_t CorePlane = 1;           // Default Core Plane if it is not specified by the user application.
#ifdef MULTIPLE_CORE_PLANES_ENABLED
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
#endif


#if defined  MCU_TYPE_MK20DX256_TEENSY_32
  // Array from 1-20 with MCU pin # associated to verbose transistor drive line name. Ex: PIN_MATRIX_DRIVE_Q1P
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
      Serial.println("  Core Pattern Arrangement. 1=Normal. 2=Opposite.");
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
    #ifdef Pin_SAO_G1_SPARE_1_CP_ADDR_0_Assigned_To_CP_ADDR_0_Output
      digitalWriteFast(Pin_SAO_G1_SPARE_1_CP_ADDR_0,  CorePlaneAddr [plane] [2] );
    #endif
    #ifdef Pin_SAO_G2_SPARE_2_CP_ADDR_1_Assigned_To_CP_ADDR_1_Output
      digitalWriteFast(Pin_SAO_G2_SPARE_2_CP_ADDR_1,  CorePlaneAddr [plane] [1] );
    #endif
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

  // TODO: This is from Core64 V0.2.0 and may not be needed any more?
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

    #ifdef Pin_SAO_G1_SPARE_1_CP_ADDR_0_Assigned_To_CP_ADDR_0_Output
    #endif
    #ifdef Pin_SAO_G2_SPARE_2_CP_ADDR_1_Assigned_To_CP_ADDR_1_Output
    #endif


  void DebugIOESpare1_On() {
    #ifdef Pin_SAO_G1_SPARE_1_CP_ADDR_0_Assigned_To_CP_ADDR_0_Output
      digitalWriteFast(Pin_SAO_G1_SPARE_1_CP_ADDR_0, 1);
    #endif
  }

  void DebugIOESpare1_Off() {
    #ifdef Pin_SAO_G1_SPARE_1_CP_ADDR_0_Assigned_To_CP_ADDR_0_Output
      digitalWriteFast(Pin_SAO_G1_SPARE_1_CP_ADDR_0, 0);
    #endif
  }

  void DebugIOESpare2_On() {
    #ifdef Pin_SAO_G2_SPARE_2_CP_ADDR_1_Assigned_To_CP_ADDR_1_Output
      digitalWriteFast(Pin_SAO_G2_SPARE_2_CP_ADDR_1, 1);
    #endif
  }

  void DebugIOESpare2_Off() {
    #ifdef Pin_SAO_G2_SPARE_2_CP_ADDR_1_Assigned_To_CP_ADDR_1_Output
      digitalWriteFast(Pin_SAO_G2_SPARE_2_CP_ADDR_1, 0);
    #endif
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

#elif defined MCU_TYPE_RP2040

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Beginning of Core16 Pico Core Driver Variables
// In order to support to support the Core16 with a Pico at runtime, all of these variables will exist all of the time.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Array length starts at 0 and goes all the way up to the highest MCU GPIO PIN # used by the Core Matrix Drive Transistors 
  // None of these numbers correspond to the external microcontroller carrier board pin numbering.
  // The array position # corresponds with the Arduino-compatible MCU GPIO #.
  // The array position will be filled with the Arduino-compatible MCU pin # associated to verbose transistor drive line name.
  // Example: Array position #2 is set to PIN_MATRIX_DRIVE_Q1P, and #define PIN_MATRIX_DRIVE_Q1P = 2.
  // Matrix Drive Line array position :         0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19
  // Core16 Pico V0.1.0 MCU PIN #     :       N/A, 1, 2, 3, 4, 5, 6, 7, 8, 9, -, -,12,13,14,15,16, -,18,19
  // Core16 Pico does not have or use Q2P/N or Q8P/N
  static uint8_t C16P_MatrixDrivePinNumber[20] = {
    0,                                // 0
    C16P_PIN_MATRIX_DRIVE_Q1P ,       // 1
    C16P_PIN_MATRIX_DRIVE_Q1N ,       // 2
    C16P_PIN_MATRIX_DRIVE_Q3P ,       // 3
    C16P_PIN_MATRIX_DRIVE_Q3N ,       // 4
    C16P_PIN_MATRIX_DRIVE_Q4P ,       // 5
    C16P_PIN_MATRIX_DRIVE_Q4N ,       // 6
    C16P_PIN_MATRIX_DRIVE_Q5P ,       // 7
    C16P_PIN_MATRIX_DRIVE_Q5N ,       // 8
    C16P_PIN_MATRIX_DRIVE_Q6P ,       // 9
    0,                                // 10
    0,                                // 11
    C16P_PIN_MATRIX_DRIVE_Q6N ,       // 12
    C16P_PIN_MATRIX_DRIVE_Q7P ,       // 13
    C16P_PIN_MATRIX_DRIVE_Q7N ,       // 14
    C16P_PIN_MATRIX_DRIVE_Q9P ,       // 15
    C16P_PIN_MATRIX_DRIVE_Q9N ,       // 16
    0,                                // 17
    C16P_PIN_MATRIX_DRIVE_Q10P,       // 18
    C16P_PIN_MATRIX_DRIVE_Q10N        // 19
  };

  // All QxN transistors are Active High.
  // All QxP transistors are Active Low.

  // V0.4 hardware (direct MCU pin control)
  // Look up the drive line number by knowing where it is in the array by it's own number.
  // In other words, the pin # has to be the same as the array position.
  // Array needs to be as big as the largest used pin number.
  // Ex:
  //                                  array position:  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19
  // Core16 Pico MCU PIN #            usable pins   :  x, 1, 2, 3, 4, 5, 6, 7, 8, 9, x, x,12,13,14,15,16, x,18,19
  // MCU output pins are set to these states to correspond to activation of the transistor needed to achieve on/off state.
  const bool C16P_MatrixDrivePinInactiveState[34] =  { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0}; // logic level to turn off transistor
  const bool C16P_MatrixDrivePinActiveState[34]   =  { 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1}; // logic level to turn on transistor

  // MCU output pin is set to these states to correspond to activation of the transistor needed to achieve active/inactive state.
  #define C16P_WRITE_ENABLE_ACTIVE   1 // logic level to turn on transistor
  #define C16P_WRITE_ENABLE_INACTIVE 0 // logic level to turn off transistor

  // Given a Core Memory Matrix Row 0 to 7 the array below specifies which 2 pins connected to transistors are required to set the row.
  // CMM front (user) view is with Row 0 on top, 7 on bottom.
  // Each row of the array corresponds to rows 0 to 7 of the CMM.
  // Each row is sequence of 2 transistors, first one connects to top four rows and second one connects to the bottom four rows.
  /* 
  The original assumption of current going left to right in a row does not work because the cores
  are not all placed in the same orientation. The cores alternate back and forth in a row, and in 
  columns, to make the circuit simpler. A new row set and clear array are required which take into 
  account that every other bit needs to have the current direction reversed in order to compensate 
  if all of the cores are to be physically addressed in an orderly sequence. 
  */
  // V0.1.x and V0.2.x and V0.4.x hardware (direct MCU pin control)

  static uint8_t C16P_CMMDSetRowByBit[][2] = {
  //  { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit  0    ROW 0
  //  { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit  1    ROW 0
  //  { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit  2    ROW 0
  //  { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit  3    ROW 0
  //  { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit  4    ROW 0
  //  { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit  5    ROW 0
  //  { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit  6    ROW 0
  //  { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit  7    ROW 0

    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit  1    ROW 0   YL0+ YL1+ YR0- YR3 open 
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit  0    ROW 0   
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit  3    ROW 0
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit  2    ROW 0   

    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q10P },  // Bit  9    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q10N },  // Bit  8    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q10P },  // Bit 11    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q10N },  // Bit 10    ROW 1

    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit  4    ROW 0
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit  5    ROW 0
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit  6    ROW 0
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit  7    ROW 0

    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q10N },  // Bit 12    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q10P },  // Bit 13    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q10N },  // Bit 14    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q10P },  // Bit 15    ROW 1
  }; 

  static uint8_t C16P_CMMDClearRowByBit[][2] = {
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit 1     ROW 0
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit 0     ROW 0
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit 3     ROW 0
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit 2     ROW 0

    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q10N },  // Bit  9    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q10P },  // Bit  8    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q10N },  // Bit 11    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q10P },  // Bit 10    ROW 1

    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit 4     ROW 0
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit 5     ROW 0
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q9N  },  // Bit 6     ROW 0
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q9P  },  // Bit 7     ROW 0      

    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q10P },  // Bit 12    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q10N },  // Bit 13    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7N , C16P_PIN_MATRIX_DRIVE_Q10P },  // Bit 14    ROW 1
    { C16P_PIN_MATRIX_DRIVE_Q7P , C16P_PIN_MATRIX_DRIVE_Q10N },  // Bit 15    ROW 1
  };

  // CMMD = Core Memory Matrix Drive
  // Given a Core Memory Matrix Column 0 to 7 the array below specifies which 2 pins connected to transistors are required to set the column.
  // CMM front (user) view is with Column 0 on left, 7 on right.
  // Each row of the array corresponds to columns 0 to 7 of the CMM.
  // Each row is sequence of 2 transitors, first one is at the top and second one is at the bottom.

  // Set is given the arbitrary definition of current flow upward in that column.
  // Top of column connected to VMEM and bottom of column connected to GNDPWR.
  uint8_t C16P_CMMDSetCol[4][2] = {
    { C16P_PIN_MATRIX_DRIVE_Q3P, C16P_PIN_MATRIX_DRIVE_Q1N },  // Column 0
    { C16P_PIN_MATRIX_DRIVE_Q4P, C16P_PIN_MATRIX_DRIVE_Q1N },  // Column 1
    { C16P_PIN_MATRIX_DRIVE_Q5P, C16P_PIN_MATRIX_DRIVE_Q1N },  // Column 2
    { C16P_PIN_MATRIX_DRIVE_Q6P, C16P_PIN_MATRIX_DRIVE_Q1N },  // Column 3
  };

  // Clear is given the arbitrary definition of current flow downward in that column.
  // Top of column connected to GNDPWR and bottom of column connected to VMEM.
  uint8_t C16P_CMMDClearCol[4][2] = {
    { C16P_PIN_MATRIX_DRIVE_Q3N, C16P_PIN_MATRIX_DRIVE_Q1P },  // Column 0
    { C16P_PIN_MATRIX_DRIVE_Q4N, C16P_PIN_MATRIX_DRIVE_Q1P },  // Column 1
    { C16P_PIN_MATRIX_DRIVE_Q5N, C16P_PIN_MATRIX_DRIVE_Q1P },  // Column 2
    { C16P_PIN_MATRIX_DRIVE_Q6N, C16P_PIN_MATRIX_DRIVE_Q1P },  // Column 3
  };
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End of Core16 Pico Variables
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    // Core64c V0.3. hardware (CMMD controlled through 3 shift registers)

    // All QxN transistors are Active High. The Inactive state is LOW.
    // All QxP transistors are Active Low. The Inactive state is HIGH.

    // Bit field for all of the Core Memory Matrix Drive (CMMD) lines to the safe state, all transistors inactive.
    //                                                Q10P                Q1N
    //                                               HGFE|
    //                                       xxxxxxxx    |
    // bit #                                32     24  20| 16       8       0
    //                                       |      |   ||  |       |       |
    uint32_t CMMDTransistorInactiveState = 0b00000000000010101010101010101010;
    uint32_t CMMDTransistorActiveState =   0b00000000000001010101010101010101;    // Useful for testing as long as Write_Enable is not active!

  // MCU output pin is set to these states to correspond to activation of the transistor needed to achieve active/inactive state.
  #define WRITE_ENABLE_ACTIVE   1 // logic level to turn on transistor
  #define WRITE_ENABLE_INACTIVE 0 // logic level to turn off transistor

  static uint32_t CMMDSetBit[64] = {
    // Each TOP and BOTTOM column sequence is the same for each group of 8 bits in a row.
    //              ROWS____TOP_____BOTTOM
    //              QQQQQQQQQQQQQQQQQQQQ
    //              11                     
    //          HGFE00998877665544332211
    //  xxxxxxxx    PNPNPNPNPNPNPNPNPNPN
      0b00000000000000010010000000100001,  // BIT  0    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 32    ROW 4 P/N swapped from ROW 0  
      0b00000000000000100001000010000001,  // BIT  1    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 33    ROW 4   
      0b00000000000000010010001000000001,  // BIT  2    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 34    ROW 4  
      0b00000000000000100001100000000001,  // BIT  3    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 35    ROW 4  
      0b00000000000000010010000000100100,  // BIT  4    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 36    ROW 4  
      0b00000000000000100001000010000100,  // BIT  5    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 37    ROW 4   
      0b00000000000000010010001000000100,  // BIT  6    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 38    ROW 4  
      0b00000000000000100001100000000100,  // BIT  7    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 39    ROW 4  
                                         
      0b00000000000010000001000000100001,  // BIT  8    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 40    ROW 5 P/N swapped from ROW 1
      0b00000000000001000010000010000001,  // BIT  9    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 41    ROW 5
      0b00000000000010000001001000000001,  // BIT 10    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 42    ROW 5
      0b00000000000001000010100000000001,  // BIT 11    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 43    ROW 5
      0b00000000000010000001000000100100,  // BIT 12    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 44    ROW 5
      0b00000000000001000010000010000100,  // BIT 13    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 45    ROW 5
      0b00000000000010000001001000000100,  // BIT 14    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 46    ROW 5
      0b00000000000001000010100000000100,  // BIT 15    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 47    ROW 5
                                         
      0b00000000000000011000000000100001,  // BIT 16    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 48    ROW 6 P/N swapped from ROW 2
      0b00000000000000100100000010000001,  // BIT 17    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 49    ROW 6 
      0b00000000000000011000001000000001,  // BIT 18    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 50    ROW 6 
      0b00000000000000100100100000000001,  // BIT 19    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 51    ROW 6 
      0b00000000000000011000000000100100,  // BIT 20    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 52    ROW 6 
      0b00000000000000100100000010000100,  // BIT 21    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 53    ROW 6 
      0b00000000000000011000001000000100,  // BIT 22    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 54    ROW 6 
      0b00000000000000100100100000000100,  // BIT 23    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 55    ROW 6 
                                         
      0b00000000000010000100000000100001,  // BIT 24    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 56    ROW 7 P/N swapped from ROW 3
      0b00000000000001001000000010000001,  // BIT 25    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 57    ROW 7
      0b00000000000010000100001000000001,  // BIT 26    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 58    ROW 7
      0b00000000000001001000100000000001,  // BIT 27    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 59    ROW 7
      0b00000000000010000100000000100100,  // BIT 28    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 60    ROW 7
      0b00000000000001001000000010000100,  // BIT 29    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 61    ROW 7
      0b00000000000010000100001000000100,  // BIT 30    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 62    ROW 7
      0b00000000000001001000100000000100,  // BIT 31    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 63    ROW 7
                                         
      0b00000000000000100001000000100001,  // BIT 32    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit  0    ROW 0
      0b00000000000000010010000010000001,  // BIT 33    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit  1    ROW 0
      0b00000000000000100001001000000001,  // BIT 34    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit  2    ROW 0
      0b00000000000000010010100000000001,  // BIT 35    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit  3    ROW 0
      0b00000000000000100001000000100100,  // BIT 36    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit  4    ROW 0
      0b00000000000000010010000010000100,  // BIT 37    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit  5    ROW 0
      0b00000000000000100001001000000100,  // BIT 38    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit  6    ROW 0
      0b00000000000000010010100000000100,  // BIT 39    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit  7    ROW 0
                                         
      0b00000000000001000010000000100001,  // BIT 40    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit  8    ROW 1
      0b00000000000010000001000010000001,  // BIT 41    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit  9    ROW 1
      0b00000000000001000010001000000001,  // BIT 42    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 10    ROW 1
      0b00000000000010000001100000000001,  // BIT 43    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 11    ROW 1
      0b00000000000001000010000000100100,  // BIT 44    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 12    ROW 1
      0b00000000000010000001000010000100,  // BIT 45    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 13    ROW 1
      0b00000000000001000010001000000100,  // BIT 46    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 14    ROW 1
      0b00000000000010000001100000000100,  // BIT 47    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 15    ROW 1
                                         
      0b00000000000000100100000000100001,  // BIT 48    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 16    ROW 2
      0b00000000000000011000000010000001,  // BIT 49    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 17    ROW 2
      0b00000000000000100100001000000001,  // BIT 50    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 18    ROW 2
      0b00000000000000011000100000000001,  // BIT 51    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 19    ROW 2
      0b00000000000000100100000000100100,  // BIT 52    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 20    ROW 2
      0b00000000000000011000000010000100,  // BIT 53    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 21    ROW 2
      0b00000000000000100100001000000100,  // BIT 54    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 22    ROW 2
      0b00000000000000011000100000000100,  // BIT 55    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 23    ROW 2
                                         
      0b00000000000001001000000000100001,  // BIT 56    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 24    ROW 3
      0b00000000000010000100000010000001,  // BIT 57    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 25    ROW 3
      0b00000000000001001000001000000001,  // BIT 58    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 26    ROW 3
      0b00000000000010000100100000000001,  // BIT 59    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 27    ROW 3
      0b00000000000001001000000000100100,  // BIT 60    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 28    ROW 3
      0b00000000000010000100000010000100,  // BIT 61    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 29    ROW 3
      0b00000000000001001000001000000100,  // BIT 62    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 30    ROW 3
      0b00000000000010000100100000000100   // BIT 63    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P }   // Bit 31    ROW 3
  }; 
      // Set is given the arbitrary definition of current flow upward in that column.
      // Top of column connected to VMEM and bottom of column connected to GNDPWR.
      //  uint8_t CMMDSetCol[8][2] = {
      //    { PIN_MATRIX_DRIVE_Q3P, PIN_MATRIX_DRIVE_Q1N },  // Column 0
      //    { PIN_MATRIX_DRIVE_Q4P, PIN_MATRIX_DRIVE_Q1N },  // Column 1
      //    { PIN_MATRIX_DRIVE_Q5P, PIN_MATRIX_DRIVE_Q1N },  // Column 2
      //    { PIN_MATRIX_DRIVE_Q6P, PIN_MATRIX_DRIVE_Q1N },  // Column 3
      //    { PIN_MATRIX_DRIVE_Q3P, PIN_MATRIX_DRIVE_Q2N },  // Column 4
      //    { PIN_MATRIX_DRIVE_Q4P, PIN_MATRIX_DRIVE_Q2N },  // Column 5
      //    { PIN_MATRIX_DRIVE_Q5P, PIN_MATRIX_DRIVE_Q2N },  // Column 6
      //    { PIN_MATRIX_DRIVE_Q6P, PIN_MATRIX_DRIVE_Q2N }   // Column 7      
      //  };

  static uint32_t CMMDClearBit[64] = {
    // Each TOP and BOTTOM column sequence is the same for each group of 8 bits in a row.
    //              ROWS____TOP_____BOTTOM
    //              QQQQQQQQQQQQQQQQQQQQ
    //              11                     
    //          HGFE00998877665544332211
    //  xxxxxxxx    PNPNPNPNPNPNPNPNPNPN
      0b00000000000000100001000000010010,  // BIT  0    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 32    ROW 4 P/N swapped from ROW 0
      0b00000000000000010010000001000010,  // BIT  1    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 33    ROW 4
      0b00000000000000100001000100000010,  // BIT  2    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 34    ROW 4
      0b00000000000000010010010000000010,  // BIT  3    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 35    ROW 4
      0b00000000000000100001000000011000,  // BIT  4    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 36    ROW 4
      0b00000000000000010010000001001000,  // BIT  5    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 37    ROW 4
      0b00000000000000100001000100001000,  // BIT  6    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 38    ROW 4
      0b00000000000000010010010000001000,  // BIT  7    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 39    ROW 4

      0b00000000000001000010000000010010,  // BIT  8    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 40    ROW 5 P/N swapped from ROW 1
      0b00000000000010000001000001000010,  // BIT  9    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 41    ROW 5
      0b00000000000001000010000100000010,  // BIT 10    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 42    ROW 5
      0b00000000000010000001010000000010,  // BIT 11    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 43    ROW 5
      0b00000000000001000010000000011000,  // BIT 12    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 44    ROW 5
      0b00000000000010000001000001001000,  // BIT 13    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 45    ROW 5
      0b00000000000001000010000100001000,  // BIT 14    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 46    ROW 5
      0b00000000000010000001010000001000,  // BIT 15    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 47    ROW 5

      0b00000000000000100100000000010010,  // BIT 16    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 48    ROW 6 P/N swapped from ROW 2
      0b00000000000000011000000001000010,  // BIT 17    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 49    ROW 6
      0b00000000000000100100000100000010,  // BIT 18    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 50    ROW 6
      0b00000000000000011000010000000010,  // BIT 19    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 51    ROW 6
      0b00000000000000100100000000011000,  // BIT 20    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 52    ROW 6
      0b00000000000000011000000001001000,  // BIT 21    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 53    ROW 6
      0b00000000000000100100000100001000,  // BIT 22    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 54    ROW 6
      0b00000000000000011000010000001000,  // BIT 23    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 55    ROW 6

      0b00000000000001001000000000010010,  // BIT 24    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 56    ROW 7 P/N swapped from ROW 3
      0b00000000000010000100000001000010,  // BIT 25    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 57    ROW 7
      0b00000000000001001000000100000010,  // BIT 26    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 58    ROW 7
      0b00000000000010000100010000000010,  // BIT 27    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 59    ROW 7
      0b00000000000001001000000000011000,  // BIT 28    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 60    ROW 7
      0b00000000000010000100000001001000,  // BIT 29    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 61    ROW 7
      0b00000000000001001000000100001000,  // BIT 30    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 62    ROW 7
      0b00000000000010000100010000001000,  // BIT 31    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 63    ROW 7

      0b00000000000000010010000000010010,  // BIT 33    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 0     ROW 0
      0b00000000000000100001000001000010,  // BIT 33    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 1     ROW 0
      0b00000000000000010010000100000010,  // BIT 34    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 2     ROW 0
      0b00000000000000100001010000000010,  // BIT 35    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 3     ROW 0
      0b00000000000000010010000000011000,  // BIT 36    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 4     ROW 0
      0b00000000000000100001000001001000,  // BIT 37    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 5     ROW 0
      0b00000000000000010010000100001000,  // BIT 38    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 6     ROW 0
      0b00000000000000100001010000001000,  // BIT 39    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 7     ROW 0

      0b00000000000010000001000000010010,  // BIT 40    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit  8    ROW 1
      0b00000000000001000010000001000010,  // BIT 41    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit  9    ROW 1
      0b00000000000010000001000100000010,  // BIT 42    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 10    ROW 1
      0b00000000000001000010010000000010,  // BIT 43    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 11    ROW 1
      0b00000000000010000001000000011000,  // BIT 44    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 12    ROW 1
      0b00000000000001000010000001001000,  // BIT 45    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 13    ROW 1
      0b00000000000010000001000100001000,  // BIT 46    //  { PIN_MATRIX_DRIVE_Q7N , PIN_MATRIX_DRIVE_Q10P },  // Bit 14    ROW 1
      0b00000000000001000010010000001000,  // BIT 47    //  { PIN_MATRIX_DRIVE_Q7P , PIN_MATRIX_DRIVE_Q10N },  // Bit 15    ROW 1

      0b00000000000000011000000000010010,  // BIT 48    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 16    ROW 2 
      0b00000000000000100100000001000010,  // BIT 49    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 17    ROW 2 
      0b00000000000000011000000100000010,  // BIT 50    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 18    ROW 2 
      0b00000000000000100100010000000010,  // BIT 51    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 19    ROW 2 
      0b00000000000000011000000000011000,  // BIT 52    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 20    ROW 2 
      0b00000000000000100100000001001000,  // BIT 53    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 21    ROW 2 
      0b00000000000000011000000100001000,  // BIT 54    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q9N  },  // Bit 22    ROW 2 
      0b00000000000000100100010000001000,  // BIT 55    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q9P  },  // Bit 23    ROW 2 

      0b00000000000010000100000000010010,  // BIT 56    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 24    ROW 3  
      0b00000000000001001000000001000010,  // BIT 57    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 25    ROW 3  
      0b00000000000010000100000100000010,  // BIT 58    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 26    ROW 3  
      0b00000000000001001000010000000010,  // BIT 59    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 27    ROW 3  
      0b00000000000010000100000000011000,  // BIT 60    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 28    ROW 3  
      0b00000000000001001000000001001000,  // BIT 61    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N },  // Bit 29    ROW 3  
      0b00000000000010000100000100001000,  // BIT 62    //  { PIN_MATRIX_DRIVE_Q8N , PIN_MATRIX_DRIVE_Q10P },  // Bit 30    ROW 3  
      0b00000000000001001000010000001000   // BIT 63    //  { PIN_MATRIX_DRIVE_Q8P , PIN_MATRIX_DRIVE_Q10N }   // Bit 31    ROW 3  
  };
      // Clear is given the arbitrary definition of current flow downward in that column.
      // Top of column connected to GNDPWR and bottom of column connected to VMEM.
      //  uint8_t CMMDClearCol[8][2] = {
      //    { PIN_MATRIX_DRIVE_Q3N, PIN_MATRIX_DRIVE_Q1P },  // Column 0
      //    { PIN_MATRIX_DRIVE_Q4N, PIN_MATRIX_DRIVE_Q1P },  // Column 1
      //    { PIN_MATRIX_DRIVE_Q5N, PIN_MATRIX_DRIVE_Q1P },  // Column 2
      //    { PIN_MATRIX_DRIVE_Q6N, PIN_MATRIX_DRIVE_Q1P },  // Column 3
      //    { PIN_MATRIX_DRIVE_Q3N, PIN_MATRIX_DRIVE_Q2P },  // Column 4
      //    { PIN_MATRIX_DRIVE_Q4N, PIN_MATRIX_DRIVE_Q2P },  // Column 5
      //    { PIN_MATRIX_DRIVE_Q5N, PIN_MATRIX_DRIVE_Q2P },  // Column 6
      //    { PIN_MATRIX_DRIVE_Q6N, PIN_MATRIX_DRIVE_Q2P }   // Column 7      
      //  };

  void Core_Driver_Setup() {
      if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
        pinMode(C16P_PIN_SENSE_PULSE, INPUT_PULLUP);
        pinMode(C16P_PIN_SENSE_RESET, OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q1P,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q1N,  OUTPUT);
        // pinMode(C16P_PIN_MATRIX_DRIVE_Q2P,  OUTPUT); // Not available or needed.
        // pinMode(C16P_PIN_MATRIX_DRIVE_Q2N,  OUTPUT); // Not available or needed.
        pinMode(C16P_PIN_MATRIX_DRIVE_Q3P,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q3N,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q4P,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q4N,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q5P,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q5N,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q6P,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q6N,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q7P,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q7N,  OUTPUT);
        // pinMode(C16P_PIN_MATRIX_DRIVE_Q8P,  OUTPUT); // Not available or needed.
        // pinMode(C16P_PIN_MATRIX_DRIVE_Q8N,  OUTPUT); // Not available or needed.
        pinMode(C16P_PIN_MATRIX_DRIVE_Q9P,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q9N,  OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q10P, OUTPUT);
        pinMode(C16P_PIN_MATRIX_DRIVE_Q10N, OUTPUT);
        pinMode(C16P_PIN_WRITE_ENABLE, OUTPUT);
      }
      else {
        pinMode(Pin_Sense_Pulse  , INPUT_PULLUP);
        pinMode(Pin_Sense_Reset  , OUTPUT);
        pinMode(PIN_WRITE_ENABLE , OUTPUT);
        pinMode(PIN_CMD_SR_LATCH , OUTPUT);
        pinMode(PIN_CMD_SR_SERIAL, OUTPUT);
        pinMode(PIN_CMD_SR_CLOCK , OUTPUT);        
      }

      Serial.println();
      Serial.println("  Core Pattern Arrangement. 1=Normal. 2=Opposite.");
      Serial.print("     Firmware default: ");
      Serial.println(CorePatternArrangement);
      CorePatternArrangement = EEPROMExtReadCorePatternArrangement();
      Serial.print("     EEPROM setting: ");
      Serial.print(CorePatternArrangement);
      Serial.println(" will be used.");

      MatrixDriveTransistorsInactive();

      // If the Core Pattern Arrangement is opposite, swap the pin matrix drive values between bits 0-31 with 32-63
      // if(CorePatternArrangement == CORE_ARRANGEMENT_OPPOSITE) {
      // uint8_t temp[1];
      // for (uint8_t i = 0; i <= 7; i++) {
      //   temp[0] = C16P_CMMDSetRowByBit[i][0];
      //   temp[1] = C16P_CMMDSetRowByBit[i][1];
      //   C16P_CMMDSetRowByBit[i][0] = C16P_CMMDSetRowByBit[(7+i)][0];
      //   C16P_CMMDSetRowByBit[i][1] = C16P_CMMDSetRowByBit[(7+i)][1];
      //   C16P_CMMDSetRowByBit[(7+i)][0] = temp[0];
      //   C16P_CMMDSetRowByBit[(7+i)][1] = temp[1];
      //
      //   temp[0] = C16P_CMMDClearRowByBit[i][0];
      //   temp[1] = C16P_CMMDClearRowByBit[i][1];
      //   C16P_CMMDClearRowByBit[i][0] = C16P_CMMDClearRowByBit[(7+i)][0];
      //   C16P_CMMDClearRowByBit[i][1] = C16P_CMMDClearRowByBit[(7+i)][1];
      //   C16P_CMMDClearRowByBit[(7+i)][0] = temp[0];
      //   C16P_CMMDClearRowByBit[(7+i)][1] = temp[1];
      // }
      // }
  }

  void Core_Plane_Select(uint8_t plane) {
    CorePlane = plane;
    //  void Core_Plane_Set_Addr(uint8_t plane) {
    //    #ifdef Pin_SAO_G1_SPARE_1_CP_ADDR_0_Assigned_To_CP_ADDR_0_Output
    //      digitalWriteFast(Pin_SAO_G1_SPARE_1_CP_ADDR_0,  CorePlaneAddr [plane] [2] );
    //    #endif
    //    #ifdef Pin_SAO_G2_SPARE_2_CP_ADDR_1_Assigned_To_CP_ADDR_1_Output
    //      digitalWriteFast(Pin_SAO_G2_SPARE_2_CP_ADDR_1,  CorePlaneAddr [plane] [1] );
    //    #endif
    //    #ifdef Pin_SPARE_3_CP_ADDR_2
    //      digitalWriteFast(Pin_SPARE_3_CP_ADDR_2,         CorePlaneAddr [plane] [0] );
    //    #endif
  }

  void MatrixEnableTransistorInactive() { 
      if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
        digitalWrite(C16P_PIN_WRITE_ENABLE, WRITE_ENABLE_INACTIVE);
      }
      else {
        digitalWrite(PIN_WRITE_ENABLE, WRITE_ENABLE_INACTIVE);
      }
  }

  void MatrixEnableTransistorActive()   { 
      if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
        digitalWrite(C16P_PIN_WRITE_ENABLE, WRITE_ENABLE_ACTIVE);
      }
      else {
        digitalWrite(PIN_WRITE_ENABLE, WRITE_ENABLE_ACTIVE);
      }
  }

  void MatrixDriveTransistorsInactive() {
      if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
        // Set all the matrix lines to the safe state, all transistors inactive.
        for (uint8_t i = 1; i <= 9; i++) {
          digitalWrite(C16P_MatrixDrivePinNumber[i], C16P_MatrixDrivePinInactiveState[i]);
        }
        for (uint8_t i = 12; i <= 16; i++) {
          digitalWrite(C16P_MatrixDrivePinNumber[i], C16P_MatrixDrivePinInactiveState[i]);
        }
        for (uint8_t i = 18; i <= 19; i++) {
          digitalWrite(C16P_MatrixDrivePinNumber[i], C16P_MatrixDrivePinInactiveState[i]);
        }
      }
      else {
        OutputToSerialShiftRegister(CMMDTransistorInactiveState);
      }
  }

  void MatrixDriveTransistorsActive() {
      if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
        // FOR TESTING! Set all the matrix lines to the unsafe state, all transistors active.
        for (uint8_t i = 1; i <= 9; i++) {
          digitalWrite(C16P_MatrixDrivePinNumber[i], C16P_MatrixDrivePinActiveState[i]);
        }
        for (uint8_t i = 12; i <= 16; i++) {
          digitalWrite(C16P_MatrixDrivePinNumber[i], C16P_MatrixDrivePinActiveState[i]);
        }
        for (uint8_t i = 18; i <= 19; i++) {
          digitalWrite(C16P_MatrixDrivePinNumber[i], C16P_MatrixDrivePinActiveState[i]);
        }        
      }
      else {
        OutputToSerialShiftRegister(CMMDTransistorActiveState);
      }
  }

  // Use row and col to select the proper place in the array
  void SetRowAndCol (uint8_t row, uint8_t col) { 
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      // decode bit # from row and col data to resolve the correct row drive polarity
      uint8_t bit = col + (row*4);
        digitalWrite( (C16P_CMMDSetRowByBit[bit] [0] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDSetRowByBit[bit] [0] ] );
        delayMicroseconds(1); 
      //TracingPulses(1);
        digitalWrite( (C16P_CMMDSetRowByBit[bit] [1] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDSetRowByBit[bit] [1] ] );
        // Use col to select the proper place in the look up table
        // columns are easier to decode with the simpler CMMDSetCol look-up table.
        delayMicroseconds(1); 
      //TracingPulses(1);
        digitalWrite( (C16P_CMMDSetCol[col] [0] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDSetCol[col] [0] ] );
        delayMicroseconds(1); 
      //TracingPulses(1);
        digitalWrite( (C16P_CMMDSetCol[col] [1] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDSetCol[col] [1] ] );
        delayMicroseconds(1); 
      //TracingPulses(1);      
    }
    else {
      uint8_t bit = col + (row*8);   // decode bit # from row and col data to resolve the correct row drive polarity
      OutputToSerialShiftRegister(CMMDSetBit[bit] ^ CMMDTransistorInactiveState);
    } 
  }

  // Use row and col to select the proper place in the array
  void ClearRowAndCol (uint8_t row, uint8_t col) {
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      // decode bit # from row and col data to resolve the correct row drive polarity
      uint8_t bit = col + (row*4);
        digitalWrite( (C16P_CMMDClearRowByBit[bit] [0] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDClearRowByBit[bit] [0] ] ); // for bit 0, pin 
        delayMicroseconds(1); 
      // TracingPulses(1);
        digitalWrite( (C16P_CMMDClearRowByBit[bit] [1] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDClearRowByBit[bit] [1] ] ); // for bit 0, pin 
        delayMicroseconds(1); 
      // TracingPulses(1);
        // columns are easier to decode with the simpler CMMDSetCol look-up table.
        digitalWrite( (C16P_CMMDClearCol[col] [0] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDClearCol[col] [0] ] ); // for bit 0, pin 
        delayMicroseconds(1); 
      // TracingPulses(1);
        digitalWrite( (C16P_CMMDClearCol[col] [1] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDClearCol[col] [1] ] ); // for bit 0, pin    
        delayMicroseconds(1); 
      // TracingPulses(1);
    }
    else {
      uint8_t bit = col + (row*8);  // decode bit # from row and col data to resolve the correct row and column drive polarity
      OutputToSerialShiftRegister(CMMDClearBit[bit] ^ CMMDTransistorInactiveState);    
    }
  }

  // Use row and col to select the proper place in the array
  void SetBit (uint8_t bit) { 
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      // Convert incoming bit to column value
      uint8_t col = 0;
      if      (bit < 4 ) { col = bit   ; }
      else if (bit < 8 ) { col = bit-4 ; }
      else if (bit < 12) { col = bit-8 ; }
      else if (bit < 16) { col = bit-12; }
      digitalWrite( (C16P_CMMDSetRowByBit[bit] [0] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDSetRowByBit[bit] [0] ] );
      delayMicroseconds(1); 
      //TracingPulses(1);
      digitalWrite( (C16P_CMMDSetRowByBit[bit] [1] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDSetRowByBit[bit] [1] ] );
      // Use col to select the proper place in the look up table
      // columns are easier to decode with the simpler CMMDSetCol look-up table.
      delayMicroseconds(1); 
      //TracingPulses(1);
      digitalWrite( (C16P_CMMDSetCol[col] [0] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDSetCol[col] [0] ] );
      delayMicroseconds(1); 
      //TracingPulses(1);
      digitalWrite( (C16P_CMMDSetCol[col] [1] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDSetCol[col] [1] ] );
      delayMicroseconds(1); 
      //TracingPulses(1);      
      /*
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q7P), 0 ); // for bit 0, row 0 YL0 to VMEM
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q9N), 1 ); // for bit 0, row 0 YL4 to GND
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q3P), 0 ); // for bit 0, col 0 XT0 to VMEM
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q1N), 1 ); // for bit 0, col 0 XB0 to GND
      */
    }
    else {
      OutputToSerialShiftRegister(CMMDSetBit[bit] ^ CMMDTransistorInactiveState); 
    }
  }

  // Use row and col to select the proper place in the array
  void ClearBit (uint8_t bit) {
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      // Convert incoming bit to column value
      uint8_t col = 0;
      if      (bit < 4 ) { col = bit   ; }
      else if (bit < 8 ) { col = bit-4 ; }
      else if (bit < 12) { col = bit-8 ; }
      else if (bit < 16) { col = bit-12; }
      digitalWrite( (C16P_CMMDClearRowByBit[bit] [0] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDClearRowByBit[bit] [0] ] ); // for bit 0, pin 
      delayMicroseconds(1); 
      // TracingPulses(1);
      digitalWrite( (C16P_CMMDClearRowByBit[bit] [1] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDClearRowByBit[bit] [1] ] ); // for bit 0, pin 
      delayMicroseconds(1); 
      // TracingPulses(1);
      // columns are easier to decode with the simpler CMMDSetCol look-up table.
      digitalWrite( (C16P_CMMDClearCol[col] [0] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDClearCol[col] [0] ] ); // for bit 0, pin 
      delayMicroseconds(1); 
      // TracingPulses(1);
      digitalWrite( (C16P_CMMDClearCol[col] [1] ), C16P_MatrixDrivePinActiveState[ C16P_CMMDClearCol[col] [1] ] ); // for bit 0, pin    
      delayMicroseconds(1); 
      // TracingPulses(1);
      /*
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q7N), 1 ); // for bit 0, row 0 YL0 to GND
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q9P), 0 ); // for bit 0, row 0 YL4 to VMEM
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q3N), 1 ); // for bit 0, col 0 XT0 to GND
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q1P), 0 ); // for bit 0, col 0 XB0 to VMEM
      */  
    }
    else {
      OutputToSerialShiftRegister(CMMDClearBit[bit] ^ CMMDTransistorInactiveState);    
    }
  }

  void ClearRowZeroAndColZero () {
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q7N), 1 ); // for bit 0, row 0 YL0 to GND
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q9P), 0 ); // for bit 0, row 0 YL4 to VMEM
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q3N), 1 ); // for bit 0, col 0 XT0 to GND
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q1P), 0 ); // for bit 0, col 0 XB0 to VMEM    
    }
    else {
      /* Bit manipulation needed to go from FlipField (which transistors to activate) to OutField (the high/low state required to activate transistors).
        uint32_t FlipField = 0b00000000000000010010000000100001;  // This is a little easier to read. 1s indicate which transistors need to be active.
        uint32_t Inactive  = 0b00000000000010101010101010101010;  // This is the normal inactive state of each transistor.
        uint32_t OutField  = 0b00000000000010111000101010011011;  // The XOR operator returns 1 only when the two bits are different.
        TRUTH TABLE:
          F I    O  Requires XOR which is F ^ A = O. The XOR operator returns 1 only when the two bits are different.
          1 0 -> 1  change to opposite
          1 1 -> 0  change to opposite
          0 0 -> 0  no change
          0 1 -> 1  no change
      */
      //                                 QQQQQQQQQQQQQQQQQQQQ
      //                                 11                     
      //                             HGFE00998877665544332211
      //                     xxxxxxxx    PNPNPNPNPNPNPNPNPNPN
      uint32_t FlipField0= 0b00000000000000100001000000010010;  // BIT 0
      uint32_t FlipField1= 0b00000000000000010010000001000010;  // BIT 1        
      OutputToSerialShiftRegister(FlipField0 ^ CMMDTransistorInactiveState);
    }
  }

  void SetRowZeroAndColZero () {
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q7P), 0 ); // for bit 0, row 0 YL0 to VMEM
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q9N), 1 ); // for bit 0, row 0 YL4 to GND
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q3P), 0 ); // for bit 0, col 0 XT0 to VMEM
      digitalWrite( (C16P_PIN_MATRIX_DRIVE_Q1N), 1 ); // for bit 0, col 0 XB0 to GND
    }
    else {
      //                                 QQQQQQQQQQQQQQQQQQQQ
      //                                 11                     
      //                             HGFE00998877665544332211
      //                     xxxxxxxx    PNPNPNPNPNPNPNPNPNPN
      uint32_t FlipField0= 0b00000000000000010010000000100001;  // BIT 0
      uint32_t FlipField1= 0b00000000000000100001000010000001;  // BIT 1
      OutputToSerialShiftRegister(FlipField0 ^ CMMDTransistorInactiveState);
    }
  }

  void CoreSenseReset() {
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      digitalWrite( C16P_PIN_SENSE_RESET, 1);
      // Teensy 3.2 compiler optimizes out the pulses if this delay is not included.
      // The delay is also useful to see the pulse on the scope.
      delayMicroseconds(1); 
      digitalWrite( C16P_PIN_SENSE_RESET, 0);    
    }
    else {
      digitalWrite( Pin_Sense_Reset, 1);
      // Teensy 3.2 compiler optimizes out the pulses if this delay is not included.
      // The delay is also useful to see the pulse on the scope.
      delayMicroseconds(1); 
      digitalWrite( Pin_Sense_Reset, 0);    
    }
  }

  bool SenseWirePulse() {
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      bool temp = 0;
      temp = digitalRead(C16P_PIN_SENSE_PULSE);
      return temp;
    }
    else {
      bool temp = 0;
      temp = digitalRead(Pin_Sense_Pulse);
      return temp;
    }
  }

  void OutputToSerialShiftRegister(uint32_t BitField) {
    digitalWrite(PIN_CMD_SR_LATCH, 0);  // Latch Disable to load in new values.
    //                              Q10P                Q1N
    //                             HGFE|
    //                     xxxxxxxx    |
    // bit #              32     24  20| 16       8       0
    //                     |      |   ||  |       |       |
    // Sample inactive = 0b00000000000010101010101010101010;
    // Reference: https://learn.adafruit.com/adafruit-arduino-lesson-4-eight-leds/the-74hc595-shift-register
    //            https://www.arduino.cc/en/Tutorial/Foundations/ShiftOut
    //
    // Shift 24 bits of data in, with clock pulse after each data bit is set.
    // Reference: https://www.arduino.cc/en/Tutorial/Foundations/BitMask
    // Manually, using only the bottom 24 bits of the 32 bit value, MSB first, start at bit #23
    // for (uint8_t i = 22; i > 0; i--) {
    //                     3      22      11         
    //                     1------43------65------87------0
//  for (uint32_t mask = 0b00000000100000000000000000000000; mask>0; mask >>= 1) { // Iterate through bit mask
// TODO: This code is a great candidate for Programmable IO functionality instead of running in the code.
    for (uint32_t mask = 0b00000000100000000000000000000000; mask>0; mask >>= 1) { // Iterate through bit mask
      uint32_t testResult = BitField & mask;
      if(testResult){ digitalWrite(PIN_CMD_SR_SERIAL,1); }
      else { digitalWrite(PIN_CMD_SR_SERIAL,0); }
      digitalWrite(PIN_CMD_SR_CLOCK, 1);  // Set clock high briefly...
      digitalWrite(PIN_CMD_SR_CLOCK, 0);  // ...and back to low.
    }
    digitalWrite(PIN_CMD_SR_LATCH, 1);  // Latch Enabled to enable the new values in the outputs.
  }

#endif
