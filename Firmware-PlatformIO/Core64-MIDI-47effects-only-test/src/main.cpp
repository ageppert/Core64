#include <Arduino.h>  // Required in order to build in PlatformIO

#include <PicoSoftwareSerial.h>
#include <MIDI.h>

#include "SubSystems/Serial_Port.h"

using Transport = MIDI_NAMESPACE::SerialMIDI<SoftwareSerial>;
int txPin = 1;  // SAO GPIO 1 (top)
int rxPin = 2;  // SAO GPIO 2 (bottom)
SoftwareSerial mySerial = SoftwareSerial(txPin, rxPin);
Transport serialMIDI(mySerial);
MIDI_NAMESPACE::MidiInterface<Transport> MIDI((Transport&)serialMIDI);

// SoftwareSerial midiSerial(1, 2);  // RX, TX
// MIDI_CREATE_INSTANCE(SoftwareSerial, midiSerial, MIDI);


void setup() {
  SerialPortSetup();
  pinMode(LED_BUILTIN, OUTPUT);
  MIDI.begin(4);                    // Launch MIDI and listen to channel 4
}

void loop() {
  Serial.print("Hello world on the hardware serial port via USB cable."); 
    // Send note 28 with velocity 127 on channel 1
    MIDI.sendNoteOn(28,  1, 1);
    // MIDI.sendNoteOn(31,  5, 1);
        digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    MIDI.sendNoteOff(28, 0, 1);     // Stop the note
    // MIDI.sendNoteOff(31, 0, 1);     // Stop the note
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
    //MIDI.sendNoteOff(28, 50, 1);     // Stop the note
    MIDI.sendNoteOn(31, 1, 1);     // Stop the note
    // MIDI.sendNoteOff(33, 50, 1);     // Stop the note
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
    //MIDI.sendNoteOff(28, 0, 1);     // Stop the note
    MIDI.sendNoteOff(31, 0, 1);     // Stop the note
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
}
