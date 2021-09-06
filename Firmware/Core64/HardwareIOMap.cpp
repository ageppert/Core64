#include <stdint.h>
#include <stdbool.h>

#include "HardwareIOMap.h"

#include "I2C_Manager.h"
#include "EEPROM_HAL.h"

uint8_t HardwareVersionMajor  = 0;  // Default
uint8_t HardwareVersionMinor  = 0;  // Default
uint8_t HardwareVersionPatch  = 0;  // Default

void DetectHardwareVersion() {
	if (I2CDetectExternalEEPROM(0x57))
	{
		HardwareVersionMajor = EEPROMExtReadHardwareVersionMajor();
		HardwareVersionMinor = EEPROMExtReadHardwareVersionMinor();
		HardwareVersionPatch = EEPROMExtReadHardwareVersionPatch();
	}
	else
	{
		// If the external EEPROM is not present, assume hardware version
		HardwareVersionMajor  = 0;
		HardwareVersionMinor  = 2;
		HardwareVersionPatch  = 0;		
	}
}
