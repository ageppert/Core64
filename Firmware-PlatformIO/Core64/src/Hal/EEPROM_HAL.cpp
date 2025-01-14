#include <stdint.h>
#include <stdbool.h>

#include "Config/HardwareIOMap.h"

#include "Hal/EEPROM_HAL.h"
#include "SubSystems/I2C_Manager.h"

uint8_t HardwareVersionMajor  = 0; // default
uint8_t HardwareVersionMinor  = 0; // default
uint8_t HardwareVersionPatch  = 0; // default
uint32_t SerialNumberLocal    = 0;
uint16_t SerialNumberLastThreeLocal = 0;

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
  // Big Endian
  SerialNumberLocal = EEPROMExtReadByte(EEPROM_ADDRESS, 5);
  SerialNumberLocal = SerialNumberLocal + (10 * EEPROMExtReadByte(EEPROM_ADDRESS, 4) );
  SerialNumberLocal = SerialNumberLocal + (100 * EEPROMExtReadByte(EEPROM_ADDRESS, 3) );
  SerialNumberLocal = SerialNumberLocal + (1000 * EEPROMExtReadByte(EEPROM_ADDRESS, 2) );
  SerialNumberLocal = SerialNumberLocal + (10000 * EEPROMExtReadByte(EEPROM_ADDRESS, 1) );
  SerialNumberLocal = SerialNumberLocal + (100000 * EEPROMExtReadByte(EEPROM_ADDRESS, 0) );  // This digit indicates Logic Board Type. See enum eLogicBoardType.
  return (SerialNumberLocal);
}

bool EEPROMExtWriteSerialNumber(uint32_t number) {
  // Big Endian
  uint32_t TempSN = 0;
  uint8_t TempDigit = 0;
  if (number <= 999999) {
    TempSN = number;
    TempDigit = number / 100000;
    number = number - (TempDigit * 100000);
    EEPROMExtWriteByte (EEPROM_ADDRESS, 0, TempDigit);
    TempDigit = number / 10000;
    number = number - (TempDigit * 10000);
    EEPROMExtWriteByte (EEPROM_ADDRESS, 1, TempDigit);
    TempDigit = number / 1000;
    number = number - (TempDigit * 1000);
    EEPROMExtWriteByte (EEPROM_ADDRESS, 2, TempDigit);
    TempDigit = number / 100;
    number = number - (TempDigit * 100);
    EEPROMExtWriteByte (EEPROM_ADDRESS, 3, TempDigit);
    TempDigit = number / 10;
    number = number - (TempDigit * 10);
    EEPROMExtWriteByte (EEPROM_ADDRESS, 4, TempDigit);
    TempDigit = number / 1;
    number = number - (TempDigit * 1);
    EEPROMExtWriteByte (EEPROM_ADDRESS, 5, TempDigit);
    return 0;
  }
  else {
    return 1;
  }
}

void EEPROMExtWriteFactoryDefaults() {
  const uint8_t string_len = 64;
  const uint8_t owner_name_start = 14;
  const uint8_t owner_name_len = 17;
  const uint8_t page_one_two_checksum_position = 31;
  const uint8_t page_three_four_checksum_position = 63;
  uint8_t checksum = 0;
  uint8_t StringToWrite[string_len] = {
  #if defined FACTORY_RESET_CORE64C_PICO_ENABLE
    // Start Byte # (# Bytes) Description of configuration setting is below the assigned EEPROM value

    // PAGE 1&2 (32 BYTES) UNIQUE IDENTIFICATION
                  1,0,0,0,0,0,                                            
    /*  000  ( 6) SERIAL NUMBER:      Each byte is a decimal value of a 6 digit decimal number
                                      0XXXXX = Core64 Logic Boards (Teensy 3.2)
                                      1XXXXX = Core64c Logic Boards (Pico)
                                      2XXXXX = Core64 Logic Boards (Pico)
                                      3XXXXX = Core16 Logic Boards (Pico) */
                  24,8,25,                                             
    /*  006  ( 3) Born on Date:       Year, Month, Day */
                  0, 7, 0,                                              
    /*  009  ( 3) Born Version PCBA:  Major.Minor.Revision */
                  3,                                                    
    /*  012  ( 1) Manufacturer ID:    Manufacturer of the Logic Board
                                      0,1 = Andy!                                            
                                      2   = Caltronics 
                                      3   => JLCPCB/LCSC */
                  3,                                                    
    /*  013  ( 1) Hardware Table Format: Version # (identifies this whole table configuration) */
                  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,            
    /*  014  (17) Owner name:         Conventional 8 bit ASCII values */
                  0,                                              
    /*  031  ( 1) Page 1&2 Checksum:  XOR TBD */

    // PAGE 3&4 (32 BYTES) HARDWARE CONFIGURATION
    // Hardware Configuration use is intended to specify the as-shipped hardware configuration so the firmware can operate accordingly.
    // Memory Location #, configuration # is 8-bit value. Value 0 = not applicable/present. Value 255 = undefined (blank EEPROM value). 
                  138,
    /*  032,      MODEL:            1 = Core64 HWV0.5.0 T32 Beta Kit (batch of 15)
                                    2 = Core64 HWV0.6.0 T32 Pre-Production (batch of 30)                                             
                                    3 = Core64 HWV0.7.0 and 0.7.1 PICO reworked Prototypes (batch of 5)                                             
                                    4 = Core64 HWV0.8.0 PICO Pre-Production (batch of 25, 25, 10, 25)                                             
                                  128 = Core64c prototype, HWV0.2.0, without modification, prototype (batch of 5)
                                  129 = Core64c prototype, HWV0.2.1, reworked HWV0.2.0 shift register wiring, prototype (batch of 5)
                                  130 = Core64c prototype, HWV0.3.0, without modification, prototype (batch of 5)
                                  131 = Core64c prototype, HWV0.4.0, without modification, prototype (1st and 2nd batch of 30)
                                  132 = Core64c prototype, HWV0.4.0, without modification, prototype (3rd batch of 30)
                                  133 = Core64c prototype, HWV0.4.0, without modification, prototype (4th batch of 30)
                                  134 = Core64c prototype, HWV0.4.0, without modification, prototype (5th batch of 30)
                                  135 = Core64c prototype, HWV0.4.0, without modification, prototype (6th batch of 30)
                                  136 = Core64c prototype, HWV0.5.0, without modification, prototype (7th batch of 10)
                                  137 = oops: files labeled V0.6 but Core64c but labeled boards (and EEPROM) 0.7.0
                                  138 => Core64c prototype, HWV0.7.0, without modification, prototype (batch of 25) */
                  2,
    /*  033,      LB EEPROM:        1 = M24C01, 128 Bytes
                                    2 => M24C02, 256 BYTES */
                  2,
    /*  034,      POWER:            1 = USB 5V
                                    2 => 4x "AAA" Generic Alkaline "stock config"
                                    3 = 4x "AAA" Long life lithium primary
                                    4 = 4x "AAA" NiMh
                                    5 = 3x "AA" generic
                                    6 = 3x "AA" long life lithium primary
                                    7 = 3x "AA" NiMh
                                    8 = 1S LiPo
                                    9 = 1S LiFe
                                  10 = 1S LiIon                                                */
                  1,
    /*  035,      CORE MEMORY:      1 => Stock, single 8x8
                                    2 = Full stack of eight 8x8 planes                                                */
                  1,
    /*  036,      HALL SENSORS:     1 => Hall Sensor SI7210 I2C 0x30-0x33
                                    2 = Hall Switch TCS40DPR.LF (Using CP5-8 pins SJ to HS1-4)                                                */
                  2,
    /*  037,      AMBIENT LIGHT:    1 = LTR-303 at 0x29
                                    2 => LTR-329 at 0x29
                                    3 = BH1730FVC-TR at 0x29
                                    4 = RPR-0521RS at 0x38 */
                  0,
    /*  038,      SAO PORT X1:      1 = fully accessible I2C and 2 GPIO pins.
                                    2 = limited access, I2C only. */
                  1,
    /*  039,      SAO PORT X2:      1 => fully accessible I2C and 2 GPIO pins
                                    2 = limited access, I2C only. */
                  1,
    /*  040,      GPIO#1:           1 => Available generic use
                                    2 = Test config */
                  1,
    /*  041,      GPIO#2:           1 => Available generic use
                                    2 = Test config */
                  1,
    /*  042,      CP1-8 PINS:       1 => 1,2 used by SAO#2, 3-8 available
                                    2 = 1-8 used by 8 core planes */
                  3,
    /*  043,      LED Array:        1 = Pimoroni Unicorn Hat 8x8 "NeoPixels"
                                    2 = Core64 LED MATRIX WS2813B-B
                                    3 => Core64 LED MATRIX WS2813C */
                  0,
    /*  044,      NEON PIXELS:      1 = SPI w/o CS, connected to SPI TFT LCD port */
                  0,
    /*  045,      LCD:              1 = color SPI 3.2"
                                    2 = color SPI ??? */
                  0,
    /*  046,      OLED Display:     1 = monochrome I2C 128x64
                                    2 = color SPI 128x128
                                    3 = TeensyView 128x32 */
                  0,
    /*  047,      SD CARD:          1 = Connected to Dedicated SD Card Expansion Header
                                    2 = Connected to LCD Header
                                    3 = Connected to OLED Header */
                  1,
    /*  048,      QWIIC PORT:       1 => available */
                  0,
    /*  049,      IR COMM:          1 = some basic config TBD */
                  0,
    /*  050,      NFMC:             1 = some basic config TBD */
                  0,
    /*  051,      RTC PARTS:        1 = populated crystal and battery */
                  1,
    /*  052,      CORE PATTERN:     0,1 => Core Alignment Normal bit 7 \ ... bit 0 /
                                    2 = Core Alignment Opposite bit 7 / ... bit 0 \  */
                  0,
    /*  053,      not used yet                                                                                */
                  0,
    /*  054,      not used yet                                                                                */
                  0,
    /*  055,      not used yet                                                                                */
                  0,
    /*  056,      not used yet                                                                                */
                  0,
    /*  057,      not used yet                                                                                */
                  0,
    /*  058,      not used yet                                                                                */
                  0,
    /*  059,      not used yet                                                                                */
                  0,
    /*  060,      not used yet                                                                                */
                  0,
    /*  061,      not used yet                                                                                */
                  0,
    /*  062,      not used yet                                                                                */
                  0
    /*  063,      Page 3&4 XOR Checksum                                                                       */
  #elif defined FACTORY_RESET_CORE64_PICO_ENABLE
    // Start Byte # (# Bytes) Description of configuration setting is below the assigned EEPROM value

    // PAGE 1&2 (32 BYTES) UNIQUE IDENTIFICATION
                  2,0,0,0,0,0,                                            
    /*  000  ( 6) SERIAL NUMBER:      Each byte is a decimal value of a 6 digit decimal number
                                      0XXXXX = Core64 Logic Boards (Teensy 3.2)
                                      1XXXXX = Core64c Logic Boards (Pico)
                                      2XXXXX = Core64 Logic Boards (Pico)
                                      3XXXXX = Core16 Logic Boards (Pico) */
                  24,10,4,                                             
    /*  006  ( 3) Born on Date:       Year, Month, Day */
                  0, 8, 4,                                              
    /*  009  ( 3) Born Version PCBA:  Major.Minor.Revision */
                  3,                                                    
    /*  012  ( 1) Manufacturer ID:    Manufacturer of the Logic Board
                                      0,1 = Andy!                                            
                                      2   = Caltronics 
                                      3   => JLCPCB/LCSC */
                  3,                                                    
    /*  013  ( 1) Hardware Table Format: Version # (identifies this whole table configuration) */
                  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,            
    /*  014  (17) Owner name:         Conventional 8 bit ASCII values */
                  0,                                              
    /*  031  ( 1) Page 1&2 Checksum:  XOR TBD */

    // PAGE 3&4 (32 BYTES) HARDWARE CONFIGURATION
    // Hardware Configuration use is intended to specify the as-shipped hardware configuration so the firmware can operate accordingly.
    // Memory Location #, configuration # is 8-bit value. Value 0 = not applicable/present. Value 255 = undefined (blank EEPROM value). 
                  4,
    /*  032,      MODEL:            1 = Core64 HWV0.5.0 T32 Beta Kit (batch of 15)
                                    2 = Core64 HWV0.6.0 T32 Pre-Production (batch of 30)                                             
                                    3 = Core64 HWV0.7.0 and 0.7.1 PICO reworked Prototypes (batch of 5)                                             
                                    4 => Core64 HWV0.8.x PICO Pre-Production (batch of 25, 25, 10, 25...)                                             
                                  128 = Core64c prototype, HWV0.2.0, without modification, prototype (batch of 5)
                                  129 = Core64c prototype, HWV0.2.1, reworked HWV0.2.0 shift register wiring, prototype (batch of 5)
                                  130 = Core64c prototype, HWV0.3.0, without modification, prototype (batch of 5)
                                  131 = Core64c prototype, HWV0.4.0, without modification, prototype (1st and 2nd batch of 30)
                                  132 = Core64c prototype, HWV0.4.0, without modification, prototype (3rd batch of 30)
                                  133 = Core64c prototype, HWV0.4.0, without modification, prototype (4th batch of 30)
                                  134 = Core64c prototype, HWV0.4.0, without modification, prototype (5th batch of 30)
                                  135 = Core64c prototype, HWV0.4.0, without modification, prototype (6th batch of 30)
                                  136 = Core64c prototype, HWV0.5.0, without modification, prototype (7th batch of 10) */
                  2,
    /*  033,      LB EEPROM:        1 = M24C01, 128 Bytes
                                    2 => M24C02, 256 BYTES */
                  2,
    /*  034,      POWER:            1 = USB 5V
                                    2 => 4x "AAA" Generic Alkalines "stock config"
                                    3 = 4x "AAA" Long life lithium primary
                                    4 = 4x "AAA" NiMh
                                    5 = 3x "AA" generic
                                    6 = 3x "AA" long life lithium primary
                                    7 = 3x "AA" NiMh
                                    8 = 1S LiPo
                                    9 = 1S LiFe
                                  10 = 1S LiIon                                                */
                  1,
    /*  035,      CORE MEMORY:      1 => Stock, single 8x8
                                    2 = Full stack of eight 8x8 planes                                                */
                  1,
    /*  036,      HALL SENSORS:     1 => Hall Sensor SI7210 I2C 0x30-0x33
                                    2 = Hall Switch TCS40DPR.LF (Using CP5-8 pins SJ to HS1-4)                                                */
                  2,
    /*  037,      AMBIENT LIGHT:    1 = LTR-303 at 0x29
                                    2 => LTR-329 at 0x29
                                    3 = BH1730FVC-TR at 0x29
                                    4 = RPR-0521RS at 0x38 */
                  1,
    /*  038,      SAO PORT X1:      1 => fully accessible I2C and 2 GPIO pins.
                                    2 = limited access, I2C only. */
                  1,
    /*  039,      SAO PORT X2:      1 => fully accessible I2C and 2 GPIO pins
                                    2 = limited access, I2C only. */
                  1,
    /*  040,      GPIO#1:           1 => Available generic use
                                    2 = Test config */
                  1,
    /*  041,      GPIO#2:           1 => Available generic use
                                    2 = Test config */
                  1,
    /*  042,      CP1-8 PINS:       1 => 1,2 used by SAO#2, 3-8 available
                                    2 = 1-8 used by 8 core planes */
                  4,
    /*  043,      LED Array:        1 = Pimoroni Unicorn Hat 8x8 "NeoPixels"
                                    2 = Core64 LED MATRIX WS2813B-B
                                    3 = Core64 LED MATRIX WS2813C
                                    4 => Core64 LED MATRIX V1.3 WS2813Bv5 by CCC (bright!) */
                  0,
    /*  044,      NEON PIXELS:      1 = SPI w/o CS, connected to SPI TFT LCD port */
                  0,
    /*  045,      LCD:              1 = color SPI 3.2"
                                    2 = color SPI ??? */
                  0,
    /*  046,      OLED Display:     1 = monochrome I2C 128x64
                                    2 = color SPI
                                    3 = TeensyView 128x32 */
                  0,
    /*  047,      SD CARD:          1 = Connected to Dedicated SD Card Expansion Header
                                    2 = Connected to LCD Header
                                    3 = Connected to OLED Header */
                  1,
    /*  048,      QWIIC PORT:       1 => available */
                  0,
    /*  049,      IR COMM:          1 = some basic config TBD */
                  0,
    /*  050,      NFMC:             1 = some basic config TBD */
                  0,
    /*  051,      RTC PARTS:        1 = populated crystal and battery */
                  1,
    /*  052,      CORE PATTERN:     0,1 => Core Alignment Normal bit 7 \ ... bit 0 /
                                    2 = Core Alignment Opposite bit 7 / ... bit 0 \  */
                  0,
    /*  053,      not used yet                                                                                */
                  0,
    /*  054,      not used yet                                                                                */
                  0,
    /*  055,      not used yet                                                                                */
                  0,
    /*  056,      not used yet                                                                                */
                  0,
    /*  057,      not used yet                                                                                */
                  0,
    /*  058,      not used yet                                                                                */
                  0,
    /*  059,      not used yet                                                                                */
                  0,
    /*  060,      not used yet                                                                                */
                  0,
    /*  061,      not used yet                                                                                */
                  0,
    /*  062,      not used yet                                                                                */
                  0
    /*  063,      Page 3&4 XOR Checksum                                                                       */
  #elif defined FACTORY_RESET_CORE16_PICO_ENABLE
    // PAGE 1&2 (32 BYTES) UNIQUE IDENTIFICATION
                  3,0,0,0,0,0,                                            
    /*  000  ( 6) SERIAL NUMBER:      Each byte is a decimal value of a 6 digit decimal number
                                      0XXXXX = Core64 Teensy Logic Boards
                                      1XXXXX = Core64c Pico Logic Boards
                                      2XXXXX = Core64 Pico Logic Boards
                                      3XXXXX => Core16 Pico Logic Boards */
                  23,10,02,                                             
    /*  006  ( 3) Born on Date:       Year, Month, Day */
                  0, 1, 0,                                              
    /*  009  ( 3) Born Version PCBA:  Major.Minor.Revision */
                  3,                                                    
    /*  012  ( 1) Manufacturer ID:    Manufacturer of the Logic Board
                                      0,1 = Andy!                                            
                                      2   = Caltronics 
                                      3   => JLCPCB/LCSC */
                  3,                                                    
    /*  013  ( 1) Hardware Table Format: Version # (identifies this whole table configuration) */
                  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,            
    /*  014  (17) Owner name:         Conventional 8 bit ASCII values */
                  0,                                              
    /*  031  ( 1) Page 1&2 Checksum:  XOR TBD */

    // PAGE 3&4 (32 BYTES) HARDWARE CONFIGURATION
    // Hardware Configuration use is intended to specify the as-shipped hardware configuration so the firmware can operate accordingly.
    // Memory Location #, configuration # is 8-bit value. Value 0 = not applicable/present. Value 255 = undefined (blank EEPROM value). 
                  201,
    /*  032,      MODEL:            1 = Core64 Beta Kit (batch of 15)
                                    2 = Core64 Pre-Production (batch of 30)                                             
                                  128 = Core64c protoype, HWV0.2.0, without modification, prototype (batch of 5)
                                  129 = Core64c protoype, HWV0.2.1, reworked HWV0.2.0 shift register wiring, prototype (batch of 5)
                                  130 = Core64c protoype, HWV0.3.0, without modification, prototype (batch of 5)
                                  131 = Core64c protoype, HWV0.4.0, without modification, prototype (1st and 2nd batch of 30)
                                  132 = Core64c protoype, HWV0.4.0, without modification, prototype (3rd batch of 30)
                                  201 => Core16 protoype, HWV0.1.0, without modification, prototype (1st batch of 5) */
                  2,
    /*  033,      LB EEPROM:        1 = M24C01, 128 Bytes
                                    2 => M24C02, 256 BYTES */
                11,
    /*  034,      POWER:            1 = USB 5V
                                    2 = 4x "AAA" Generic Alkalines "stock config"
                                    3 = 4x "AAA" Long life lithium primary
                                    4 = 4x "AAA" NiMh
                                    5 = 3x "AA" generic
                                    6 = 3x "AA" long life lithium primary
                                    7 = 3x "AA" NiMh
                                    8 = 1S LiPo
                                    9 = 1S LiFe
                                  10 = 1S LiIon
                                  11 => 3x "AAA" Generic Alkalines "stock config" */
                  3,
    /*  035,      CORE MEMORY:      1 = Core64 Stock, single 8x8
                                    2 = Core64 Full stack of eight 8x8 planes
                                    3 = Core16 Built-in single stack 4x4 */
                  1,
    /*  036,      HALL SENSORS:     1 => Hall Sensor SI7210 I2C 0x30-0x33
                                    2 = Hall Switch TCS40DPR.LF (Using CP5-8 pins SJ to HS1-4)                                                */
                  0,
    /*  037,      AMBIENT LIGHT:    0 => NONE
                                    2 = LTR-303 at 0x29
                                    2 = LTR-329 at 0x29
                                    3 = BH1730FVC-TR at 0x29
                                    4 = RPR-0521RS at 0x38 */
                  1,
    /*  038,      SAO PORT X1:      1 => fully accessible I2C and 2 GPIO pins.
                                    2 = limited access, I2C only. */
                  0,
    /*  039,      SAO PORT X2:      1 = fully accessible I2C and 2 GPIO pins
                                    2 = limited access, I2C only. */
                  1,
    /*  040,      GPIO#1:           1 => Available generic use
                                    2 = Test config */
                  1,
    /*  041,      GPIO#2:           1 => Available generic use
                                    2 = Test config */
                  0,
    /*  042,      CP1-8 PINS:       0 => Not available
                                    1 = 1,2 used by SAO#2, 3-8 available
                                    2 = 1-8 used by 8 core planes */
                  4,
    /*  043,      LED Array:        1 = Pimoroni Unicorn Hat 8x8 "NeoPixels"
                                    2 = Core64 LED MATRIX 8x8 WS2813B-B (Black top and BRIGHT!)
                                    3 = Core64 LED MATRIX 8x8 WS2813C
                                    4 => Core16 LED MATRIX 4x4 SK6812 */
                  0,
    /*  044,      NEON PIXELS:      1 = SPI w/o CS, connected to SPI TFT LCD port */
                  0,
    /*  045,      LCD:              1 = color SPI 3.2"
                                    2 = color SPI ??? */
                  0,
    /*  046,      OLED Display:     1 = monochrome I2C 128x64
                                    2 = color SPI
                                    3 = TeensyView 128x32 */
                  0,
    /*  047,      SD CARD:          1 = Connected to Dedicated SD Card Expansion Header
                                    2 = Connected to LCD Header
                                    3 = Connected to OLED Header */
                  1,
    /*  048,      QWIIC PORT:       1 => available */
                  0,
    /*  049,      IR COMM:          1 = some basic config TBD */
                  0,
    /*  050,      NFMC:             1 = some basic config TBD */
                  0,
    /*  051,      RTC PARTS:        1 = populated crystal and battery */
                  1,
    /*  052,      CORE PATTERN:     0,1 => Core Alignment Normal bit 7 \ ... bit 0 /
                                    2 = Core Alignment Opposite bit 7 / ... bit 0 \  */
                  0,
    /*  053,      not used yet                                                                                */
                  0,
    /*  054,      not used yet                                                                                */
                  0,
    /*  055,      not used yet                                                                                */
                  0,
    /*  056,      not used yet                                                                                */
                  0,
    /*  057,      not used yet                                                                                */
                  0,
    /*  058,      not used yet                                                                                */
                  0,
    /*  059,      not used yet                                                                                */
                  0,
    /*  060,      not used yet                                                                                */
                  0,
    /*  061,      not used yet                                                                                */
                  0,
    /*  062,      not used yet                                                                                */
                  0
    /*  063,      Page 3&4 XOR Checksum                                                                       */
  #else
    0
  #endif
                  };

  // Update String to Write based on existing Serial Number
  StringToWrite[0] = EEPROMExtReadByte(EEPROM_ADDRESS, 0);
  StringToWrite[1] = EEPROMExtReadByte(EEPROM_ADDRESS, 1);
  StringToWrite[2] = EEPROMExtReadByte(EEPROM_ADDRESS, 2);
  StringToWrite[3] = EEPROMExtReadByte(EEPROM_ADDRESS, 3);
  StringToWrite[4] = EEPROMExtReadByte(EEPROM_ADDRESS, 4);
  StringToWrite[5] = EEPROMExtReadByte(EEPROM_ADDRESS, 5);

  // If S/N 300001 to 300005 then HWV = 0.1.0 and manufacturer is 3 JLCPCB.
  // If S/N 300006 to 300105 then HWV = 0.2.0 and manufacturer is 4 TexElec.
  /*
  if ( (EEPROMExtReadSerialNumber() >= 300001) && (EEPROMExtReadSerialNumber() <= 300005) ) {
    StringToWrite[ 9] = 0;
    StringToWrite[10] = 1;
    StringToWrite[11] = 0;
    StringToWrite[12] = 3;
  }
  if ( (EEPROMExtReadSerialNumber() >= 300006) && (EEPROMExtReadSerialNumber() <= 300105) ) {
    StringToWrite[ 9] = 0;
    StringToWrite[10] = 2;
    StringToWrite[11] = 0;
    StringToWrite[12] = 4;
  }
  */

  /*
        Calculate checksums
        Tested with: https://toolslick.com/math/bitwise/xor-calculator
  */
  // Hard coded for bytes 0-30 with checksum stored in byte 31
  checksum = StringToWrite[0];
  for (uint8_t address = 0; address < 30; address++) {
    checksum = checksum ^ StringToWrite[address+1];
    StringToWrite[page_one_two_checksum_position] = checksum;
  }
  // Hard coded for bytes 32-62 with checksum stored in byte 63
  checksum = StringToWrite[32];
  for (uint8_t address = 32; address < 62; address++) {
    checksum = checksum ^ StringToWrite[address+1];
    StringToWrite[page_three_four_checksum_position] = checksum;
  }

  /*
        Write the data
  */
  for (uint8_t address=0; address < string_len; address++) {
    uint8_t dataWrite = 0;
    dataWrite = StringToWrite[address];
    EEPROMExtWriteByte(EEPROM_ADDRESS, address, dataWrite);
  }

    /*
        Read and print the data to the serial port
  */
  uint8_t fails = 0;
  for (uint16_t address=0; address <= 63; address++) 
  {
    uint8_t dataRead = 0;
    dataRead =  EEPROMExtReadByte(EEPROM_ADDRESS, address);
    Serial.print(address);
    Serial.print(" : ");
    Serial.print(dataRead,DEC);
    if (dataRead == StringToWrite[address]) {
      Serial.println(" PASS");
    }
    else {
      Serial.println("       FAIL");
      fails++;
    }
  }
  if (fails==0){
    Serial.println();
    Serial.println("  ALL PASSED");
  }
  else {
    Serial.println("  ");
    Serial.print(fails);
    Serial.println(" FAILED");
  }
    Serial.println();
}

void EEPROMExtSetLastThree(uint16_t number) {
  if (number != 0) {
    SerialNumberLastThreeLocal = number;
  }
}

uint16_t EEPROMExtGetLastThree() {
  return (SerialNumberLastThreeLocal);
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
    if (temp <= 3) { LogicBoardTypeSet(temp);         }
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