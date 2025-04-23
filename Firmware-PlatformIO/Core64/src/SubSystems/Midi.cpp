#include <stdint.h>
#include <stdbool.h>

#include "SubSystems/Midi.h"
#include "Config/HardwareIOMap.h"

#if defined  MCU_TYPE_MK20DX256_TEENSY_32 
  #include <SoftwareSerial.h>
#elif defined MCU_TYPE_RP2040
  #include <PicoSoftwareSerial.h>
#endif


uint8_t txPin = 1;  // SAO GPIO 1 (top)
uint8_t rxPin = 2;  // SAO GPIO 2 (bottom)
SoftwareSerial mySerial = SoftwareSerial(txPin, rxPin);

void MidiSetup() {
  #if defined SAO_MIDI
    mySerial.begin(MIDI_PORT_SPEED);
  #endif
}

void noteOn(uint8_t cmd, uint8_t pitch, uint8_t velocity) {
  #if defined SAO_MIDI
    mySerial.write(cmd);
    mySerial.write(pitch);
    mySerial.write(velocity);
  #endif
}

void noteOff(uint8_t cmd, uint8_t pitch) {
  #if defined SAO_MIDI
    mySerial.write(cmd);
    mySerial.write(pitch);
    uint8_t VelocityOff = 0;
    mySerial.write(VelocityOff);
  #endif
}

uint8_t MidiPortRead() {
  #if defined SAO_MIDI
    return mySerial.read();
  #endif
}