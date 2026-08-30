/*
  Actuator Control - Center Range (+/- 10mm)
  Hardware: Arduino UNO R4 WiFi + BTS7960 + SOVIK Actuator

  Logic:
  - Full Travel (50mm): 6000 ms (120ms per mm)
  - Middle (25mm): 3000 ms
  - Startup: Assumes actuator is manually positioned at MIDDLE.
             Sets internal position to Middle immediately.
  
  Commands:
  - 'l': Move Left (Middle + travelDistMM)
  - 'r': Move Right (Middle - travelDistMM)
  - 'm': Move Middle (0mm from center) -> 25mm -> 3000 ms
  - 's': Stop (Manual override).

  Instead of using differential drive in @Main%20Project%20Code.ino , I want to make use of the actuator that we have configured in @actuator.ino .  Don't write any code, just tell me if it'll work and what to consider, and any shortcomings.
*/

const int RPWM_PIN = 2; // Extend
const int LPWM_PIN = 3; // Retract
const int R_EN_PIN = 4;
const int L_EN_PIN = 5;

// TIMING CONSTANTS (Based on 6000ms = 50mm)
const unsigned long FULL_TRAVEL_MS = 6000; 
const unsigned long POS_MIDDLE_MS = 3000;  // 25mm

// CONVERSION & CONFIGURATION
const int MS_PER_MM = 120; // 60ms / 50mm = 120ms/mm
int travelDistMM = 10;     // Distance to move left/right from center (Max 25mm)

// STATE TRACKING
unsigned long currentPosMs = 0; // 0 = Fully Retracted
int motorSpeed = 255;

void setup() {
  Serial.begin(9600);
  
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  pinMode(R_EN_PIN, OUTPUT);
  pinMode(L_EN_PIN, OUTPUT);

  // Enable Driver
  digitalWrite(R_EN_PIN, HIGH);
  digitalWrite(L_EN_PIN, HIGH);
  
  stopMotor();
  
  Serial.println("--- Actuator Center-Range Control ---");
  Serial.println("Initializing Position...");

  // 1. ASSUME CURRENT POSITION IS MIDDLE
  currentPosMs = POS_MIDDLE_MS; 
  Serial.println("System Initialized.");
  Serial.println("ASSUMPTION: Actuator is currently at Center."); 

  Serial.println("Initialization Complete. At Middle (3000ms).");
  Serial.print("Configured Travel from Center: "); Serial.print(travelDistMM); Serial.println("mm");
  Serial.println("Commands: 'l' (Left), 'r' (Right), 'm' (Middle)");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    if (cmd == 'l') {
      unsigned long posLeftMs = POS_MIDDLE_MS + (travelDistMM * MS_PER_MM);
      goToPosition(posLeftMs, "LEFT");
    }
    else if (cmd == 'r') {
      unsigned long posRightMs = POS_MIDDLE_MS - (travelDistMM * MS_PER_MM);
      goToPosition(posRightMs, "RIGHT");
    }
    else if (cmd == 'm') {
      goToPosition(POS_MIDDLE_MS, "MIDDLE (25mm)");
    }
  }
}

// --- GENERIC MOVE FUNCTION ---
void goToPosition(unsigned long targetMs, const char* name) {
  if (currentPosMs == targetMs) {
    Serial.print("Already at "); Serial.println(name);
    return;
  }

  if (currentPosMs < targetMs) {
    // Need to EXTEND
    unsigned long moveTime = targetMs - currentPosMs;
    Serial.print("Extending to "); Serial.print(name); 
    Serial.print(". Time: "); Serial.println(moveTime);
    driveMotor(true, moveTime);
  } else {
    // Need to RETRACT
    unsigned long moveTime = currentPosMs - targetMs;
    Serial.print("Retracting to "); Serial.print(name); 
    Serial.print(". Time: "); Serial.println(moveTime);
    driveMotor(false, moveTime);
  }
  
  currentPosMs = targetMs;
  Serial.print("Arrived at "); Serial.println(name);
}

// --- MOTOR HELPER ---
// Driving for a specific duration (Blocking)
void driveMotor(bool extend, unsigned long duration) {
  if (extend) {
     analogWrite(LPWM_PIN, 0);
     analogWrite(RPWM_PIN, motorSpeed);
  } else {
     analogWrite(RPWM_PIN, 0);
     analogWrite(LPWM_PIN, motorSpeed);
  }

  delay(duration); // Simple blocking delay for fixed movements

  stopMotor();
}

void stopMotor() {
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, 0);
}
