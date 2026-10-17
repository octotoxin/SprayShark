#include <Arduino.h>

static const uint32_t BAUD_RATE = 115200;
static const int RPWM_PIN = 2;
static const int LPWM_PIN = 3;
static const int R_EN_PIN = 4;
static const int L_EN_PIN = 5;
static const int R_IS_PIN = 6;
static const int L_IS_PIN = 7;

static void stopActuator() {
  digitalWrite(RPWM_PIN, LOW);
  digitalWrite(LPWM_PIN, LOW);
}

static void setActuatorDirection(bool extend) {
  stopActuator();
  digitalWrite(RPWM_PIN, extend ? HIGH : LOW);
  digitalWrite(LPWM_PIN, extend ? LOW : HIGH);
}

static void reportStatus(const char *message) {
  Serial.println(message);
  Serial0.println(message);
}

static void processCommand(char command) {
  if (command >= 'A' && command <= 'Z') {
    command = static_cast<char>(command - 'A' + 'a');
  }

  switch (command) {
    case 'e':
      setActuatorDirection(true);
      reportStatus("EXTENDING");
      break;

    case 'r':
      setActuatorDirection(false);
      reportStatus("RETRACTING");
      break;

    case 'x':
      stopActuator();
      reportStatus("EMERGENCY_STOP");
      break;

    case '\r':
    case '\n':
    case ' ':
    case '\t':
      break;

    default:
      reportStatus("UNKNOWN_COMMAND");
      break;
  }
}

static void pollCommandPort(Stream &port) {
  while (port.available() > 0) {
    processCommand(static_cast<char>(port.read()));
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
  pollCommandPort(Serial);
  pollCommandPort(Serial0);
}