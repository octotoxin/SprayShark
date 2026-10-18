#include <Arduino.h>
#include <string.h>

#include "communication.h"
#include "movement.h"

void actuatorLeft();
void actuatorRight();
void actuatorStop();

namespace {
constexpr size_t COMMAND_BUFFER_SIZE = 32;
char commandBuffer[COMMAND_BUFFER_SIZE];
size_t commandLength = 0;
bool commandOverflow = false;

void stopForInvalidCommand() {
  actuatorStop();
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
  } else if (strcmp(commandBuffer, "ACTUATOR_RIGHT") == 0) {
    actuatorRight();
  } else if (strcmp(commandBuffer, "ACTUATOR_STOP") == 0) {
    actuatorStop();
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
  actuatorStop();
}

void communicationProcess() {
  while (Serial.available() > 0) {
    receiveByte(static_cast<char>(Serial.read()));
  }
}