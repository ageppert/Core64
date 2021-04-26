/*
PURPOSE: Set-up and manage I2C hardware that is attached to the MCU.
SETUP:
- Configure in 
*/
 
#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

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

// Prints setup message to serial port.
void I2CManagerSetup();
// Scans all 7-bit addresses on the bus. Prints devices that are present to serial port.
void I2CManagerBusScan();
// Call with expected address of external EEPROM, returns 1 if present, 0 if not.
bool I2CDetectExternalEEPROM(uint8_t address);

//#ifdef __cplusplus
//} // extern "C"
//#endif

#endif
