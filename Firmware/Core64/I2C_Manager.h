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

	void I2CManagerSetup();								// Prints setup message to serial port.
	void I2CManagerBusScan();							// Scans all 7-bit addresses on the bus. Prints devices that are present to serial port.
	bool I2CDetectExternalEEPROM(uint8_t address);		// Call with expected address of external EEPROM, returns 1 if present, 0 if not.


#endif
