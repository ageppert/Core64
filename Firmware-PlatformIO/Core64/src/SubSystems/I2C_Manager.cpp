#include <stdint.h>
#include <stdbool.h>

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include "SubSystems/I2C_Manager.h"
#include "Config/HardwareIOMap.h"
#include "Hal/Debug_Pins_HAL.h"
#include <Wire.h>

#define EEPROM_MAX_WRITE_TIME_MS         5

/* TEENSY LC
// Default is SCL0 and SDA0 on pins 19/18 of Teensy LC
// #define not needed, as I2C1.h library takes care of this pin configuration.
// #define Pin_I2C_Bus_Data       18
// #define Pin_I2C_Bus_Clock      19

// RASPI PICO WITH ARDUINO EARLE PHILHOWER CORE
    SOLVED in "Core64c_Test_Pico_I2C_Scanner"
    Using the native Arduino Pico Board support through Boards Manager (Not the Earle version)
    To enable RP2040 I2C channel 1 as Wire (this really needs to be cleaned up by Arduino so I2C channel 0 is Wire, and Channel 1 is Wire1),
    change as follows in file "pins_arduino.h" located in Macintosh HD ▸ Users ▸ ageppert ▸ Library ▸ Arduino15 ▸ packages ▸ arduino ▸ hardware ▸ mbed_rp2040 ▸ 2.3.1 ▸ variants ▸ RASPBERRY_PI_PICO
      // Wire
      #define PIN_WIRE_SDA        (10u)  // I2C1_SDA GP10 is Pico pin 14
      #define PIN_WIRE_SCL        (11u)  // I2C1_SCL GP11 is Pico pin 15
      
      // Default was:
      // #define PIN_WIRE_SDA        (6u)  // I2C1_SDA GP6 is Pico pin 9
      // #define PIN_WIRE_SCL        (7u)  // I2C1_SCL GP7 is Pico pin 10
      // See https://github.com/arduino/ArduinoCore-mbed/issues/194

    The above changes are not great, because it is a manual modificaiton of the core, but worked in the Arduino IDE. But I don't want to resort to that hack with PlatformIO.

*/

#if defined BOARD_CORE64_TEENSY_32
  // Nothing to do here with the Arduino Core for I2C.
#elif defined BOARD_CORE64C_RASPI_PICO
  /*
    Jan 2022: I'm trying to get I2C1 working in PlatformIO. Trying this stuff but it didn't work.
    // Wire
          #define PIN_WIRE_SDA        (p22)
          #define PIN_WIRE_SCL        (p23)

          #define PIN_WIRE_SDA1       (p15)
          #define PIN_WIRE_SCL1       (p16)
  */
  // Arduino-MBED Core needs different configuration with I2C and Pico: https://github.com/arduino/ArduinoCore-mbed/issues/194
  // This works:
  MbedI2C I2C1(p10,p11); // Use GPO pin numbers. Not board pin numbers (14,15). Default frequency Pico is 100 MHz.
  // Other stuff to try later:
  // In ArduinoCore-MBED v2.6.1 it looks like Wire1.begin() may work as expected, if called after #define PIN_WIRE_SDA1 (p15), #define PIN_WIRE_SCL1 (p16)
  // or
  // bool setSDA(pin_size_t sda);
  // bool setSCL(pin_size_t scl);
#endif

/* TO DO: 
    Keep track of which I2C peripherals are present to use in other functions.
    Read the Chip ID and report that.
 * REFERENCE:
    Master list of I2C addresses at https://learn.adafruit.com/i2c-addresses/the-list
*/

static bool HardwareAvailableButtonHallSensors = false;

bool HardwareConnectedCheckButtonHallSensors() {
  return HardwareAvailableButtonHallSensors;
}

void I2CManagerSetup() {
  #if defined BOARD_CORE64_TEENSY_32
    Wire.begin(); // Nothing to do here with the Arduino Core for I2C.
  #elif defined BOARD_CORE64C_RASPI_PICO
    I2C1.begin(); // testing this as a replacement to wire.xxxxx calls.
  #endif
  Serial.println(F("\n  I2C Manager: Setup Complete."));
}

void printKnownChips(byte address)
{
  // Is this list missing part numbers for chips you use?
  // Please suggest additions here:
  // https://github.com/PaulStoffregen/Wire/issues/new
  switch (address) {
    case 0x00: Serial.print(F("AS3935")); break;
    case 0x01: Serial.print(F("AS3935")); break;
    case 0x02: Serial.print(F("AS3935")); break;
    case 0x03: Serial.print(F("AS3935")); break;
    case 0x0A: Serial.print(F("SGTL5000")); break; // MCLK required
    case 0x0B: Serial.print(F("SMBusBattery?")); break;
    case 0x0C: Serial.print(F("AK8963")); break;
    case 0x10: Serial.print(F("CS4272")); break;
    case 0x11: Serial.print(F("Si4713")); break;
    case 0x13: Serial.print(F("VCNL4000,AK4558")); break;
    case 0x18: Serial.print(F("LIS331DLH")); break;
    case 0x19: Serial.print(F("LSM303,LIS331DLH")); break;
    case 0x1A: Serial.print(F("WM8731")); break;
    case 0x1C: Serial.print(F("LIS3MDL")); break;
    case 0x1D: Serial.print(F("LSM303D,LSM9DS0,ADXL345,MMA7455L,LSM9DS1,LIS3DSH")); break;
    case 0x1E: Serial.print(F("LSM303D,HMC5883L,FXOS8700,LIS3DSH")); break;
    
    // AND!XOR GPIO EXPANDER
    case 0x20: Serial.print(F("MCP23017,MCP23008,PCF8574,FXAS21002,SoilMoisture")); break;
    
    case 0x21: Serial.print(F("MCP23017,MCP23008,PCF8574")); break;
    case 0x22: Serial.print(F("MCP23017,MCP23008,PCF8574")); break;
    case 0x23: Serial.print(F("MCP23017,MCP23008,PCF8574")); break;
    case 0x24: Serial.print(F("MCP23017,MCP23008,PCF8574")); break;
    case 0x25: Serial.print(F("MCP23017,MCP23008,PCF8574")); break;
    
    // IO EXPANDERS
    case 0x26: Serial.print(F("MCP23017,IO EXPANDER 1")); break;
    case 0x27: Serial.print(F("MCP23017,IO EXPANDER 2")); break;
    
    case 0x28: Serial.print(F("BNO055,EM7180,CAP1188")); break;
    // case 0x29: Serial.print(F("TSL2561,VL6180,TSL2561,TSL2591,BNO055,CAP1188")); break;
    case 0x29: Serial.print(F("LTR-329ALS_Ambient_Light_Sensor")); break;
    case 0x2A: Serial.print(F("SGTL5000,CAP1188")); break;
    case 0x2B: Serial.print(F("CAP1188")); break;
    case 0x2C: Serial.print(F("MCP44XX ePot")); break;
    case 0x2D: Serial.print(F("MCP44XX ePot")); break;
    case 0x2E: Serial.print(F("MCP44XX ePot")); break;
    case 0x2F: Serial.print(F("MCP44XX ePot")); break;
    
    // HALL SENSORS
    case 0x30: Serial.print(F("SI7210-B-01, Hall Sensor 1")); HardwareAvailableButtonHallSensors = true; break;
    case 0x31: Serial.print(F("SI7210-B-02, Hall Sensor 2")); HardwareAvailableButtonHallSensors = true; break;
    case 0x32: Serial.print(F("SI7210-B-03, Hall Sensor 3")); HardwareAvailableButtonHallSensors = true; break;
    case 0x33: Serial.print(F("SI7210-B-04, Hall Sensor 4")); HardwareAvailableButtonHallSensors = true; break;
    
    case 0x34: Serial.print(F("MAX11612,MAX11613")); break;
    case 0x35: Serial.print(F("MAX11616,MAX11617")); break;
    case 0x38: Serial.print(F("RA8875,FT6206")); break;
    case 0x39: Serial.print(F("TSL2561, APDS9960")); break;
    
    // OLED SCREEN
    case 0x3C: Serial.print(F("SSD1306,DigisparkOLED")); break;
    
    case 0x3D: Serial.print(F("SSD1306")); break;
    case 0x40: Serial.print(F("PCA9685,Si7021")); break;
    case 0x41: Serial.print(F("STMPE610,PCA9685")); break;
    case 0x42: Serial.print(F("PCA9685")); break;
    case 0x43: Serial.print(F("PCA9685")); break;
    case 0x44: Serial.print(F("PCA9685, SHT3X")); break;
    case 0x45: Serial.print(F("PCA9685, SHT3X")); break;
    case 0x46: Serial.print(F("PCA9685")); break;
    case 0x47: Serial.print(F("PCA9685")); break;
    case 0x48: Serial.print(F("VEML6030_Ambient_Light_Sensor")); break;
    case 0x49: Serial.print(F("ADS1115,TSL2561,PCF8591")); break;
    case 0x4A: Serial.print(F("ADS1115")); break;
    case 0x4B: Serial.print(F("ADS1115,TMP102")); break;
    
    // AND!XOR EEPROM
    case 0x50: Serial.print(F("EEPROM")); break;

    case 0x51: Serial.print(F("EEPROM")); break;
    case 0x52: Serial.print(F("Nunchuk,EEPROM")); break;
    case 0x53: Serial.print(F("ADXL345,EEPROM")); break;
    case 0x54: Serial.print(F("EEPROM")); break;
    case 0x55: Serial.print(F("EEPROM")); break;
    case 0x56: Serial.print(F("EEPROM")); break;
    
    // EEPROM BOARD ID
    case 0x57: Serial.print(F("EEPROM BOARD ID")); break;
    
// TO DO: Don't make the hardware decisions in here. Make a function in HardwareIOMap.cpp dedicated.
    // Just build an array in I2C Manager to catalog which I2C devices are present.

    case 0x58: Serial.print(F("TPA2016,MAX21100")); break;
    case 0x5A: Serial.print(F("MPR121")); break;
    case 0x60: Serial.print(F("MPL3115,MCP4725,MCP4728,TEA5767,Si5351")); break;
    case 0x61: Serial.print(F("MCP4725,AtlasEzoDO")); break;
    case 0x62: Serial.print(F("LidarLite,MCP4725,AtlasEzoORP")); break;
    case 0x63: Serial.print(F("MCP4725,AtlasEzoPH")); break;
    case 0x64: Serial.print(F("AtlasEzoEC")); break;
    case 0x66: Serial.print(F("AtlasEzoRTD")); break;
    case 0x68: Serial.print(F("DS1307,DS3231,MPU6050,MPU9050,MPU9250,ITG3200,ITG3701,LSM9DS0,L3G4200D")); break;
    case 0x69: Serial.print(F("MPU6050,MPU9050,MPU9250,ITG3701,L3G4200D")); break;
    case 0x6A: Serial.print(F("LSM9DS1")); break;
    case 0x6B: Serial.print(F("LSM9DS0")); break;
    case 0x70: Serial.print(F("HT16K33")); break;
    case 0x71: Serial.print(F("SFE7SEG,HT16K33")); break;
    case 0x72: Serial.print(F("HT16K33")); break;
    case 0x73: Serial.print(F("HT16K33")); break;
    case 0x76: Serial.print(F("MS5607,MS5611,MS5637,BMP280")); break;
    case 0x77: Serial.print(F("BMP085,BMA180,BMP280,MS5611")); break;
    default: Serial.print(F("unknown chip"));
  }
}

void I2CManagerBusScan() {
  byte error, address;
  int nDevices;
  Debug_Pin_1(1);
  Serial.println(F("  I2C_Manager: Scanning addresses 1 to 127..."));

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    #if defined BOARD_CORE64_TEENSY_32
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
    #elif defined BOARD_CORE64C_RASPI_PICO
      I2C1.beginTransmission(address);
      error = I2C1.endTransmission();
    #endif

    if (error == 0) {
      Serial.print(F("    Device found at address 0x"));
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address,HEX);
      Serial.print("  (");
      printKnownChips(address);
      Serial.println(")");

      nDevices++;
    } else if (error==4) {
      Serial.print(F("    Unknown error at address 0x"));
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println(F("    No I2C devices found.\n"));
  } else {
    Serial.println(F("  I2C_Manager: Scan complete.\n"));
    delay(100);
  }
  Debug_Pin_1(0);
}

bool I2CDetectExternalEEPROM(uint8_t address) {
  bool presentnpresent = 0;
  #if defined BOARD_CORE64_TEENSY_32
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission(); // Normal Arduino 
  #elif defined BOARD_CORE64C_RASPI_PICO
    I2C1.beginTransmission(address);
    uint8_t error = I2C1.endTransmission(); // Normal Arduino 
  #endif
  if (error == 0) { presentnpresent = 1; }
  else { presentnpresent = 0; }
  return (presentnpresent);
}

#if defined BOARD_CORE64_TEENSY_32
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

#elif defined BOARD_CORE64C_RASPI_PICO
  void EEPROMExtWriteByte(int deviceaddress, unsigned int eeaddress, uint8_t data ) 
  {
    I2C1.beginTransmission(deviceaddress);
    I2C1.write((int)(eeaddress));
    I2C1.write(data);
    I2C1.endTransmission();
    delay(EEPROM_MAX_WRITE_TIME_MS);
  }

  byte EEPROMExtReadByte(int deviceaddress, unsigned int eeaddress ) 
  {
    byte rdata = 0xFF;
    I2C1.beginTransmission(deviceaddress);
    I2C1.write((int)(eeaddress));
    I2C1.endTransmission();
    I2C1.requestFrom(deviceaddress,1);
    if (I2C1.available()) rdata = I2C1.read();
    return rdata;
  }

  byte EEPROMExtDefaultReadByte(int deviceaddress, uint8_t eeaddress ) 
  {
    // uint8_t deviceaddress = EEPROM_ADDRESS;
    byte rdata = 0xFF;
    I2C1.beginTransmission(deviceaddress);
    I2C1.write((int)(eeaddress));
    I2C1.endTransmission();
    I2C1.requestFrom(deviceaddress,1);
    if (I2C1.available()) rdata = I2C1.read();
    return rdata;
  }
#endif