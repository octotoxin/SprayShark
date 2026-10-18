#include <Arduino.h>
#include <string.h>

#include "communication.h"
#include "movement.h"

void actuatorLeft();
void actuatorRight();
void actuatorStop();

namespace {
constexpr size_t COMMAND_BUFFER_SIZE = 32;
constexpr uint32_t MOVEMENT_COMMAND_TIMEOUT_MS = 750;
char commandBuffer[COMMAND_BUFFER_SIZE];
size_t commandLength = 0;
bool commandOverflow = false;
bool movementCommandActive = false;
uint32_t lastMovementCommandAt = 0;

void stopForInvalidCommand() {
  actuatorStop();
  movementCommandActive = false;
}

void dispatchCommand() {
  if (commandOverflow) {
    stopForInvalidCommand();
    commandLength = 0;
    commandOverflow = false;
    return;
  }

  if (commandLength > 0 && commandBuffer[commandLength - 1] == '\r') {
    --commandLength;
  }
  commandBuffer[commandLength] = '\0';

  if (strcmp(commandBuffer, "ACTUATOR_LEFT") == 0) {
    actuatorLeft();
    movementCommandActive = true;
    lastMovementCommandAt = millis();
  } else if (strcmp(commandBuffer, "ACTUATOR_RIGHT") == 0) {
    actuatorRight();
    movementCommandActive = true;
    lastMovementCommandAt = millis();
  } else if (strcmp(commandBuffer, "ACTUATOR_STOP") == 0) {
    actuatorStop();
    movementCommandActive = false;
  } else {
    stopForInvalidCommand();
  }

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
    stopForInvalidCommand();
    return;
  }
  commandBuffer[commandLength++] = value;
}
}

void communicationInit() {
  commandLength = 0;
  commandOverflow = false;
  movementCommandActive = false;
  lastMovementCommandAt = 0;
  actuatorStop();
}

void communicationProcess() {
  while (Serial.available() > 0) {
    receiveByte(static_cast<char>(Serial.read()));
  }

  if (movementCommandActive &&
      static_cast<uint32_t>(millis() - lastMovementCommandAt) >=
          MOVEMENT_COMMAND_TIMEOUT_MS) {
    actuatorStop();
    movementCommandActive = false;
  }
}
