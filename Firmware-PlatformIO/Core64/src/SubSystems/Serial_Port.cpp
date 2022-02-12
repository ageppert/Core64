#include <stdint.h>
#include <stdbool.h>

#include "SubSystems/Serial_Port.h"
#include "Config/HardwareIOMap.h"

#define SERIAL_PORT_SPEED   15200

uint8_t DebugLevel = 0;

// TO DO: Consider using a <FastSerial.h> library?

void SerialPortSetup() {
  Serial.begin(SERIAL_PORT_SPEED);
  delay(500);                               // Provide a little time for a connected serial monitor to auto-open, such as in PlatformIO
  Serial.println();
  Serial.println("Serial Port Started.");
  Serial.print("Debug Level = ");
  Serial.println(DebugLevel);
}

void SerialPortProcess() {

}

void SetDebugLevel (uint8_t value) {   DebugLevel = value; }
uint8_t GetDebugLevel ()           {   return (DebugLevel); }
