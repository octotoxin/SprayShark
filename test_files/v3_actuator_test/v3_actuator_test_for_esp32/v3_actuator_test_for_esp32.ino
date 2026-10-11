#include <Arduino.h>
#include <string.h>

static const uint32_t BAUD_RPI = 115200;
static const int RPWM_PIN = 2;
static const int LPWM_PIN = 3;
static const int R_EN_PIN = 4;
static const int L_EN_PIN = 5;
static const int R_IS_PIN = 6;
static const int L_IS_PIN = 7;

static const unsigned long REVERSE_DEADTIME_MS = 50;
static const unsigned long CURRENT_SAMPLE_INTERVAL_MS = 10;
static const unsigned long CURRENT_STARTUP_BLANK_MS = 100;
static const unsigned long CURRENT_CALIBRATION_END_MS = 500;
static const unsigned long MAX_TRAVEL_DURATION_MS = 20000;
static const int CURRENT_RISE_PERCENT = 20;
static const int MIN_CURRENT_RISE_ADC = 40;
static const unsigned int STALL_CONFIRMATION_SAMPLES = 4;
static const unsigned int TRAVELS_TO_MEASURE = 5;
static const size_t COMMAND_BUFFER_SIZE = 16;

enum TestPhase {
  IDLE,
  MOVING_TO_LIMIT,
  REVERSING
};

static TestPhase testPhase = IDLE;
static unsigned long phaseStartedAt = 0;
static unsigned long travelStartedAt = 0;
static unsigned long lastCurrentSampleAt = 0;
static uint32_t currentCalibrationSum = 0;
static unsigned int currentCalibrationSamples = 0;
static int currentBaseline = 0;
static bool currentBaselineReady = false;
static unsigned int overCurrentSamples = 0;
static unsigned int completedTravels = 0;
static uint32_t totalTravelMs = 0;
static bool movingExtend = false;
static bool nextTravelExtends = true;

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

static void startTest(bool startRetracted) {
  if (testPhase != IDLE) {
    reportStatus("BUSY");
    return;
  }

  completedTravels = 0;
  totalTravelMs = 0;
  movingExtend = startRetracted;
  nextTravelExtends = !movingExtend;
  testPhase = MOVING_TO_LIMIT;
  travelStartedAt = millis();
  phaseStartedAt = travelStartedAt;
  lastCurrentSampleAt = travelStartedAt;
  currentCalibrationSum = 0;
  currentCalibrationSamples = 0;
  currentBaseline = 0;
  currentBaselineReady = false;
  overCurrentSamples = 0;
  setActuatorDirection(movingExtend);
  reportStatus("TEST_STARTED");
}

static void processCommand(const char *command) {
  if (strcmp(command, "START") == 0) {
    startTest(false);
  } else if (strcmp(command, "START_RETRACTED") == 0) {
    startTest(true);
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

static void beginTravel(bool extend, unsigned long now) {
  movingExtend = extend;
  travelStartedAt = now;
  phaseStartedAt = now;
  lastCurrentSampleAt = now;
  currentCalibrationSum = 0;
  currentCalibrationSamples = 0;
  currentBaseline = 0;
  currentBaselineReady = false;
  overCurrentSamples = 0;
  testPhase = MOVING_TO_LIMIT;
  setActuatorDirection(extend);
}

static void failTravelTimeout() {
  stopActuator();
  testPhase = IDLE;
  reportStatus(movingExtend ? "ERROR,TRAVEL_TIMEOUT,EXTEND"
                            : "ERROR,TRAVEL_TIMEOUT,RETRACT");
}

static void completeTravel(unsigned long now) {
  stopActuator();
  const unsigned long travelMs = now - travelStartedAt;
  ++completedTravels;
  totalTravelMs += travelMs;

  char message[40];
  snprintf(message, sizeof(message), "TRAVEL_%u_MS=%lu", completedTravels,
           travelMs);
  reportStatus(message);

  if (completedTravels == TRAVELS_TO_MEASURE) {
    const unsigned long averageMs =
        (totalTravelMs + TRAVELS_TO_MEASURE / 2) / TRAVELS_TO_MEASURE;
    snprintf(message, sizeof(message), "AVERAGE_TRAVEL_MS=%lu", averageMs);
    reportStatus(message);
    testPhase = IDLE;
    reportStatus("TEST_COMPLETE");
    return;
  }

  nextTravelExtends = !movingExtend;
  testPhase = REVERSING;
  phaseStartedAt = now;
}

static void updateTest() {
  const unsigned long now = millis();

  if (testPhase == REVERSING) {
    if (now - phaseStartedAt >= REVERSE_DEADTIME_MS) {
      beginTravel(nextTravelExtends, now);
    }
    return;
  }

  if (testPhase != MOVING_TO_LIMIT) {
    return;
  }

  const unsigned long travelElapsed = now - travelStartedAt;
  if (travelElapsed >= MAX_TRAVEL_DURATION_MS) {
    failTravelTimeout();
    return;
  }

  if (now - lastCurrentSampleAt < CURRENT_SAMPLE_INTERVAL_MS) {
    return;
  }
  lastCurrentSampleAt = now;

  const int sensePin = movingExtend ? R_IS_PIN : L_IS_PIN;
  const int currentReading = analogRead(sensePin);

  if (travelElapsed >= CURRENT_STARTUP_BLANK_MS &&
      travelElapsed < CURRENT_CALIBRATION_END_MS) {
    currentCalibrationSum += currentReading;
    ++currentCalibrationSamples;
    return;
  }

  if (travelElapsed < CURRENT_CALIBRATION_END_MS) {
    return;
  }

  if (currentCalibrationSamples == 0) {
    failTravelTimeout();
    return;
  }
  if (!currentBaselineReady) {
    currentBaseline =
        currentCalibrationSum / currentCalibrationSamples;
    currentBaselineReady = true;
  }

  int requiredRise = currentBaseline * CURRENT_RISE_PERCENT / 100;
  if (requiredRise < MIN_CURRENT_RISE_ADC) {
    requiredRise = MIN_CURRENT_RISE_ADC;
  }

  if (currentReading >= currentBaseline + requiredRise) {
    ++overCurrentSamples;
    if (overCurrentSamples >= STALL_CONFIRMATION_SAMPLES) {
      completeTravel(now);
    }
  } else {
    overCurrentSamples = 0;
  }
}

void setup() {
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  stopActuator();

  pinMode(R_IS_PIN, INPUT);
  pinMode(L_IS_PIN, INPUT);
  analogReadResolution(12);

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