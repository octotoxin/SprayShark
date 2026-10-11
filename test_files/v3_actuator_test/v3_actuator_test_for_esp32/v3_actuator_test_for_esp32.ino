#include <Arduino.h>
#include <string.h>

static const uint32_t BAUD_RPI = 115200;
static const int RPWM_PIN = 2;
static const int LPWM_PIN = 3;
static const int R_EN_PIN = 4;
static const int L_EN_PIN = 5;

static const unsigned long EXTEND_DURATION_MS = 10000;
static const unsigned long STOP_DURATION_MS = 3000;
static const unsigned long RETRACT_DURATION_MS = 10000;
static const size_t COMMAND_BUFFER_SIZE = 16;

enum TestPhase {
  IDLE,
  EXTENDING,
  STOPPING_AFTER_EXTEND,
  RETRACTING,
  STOPPING_AFTER_RETRACT
};

static TestPhase testPhase = IDLE;
static unsigned long phaseStartedAt = 0;

static char serialCommand[COMMAND_BUFFER_SIZE];
static size_t serialCommandLength = 0;
static bool serialCommandOverflow = false;
static char serial0Command[COMMAND_BUFFER_SIZE];
static size_t serial0CommandLength = 0;
static bool serial0CommandOverflow = false;

static void stopActuator() {
  digitalWrite(RPWM_PIN, LOW);
  digitalWrite(LPWM_PIN, LOW);
}

static void setActuatorDirection(bool extend) {
  digitalWrite(RPWM_PIN, extend ? HIGH : LOW);
  digitalWrite(LPWM_PIN, extend ? LOW : HIGH);
}

static void reportStatus(const char *message) {
  Serial.println(message);
  Serial0.println(message);
}

static void startTest() {
  if (testPhase != IDLE) {
    reportStatus("BUSY");
    return;
  }

  testPhase = EXTENDING;
  phaseStartedAt = millis();
  setActuatorDirection(true);
  reportStatus("TEST_STARTED");
}

static void processCommand(const char *command) {
  if (strcmp(command, "START") == 0) {
    startTest();
  } else {
    reportStatus("UNKNOWN_COMMAND");
  }
}

static void pollCommandPort(
    Stream &port,
    char *buffer,
    size_t &length,
    bool &overflow) {
  while (port.available() > 0) {
    const char incoming = static_cast<char>(port.read());

    if (incoming == '\r') {
      continue;
    }

    if (incoming == '\n') {
      if (!overflow && length > 0) {
        buffer[length] = '\0';
        processCommand(buffer);
      }
      length = 0;
      overflow = false;
      continue;
    }

    if (!overflow) {
      if (length < COMMAND_BUFFER_SIZE - 1) {
        buffer[length++] = incoming;
      } else {
        overflow = true;
      }
    }
  }
}

static void updateTest() {
  const unsigned long now = millis();

  switch (testPhase) {
    case EXTENDING:
      if (now - phaseStartedAt >= EXTEND_DURATION_MS) {
        stopActuator();
        testPhase = STOPPING_AFTER_EXTEND;
        phaseStartedAt = now;
      }
      break;

    case STOPPING_AFTER_EXTEND:
      if (now - phaseStartedAt >= STOP_DURATION_MS) {
        testPhase = RETRACTING;
        phaseStartedAt = now;
        setActuatorDirection(false);
      }
      break;

    case RETRACTING:
      if (now - phaseStartedAt >= RETRACT_DURATION_MS) {
        stopActuator();
        testPhase = STOPPING_AFTER_RETRACT;
        phaseStartedAt = now;
      }
      break;

    case STOPPING_AFTER_RETRACT:
      if (now - phaseStartedAt >= STOP_DURATION_MS) {
        stopActuator();
        testPhase = IDLE;
        reportStatus("TEST_COMPLETE");
      }
      break;

    case IDLE:
      break;
  }
}

void setup() {
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  stopActuator();

  pinMode(R_EN_PIN, OUTPUT);
  pinMode(L_EN_PIN, OUTPUT);
  digitalWrite(R_EN_PIN, HIGH);
  digitalWrite(L_EN_PIN, HIGH);

  Serial.begin(BAUD_RPI);
  Serial0.begin(BAUD_RPI);
}

void loop() {
  pollCommandPort(
      Serial, serialCommand, serialCommandLength, serialCommandOverflow);
  pollCommandPort(
      Serial0, serial0Command, serial0CommandLength, serial0CommandOverflow);
  updateTest();
}