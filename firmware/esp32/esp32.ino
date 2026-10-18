#include <Arduino.h>

#include "communication.h"
#include "movement.h"

void communicationInit();
void communicationProcess();
void movementInit();

constexpr uint32_t SERIAL_BAUD_RATE = 115200;

void setup() {
  movementInit();
  Serial.begin(SERIAL_BAUD_RATE);
  communicationInit();
}

void loop() {
  communicationProcess();
}