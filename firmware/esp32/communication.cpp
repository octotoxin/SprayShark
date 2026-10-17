#include <Arduino.h>
#include <string.h>

#include "communication.h"
#include "movement.h"

namespace {
constexpr size_t COMMAND_BUFFER_SIZE = 32;
constexpr uint32_t COMMAND_TIMEOUT_MS = 750;
char commandBuffer[COMMAND_BUFFER_SIZE];
size_t commandLength = 0;
bool commandOverflow = false;
bool actuatorMoving = false;
uint32_t lastCommandAt = 0;

void dispatchCommand() {
  if (commandOverflow) {
    actuatorStop();
    actuatorMoving = false;
    commandLength = 0;
    commandOverflow = false;
    return;
  }

  if (commandLength > 0 && commandBuffer[commandLength - 1] == '\r') {
    --commandLength;
  }
  commandBuffer[commandLength] = '\0';

  if (strcmp(commandBuffer, "ACTUATOR LEFT") == 0) {
    actuatorLeft();
    actuatorMoving = true;
  } else if (strcmp(commandBuffer, "ACTUATOR RIGHT") == 0) {
    actuatorRight();
    actuatorMoving = true;
  } else if (strcmp(commandBuffer, "ACTUATOR STOP") == 0) {
    actuatorStop();
    actuatorMoving = false;
  } else {
    actuatorStop();
    actuatorMoving = false;
    commandLength = 0;
    return;
  }

  lastCommandAt = millis();
  commandLength = 0;
}

void receiveByte(char value) {
  if (value == '\n') {
    dispatchCommand();
    return;
  }
  if (commandOverflow) {
    return;
  }
  if (commandLength >= COMMAND_BUFFER_SIZE - 1) {
    commandOverflow = true;
    actuatorStop();
    actuatorMoving = false;
    return;
  }
  commandBuffer[commandLength++] = value;
}
}

void communicationInit() {
  commandLength = 0;
  commandOverflow = false;
  actuatorMoving = false;
  lastCommandAt = millis();
  actuatorStop();
}

void communicationProcess() {
  while (Serial.available() > 0) {
    receiveByte(static_cast<char>(Serial.read()));
  }

  if (actuatorMoving &&
      static_cast<uint32_t>(millis() - lastCommandAt) >= COMMAND_TIMEOUT_MS) {
    actuatorStop();
    actuatorMoving = false;
  }
}