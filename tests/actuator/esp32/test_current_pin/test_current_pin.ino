#include <Arduino.h>
#include <string.h>

static const uint32_t BAUD_RATE = 115200;
static const int RPWM_PIN = 2;
static const int LPWM_PIN = 3;
static const int R_EN_PIN = 4;
static const int L_EN_PIN = 5;
static const int R_IS_PIN = 6;
static const int L_IS_PIN = 7;

static const unsigned long PHASE_DURATION_MS = 6000;
static const unsigned long SENSOR_REPORT_INTERVAL_MS = 10;
static const float SENSOR_SMOOTHING_FACTOR = 0.25f;
static const size_t COMMAND_BUFFER_SIZE = 16;

enum CalibrationPhase {
  IDLE,
  RETRACTING,
  EXTENDING
};

static CalibrationPhase phase = IDLE;
static unsigned long phaseStartedAt = 0;
static unsigned long lastSensorReportAt = 0;
static float smoothedRIsValue = 0.0f;
static float smoothedLIsValue = 0.0f;
static bool sensorValuesInitialized = false;
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

static void startCalibration() {
  if (phase != IDLE) {
    reportStatus("BUSY");
    return;
  }

  setActuatorDirection(false);
  phase = RETRACTING;
  phaseStartedAt = millis();
  reportStatus("CALIBRATION_STARTED");
}

static void processCommand(const char *command) {
  if (strcmp(command, "START") == 0) {
    startCalibration();
  } else if (strcmp(command, "STOP") == 0) {
    stopActuator();
    phase = IDLE;
    reportStatus("CALIBRATION_STOPPED");
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

static void updateCalibration(unsigned long now) {
  if (phase == IDLE || now - phaseStartedAt < PHASE_DURATION_MS) {
    return;
  }

  if (phase == RETRACTING) {
    setActuatorDirection(true);
    phase = EXTENDING;
  } else {
    setActuatorDirection(false);
    phase = RETRACTING;
  }
  phaseStartedAt = now;
}

static void reportSensorValues(unsigned long now) {
  if (now - lastSensorReportAt < SENSOR_REPORT_INTERVAL_MS) {
    return;
  }

  lastSensorReportAt = now;
  const int rawRIsValue = analogRead(R_IS_PIN);
  const int rawLIsValue = analogRead(L_IS_PIN);
  if (!sensorValuesInitialized) {
    smoothedRIsValue = static_cast<float>(rawRIsValue);
    smoothedLIsValue = static_cast<float>(rawLIsValue);
    sensorValuesInitialized = true;
  } else {
    smoothedRIsValue += SENSOR_SMOOTHING_FACTOR *
        (static_cast<float>(rawRIsValue) - smoothedRIsValue);
    smoothedLIsValue += SENSOR_SMOOTHING_FACTOR *
        (static_cast<float>(rawLIsValue) - smoothedLIsValue);
  }

  const int rIsValue = static_cast<int>(smoothedRIsValue + 0.5f);
  const int lIsValue = static_cast<int>(smoothedLIsValue + 0.5f);
  Serial.print("R_IS:");
  Serial.print(rIsValue);
  Serial.print("    L_IS:");
  Serial.println(lIsValue);
  Serial0.print("R_IS:");
  Serial0.print(rIsValue);
  Serial0.print("    L_IS:");
  Serial0.println(lIsValue);
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

  const unsigned long now = millis();
  updateCalibration(now);
  reportSensorValues(now);
}