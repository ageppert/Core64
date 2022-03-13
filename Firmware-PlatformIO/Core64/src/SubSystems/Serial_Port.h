/*
PURPOSE: Set-up and share serial port messages with non-blocking code
SETUP: Use the built-in serial port, set up in Serial_Port.cpp
*/

#ifndef SERIAL_PORT_H
    #define SERIAL_PORT_H

    #if (ARDUINO >= 100)
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif

    #include <stdint.h>

    extern uint8_t DebugLevel;

    #define SERIAL_PORT_SPEED   15200

    void SerialPortSetup();
    void SerialPortProcess();

    void SetDebugLevel (uint8_t value);
    uint8_t GetDebugLevel ();

#endif // SERIAL_PORT_H
