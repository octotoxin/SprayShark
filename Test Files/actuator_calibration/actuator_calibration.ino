/*
  Actuator Calibration & Manual Control
  Hardware: Arduino UNO R4 WiFi + BTS7960 + SOVIK Actuator

  Use these commands to manually move the actuator if it loses its position
  or needs to be sent to the middle manually.

  Commands:
  - 'e': Extend Actuator
  - 'r': Retract Actuator
  - 's': Stop Actuator
*/

const int RPWM_PIN = 2; // Extend
const int LPWM_PIN = 3; // Retract
const int R_EN_PIN = 4;
const int L_EN_PIN = 5;
const int R_IS_PIN = 12; // Current Sense Right (Forward/Extend)
const int L_IS_PIN = 11; // Current Sense Left (Reverse/Retract)

const int NUDGE_MS = 100; // Duration for each pulse in milliseconds
bool isMoving = false; // Track if actuator is currently commanded to move

void setup() {
  Serial.begin(9600);
  
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  pinMode(R_EN_PIN, OUTPUT);
  pinMode(L_EN_PIN, OUTPUT);
  pinMode(R_IS_PIN, INPUT);
  pinMode(L_IS_PIN, INPUT);

  // Enable Driver
  digitalWrite(R_EN_PIN, HIGH);
  digitalWrite(L_EN_PIN, HIGH);
  
  stopMotor();
  
  Serial.println("--- Actuator Manual Calibration (Nudge Mode) ---");
  Serial.println("Send commands via Serial Monitor:");
  Serial.println(" 'e' - Nudge (Extend 100ms)");
  Serial.println(" 'r' - Nudge (Retract 100ms)");
  Serial.println(" 's' - Emergency Stop");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    if (cmd == 'e' || cmd == 'E') {
      Serial.print("Nudging Extend...");
      extendMotor();
      delay(NUDGE_MS);
      stopMotor();
      Serial.println(" Done.");
    }
    else if (cmd == 'r' || cmd == 'R') {
      Serial.print("Nudging Retract...");
      retractMotor();
      delay(NUDGE_MS);
      stopMotor();
      Serial.println(" Done.");
    }
    else if (cmd == 's' || cmd == 'S') {
      Serial.println("Stopping Actuator...");
      stopMotor();
    }
  }

  // --- STALL DETECTION ---
  if (isMoving && (digitalRead(R_IS_PIN) == HIGH || digitalRead(L_IS_PIN) == HIGH)) {
    Serial.println("!!! STALL DETECTED - EMERGENCY STOP !!!");
    stopMotor();
  }
}

void extendMotor() {
  digitalWrite(LPWM_PIN, LOW);
  digitalWrite(RPWM_PIN, HIGH);
  isMoving = true;
}

void retractMotor() {
  digitalWrite(RPWM_PIN, LOW);
  digitalWrite(LPWM_PIN, HIGH);
  isMoving = true;
}

void stopMotor() {
  digitalWrite(RPWM_PIN, LOW);
  digitalWrite(LPWM_PIN, LOW);
  isMoving = false;
}
