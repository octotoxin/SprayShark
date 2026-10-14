#include <Arduino.h>
#include <string.h>

static const uint32_t BAUD_RATE = 115200;
static const int RPWM_PIN = 2;
static const int LPWM_PIN = 3;
static const int R_EN_PIN = 4;
static const int L_EN_PIN = 5;
static const int R_IS_PIN = 6;
static const int L_IS_PIN = 7;
static const uint8_t CALIBRATION_TRIALS = 3;
static const unsigned long LIMIT_ZERO_CONFIRMATION_MS = 10;
static const size_t COMMAND_BUFFER_SIZE = 16;

enum CalibrationPhase {
  IDLE,
  INITIAL_RETRACT,
  EXTENDING,
  RETRACTING
};

static CalibrationPhase phase = IDLE;
static unsigned long phaseStartedAt = 0;
static unsigned long limitZeroStartedAt = 0;
static bool limitZeroTiming = false;
static bool directionObserved = false;
static uint8_t completedTrials = 0;
static uint64_t extendDurationTotal = 0;
static uint64_t retractDurationTotal = 0;
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

static void resetLimitZeroTimer() {
  limitZeroTiming = false;
}

static bool isLimitZeroConfirmed(bool isZero, unsigned long now) {
  if (!isZero) {
    resetLimitZeroTimer();
    return false;
  }

  if (!limitZeroTiming) {
    limitZeroStartedAt = now;
    limitZeroTiming = true;
  }

  return now - limitZeroStartedAt >= LIMIT_ZERO_CONFIRMATION_MS;
}

static void reportDuration(const char *name, uint8_t trial,
                           unsigned long duration) {
  Serial.print(name);
  Serial.print('_');
  Serial.print(trial);
  Serial.print("_MS:");
  Serial.println(duration);
  Serial0.print(name);
  Serial0.print('_');
  Serial0.print(trial);
  Serial0.print("_MS:");
  Serial0.println(duration);
}

static void beginExtension() {
  setActuatorDirection(true);
  phase = EXTENDING;
  phaseStartedAt = millis();
  directionObserved = false;
  resetLimitZeroTimer();
}

static void beginRetraction() {
  setActuatorDirection(false);
  phase = RETRACTING;
  phaseStartedAt = millis();
  directionObserved = false;
  resetLimitZeroTimer();
}

static void startCalibration() {
  if (phase != IDLE) {
    reportStatus("BUSY");
    return;
  }

  completedTrials = 0;
  extendDurationTotal = 0;
  retractDurationTotal = 0;
  setActuatorDirection(false);
  phase = INITIAL_RETRACT;
  resetLimitZeroTimer();
  reportStatus("CALIBRATION_STARTED");
}

static void stopCalibration() {
  stopActuator();
  phase = IDLE;
  resetLimitZeroTimer();
  reportStatus("CALIBRATION_STOPPED");
}

static void processCommand(const char *command) {
  if (strcmp(command, "START") == 0) {
    startCalibration();
  } else if (strcmp(command, "STOP") == 0) {
    stopCalibration();
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

static void reportCalibrationResults() {
  const unsigned long averageExtendMs =
      static_cast<unsigned long>(extendDurationTotal / CALIBRATION_TRIALS);
  const unsigned long averageRetractMs =
      static_cast<unsigned long>(retractDurationTotal / CALIBRATION_TRIALS);

  Serial.print("AVERAGE_EXTEND_MS:");
  Serial.println(averageExtendMs);
  Serial.print("AVERAGE_RETRACT_MS:");
  Serial.println(averageRetractMs);
  Serial0.print("AVERAGE_EXTEND_MS:");
  Serial0.println(averageExtendMs);
  Serial0.print("AVERAGE_RETRACT_MS:");
  Serial0.println(averageRetractMs);
}

static void updateCalibration(unsigned long now) {
  const int rIsValue = analogRead(R_IS_PIN);
  const int lIsValue = analogRead(L_IS_PIN);

  switch (phase) {
    case INITIAL_RETRACT:
      if (isLimitZeroConfirmed(lIsValue == 0, now)) {
        beginExtension();
      }
      break;

    case EXTENDING:
      if (rIsValue > lIsValue) {
        directionObserved = true;
      }
      if (!directionObserved) {
        resetLimitZeroTimer();
      } else if (isLimitZeroConfirmed(rIsValue == 0, now)) {
        const unsigned long duration = limitZeroStartedAt - phaseStartedAt;
        extendDurationTotal += duration;
        reportDuration("EXTEND", completedTrials + 1, duration);
        beginRetraction();
      }
      break;

    case RETRACTING:
      if (lIsValue > rIsValue) {
        directionObserved = true;
      }
      if (!directionObserved) {
        resetLimitZeroTimer();
      } else if (isLimitZeroConfirmed(lIsValue == 0, now)) {
        const unsigned long duration = limitZeroStartedAt - phaseStartedAt;
        retractDurationTotal += duration;
        reportDuration("RETRACT", completedTrials + 1, duration);
        ++completedTrials;

        if (completedTrials == CALIBRATION_TRIALS) {
          stopActuator();
          phase = IDLE;
          resetLimitZeroTimer();
          reportCalibrationResults();
          reportStatus("CALIBRATION_COMPLETE");
        } else {
          beginExtension();
        }
      }
      break;

    case IDLE:
      break;
  }
}

void setup() {
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  pinMode(R_EN_PIN, OUTPUT);
  pinMode(L_EN_PIN, OUTPUT);
  pinMode(R_IS_PIN, INPUT);
  pinMode(L_IS_PIN, INPUT);
  stopActuator();

  digitalWrite(R_EN_PIN, HIGH);
  digitalWrite(L_EN_PIN, HIGH);

  Serial.begin(BAUD_RATE);
  Serial0.begin(BAUD_RATE);
}

void loop() {
  pollCommandPort(
      Serial, serialCommand, serialCommandLength, serialCommandOverflow);
  pollCommandPort(
      Serial0, serial0Command, serial0CommandLength, serial0CommandOverflow);
  updateCalibration(millis());
}
