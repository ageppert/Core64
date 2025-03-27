#include <Arduino.h>  // Required in order to build in PlatformIO

#include <PicoSoftwareSerial.h>

#include "SubSystems/Serial_Port.h"

int txPin = 1;  // SAO GPIO 1 (top)
int rxPin = 2;  // SAO GPIO 2 (bottom)
SoftwareSerial mySerial = SoftwareSerial(txPin, rxPin);

// plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that
// data values are less than 127:
void noteOn(int cmd, int pitch, int velocity) {
  mySerial.write(cmd);
  mySerial.write(pitch);
  mySerial.write(velocity);
}

void setup() {
  SerialPortSetup();
  // Set MIDI baud rate:
  mySerial.begin(31250);

}

void loop() {
  Serial.print("Hello world on the hardware serial port via USB cable."); 
  // play notes from F#-0 (0x1E) to F#-5 (0x5A):
  for (int note = 28; note < 33; note++) {
    //Note on channel 1 (0x90), some note value (note), middle velocity (0x45):
    noteOn(0x90, note, 0x01);
    delay(100);
    //Note on channel 1 (0x90), some note value (note), silent velocity (0x00):
    noteOn(0x90, note, 0x00);
    delay(100);
  }
}
