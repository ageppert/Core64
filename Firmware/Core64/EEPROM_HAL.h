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

//#ifdef __cplusplus
//extern "C" {
//#endif

void EEPROM_Setup();
// void EEPROM_Hardware_Version_Write(uint8_t address, uint8_t byte);
uint8_t EEPROM_Hardware_Version_Read(uint8_t address);
uint8_t EEPROMExtReadHardwareVersionMajor();
uint8_t EEPROMExtReadHardwareVersionMinor();
uint8_t EEPROMExtReadHardwareVersionPatch();
uint32_t EEPROMExtReadSerialNumber();
uint8_t EEPROMExtReadBornOnYear();
uint8_t EEPROMExtReadBornOnMonth();
uint8_t EEPROMExtReadBornOnDay();

//#ifdef __cplusplus
//} // extern "C"
//#endif

#endif // EEPROM_HAL_H
