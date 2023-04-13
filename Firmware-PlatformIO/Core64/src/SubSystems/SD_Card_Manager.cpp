#include <stdint.h>
#include <stdbool.h>

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include "SubSystems/SD_Card_Manager.h"
#include "Config/HardwareIOMap.h"
#include "SubSystems/Analog_Input_Test.h"
#include "SubSystems/Ambient_Light_Sensor.h"

#define DEBUG_SDCARD

bool SDCardPresent = false;     // Keep track of whether the card is present or not.

#if defined  MCU_TYPE_MK20DX256_TEENSY_32

  #ifdef SDCARD_ENABLE // is true
    #include "SD_Card_Manager.h"
    #include <SD.h>
    #include <SPI.h>

    // set up variables using the SD utility library functions:
    Sd2Card card;
    SdVolume volume;
    SdFile root;

    const int chipSelect = Pin_SPI_SD_CS;
    
    // The SD library does not seem to care about the Card Detect input to work or not. It's more of a user nicety.
    // The SD library just goes by the communication response on the SPI bus. 
    #ifdef Pin_SPARE_3_Assigned_To_SPI_SD_CD_Input
    const int cardDetect = Pin_SPARE_3_CP_ADDR_2;
    #else
    const int cardDetect = Pin_Sense_Pulse;  // A safe pin to use since it already set as an input pin in the Core_Driver
    #endif

    // Log file base name.  Must be six characters or less.
    #define FILE_BASE_NAME "Data"

    enum SDCardState
      {
        STATE_START_UP = 0,               //  0 
        STATE_CARD_DETECT,                //  1 
        STATE_CHECK_FILE_PRESENT,         //  2 
        STATE_CREATE_FILE,                //  3 
        STATE_APPEND_FILE,                //  4 
        STATE_WRITE_LINE,                 //  5 
        STATE_CLOSE_FILE,                 //  6
        STATE_ERROR                       //  7
      };
    static uint8_t SDCardState = STATE_START_UP;
    
    // SD Card File Handling for Logging
    const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
    char fileName[13] = FILE_BASE_NAME "00.csv";
    // SdFile file;
    // Error messages stored in flash.
    // #define error(msg) sd.errorHalt(F(msg))

    void SDCardSelectTwiddle (uint8_t number)
    {
      while(number)
      {
        digitalWriteFast(chipSelect, 0);
        number--;
        delay(1);
        digitalWriteFast(chipSelect, 1);
      }
    }

    void SDCardSetup()
    {
      if(HardwareVersionMinor==2)
        {return;}
      else
      {
        pinMode(cardDetect, INPUT_PULLUP);

        SPI.setMISO(Pin_SPI_SDI); // 12
        SPI.setMOSI(Pin_SPI_SDO); // 11
        SPI.setSCK(Pin_SPI_CLK);  // 13
        SPI.begin();                  //   <<<--- THE MISSING KEY TO MAKING THE setCLK assignment work!!!

        Serial.println("\nInitializing SD card...");
        Serial.print("SPI CLK PIN:");
        Serial.println(Pin_SPI_CLK);
        Serial.print("SPI SDO PIN:");
        Serial.println(Pin_SPI_SDO);
        Serial.print("SPI SDI PIN:");
        Serial.println(Pin_SPI_SDI);
        Serial.print("SPI CS PIN:");
        Serial.println(Pin_SPI_SD_CS);
        Serial.print("SPI CD PIN:");
        Serial.println(cardDetect);

    // see if the card is present and can be initialized:
      SDCardSelectTwiddle(1);
      if (!SD.begin(chipSelect)) {
        Serial.println("Card communication failed.");
        if (digitalReadFast(cardDetect)==1)
        {
          SDCardPresent = true;
          Serial.println("SDCardPresent = true");
        }
        else
        {
           SDCardPresent = false; 
           Serial.println("SDCardPresent = false");          
        }
        return;
      }
      Serial.println("card initialized.");
    
        // we'll use the initialization code from the utility libraries
        // since we're just testing if the card is working!
        if (!card.init(SPI_HALF_SPEED, chipSelect)) {
          Serial.println("initialization failed. Things to check:");
          Serial.println("* is a card inserted?");
          Serial.println("* is your wiring correct?");
          Serial.println("* did you change the chipSelect pin to match your shield or module?");
          if (digitalReadFast(cardDetect)==0)
           {
             SDCardPresent = false; 
             Serial.println("SDCardPresent = false");
           }
          return;
        } 
        else {
          Serial.println("Wiring (CLK, SDO, SDI, CS) is correct.");
          if (digitalReadFast(cardDetect)==1)
          {
            SDCardPresent = true;
            Serial.println("SDCardPresent = true");
          }
          else
          {
             SDCardPresent = false; 
             Serial.println("SDCardPresent = false");          
          }
        }


        // print the type of card
        Serial.print("\nCard type: ");
        switch(card.type()) {
          case SD_CARD_TYPE_SD1:
            Serial.println("SD1");
            break;
          case SD_CARD_TYPE_SD2:
            Serial.println("SD2");
            break;
          case SD_CARD_TYPE_SDHC:
            Serial.println("SDHC");
            break;
          default:
            Serial.println("Unknown");
        }

        // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
        if (!volume.init(card)) {
          Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
          return;
        }


        // print the type and size of the first FAT-type volume
        uint32_t volumesize;
        Serial.print("\nVolume type is FAT");
        Serial.println(volume.fatType(), DEC);
        Serial.println();
        
        volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
        volumesize *= volume.clusterCount();       // we'll have a lot of clusters
        if (volumesize < 8388608ul) {
          Serial.print("Volume size (bytes): ");
          Serial.println(volumesize * 512);        // SD card blocks are always 512 bytes
        }
        Serial.print("Volume size (Kbytes): ");
        volumesize /= 2;
        Serial.println(volumesize);
        Serial.print("Volume size (Mbytes): ");
        volumesize /= 1024;
        Serial.println(volumesize);

        /*
        Serial.println("\nFiles found on the card (name, date and size in bytes): ");
        root.openRoot(volume);
        
        // list all files in the card with date and size
        root.ls(LS_R | LS_DATE | LS_SIZE);
        */
      }
    }

  void SDInfo(){
    return;
  }

  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  void SDCardWriteVoltageLine()
    {
      switch(SDCardState)
        {
        case STATE_START_UP:
          Serial.println("SDCardState START_UP");
          SDCardState = STATE_CARD_DETECT;
          break;

        case STATE_CARD_DETECT:
          Serial.println("SDCardState CARD_DETECT");
          if (digitalReadFast(cardDetect)==1)
          {
            SDCardPresent = true;
            Serial.println("SDCardPresent = true");
            SDCardState = STATE_CHECK_FILE_PRESENT;
          }
          else
          {
            SDCardPresent = false;
            Serial.println("SDCardPresent = false");
            SDCardState = STATE_START_UP;          
          }
          break;

        case STATE_CHECK_FILE_PRESENT:
          Serial.println("SDCardState CHECK_FILE_PRESENT");

          SDCardState = STATE_CREATE_FILE;          
          break;

        case STATE_CREATE_FILE:
          {
            Serial.println("SDCardState CREATE_FILE");
            static bool FirstTimeRun = 0;

            if (FirstTimeRun==0)
            {
              // make a string for assembling the data to log:
              String HeaderString = "";
              HeaderString += "Millis, Battery mV, brightness 8bit ***************"; 

              dataFile = SD.open("datalog.txt", FILE_WRITE);
              // if the file is available, write to it:
              if (dataFile) {
                dataFile.println(HeaderString);
                dataFile.close();
                // print to the serial port too:
                Serial.println(HeaderString);
              }  
              // if the file isn't open, pop up an error:
              else {
                Serial.println("error opening datalog.txt");
              }
              FirstTimeRun = 1;
            } 

    /*
            // Find an unused file name.
            if (BASE_NAME_SIZE > 6) {
              error("FILE_BASE_NAME too long");
            }
            while (sd.exists(fileName)) {
              if (fileName[BASE_NAME_SIZE + 1] != '9') {
                fileName[BASE_NAME_SIZE + 1]++;
              } else if (fileName[BASE_NAME_SIZE] != '9') {
                fileName[BASE_NAME_SIZE + 1] = '0';
                fileName[BASE_NAME_SIZE]++;
              } else {
                error("Can't create file name");
              }
            }
            if (!file.open(fileName, O_WRONLY | O_CREAT | O_EXCL)) {
              error("file.open");
            }
            Serial.print(F("Logging to: "));
            Serial.println(fileName);
    */
  /*
            // make a string for assembling the data to log:
            String dataString = "";
            uint32_t timer = millis();
            uint16_t sensor = GetBatteryVoltagemV();
            uint8_t light = GetAmbientLightLevel8BIT();
            dataString += String(timer);
            dataString += ",";
            dataString += String(sensor);
            dataString += ",";
            dataString += String(light);

            dataFile = SD.open("datalog.txt", FILE_WRITE);
            // if the file is available, write to it:
            if (dataFile) {
              dataFile.println(dataString);
              dataFile.close();
              // print to the serial port too:
              Serial.println(dataString);
            }  
            // if the file isn't open, pop up an error:
            else {
              Serial.println("error opening datalog.txt");
            } 
  */

            SDCardState = STATE_WRITE_LINE;     
            break;
          }
        case STATE_APPEND_FILE:
          Serial.println("SDCardState APPEND_FILE");
  /*
          // make a string for assembling the data to log:
          String dataString = "";
          uint16_t sensor = GetBatteryVoltagemV();
          dataString += String(sensor);

          dataFile = SD.open("datalog.txt", FILE_WRITE);
          // if the file is available, write to it:
          if (dataFile) {
            dataFile.println(dataString);
            dataFile.close();
            // print to the serial port too:
            Serial.println(dataString);
          }  
          // if the file isn't open, pop up an error:
          else {
            Serial.println("error opening datalog.txt");
          } 
  */
          break;

        case STATE_WRITE_LINE:
          {
            Serial.println("SDCardState WRITE_LINE");

            // make a string for assembling the data to log:
            String dataString = "";
            uint32_t timer = millis();
            uint16_t sensor = GetBatteryVoltagemV();
            uint8_t light = GetAmbientLightLevel8BIT();
            dataString += String(timer);
            dataString += ",";
            dataString += String(sensor);
            dataString += ",";
            dataString += String(light);

            dataFile = SD.open("datalog.txt", FILE_WRITE);
            // if the file is available, write to it:
            if (dataFile) {
              dataFile.println(dataString);
              dataFile.close();
              // print to the serial port too:
              Serial.println(dataString);
            }  
            // if the file isn't open, pop up an error:
            else {
              Serial.println("error opening datalog.txt");
            } 

          break;
          }
        case STATE_CLOSE_FILE:
          Serial.println("SDCardState CLOSE_FILE");

          break;

        case STATE_ERROR:
          Serial.println("SDCardState ERROR");

          break;

        default:
          Serial.println("SDCardState UNDEFINED");
          break;
        }

    }

    void SDCardVoltageLog(uint32_t UpdatePeriodms)
    {
      static uint32_t NowTime = 0;
      static uint32_t UpdateTimer = 0;
      if ((HardwareVersionMajor == 0) && (HardwareVersionMinor == 3))
      {
        NowTime = millis();
        SDCardSelectTwiddle(5); // testing to see if twiddling CS for other use affects the SD Card
        if ((NowTime - UpdateTimer) >= UpdatePeriodms)
        {
          UpdateTimer = NowTime;
          SDCardWriteVoltageLine();                          
        }
      }
    }

  #else // SDCARD_ENABLE // is false
    void SDCardSetup()
    {
      if ((HardwareVersionMajor == 0) && (HardwareVersionMinor == 3))
      {
        pinMode(Pin_SPI_SD_CS, OUTPUT);
        digitalWriteFast(Pin_SPI_SD_CS, 1);
      }
    }
    void SDCardVoltageLog(uint32_t UpdatePeriodms)
    {
    }

    void SDInfo(){
      Serial.println("  SDInfo did not run because SDCARD is not enabled in HardwareIOMap.h");
      return;
    }
  #endif // SDCARD_ENABLE

#elif defined MCU_TYPE_RP2040
  #ifdef SDCARD_ENABLE
    /****************************************************************************************************************************
      ReadWrite.ino
      
      For all RP2040 boads using Arduimo-mbed or arduino-pico core
      
      RP2040_SD is a library enable the usage of SD on RP2040-based boards
      
      This Library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
      as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

      This Library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

      You should have received a copy of the GNU General Public License along with the Arduino SdFat Library.  
      If not, see <http://www.gnu.org/licenses/>.
      
      Based on and modified from  Arduino SdFat Library (https://github.com/arduino/Arduino)
      
      (C) Copyright 2009 by William Greiman
      (C) Copyright 2010 SparkFun Electronics
      (C) Copyright 2021 by Khoi Hoang
      
      Built by Khoi Hoang https://github.com/khoih-prog/RP2040_SD
      Licensed under GPL-3.0 license
    *****************************************************************************************************************************/
    /*
      SD card connection

      This example shows how to read and write data to and from an SD card file
      The circuit:
      SD card attached to SPI bus as follows:
      // Arduino-pico core
      ** MISO - pin 16
      ** MOSI - pin 19
      ** CS   - pin 17
      ** SCK  - pin 18

      // Arduino-mbed core
      ** MISO - pin 4
      ** MOSI - pin 3
      ** CS   - pin 5
      ** SCK  - pin 2
    */


    #if !defined(ARDUINO_ARCH_RP2040)
      #error For RP2040 only
    #endif

    #if defined(ARDUINO_ARCH_MBED)
      
      #define PIN_SD_MOSI       PIN_SPI_MOSI
      #define PIN_SD_MISO       PIN_SPI_MISO
      #define PIN_SD_SCK        PIN_SPI_SCK
      #define PIN_SD_SS         PIN_SPI_SS

    #else

      #define PIN_SD_MOSI       PIN_SPI0_MOSI
      #define PIN_SD_MISO       PIN_SPI0_MISO
      #define PIN_SD_SCK        PIN_SPI0_SCK
      #define PIN_SD_SS         PIN_SPI0_SS
      
    #endif

    #define _RP2040_SD_LOGLEVEL_       4

    #include <SPI.h>
    #include <RP2040_SD.h>

    File myFile;

// #define RP2040_SOFT_SPI    /// It should be OK to remove this line but I haven't tried it with an SD Card yet.
// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
File root;
const int chipSelect = PIN_SD_SS;

    void SDCardSetup() 
    {
      // Open serial communications and wait for port to open:
      // Serial.begin(115200);
      // while (!Serial);
      // delay(1000);

    #if defined(ARDUINO_ARCH_MBED)
      Serial.print("Starting SD Card ReadWrite on MBED ");
    #else
      Serial.print("Starting SD Card ReadWrite on ");
    #endif
      
      Serial.println(BOARD_NAME);
      Serial.println(RP2040_SD_VERSION);
      
      Serial.print("Initializing SD card with SS = ");  Serial.println(PIN_SD_SS);
      Serial.print("SCK = ");   Serial.println(PIN_SD_SCK);
      Serial.print("MOSI = ");  Serial.println(PIN_SD_MOSI);
      Serial.print("MISO = ");  Serial.println(PIN_SD_MISO);

      if (!SD.begin(PIN_SD_SS)) {
        Serial.println("Initialization failed!");
        SDCardPresent = false; 
        Serial.println("SDCardPresent = false");     
        return;
      }
      else {
        Serial.println("Initialization done.");
        SDCardPresent = true;
        Serial.println("SDCardPresent = true");
      }

      #define fileName  "newtest0.txt"
      char writeData[]  = "Testing writing to " fileName;
      
      // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      myFile = SD.open(fileName, FILE_WRITE);

      // if the file opened okay, write to it:
      if (myFile) 
      {
        Serial.print("Writing to "); Serial.print(fileName); 
        Serial.print(" ==> "); Serial.println(writeData);

        myFile.println(writeData);
        
        // close the file:
        myFile.close();
        Serial.println("done.");
      } 
      else 
      {
        // if the file didn't open, print an error:
        Serial.print("Error opening "); Serial.println(fileName);
      }

      // re-open the file for reading:
      myFile = SD.open(fileName, FILE_READ);
      
      if (myFile) 
      {
        Serial.print("Reading from "); Serial.println(fileName);
        Serial.println("===============");

        // read from the file until there's nothing else in it:
        while (myFile.available()) 
        {
          Serial.write(myFile.read());
        }

        // close the file:
        myFile.close();

        Serial.println("===============");
      } 
      else 
      {
        // if the file didn't open, print an error:
        Serial.print("Error opening "); Serial.println(fileName);
      }
    }

void printDirectory(File dir, int numTabs) 
{
  while (true) 
  {
    File entry =  dir.openNextFile();

    if (! entry) 
    {
      // no more files
      break;
    }
    
    for (uint8_t i = 0; i < numTabs; i++) 
    {
      Serial.print('\t');
    }
    
    Serial.print(entry.name());
    
    if (entry.isDirectory()) 
    {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } 
    else 
    {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    
    entry.close();
  }
}

void SDInfo() {
  #if defined(ARDUINO_ARCH_MBED)
    Serial.print("Starting SD Card CardInfo on MBED ");
  #else
    Serial.print("Starting SD Card CardInfo on ");
  #endif
    
    Serial.println(BOARD_NAME);
    Serial.println(RP2040_SD_VERSION);
    
    Serial.print("Initializing SD card with SS = ");  Serial.println(PIN_SD_SS);
    Serial.print("SCK = ");   Serial.println(PIN_SD_SCK);
    Serial.print("MOSI = ");  Serial.println(PIN_SD_MOSI);
    Serial.print("MISO = ");  Serial.println(PIN_SD_MISO);

    // we'll use the initialization code from the utility libraries
    // since we're just testing if the card is working!
    if (!card.init(SPI_HALF_SPEED, PIN_SD_SS)) 
    {
      Serial.println("initialization failed. Things to check:");
      Serial.println("* is a card inserted?");
      Serial.println("* is your wiring correct?");
      Serial.println("* did you change the chipSelect pin to match your shield or module?");
      SDCardPresent = false;
      Serial.println("SDCardPresent = false");
      // while (1);
      return;
    } 
    else 
    {
      Serial.println("Wiring is correct and a card is present.");
      SDCardPresent = true;
      Serial.println("SDCardPresent = true");
    }

    // print the type of card
    Serial.println();
    Serial.print("Card type:         ");
    
    switch (card.type()) 
    {
      case SD_CARD_TYPE_SD1:
        Serial.println("SD1");
        break;
      case SD_CARD_TYPE_SD2:
        Serial.println("SD2");
        break;
      case SD_CARD_TYPE_SDHC:
        Serial.println("SDHC");
        break;
      default:
        Serial.println("Unknown");
    }

    // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
    if (!volume.init(card)) 
    {
      Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
      // while (1);
      return;
    }

    Serial.print("Clusters:          ");
    Serial.println(volume.clusterCount());
    Serial.print("Blocks x Cluster:  ");
    Serial.println(volume.blocksPerCluster());

    Serial.print("Total Blocks:      ");
    Serial.println(volume.blocksPerCluster() * volume.clusterCount());
    Serial.println();

    // print the type and size of the first FAT-type volume
    uint32_t volumesize;
    Serial.print("Volume type is:    FAT");
    Serial.println(volume.fatType(), DEC);

    volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
    volumesize *= volume.clusterCount();       // we'll have a lot of clusters
    volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
    Serial.print("Volume size (Kb):  ");
    Serial.println(volumesize);
    Serial.print("Volume size (Mb):  ");
    volumesize /= 1024;
    Serial.println(volumesize);
    Serial.print("Volume size (Gb):  ");
    Serial.println((float)volumesize / 1024.0);

    if (!SD.begin(PIN_SD_SS)) {
      Serial.println("Initialization failed!");
      return;
    }
    else {
      Serial.println("Initialization done.");
      // Serial.println("\nFiles found on the card (name, date and size in bytes): ");
      // root = SD.open("/");
      // printDirectory(root, 0);
    }
  }

  #else
    void SDInfo()
    {
      Serial.println("  SDInfo did not run because SDCARD is not enabled in HardwareIOMap.h");
      return;
    }

  #endif // SDCARD_ENABLE

#endif
