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

    #if defined BOARD_CORE64_TEENSY_32
        #define PROMPT "Core64> "
    #elif defined BOARD_CORE64C_RASPI_PICO
        #define PROMPT "Core64c: "
    #else
        #define PROMPT "unknown: "
    #endif

    void CommandLineSetup ();
    void handleArrangement(char* tokens);
    void handleCoreTest(char* tokens);
    void handleDebug(char* tokens);
    void handleHelp(char* tokens);
    void handleInfo(char* tokens);
    void handleMode(char* tokens);
    void handleReboot(char* tokens);
    void handleSplash(char* tokens);
    void handleStream(char* tokens);
    void CommandLineUpdate();

#endif
