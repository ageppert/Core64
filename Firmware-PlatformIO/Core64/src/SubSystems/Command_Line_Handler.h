/*
PURPOSE: Handle the commands received in the serial port.
SETUP:
*/
 
#ifndef COMMAND_LINE_HANDLER_H
    #define COMMAND_LINE_HANDLER_H

    #if (ARDUINO >= 100)
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif

    #include <stdint.h>
    #include "Config/HardwareIOMap.h"   // Detects board type

    #define PROMPT "Core> "

    void StreamTopLevelModeEnableSet (bool value);
    bool StreamTopLevelModeEnableGet ();
    void CommandLineEnableSet (bool value);
    bool CommandLineEnableGet ();

    void CommandLineSetup ();
    void handleArrangement(char* tokens);
    void handleCoreTest(char* tokens);
    void handleCoreToStartTest(char* tokens);
    void handleCoreToEndTest(char* tokens);
    void handleDebug(char* tokens);
    void handleDgauss(char* tokens);
    void handleHelp(char* tokens);
    void handleInfo(char* tokens);
    void handleMode(char* tokens);
    void handleReboot(char* tokens);
    void handleRestart(char* tokens);
    void handleSerialNumber(char* tokens);
    void handleSplash(char* tokens);
    void handleStream(char* tokens);
    void handleThanks(char* tokens);
    void CommandLineUpdate();

#endif
