#include <stdint.h>
#include <stdbool.h>

#include "Config/HardwareIOMap.h"

#include "Hal/EEPROM_HAL.h"
#include "SubSystems/I2C_Manager.h"

uint8_t HardwareVersionMajor  = 0; // default
uint8_t HardwareVersionMinor  = 0; // default
uint8_t HardwareVersionPatch  = 0; // default

uint8_t EEPROMExtReadHardwareVersionMajor() {
  return (EEPROMExtReadByte(EEPROM_ADDRESS, 9));
}

uint8_t EEPROMExtReadHardwareVersionMinor() {
  return (EEPROMExtReadByte(EEPROM_ADDRESS, 10));
}

uint8_t EEPROMExtReadHardwareVersionPatch() {
  return (EEPROMExtReadByte(EEPROM_ADDRESS, 11));
}

uint32_t EEPROMExtReadSerialNumber() {
  uint32_t SerialNumberLocal = 0;
  SerialNumberLocal = EEPROMExtReadByte(EEPROM_ADDRESS, 5);
  SerialNumberLocal = SerialNumberLocal + (10 * EEPROMExtReadByte(EEPROM_ADDRESS, 4) );
  SerialNumberLocal = SerialNumberLocal + (100 * EEPROMExtReadByte(EEPROM_ADDRESS, 3) );
  SerialNumberLocal = SerialNumberLocal + (1000 * EEPROMExtReadByte(EEPROM_ADDRESS, 2) );
  SerialNumberLocal = SerialNumberLocal + (10000 * EEPROMExtReadByte(EEPROM_ADDRESS, 1) );
  SerialNumberLocal = SerialNumberLocal + (100000 * EEPROMExtReadByte(EEPROM_ADDRESS, 0) );  // This digit indicates Logic Board Type. See enum eLogicBoardType.
  return (SerialNumberLocal);
}

uint8_t EEPROMExtReadBornOnYear() {
  return (EEPROMExtReadByte(EEPROM_ADDRESS, 6));
}

uint8_t EEPROMExtReadBornOnMonth() {
  return (EEPROMExtReadByte(EEPROM_ADDRESS, 7));
}

uint8_t EEPROMExtReadBornOnDay() {
  return (EEPROMExtReadByte(EEPROM_ADDRESS, 8));
}

uint8_t EEPROMExtReadCorePatternArrangement() {
  // If this values reads as 0, change it from that undefined value to a default value of 1.
  if (EEPROMExtReadByte(EEPROM_ADDRESS, 52) == 0) {
    EEPROMExtWriteByte(EEPROM_ADDRESS, 52, 1);
  }
  return (EEPROMExtReadByte(EEPROM_ADDRESS, 52));
}
void EEPROMExtWriteCorePatternArrangement(uint8_t value) {
  EEPROMExtWriteByte(EEPROM_ADDRESS, 52, value);
}

bool ReadHardwareVersion() {
	if (I2CDetectExternalEEPROM(0x57))
	{
		HardwareVersionMajor = EEPROMExtReadHardwareVersionMajor();
		HardwareVersionMinor = EEPROMExtReadHardwareVersionMinor();
		HardwareVersionPatch = EEPROMExtReadHardwareVersionPatch();
    return true;
	}
	else
	{
		// If the external EEPROM is not present, assume hardware version
		HardwareVersionMajor  = 0;
		HardwareVersionMinor  = 2;
		HardwareVersionPatch  = 0;
    return false;
	}
}

bool ReadLogicBoardType () {
	if (I2CDetectExternalEEPROM(0x57))
	{
		uint8_t temp =  EEPROMExtReadByte(EEPROM_ADDRESS, 0);
    if (temp <= 2) { LogicBoardTypeSet(temp);         }
    else           { LogicBoardTypeSet(eLBT_UNKNOWN); }
    return true;
	}
	else
	{
		// If the external EEPROM is not present, assume Logic Board Type Unknown
    LogicBoardTypeSet(eLBT_UNKNOWN);
    return false;
	}
}