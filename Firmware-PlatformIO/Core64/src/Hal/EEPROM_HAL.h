/*
PURPOSE: Abstraction to EEPROM data for Teensy LC with 128 bytes of EEPROM
SETUP: None

https://www.pjrc.com/teensy/td_libs_EEPROM.html
http://arduino.cc/en/Reference/EEPROM

*/
 
#ifndef EEPROM_HAL_H
#define EEPROM_HAL_H

#if (ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif

#include <stdint.h>
#include <stdbool.h>

uint8_t EEPROMExtReadHardwareVersionMajor();
uint8_t EEPROMExtReadHardwareVersionMinor();
uint8_t EEPROMExtReadHardwareVersionPatch();
uint32_t EEPROMExtReadSerialNumber();
uint8_t EEPROMExtReadBornOnYear();
uint8_t EEPROMExtReadBornOnMonth();
uint8_t EEPROMExtReadBornOnDay();
uint8_t EEPROMExtReadCorePatternArrangement();
void EEPROMExtWriteCorePatternArrangement(uint8_t value);
bool ReadHardwareVersion();									// Use once to detect and set the hardware version variables. Return true if EEPROM is present and the value is in bounds.
bool ReadLogicBoardType();								// Use once to detect and set the Logic Board Type variables. Return true if EEPROM is present and the value is in bounds.

#endif // EEPROM_HAL_H
