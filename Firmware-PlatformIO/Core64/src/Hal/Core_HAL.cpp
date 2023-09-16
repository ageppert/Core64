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

#include <Wire.h>   // Default is SCL0 and SDA0 on pins 19/18 of Teensy LC
#include "Hal/Core_HAL.h"
#include "Drivers/Core_Driver.h" 
#include "Config/CharacterMap.h"
#include "Hal/LED_Array_Hal.h"

#include "SubSystems/Analog_Input_Test.h"
#include "Hal/Debug_Pins_HAL.h"

bool ScrollTextToCoreMemoryCompleteFlag = false;

  // TO DO: The CoreArrayMemory should not be written to and read from outside of the Core HAL.
  // The CoreArrayMemory is used as a buffer between external API calls and the real state of the core memory.
  // The CoreArrayMemory is only accurate after all real core memory bits have been read once.
  // It us up to the user to request CoreReadArray() in order provide an initial update to the CoreArrayMemory array.

  // Symbol "64" numbers
  bool CoreArrayMemory [8][8] = {
                                    {1,1,1,0,1,0,1,0},
                                    {1,0,0,0,1,0,1,0},
                                    {1,1,1,0,1,1,1,0},
                                    {1,0,1,0,0,0,1,0},
                                    {1,1,1,0,0,0,1,0},
                                    {0,0,0,0,0,0,0,0},
                                    {0,0,0,0,0,0,0,0},
                                    {0,0,0,0,0,0,0,0}  };
  // Symbol "CORE" letters
  //bool    CoreArrayMemory [8][8] = {
  //                                  {1,1,1,0,1,1,1,0},
  //                                  {1,0,0,0,1,0,1,0},
  //                                  {1,1,1,0,1,1,1,0},
  //                                  {0,0,0,0,0,0,0,0},
  //                                  {1,1,1,0,1,1,1,0},
  //                                  {1,1,0,0,1,1,0,0},
  //                                  {1,0,1,0,1,1,1,0},
  //                                  {0,0,0,0,0,0,0,0}  };

  void CoreSetup() {
    Core_Driver_Setup();
    AllDriveIoSafe();
  }

  void AllDriveIoSafe() {
    MatrixEnableTransistorInactive();
    // MatrixDriveTransistorsInactive();
    // TODO: Disable all core plane select lines, if multiple core planes are available.
  }

  void CoreClearAll() {
    for (uint8_t i = 0; i <64; i++) { 
      Core_Mem_Bit_Write(i,0);
      delayMicroseconds(40); // This 40us delay is required or LED array, first 3-4 pixels in the electronic string, get weird! RF?!?? 
    }
    for (uint8_t x=1; x<=7; x++) {
      for (uint8_t y=0; y<=7; y++) {
        CoreArrayMemory [y][x] = 0;
      }
    }
  }

  void CoreSetAll() {
    for (uint8_t i = 0; i <64; i++) {
      Core_Mem_Bit_Write(i,1);
    }
    for (uint8_t x=1; x<=7; x++) {
      for (uint8_t y=0; y<=7; y++) {
        CoreArrayMemory [y][x] = 1;
      }
    }
  }

  void Core_Mem_Array_Write() {
    uint8_t rowcolmax = 8;
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { rowcolmax = 4; }
    for (uint8_t y=0; y<rowcolmax; y++)
    {
      for (uint8_t x=0; x<rowcolmax; x++)
      {
        // CoreArrayMemory [y][x] = pgm_read_byte(&(character_font_wide[3][y][x])); // testing
        Core_Mem_Bit_Write( (y*rowcolmax)+x, !CoreArrayMemory [y][x]);  // Must invert the bits because 0 is LED on, 1 is LED off.
        delayMicroseconds(40); // This 40us delay is required or LED array, first 3-4 pixels in the electronic string, get weird!
      }
    }
  }

  void Core_Mem_Array_Write_Test_Pattern() {
    uint8_t rowcolmax = 8;
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { rowcolmax = 4; }
    for (uint8_t y=0; y<rowcolmax; y=y+1)
    {
      for (uint8_t x=0; x<rowcolmax; x=x+1)
      {
        if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
          Core_Mem_Bit_Write( (y*rowcolmax)+x, pgm_read_byte(&(character_font_wide_16bit[2][y][x])) );
        }
        else {
          Core_Mem_Bit_Write( (y*rowcolmax)+x, pgm_read_byte(&(character_font_wide[2][y][x])) );
        }
        delayMicroseconds(40); // This 40us delay is required or LED array, first 3-4 pixels in the electronic string, get weird!
      }
    }
  }

  void Core_Mem_Array_Read() {
    uint8_t rowcolmax = 8;
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) { rowcolmax = 4; }
    for (uint8_t y=0; y<rowcolmax; y++)
    {
      for (uint8_t x=0; x<rowcolmax; x++)
      {
        CoreArrayMemory [y][x] = (Core_Mem_Bit_Read( (y*rowcolmax)+x ));
        delayMicroseconds(40); // This 40us delay is required or LED array, first 3-4 pixels in the electronic string, get weird!
      }
    }
  }

  //
  // Monitor for flux interference in Core Memory RAM, reported through CoreArrayMemory
  // Pre-conditions: none
  // Inputs: none
  // Outputs: indirectly through CoreArrayMemory(8x8)
  // The process is destructive to core memory RAM, with any bits not affected by an external magnetic flux in a zero state.
  // Each core is cleared to one, tested to still be one, CoreArrayMemory bit is updated.
  // CoreArrayMemory reflects whether a bit is being affected by a magnetic flux, 1, or not, 0.

  void Core_Mem_Scan_For_Magnet() {     
    uint8_t rowcolmax = 8;
    if (LogicBoardTypeGet()==eLBT_CORE16_PICO) { rowcolmax = 4; }
    uint8_t bit;
    for (uint8_t y=0; y<rowcolmax; y++)
    {
      for (uint8_t x=0; x<rowcolmax; x++)
      {
        bit = (y*rowcolmax)+x;
        Core_Mem_Bit_Write(bit , 1);
        CoreArrayMemory [y][x] = Core_Mem_Bit_Read(bit);
        delayMicroseconds(40); // This 40us (may be able to use less here) delay is required or LED array, first 3-4 pixels in the electronic string, get weird!
      }
    }
    // In the case of Core16, fill in the core memory bits greater than 3,3 with zeros to avoid false triggers in screen detection.
    if (LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      for (uint8_t y=4; y<=7; y++)
      {
        for (uint8_t x=4; x<=7; x++)
        {
          CoreArrayMemory [y][x] = 0;
        }
      }
    }
  }

#if defined  MCU_TYPE_MK20DX256_TEENSY_32

    void Core_Mem_Bit_Write(uint8_t bit, bool value) {
      // Turn off all of the matrix signals
      cli();                                            // Testing for consistent timing.
      CoreSenseReset();                                 // Reset sense pulse flip-flop in case this write is called from read.
      TracingPulses(1);
      MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
      MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
      // Enable the matrix drive transistors
      TracingPulses(2);
      // Activate the selected matrix drive transistors according to bit position and the set/clear request
      if (value == 1) { AllDriveIoSetBit(bit); } 
      else { AllDriveIoClearBit(bit); }
      TracingPulses(3);
      MatrixEnableTransistorActive();                   // Enable the matrix drive transistor (V0.3 takes .8ms to do this)
      delayMicroseconds(20);                            // give the core time to change state
      MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
      // Turn off all of the matrix signals
      MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
      TracingPulses(4);
      CoreSenseReset();
      sei();                                            // Testing for consistent timing.
    }

  void Core_Mem_Bit_Write_With_V_MON(uint8_t bit, bool value) {
    // Turn off all of the matrix signals
    // cli();                                            // Testing for consistent timing.
    CoreSenseReset();                                 // Reset sense pulse flip-flop in case this write is called from read.
    TracingPulses(1);
    MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
    MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
    // Enable the matrix drive transistors
    TracingPulses(2);
    // Activate the selected matrix drive transistors according to bit position and the set/clear request
    if (value == 1) { AllDriveIoSetBit(bit); } 
    else { AllDriveIoClearBit(bit); }
    TracingPulses(3);
    MatrixEnableTransistorActive();                   // Enable the matrix drive transistor (V0.3 takes .8ms to do this)
    delayMicroseconds(20);                            // give the core time to change state
    // delay(10);                                        // give the 3V3 regulator more time to sag lower
    #if defined  MCU_TYPE_MK20DX256_TEENSY_32
      // AnalogUpdateCoresOnly();                        // Testing analog updates only during active core time. All voltages.
      // AnalogUpdateCoresOnly3V3();                     // Serial print only 3V3 voltage.
      AnalogUpdateCoresOnlyBC0Mon();                  // Serial print CAE top of FET voltage as proxy for current.
    #endif
    MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
    // Turn off all of the matrix signals
    MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
    TracingPulses(4);
    CoreSenseReset();
    // sei();                                            // Testing for consistent timing.
  }

  bool Core_Mem_Bit_Read(uint8_t bit) {
    static bool value = 0;
    // cli();                                            // Testing for consistent timing. Disable interrupts while poling for sense pulse.
    CoreStateChangeFlag(1);                           // Clear the sense flag
    MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
    MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
    // Activate the selected matrix drive transistors according to bit position and SET it to 1.
    // TracingPulses(1); 
    // AllDriveIoSetBit(bit);
    CoreSenseReset();
    MatrixEnableTransistorActive();                   // Enable the matrix drive transistor
    AllDriveIoClearBit(bit);
    // TO DO *** When ENABLE is moved here, the flux detection mode on V0.2 has jittery LEDs. Not sure why, but it doesn't look good.
    // loop around this to detect it - not sure on timing needs
    // TracingPulses(2); 
      CoreStateChangeFlag(0);                         // Polling for a change inside this function is faster than the for-loop.
    // Turn off all of the matrix signals
    MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
    MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
    if (CoreStateChangeFlag(0) == true)               // If the core changed state, then it was a 0, and is now 1...
    {
      //Core_Mem_Bit_Write(bit,0);                      // ...so return the core to 0
      Core_Mem_Bit_Write(bit,1);
      value = 0;                                      // ...update value to represent the core state
    // TracingPulses(4); 
    }
    else                                              // otherwise the core was already 1
    {
      value = 1;                                      // ...update value to represent the core state
    // TracingPulses(3); 
    }
    CoreSenseReset();
    // sei();                                            // Testing for consistent timing. Enable interrupts when done poling for sense pulse.
    return (value);                                   // Return the value of the core
  }

  void CoreWriteLongInt(uint64_t value) {

  }

  uint64_t CoreReadLongInt() {
    static bool value;
    return (value);
  }

  void CoreWriteArray() {     // TO DO Add a pointer to the array

  }

  /*
  uint64_t CoreReadArray() {  // TO DO Add a pointer to the array
    uint64_t value = 0;
    bool bitValue = 0;
    for (uint8_t x=0; x<=7; x++) {
      for (uint8_t y=0; y<=7; y++) {
        bitValue = CoreReadBit( (y*8) + x );
        CoreArrayMemory [y][x] = !bitValue;     // For drawing on the screen, invert the bitValue so pixels are on because a read assumes write of 1 (setting core)
        value = value + bitValue;
        value = value << 1;
      }
    }
    return (value);
  }
  */

  void AllDriveIoReadAndStore() {

  }

  void AllDriveIoRecallAndWrite() {

  }

  void AllDriveIoSetBit(uint8_t bit) {
    if      (bit < 8 ) { SetRowAndCol(0, bit    ); }
    else if (bit < 16) { SetRowAndCol(1,(bit-8 )); }
    else if (bit < 24) { SetRowAndCol(2,(bit-16)); }
    else if (bit < 32) { SetRowAndCol(3,(bit-24)); }
    else if (bit < 40) { SetRowAndCol(4,(bit-32)); }
    else if (bit < 48) { SetRowAndCol(5,(bit-40)); }
    else if (bit < 56) { SetRowAndCol(6,(bit-48)); }
    else if (bit < 64) { SetRowAndCol(7,(bit-56)); }
  }

  void AllDriveIoClearBit(uint8_t bit) {
    if      (bit < 8 ) { ClearRowAndCol(0, bit    ); }
    else if (bit < 16) { ClearRowAndCol(1,(bit-8 )); }
    else if (bit < 24) { ClearRowAndCol(2,(bit-16)); }
    else if (bit < 32) { ClearRowAndCol(3,(bit-24)); }
    else if (bit < 40) { ClearRowAndCol(4,(bit-32)); }
    else if (bit < 48) { ClearRowAndCol(5,(bit-40)); }
    else if (bit < 56) { ClearRowAndCol(6,(bit-48)); }
    else if (bit < 64) { ClearRowAndCol(7,(bit-56)); }
  }

  void AllDriveIoEnable() {
    MatrixEnableTransistorActive();
  }

  void AllDriveIoDisable() {
    MatrixEnableTransistorInactive();
  }

  bool CoreStateChangeFlag(bool clearFlag) {                    // Send this function a 0 to poll it, 1 to clear the flag
    static bool CoreStateChangeFlag = 0;
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      // TracingPulses(2);     
    if (clearFlag == true) { CoreStateChangeFlag = 0; }           // Override detected state when user requests to clear the flag
    return CoreStateChangeFlag;
  }

  void IOESpare1_On() {
    DebugIOESpare1_On();
  }

  void IOESpare1_Off() {
    DebugIOESpare1_Off();
  }

  void IOESpare2_On() {
    DebugIOESpare2_On();
  }

  void IOESpare2_Off() {
    DebugIOESpare2_Off();
  }

#elif defined MCU_TYPE_RP2040
  void AllDriveIoSetBit(uint8_t bit) {
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      if      (bit < 4 ) { SetRowAndCol(0, bit    ); }
      else if (bit < 8)  { SetRowAndCol(1,(bit-4 )); }
      else if (bit < 12) { SetRowAndCol(2,(bit-8 )); }
      else if (bit < 16) { SetRowAndCol(3,(bit-12)); }
    }
    else {
      if      (bit < 8 ) { SetRowAndCol(0, bit    ); }
      else if (bit < 16) { SetRowAndCol(1,(bit-8 )); }
      else if (bit < 24) { SetRowAndCol(2,(bit-16)); }
      else if (bit < 32) { SetRowAndCol(3,(bit-24)); }
      else if (bit < 40) { SetRowAndCol(4,(bit-32)); }
      else if (bit < 48) { SetRowAndCol(5,(bit-40)); }
      else if (bit < 56) { SetRowAndCol(6,(bit-48)); }
      else if (bit < 64) { SetRowAndCol(7,(bit-56)); }
    }
  }

  void AllDriveIoClearBit(uint8_t bit) {
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      if      (bit < 4 ) { ClearRowAndCol(0, bit    ); }
      else if (bit < 8)  { ClearRowAndCol(1,(bit-4 )); }
      else if (bit < 12) { ClearRowAndCol(2,(bit-8 )); }
      else if (bit < 16) { ClearRowAndCol(3,(bit-12)); }
    }
    else {
      if      (bit < 8 ) { ClearRowAndCol(0, bit    ); }
      else if (bit < 16) { ClearRowAndCol(1,(bit-8 )); }
      else if (bit < 24) { ClearRowAndCol(2,(bit-16)); }
      else if (bit < 32) { ClearRowAndCol(3,(bit-24)); }
      else if (bit < 40) { ClearRowAndCol(4,(bit-32)); }
      else if (bit < 48) { ClearRowAndCol(5,(bit-40)); }
      else if (bit < 56) { ClearRowAndCol(6,(bit-48)); }
      else if (bit < 64) { ClearRowAndCol(7,(bit-56)); }
    }
  }

  void AllDriveIoEnable() {
    MatrixEnableTransistorActive();
  }

  void AllDriveIoDisable() {
    MatrixEnableTransistorInactive();
  }

    void Core_Mem_Bit_Write(uint8_t bit, bool value) {
      if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
        // Turn off all of the matrix signals
        // cli();                                            // Testing for consistent timing.
        CoreSenseReset();                                 // Reset sense pulse flip-flop in case this write is called from read.
        // TracingPulses(1);
        MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
        MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
        // Enable the matrix drive transistors
        // TracingPulses(2);
        // Activate the selected matrix drive transistors according to bit position and the set/clear request
        if (value == 1) { AllDriveIoSetBit(bit); } 
        else { AllDriveIoClearBit(bit); }
        // TracingPulses(3);
        MatrixEnableTransistorActive();                   // Enable the matrix drive transistor (V0.3 takes .8ms to do this)
        delayMicroseconds(20);                            // give the core time to change state
        MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
        // Turn off all of the matrix signals
        MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
        // TracingPulses(4);
        CoreSenseReset();
        // sei();                                            // Testing for consistent timing.
      }
      else {
        // Turn off all of the matrix signals
        // cli();                                            // Testing for consistent timing.
        // CoreSenseReset();                                 // Reset sense pulse flip-flop in case this write is called from read.
        // TracingPulses_Debug_Pin_1(1);
        MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
        // MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
        // Enable the matrix drive transistors
        // TracingPulses_Debug_Pin_1(2);
        // Activate the selected matrix drive transistors according to bit position and the set/clear request
        // if (value == 1) { AllDriveIoSetBit(bit); } 
        // else { AllDriveIoClearBit(bit); }

        if (value == 1) { SetBit (bit); }       // This does work for Core64c
        else { ClearBit (bit); }                // This does work for Core64c

        // TracingPulses_Debug_Pin_1(3);
        MatrixEnableTransistorActive();                   // Enable the matrix drive transistor (V0.3 takes .8ms to do this)
        delayMicroseconds(20);                            // give the core time to change state
        MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
        // Turn off all of the matrix signals
        MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
        // TracingPulses_Debug_Pin_1(4);
        // CoreSenseReset();
        // sei();                                            // Testing for consistent timing.
      }
    }

    void Core_Mem_Bit_Write_With_V_MON(uint8_t bit, bool value) {
      // Turn off all of the matrix signals
      // cli();                                            // Testing for consistent timing.
      // CoreSenseReset();                                 // Reset sense pulse flip-flop in case this write is called from read.
      // TracingPulses_Debug_Pin_1(1);
      MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
      // MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
      // Enable the matrix drive transistors
      // TracingPulses_Debug_Pin_1(2);
      // Activate the selected matrix drive transistors according to bit position and the set/clear request
      // if (value == 1) { AllDriveIoSetBit(bit); } 
      // else { AllDriveIoClearBit(bit); }

      if (value == 1) { SetBit (bit); }       // This does work for Core64c
      else { ClearBit (bit); }                // This does work for Core64c

      // TracingPulses_Debug_Pin_1(3);
      MatrixEnableTransistorActive();                   // Enable the matrix drive transistor (V0.3 takes .8ms to do this)
      AnalogUpdateCoresOnly();                        // Testing analog updates only during active core time. All voltages.
      // AnalogUpdateCoresOnly3V3();                     // Serial print only 3V3 voltage.
      // delayMicroseconds(20);                            // give the core time to change state
      MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
      // Turn off all of the matrix signals
      MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
      // TracingPulses_Debug_Pin_1(4);
      // CoreSenseReset();
      // sei();                                            // Testing for consistent timing.
    }

  bool Core_Mem_Bit_Read(uint8_t bit) {
    if(LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      static bool value = 0;
      // cli();                                            // Testing for consistent timing. Disable interrupts while poling for sense pulse.
      CoreStateChangeFlag(1);                           // Clear the sense flag
      MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
      MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
      // Activate the selected matrix drive transistors according to bit position and SET it to 1.
      // TracingPulses(1); 
      // AllDriveIoSetBit(bit);
      CoreSenseReset();
      MatrixEnableTransistorActive();                   // Enable the matrix drive transistor
      AllDriveIoClearBit(bit);
      // TO DO *** When ENABLE is moved here, the flux detection mode on V0.2 has jittery LEDs. Not sure why, but it doesn't look good.
      // loop around this to detect it - not sure on timing needs
      // TracingPulses(2); 
        CoreStateChangeFlag(0);                         // Polling for a change inside this function is faster than the for-loop.
      // Turn off all of the matrix signals
      MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
      MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
      if (CoreStateChangeFlag(0) == true)               // If the core changed state, then it was a 0, and is now 1...
      {
        //Core_Mem_Bit_Write(bit,0);                      // ...so return the core to 0
        Core_Mem_Bit_Write(bit,1);
        value = 0;                                      // ...update value to represent the core state
      // TracingPulses(4); 
      }
      else                                              // otherwise the core was already 1
      {
        value = 1;                                      // ...update value to represent the core state
      // TracingPulses(3); 
      }
      CoreSenseReset();
      // sei();                                            // Testing for consistent timing. Enable interrupts when done poling for sense pulse.
      return (value);                                   // Return the value of the core
    }
    else {
      static bool value = 0;
      // cli();                                            // Testing for consistent timing. Disable interrupts while poling for sense pulse.
      CoreStateChangeFlag(1);                           // Clear the sense flag
      MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
      // MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
      // Activate the selected matrix drive transistors according to bit position and SET it to 1.
      // TracingPulses(1); 
      // AllDriveIoSetBit(bit);
      CoreSenseReset();
      MatrixEnableTransistorActive();                   // Enable the matrix drive transistor
      // AllDriveIoClearBit(bit);
      ClearBit(bit); // Core64c testing
      // TO DO *** When ENABLE is moved here, the flux detection mode on V0.2 has jittery LEDs. Not sure why, but it doesn't look good.
      // loop around this to detect it - not sure on timing needs
      // TracingPulses(2); 
        CoreStateChangeFlag(0);                         // Polling for a change inside this function is faster than the for-loop.
      // Turn off all of the matrix signals
      MatrixEnableTransistorInactive();                 // Make sure the whole matrix is off by de-activating the enable transistor
      // MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
      if (CoreStateChangeFlag(0) == true)               // If the core changed state, then it was a 0, and is now 1...
      {
        Core_Mem_Bit_Write(bit,0);                      // ...so return the core to 0
        //Core_Mem_Bit_Write(bit,1);
        value = 0;                                      // ...update value to represent the core state
      // TracingPulses(4); 
      }
      else                                              // otherwise the core was already 1
      {
        value = 1;                                      // ...update value to represent the core state
        MatrixDriveTransistorsInactive();                 // De-activate all of the individual matrix drive transistors
      // TracingPulses(3); 
      }
      // CoreSenseReset();
      // sei();                                            // Testing for consistent timing. Enable interrupts when done poling for sense pulse.
      return (value);                                   // Return the value of the core
    } 
  }

  bool CoreStateChangeFlag(bool clearFlag) {                    // Send this function a 0 to poll it, 1 to clear the flag
    static bool CoreStateChangeFlag = 0;
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      if (SenseWirePulse() == true) { CoreStateChangeFlag = 1; }
      // TracingPulses(2);     
    if (clearFlag == true) { CoreStateChangeFlag = 0; }           // Override detected state when user requests to clear the flag
    return CoreStateChangeFlag;
  }

  void Core_Mem_All_Drive_IO_Toggle() {
    MatrixEnableTransistorInactive();   // Make sure power to the whole matrix is disabled.
    MatrixDriveTransistorsActive();     // Switch all 20 matrix transistors to the active state.
    delayMicroseconds(5);
    MatrixDriveTransistorsInactive();   // And back to inactive state.
  }

#endif

bool ScrollTextToCoreMemoryCompleteFlagCheck(){     // Return TRUE if the text scrolling has completed.
  return ScrollTextToCoreMemoryCompleteFlag;
}

void ScrollTextToCoreMemoryCompleteFlagClear(){     // Clear the flag indicating text has finished scrolling.
  ScrollTextToCoreMemoryCompleteFlag = false;
}

void ScrollTextToCoreMemory() {
  static unsigned long UpdatePeriodms = 80;  
  static unsigned long NowTime = 0;
  static unsigned long UpdateTimer = 0;

  static uint8_t stringPosition = 0;
  static uint8_t stringLength = 15; // I [heart] C O R E [space] M E M O R Y ! [space]
  static uint8_t characterColumn = 0;
  static bool newBit = 0; 
  static uint8_t ScrollingTextHue = 135;
  static bool ScrollingColorChangeEnable = true;

  NowTime = millis();
  if (LogicBoardTypeGet()==eLBT_CORE16_PICO) { UpdatePeriodms = 160; } // Slow down the scrolling speed for the tiny 4x4 screen!
  // Is it time to scroll again?
  if ((NowTime - UpdateTimer) >= UpdatePeriodms)
  {
    UpdateTimer = NowTime;
    if (LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      stringLength = NumberOfFontCharactersToScroll16bit; // This message is a different length than the 8x8 message.
      // Shift existing core memory data to the left. (leftmost column is 0, rightmost is 7)
        for (uint8_t x=1; x<=3; x++)
        {
          for (uint8_t y=0; y<=3; y++)
          {
            CoreArrayMemory [y][x-1] = CoreArrayMemory [y][x];
          }
        }
      // Bring in a new column on right edge.
        if (characterColumn == 4) {characterColumn = 0; stringPosition++;}
      // Out of characters? Go back to the beginning and scroll again.
        if (stringPosition == stringLength) { stringPosition = 0; characterColumn = 0; ScrollTextToCoreMemoryCompleteFlag = true; }
        for (uint8_t y=0; y<=3; y++) // iterate through the rows
        {
          newBit = pgm_read_byte(&(character_font_wide_16bit[stringPosition][y][characterColumn]));
          CoreArrayMemory [y][3] = newBit;
        }
        characterColumn++; // prepare for next column
        if (ScrollingColorChangeEnable) {
          LED_Array_Monochrome_Increment_Color(4);
        }
    }
    else {
      // Shift existing core memory data to the left. (leftmost column is 0, rightmost is 7)
        for (uint8_t x=1; x<=7; x++)
        {
          for (uint8_t y=0; y<=7; y++)
          {
            CoreArrayMemory [y][x-1] = CoreArrayMemory [y][x];
          }
        }
      // Bring in a new column on right edge.
        if (characterColumn == 8) {characterColumn = 0; stringPosition++;}
      // Out of characters? Go back to the beginning and scroll again.
        if (stringPosition == stringLength) { stringPosition = 0; characterColumn = 0; ScrollTextToCoreMemoryCompleteFlag = true; }
        for (uint8_t y=0; y<=7; y++) // iterate through the rows
        {
          newBit = pgm_read_byte(&(character_font_wide[stringPosition][y][characterColumn]));
          CoreArrayMemory [y][7] = newBit;
        }
        characterColumn++; // prepare for next column
        if (ScrollingColorChangeEnable) {
          LED_Array_Monochrome_Increment_Color(4);
        }
    }
  }
}
