#include <stdint.h>
#include <stdbool.h>

#include "SubSystems/Serial_Port.h"

#define SERIAL_PORT_SPEED_HARDWARE 115200
// #define SERIAL_PORT_SPEED_SOFTWARE  31250

void SerialPortSetup() {
  Serial.begin(SERIAL_PORT_SPEED_HARDWARE);
  delay(500);                               // Provide a little time for a connected serial monitor to auto-open, such as in PlatformIO
  Serial.println();
  Serial.println();
  Serial.println("  Hardware Serial Port Started.");
}
