#include <stdint.h>
#include <stdbool.h>

#include "HardwareIOMap.h"

#include "EEPROM_HAL.h"
#include <Wire.h>

#define EEPROM_ST_M24C02_2KBIT

#ifdef EEPROM_ST_M24C02_2KBIT
  #define EEPROM_ADDRESS    0b1010111       // 0b1010+A2_A1_A0): Core64 BOARD ID EEPROM is 0x57 (87 dec) 1010+111
  #define MEM_SIZE_BYTES          256
  #define PAGE_SIZE_BYTES          16
  #define MAX_WRITE_TIME_MS         5
#endif

void EEPROM_Setup() {
  Wire.begin();
}

void EEPROMExtWriteByte(int deviceaddress, unsigned int eeaddress, uint8_t data ) 
{
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress));
  Wire.write(data);
  Wire.endTransmission();
  delay(MAX_WRITE_TIME_MS);
}

byte EEPROMExtReadByte(int deviceaddress, unsigned int eeaddress ) 
{
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress));
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress,1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}

byte EEPROMExtDefaultReadByte(uint8_t eeaddress ) 
{
  uint8_t deviceaddress = EEPROM_ADDRESS;
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress));
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress,1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}

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
  SerialNumberLocal = SerialNumberLocal + (100000 * EEPROMExtReadByte(EEPROM_ADDRESS, 0) );
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
