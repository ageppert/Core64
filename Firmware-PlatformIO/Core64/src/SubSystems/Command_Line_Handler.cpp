#include "SubSystems/Command_Line_Handler.h"
#include "Mode_Manager.h"

#include <stdint.h>
#include <stdbool.h>

#include "Config/HardwareIOMap.h"
#include "Config/Firmware_Version.h"
#include "SubSystems/Heart_Beat.h"
#include "SubSystems/Serial_Port.h"
#include "Hal/LED_Array_HAL.h"
#include "Hal/Neon_Pixel_HAL.h"
#include "SubSystems/OLED_Screen.h"
#include "SubSystems/Analog_Input_Test.h"
#include "Hal/Buttons_HAL.h"
#include "Hal/Core_HAL.h"
#include "Hal/EEPROM_HAL.h"
#include "SubSystems/I2C_Manager.h"
#include "SubSystems/SD_Card_Manager.h"
#include "SubSystems/Ambient_Light_Sensor.h"
#include "Libraries/CommandLine/CommandLine.h"
#include "Drivers/Core_Driver.h"
#include "SubSystems/Test_Functions.h"
#include "Hal/Debug_Pins_HAL.h"
#include "Mode_Manager.h"

uint32_t SerialNumber;          // Default value is 0 and should be non-zero if the Serial Number is valid.

int StreamTopLevelModeEnable;

CommandLine commandLine(Serial, PROMPT);        // CommandLine instance.

// Teensy 3.2 Software Reset Definition
#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C                // Teensy 3.2
#define CPU_RESTART_VAL 0x5FA0004                              // Teensy 3.2
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);     // Teensy 3.2
// Raspi Pico Software Reset Definition
  // TO DO

void StreamTopLevelModeEnableSet     (bool value)  {   StreamTopLevelModeEnable = value; }
bool StreamTopLevelModeEnableGet ()                {   return (StreamTopLevelModeEnable); }

void coreTesting() {
  static uint8_t BitToTest = 63;
  /*
  static uint8_t c = 0;
  for (uint8_t i = 1; i <=2; i++)
  {
    CoreWriteBit(c,0); CoreWriteBit(c,1);
  }
  c++;
  if (c == 64) {c=0;}
  */
  // Read testing
  delay(1000);
  Core_Mem_Bit_Write(BitToTest,1);
  LED_Array_String_Write(BitToTest,1);
  LED_Array_String_Display();
  delay(1000);
  Core_Mem_Bit_Write(BitToTest,0);
  LED_Array_String_Write(BitToTest,0);
  LED_Array_String_Display();
  // CoreArrayMemory [0][3] = CoreReadBit(3);
  // Whole Array Testing
  //  for (uint8_t i = 0; i <= 63; i++ ) { CoreWriteBit(i,1); }
  //  for (uint8_t i = 0; i <= 63; i++ ) { CoreWriteBit(i,0); }
  // Column Testing
  // for (uint8_t i = 1; i <= 63; i=i+8 ) { CoreWriteBit(i,0); CoreWriteBit(i,1); }
  //for (uint8_t i = 0; i <= 7; i++ ) { CoreWriteBit(i,1); }
  // Row Testing
  // for (uint8_t i = 0; i <= 63; i=i+8 ) { CoreWriteBit(i,1); }
  // for (uint8_t i = 0; i <= 63; i=i+8 ) { CoreWriteBit(i,0); }
  /*
  CoreWriteBit(0,1);
  CoreWriteBit(0,1);
  CoreWriteBit(0,0);
  CoreWriteBit(7,1);
  CoreWriteBit(7,1);
  CoreWriteBit(0,0);
  //CoreReadBit(0);
  //CoreClearAll();
  */
}

void CommandLineSetup ()
{
  #if defined BOARD_CORE64_TEENSY_32
  // Pre-defined commands

  // On-the-fly commands -- instance is allocated dynamically
  commandLine.add("arrangement", &handleArrangement);
  commandLine.add("coretest", &handleCoreTest);
  commandLine.add("debug", &handleDebug);
  commandLine.add("dgauss", &handleDgauss);
  commandLine.add("help", &handleHelp);
  commandLine.add("info", &handleInfo);
  commandLine.add("mode", &handleMode);
  commandLine.add("reboot", &handleReboot);
  commandLine.add("restart", &handleRestart);
  commandLine.add("splash", &handleSplash);
  commandLine.add("stream", &handleStream);
  commandLine.add("thanks", &handleThanks);
  #elif defined BOARD_CORE64C_RASPI_PICO
  // Pre-defined commands

  // On-the-fly commands -- instance is allocated dynamically
  commandLine.add("debug", &handleDebug);
  commandLine.add("help", &handleHelp);
  commandLine.add("info", &handleInfo);
  commandLine.add("mode", &handleMode);
  commandLine.add("splash", &handleSplash);
  #endif
  Serial.println("  Command Line Setup complete.");
}

void  CommandLineUpdate()
{
  commandLine.update();
}

  void handleArrangement(char* tokens)
  {
    char* token = strtok(NULL, " ");
    Serial.print("  Core arrangement EEPROM value ");
    if (token == NULL)
    {
        Serial.print("is ");
        Serial.println(EEPROMExtReadCorePatternArrangement());
    }
    else if(strcmp(token,"normal") == 0)
    {
      Serial.println("  set to (1) normal. Requires reboot.");
      EEPROMExtWriteCorePatternArrangement(1);
      CoreSetup();
    }
    else if(strcmp(token,"opposite") == 0)
    {
      Serial.println("  set to (2) opposite.");
      EEPROMExtWriteCorePatternArrangement(2);
      CoreSetup();        
    }
    else
    {
      Serial.println("  invalid token.");    
    }
  }

  void handleCoreTest(char* tokens)
  {
    Serial.println("  Core Test");
    coreTesting();
  }

  void handleDebug(char* tokens)
  {
    char* token = strtok(NULL, " ");

    if (token != NULL) {
      SetDebugLevel(atoi(token));
    } 
    else {
      Serial.print("  Debug Level is: ");
      Serial.println(GetDebugLevel());
      Serial.println("    1 = housekeeping start/end");
      Serial.println("    2 = hall sensors and switches");
      Serial.println("    3 = system timers");
      Serial.println("    4 = sub-system app/game state machine number");
    }
  }

  void handleDgauss(char* tokens)
  {
    if (TopLevelModeGet()==MODE_DGAUSS_MENU) {
      if (TopLevelModePreviousGet() == 10) { TopLevelModePreviousSet(MODE_START_POWER_ON); } // If previous mode was DGAUSS menu, go back to STARTUP
      TopLevelModeSet(TopLevelModePreviousGet());
      TopLevelModePreviousSet(MODE_DGAUSS_MENU);
      TopLevelModeSetChanged (true);
      Serial.println("  Exit DGAUSS Menu.");
    }
    else {
      TopLevelModePreviousSet(TopLevelModeGet());
      TopLevelModeSet(MODE_DGAUSS_MENU);
      TopLevelModeSetChanged (true);
      Serial.println("  Jump to DGAUSS Menu.");
    }
  }

  void handleHelp(char* tokens)
  {
    Serial.println("  ---------------------");
    Serial.println("  |     HELP MENU     |");
    Serial.println("  ---------------------");
    Serial.println("  SOFT BUTTONS: M - + S");
    Serial.println("    M BUTTON to enter/exit DGAUSS Menu. Or back out of sub-menu.");
    Serial.println("      d = Demos ...cycle through demo modes with +/- BUTTONS.");
    Serial.println("      G = Games");
    Serial.println("      A = App");
    Serial.println("      u = Utils");
    Serial.println("      s = Special");
    Serial.println("      s = Settings");
    Serial.println("    + BUTTON scroll to next item in sub-menu list.");
    Serial.println("    - BUTTON scroll to previous item in sub-menu list.");
    Serial.println("    S BUTTON to select from sub-menu list.");
    Serial.println("");
    Serial.println("  SERIAL COMMANDS:");
    Serial.println("    arrangement            -> Query EEPROM for core arrangement value.");
    Serial.println("    arrangement normal     -> Set EEPROM for core arrangement normal / \\. Requires power cycle.");
    Serial.println("    arrangement opposite   -> Set EEPROM for core arrangement opposite \\ /.");
    Serial.println("    coretest               -> Test one core.");
    Serial.println("    debug [#]              -> Query or optionally set Debug Level.");
    Serial.println("    dgauss                 -> Enter/exit DGAUSS menu.");
    Serial.println("    help                   -> This help menu.");
    Serial.println("    info                   -> Query hardware and firmware info.");
    Serial.println("    mode [#]               -> Query or optionally set Top Level Mode.");
    Serial.println("    reboot                 -> Software initiated hard reboot.");
    Serial.println("    restart                -> Software initiated soft restart.");
    Serial.println("    splash                 -> Splash screen text.");
    Serial.println("    stream                 -> Togggles the streaming mode.");
    Serial.println("    stream start           -> Starts the streaming mode.");
    Serial.println("    stream stop            -> Stops the streaming mode.");
    Serial.println();
  }

  void handleInfo(char* tokens)
  {
    ReadHardwareVersion();
    SerialNumber = EEPROMExtReadSerialNumber();
    Serial.println("  ----------------");
    Serial.println("  |     INFO     |");
    Serial.println("  ----------------");

    #if defined BOARD_CORE64_TEENSY_32
      Serial.print("  Core64 Logic Board with ");
      Serial.print(BOARD);
      Serial.println(". Hardware configuration in HardwareIOMap.h");
    #elif defined BOARD_CORE64C_RASPI_PICO
      Serial.print("  Core64c Logic Board with ");
      Serial.print(BOARD);
      Serial.println(". Hardware configuration in HardwareIOMap.h");
    #else
      Serial.println("  Unknown Logic Board with unknown Microcontroller Board.");
    #endif

    Serial.print("  Hardware Version: ");
    Serial.print(HardwareVersionMajor);
    Serial.print(".");
    Serial.print(HardwareVersionMinor);
    Serial.print(".");
    Serial.print(HardwareVersionPatch);
    Serial.print("     Serial Number: ");
    #if defined BOARD_CORE64_TEENSY_32
      char SerialNumberPadded[6];
      sprintf(SerialNumberPadded, "%06lu", SerialNumber);
      Serial.print(SerialNumberPadded);
    #elif defined BOARD_CORE64C_RASPI_PICO
      //TODO: print the serial number padded with leading zeros with RP2040
      Serial.print(SerialNumber);
    #endif
    Serial.print("     Born on: 20");
    Serial.print(EEPROMExtReadBornOnYear());
    Serial.print("-");
    Serial.print(EEPROMExtReadBornOnMonth());
    Serial.print("-");
    Serial.println(EEPROMExtReadBornOnDay());    

    Serial.print("  Voltages: Input (USB or Bat.): ");
    Serial.print(GetBatteryVoltageV(),2);
    Serial.print(", 5V0 Rail: ");
    Serial.print(GetBus5V0VoltageV(),2);
    Serial.print(", 3V3 Rail: ");
    Serial.println(GetBus3V3VoltageV(),2);

    Serial.print("  Firmware Version: ");
    Serial.print(FirmwareVersionMajor);
    Serial.print(".");
    Serial.print(FirmwareVersionMinor);
    Serial.print(".");
    Serial.print(FirmwareVersionPatch);
//    Serial.print("-");
    Serial.print(FirmwareVersion);
    Serial.print(" (Built on ");
    Serial.print(compile_date);
    Serial.println(")");

    Serial.print("  ");
    Serial.println(FIRMWARE_DESCRIPTION);
    Serial.println("  For more details see https://www.github.com/ageppert/Core64");
    if( HardwareConnectedCheckButtonHallSensors() ) {
      Serial.println("      HardwareConnectedCheckButtonHallSensors = true");
    }
    Serial.println();
  }

  /*
  Notes:
    In order to compare the token string to a text string, the "compareTo" function does not work
      if (token.compareTo("text") == 0 )
    Instead, use "strcmp"
      if (strcmp(token,"text") == 0 )
    Because nameBuffer cannot be accessed with a . operator. See https://stackoverflow.com/questions/27689345/request-for-member-compareto-in-chararraybuffer-which-is-of-non-class-type-c
  */

  void handleMode(char* tokens)
  {  
    char* token = strtok(NULL, " ");

    if (token != NULL) {
      TopLevelModePreviousSet(TopLevelModeGet());
      TopLevelModeSet(atoi(token));
      TopLevelModeSetChanged (true);
    } 
    else {
      for( uint16_t i = 0; i <= MODE_LAST; i++) {
        Serial.print("  ");
        if (i<10) {
          Serial.print(" ");
        }        
        Serial.print(i);
        Serial.print(" = ");
        Serial.println(TOP_LEVEL_MODE_NAME_ARRAY[i]);
      }
      Serial.print("  Current TopLevelMode is: ");
      Serial.print(TopLevelModeGet());
      Serial.print(" ");
      Serial.println(TOP_LEVEL_MODE_NAME_ARRAY[TopLevelModeGet()]);
    }
  }

  void handleReboot(char* tokens)
  {
    Serial.println("  SOFTWARE INITIATED HARD REBOOT NOW!");
    delay(1000);
    CPU_RESTART; // Teensy 3.2       
  }

  void handleRestart(char* tokens)
  {
    Serial.println("  Software initiated soft restart now.");
    delay(1000);
    TopLevelModePreviousSet(TopLevelModeGet());
    TopLevelModeSet(0);
    TopLevelModeSetChanged (true); 
  }


  void handleSplash(char* tokens)
  {
    Serial.println();
    Serial.println("    ____                __   _  _   ");
    Serial.println("   / ___|___  _ __ ___ / /_ | || |  ");
    Serial.println("  | |   / _ \\| '__/ _ \\ '_ \\| || |_ ");
    Serial.println("  | |__| (_) | | |  __/ (_) |__   _|");
    Serial.println("   \\____\\___/|_|  \\___|\\___/   |_|  ");
    Serial.println();
    Serial.println("  Core64 Interactive Core Memory - Project website: www.Core64.io");
    Serial.println("  2019-2022 Concept and Design by Andy Geppert at www.MachineIdeas.com");
    Serial.println("  This source code: https://www.github.com/ageppert/Core64");
    Serial.println("  See main.cpp for IDE and library requirements.");
    Serial.println("  See HardwareIOMap.h for hardware configuration.");
    Serial.println();
  }

  void handleStream(char* tokens)
  {
    char* token = strtok(NULL, " ");
    Serial.print("  Stream ");
    if (token == NULL)
    {
        Serial.println("toggled.");
        StreamTopLevelModeEnable = !StreamTopLevelModeEnable ;
    }
    else if(strcmp(token,"start") == 0)
    {
        Serial.println("started.");
        StreamTopLevelModeEnable = 1 ;
    }
    else if(strcmp(token,"stop") == 0)
    {
        Serial.println("stopped.");
        StreamTopLevelModeEnable = 0 ;
    }
    else
    {
        Serial.println("invalid token.");    
    }
  }

  void handleThanks(char* tokens)
  {
    Serial.println();
    Serial.println("  Thank you for YOUR support! And thank you to the following people for inspiration and encouragement:");
    Serial.println("    Ben North and Olver Nash's Magnetic Core Memory Reborn Report,");
    Serial.println("    Jussi Kilpelainen's Arduino Core Memory Shield, Rolfe Bozier's 8x8 core demo,");
    Serial.println("    Element14 Magnetism Contest, Hack-a-Day crew and fans, my beta test customers,");
    Serial.println("    by supportive family and friends, Alex Glo @ Hackster, VCFed crew and fans, ");
    Serial.println("    Dag @ The CHM, and Adafruit / Sparkfun tutorials.");
    Serial.println();
  }
