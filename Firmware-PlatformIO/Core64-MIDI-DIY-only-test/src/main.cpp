#include <Arduino.h>  // Required in order to build in PlatformIO
#include <PicoSoftwareSerial.h>

int txPin = 1;  // SAO GPIO 1 (top)
int rxPin = 2;  // SAO GPIO 2 (bottom)
SoftwareSerial mySerial = SoftwareSerial(txPin, rxPin);

void noteOn(int cmd, int pitch, int velocity) {
  mySerial.write(cmd);
  mySerial.write(pitch);
  mySerial.write(velocity);
}

void setup() {
  // Set MIDI baud rate:
  mySerial.begin(31250);
}

void loop() {
  // play notes from F#-0 (0x1E) to F#-5 (0x5A):
  for (int note = 28; note < 33; note++) {
    //Note on channel 1 (0x90), some note value (note), middle velocity (0x45):
    noteOn(0x90, note, 0x01);
    delay(1500);
    //Note on channel 1 (0x90), some note value (note), silent velocity (0x00):
    noteOn(0x90, note, 0x00);
    delay(100);
  }
}
