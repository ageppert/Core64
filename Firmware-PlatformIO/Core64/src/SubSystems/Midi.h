/*
PURPOSE: Midi via a software serial port
SETUP: MidiSetup() to start the serial port for MIDI communications
USE: 
    COMMAND             |   NOTE F#-0 (0x1E) to F#-5 (0x5A)     |   Velocity
    --------------------------------------------------------------------------------------------
    Channel 1 (0x90)    |   Note value                          |   0 to 127

*/

#ifndef MIDI_H
    #define MIDI_H

    #if (ARDUINO >= 100)
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif

    #include <stdint.h>

    #define MIDI_PORT_SPEED   31250

    void MidiSetup();
    void noteOn(uint8_t cmd, uint8_t pitch, uint8_t velocity);
    void noteOff(uint8_t cmd, uint8_t pitch);
    uint8_t MidiPortRead();

#endif // MIDI_H
