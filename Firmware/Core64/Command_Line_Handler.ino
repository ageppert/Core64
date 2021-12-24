// Command Line Stuff. This gets included in the main Core64.ino file.

// Teensy 3.2 Software Reset Definition
  #define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C                // Teensy 3.2
  #define CPU_RESTART_VAL 0x5FA0004                              // Teensy 3.2
  #define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);     // Teensy 3.2
// Raspi Pico Software Reset Definition
  // TO DO

void CommandLineSetup ()
{
  #if defined BOARD_CORE64_TEENSY_32
  // Pre-defined commands

  // On-the-fly commands -- instance is allocated dynamically
  commandLine.add("arrangement", &handleArrangement);
  commandLine.add("coretest", &handleCoreTest);
  commandLine.add("help", &handleHelp);
  commandLine.add("info", &handleInfo);
  commandLine.add("mode", &handleMode);
  commandLine.add("reboot", &handleReboot);
  commandLine.add("splash", &handleSplash);
  commandLine.add("stream", &handleStream);
  #elif defined BOARD_CORE64C_RASPI_PICO
  // Pre-defined commands

  // On-the-fly commands -- instance is allocated dynamically
  commandLine.add("help", &handleHelp);
  commandLine.add("info", &handleInfo);
  commandLine.add("mode", &handleMode);
  commandLine.add("splash", &handleSplash);
  #endif
}

/**
 * Handle the count command. The command has one additional argument that can be the integer to set the count to.
 *
 * @param tokens The rest of the input command.
 */
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

  /**
   * Print some help.
   *
   * @param tokens The rest of the input command.
   */
  void handleHelp(char* tokens)
  {
    Serial.println("  ---------------------");
    Serial.println("  ----- HELP MENU -----");
    Serial.println("  ---------------------");
    Serial.println("  arrangement            // Query EEPROM for core arrangement value.");
    Serial.println("  arrangement normal     // Set EEPROM for core arrangement normal / \\. Requires reboot.");
    Serial.println("  arrangement opposite   // Set EEPROM for core arrangement opposite \\ /.");
    Serial.println("  coretest               // Test one core.");
    Serial.println("  help                   // This help menu.");
    Serial.println("  info                   // Query hardware and firmware info.");
    Serial.println("  mode                   // Query or set TopLevelMode.");
    Serial.println("  reboot                 // Software reboot.");
    Serial.println("  splash                 // Splash screen text.");
    Serial.println("  stream                 // Togggles the streaming mode.");
    Serial.println("  stream start           // Starts the streaming mode.");
    Serial.println("  stream stop            // Stops the streaming mode.");
    Serial.println();
  }

  void handleInfo(char* tokens)
  {
    DetectHardwareVersion();
    SerialNumber = EEPROMExtReadSerialNumber();
    Serial.println("  ----------------");
    Serial.println("  ----- INFO -----");
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
    Serial.print(SerialNumber);
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
    Serial.print("-");
    Serial.print(FirmwareVersion);
    Serial.print(" (Compiled on: ");
    Serial.print(compile_date);
    Serial.println(")");

    Serial.print("  ");
    Serial.println(FIRMWARE_SUMMARY);
    Serial.println("  For more details see https://www.github.com/ageppert/Core64");

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
      TopLevelMode = atoi(token);
      TopLevelModeChanged = true;
    } 
    else {
      Serial.print("  Mode is: ");
      Serial.println(TopLevelMode);
    }
  }

  void handleReboot(char* tokens)
  {
    Serial.println("  SOFTWARE INITIATED REBOOT NOW!");
    delay(1000);
    CPU_RESTART; // Teensy 3.2       
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
    Serial.println("  2019-2021 Concept and Design by Andy Geppert of www.MachineIdeas.com");
    Serial.println("  This source code: https://www.github.com/ageppert/Core64");
    Serial.println("  See Core64.ino for IDE and library requirements.");
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
        StreamEnable = !StreamEnable ;
    }
    else if(strcmp(token,"start") == 0)
    {
        Serial.println("started.");
        StreamEnable = 1 ;
    }
    else if(strcmp(token,"stop") == 0)
    {
        Serial.println("stopped.");
        StreamEnable = 0 ;
    }
    else
    {
        Serial.println("invalid token.");    
    }
  }
