/*
  SprayShark_Bluetooth_Joystick_Demo.ino
  
  Purpose:
  Control two BLDC motors (Left/Right) via Bluetooth using the "Dabble" App (Joystick Module).
  
  Features:
  - Custom Packet Parser for Uno R4 WiFi Native BLE
  - Smooth Ramping (Acceleration/Deceleration)
  - Emergency Brake (Cross/X Button)
  - Obstacle Detection (HC-SR04): Stops if < 1ft (30cm)
  - LED Matrix Feedback: Shows DANGER icon on E-Brake or Obstacle
  
  Hardware:
  - Arduino Uno R4 WiFi
  - Left Motor ESC -> Pin 9
  - Right Motor ESC -> Pin 10
  - HC-SR04 Trig   -> Pin 7
  - HC-SR04 Echo   -> Pin 6
  
  Controls (Dabble App -> Gamepad -> Joystick Mode):
  - Start: ARM MOTORS (Safety Sequence)
  - Cross (X blue): EMERGENCY BRAKE (Immediate Stop)
  - Joystick: Smooth Analog Drive Control
*/

#include <ArduinoBLE.h>
#include <Servo.h>
#include "Arduino_LED_Matrix.h" // Include LED Matrix library

// Simple "Water Drop" Icon
const uint32_t LEDMATRIX_PUMP_ON[3] = {
    0x31842442,
    0x44231800,
    0x00000000
};


// "X" / Disabled Icon  
const uint32_t LEDMATRIX_PUMP_OFF[3] = {
    0x81442214,
    0x08142241,
    0x80000000
};


// --- CONFIGURATION ---
const int leftEscPin = 9;
const int rightEscPin = 10;
const int trigPin = 7;
const int echoPin = 6;

const float safetyDistCm = 60.0; // 1 foot is approx 30.48 cm

// Motor Pulse Settings
const int minPulse = 1000;      // Max Reverse
const int neutralPulse = 1500;  // Stop
const int maxPulse = 2000;      // Max Forward

// Speed Targets
// Base: 40% (Pulse 1700/1300)
// 75% of Base: 30% (Pulse 1650/1350)
// Low: 20% (Pulse 1600/1400)

const int speed40Fwd = 1700;      // 40% Forward (Base)
const int speed40Rev = 1300;      // 40% Reverse
const int speed30Fwd = 1650;      // 30% Forward (75% of Base)
const int speed30Rev = 1350;      // 30% Reverse
const int speed20Fwd = 1600;      // 20% Forward (Low)
const int speed20Rev = 1400;      // 20% Reverse

// Ramping Settings (SLOWER for smoother operation)
// Ramping Settings (Adjusted for ~2 second transition)
const int rampStep = 1;         // Smallest step
const unsigned long rampInterval = 25; // Interval to hit ~1.8s for 15% change (75 steps)

// --- OBJECTS ---
Servo leftEsc;
Servo rightEsc;
ArduinoLEDMatrix matrix; // Matrix Object

// BLE UUIDs
const char* serviceUuid    = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* rxCharUuid     = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; 
const char* txCharUuid     = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; 

BLEService uartService(serviceUuid);
BLECharacteristic rxCharacteristic(rxCharUuid, BLEWrite | BLEWriteWithoutResponse, 20);
BLECharacteristic txCharacteristic(txCharUuid, BLENotify, 20);

// --- STATE ---
bool isArmed = false;
bool isBlocked = false;   // True if obstacle detected
unsigned long lastSensorTime = 0;
unsigned long lastObstacleTime = 0; // Time when obstacle was last seen
unsigned long dangerDisplayStartTime = 0; // Timer for E-Brake display

bool isDangerDisplayActive = false;


// Ramp State
float currentLeft = neutralPulse;
float currentRight = neutralPulse;
int targetLeft = neutralPulse;
int targetRight = neutralPulse;
unsigned long lastRampTime = 0;

// Relay State
const int relayPin = 8;
bool relayState = false;      // Current state of relay (ON/OFF)
bool lastTriangleState = false; // Previous state of Triangle button for edge detection

// --- PUMP CONTROL STATE ---
bool isPumpSystemEnabled = true; // Master Toggle (Triangle Triple Press)
unsigned long movementStartTime = 0; // Timer for 5s delay
bool isPumpRunning = false;          // Actual pump status

// Triple Press Logic
int trianglePressCount = 0;
unsigned long lastTriangleReleaseTime = 0; // For timeout
bool triangleWasPressed = false;      // track state for rising edge

void setup() {
  Serial.begin(115200);
  matrix.begin(); // Start Matrix
  
  Serial.println("--- SprayShark Joystick Demo Starting ---");

  // 1. Setup Motors
  setupMotors();
  
  // 2. Setup Sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Setup Relay
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Start OFF

  // 3. Setup BLE
  if (!BLE.begin()) {
    Serial.println("starting BLE failed! Check hardware.");
    while (1);
  }

  BLE.setDeviceName("SprayShark"); 
  BLE.setLocalName("SprayShark");
  
  uartService.addCharacteristic(rxCharacteristic);
  uartService.addCharacteristic(txCharacteristic);
  BLE.addService(uartService);
  BLE.setAdvertisedService(uartService);
  BLE.advertise();

  Serial.println("BLE Ready. Connect Dabble App -> Gamepad -> Joystick.");
  Serial.println("Auto-Arming Motors...");
  performArmingSequence();
}

void loop() {
  BLEDevice central = BLE.central();

  // 1. Check Safety Sensor (every 60ms)
  checkSafetySensor();

  // 2. Clear E-Brake Display (Timer)
  if (isDangerDisplayActive && millis() - dangerDisplayStartTime > 1000) {
    if (!isBlocked) { // Only clear if not also blocked by sensor
      matrix.clear();
    }
    isDangerDisplayActive = false;
  }
  
  // 3. Update Pump Logic
  updatePumpLogic();

  


  // 4. Update Motor Ramps
  updateMotorRamp();

  // 4. Process BLE
  if (central) {
    if (central.connected()) {
      if (rxCharacteristic.written()) {
        int len = rxCharacteristic.valueLength();
        const uint8_t* data = rxCharacteristic.value();
        processDabblePacket(data, len);
      }
    } else {
      // Disconnected
      targetLeft = neutralPulse;
      targetRight = neutralPulse;
      // Keep Armed even if disconnected, so re-connection works instantly
      matrix.clear();
    }
  }
}

// --- SENSOR LOGIC ---
void checkSafetySensor() {
  unsigned long now = millis();
  if (now - lastSensorTime > 60) { // Check ~15 times/sec
    lastSensorTime = now;
    
    float dist = getDistance();
    
    // Check if obstacle is too close (and valid 0 check)
    if (dist > 0 && dist < safetyDistCm) {
      lastObstacleTime = now; // Update last seen time
      if (!isBlocked) {
        Serial.print("OBSTACLE DETECTED! Dist: ");
        Serial.println(dist);
        // Only stop if we are currently moving FORWARD (above neutral)
        if (targetLeft > neutralPulse || targetRight > neutralPulse) {
           emergencyStop();
        }
        // Show Danger Icon
        matrix.loadFrame(LEDMATRIX_DANGER);
      }
      isBlocked = true;
    } else {
      // Logic: Only clear blocked status if > 1 second passed since last obstacle
      if (isBlocked && (now - lastObstacleTime > 1000)) {
        // Obstacle CLEARED
        if (!isDangerDisplayActive) matrix.clear(); // Clear only if not showing E-Brake
        isBlocked = false;
        Serial.println("Obstacle Cleared.");
      }
    }
  }
}

float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 25000); // Timeout 25ms (~4m max)
  if (duration == 0) return 0; // Timeout/Error
  
  return duration * 0.034 / 2;
}


// --- RAMPING LOGIC ---
void updateMotorRamp() {
  unsigned long now = millis();
  if (now - lastRampTime >= rampInterval) {
    lastRampTime = now;

    // Move Left Motor towards Target
    if (currentLeft < targetLeft) {
      currentLeft += rampStep;
      if (currentLeft > targetLeft) currentLeft = targetLeft;
    } else if (currentLeft > targetLeft) {
      currentLeft -= rampStep;
      if (currentLeft < targetLeft) currentLeft = targetLeft;
    }

    // Move Right Motor towards Target
    if (currentRight < targetRight) {
      currentRight += rampStep;
      if (currentRight > targetRight) currentRight = targetRight;
    } else if (currentRight > targetRight) {
      currentRight -= rampStep;
      if (currentRight < targetRight) currentRight = targetRight;
    }

    // Write to ESCs
    leftEsc.writeMicroseconds((int)currentLeft);
    rightEsc.writeMicroseconds((int)currentRight);
  }
}

// --- PACKET PARSER ---
void processDabblePacket(const uint8_t* data, int len) {
  // Debug Print (Keep this to find Buttons!)
  Serial.print("PKT: ");
  for(int i=0; i<len; i++) {
    Serial.print(data[i], HEX); Serial.print(" ");
  }
  Serial.println();

  if (len < 7 || data[0] != 0xFF || data[1] != 0x01) return;

  uint8_t moduleId = data[2]; // 0x02 = Joystick
  
  if (moduleId == 0x02) {
    // --- JOYSTICK MODE ---
    // Header: FF 01 02 01 02 [Byte5] [AngleByte] [Byte7]
    
    // We suspect Byte 5 is Buttons or Radius (but it was 0).
    // Let's monitor Byte 5 and 7 for changes when buttons are pressed.
    
    uint8_t angleByte = data[6]; 
    bool isCenter = (angleByte == 0 && data[5] == 0); // Heuristic for now

    // --- BUTTONS IN JOYSTICK MODE ---
    // Byte 5 carries the button state (Start=0x01, Select=0x02, Triangle=0x04, Circle=0x08, Cross=0x10, Square=0x20, L3=0x40, R3=0x80)
    // NOTE: In Joystick mode, Dabble sends buttons in Byte 5.
    uint8_t buttons = data[5];


    // E-BRAKE (Cross = 0x10)
    if (buttons & 0x10) {
      Serial.println("!!! E-BRAKE TRIGGERED (Joy) !!!");
      emergencyStop();
      matrix.loadFrame(LEDMATRIX_DANGER);
      dangerDisplayStartTime = millis();
      isDangerDisplayActive = true;
      return;
    }

    // TRIANGLE (0x04) - Pump Settings Toggle
    bool trianglePressed = buttons & 0x04;
    
    // Rising Edge Detection
    if (trianglePressed && !triangleWasPressed) {
       // Check time since last press to determine "streak"
       unsigned long now = millis();
       if (now - lastTriangleReleaseTime < 1000) { // 1 second timeout between presses
         trianglePressCount++;
       } else {
         trianglePressCount = 1; // Start new chain
       }
       lastTriangleReleaseTime = now; // Update time
       
       Serial.print("Triangle Press: "); Serial.println(trianglePressCount);

       if (trianglePressCount >= 3) {
         // TOGGLE SETTING
         isPumpSystemEnabled = !isPumpSystemEnabled;
         trianglePressCount = 0; // Reset
         
         Serial.print("PUMP SYSTEM: "); 
         Serial.println(isPumpSystemEnabled ? "ENABLED" : "DISABLED");
         
         // Visual Feedback
         if (isPumpSystemEnabled) {
           matrix.loadFrame(LEDMATRIX_PUMP_ON);
         } else {
           matrix.loadFrame(LEDMATRIX_PUMP_OFF);
         }
         // Show status for 1.5s
         dangerDisplayStartTime = millis(); 
         isDangerDisplayActive = true; 
       }
    }
    triangleWasPressed = trianglePressed;

    // ARMING (Start = 0x01)


    // ARMING (Start = 0x01)
    if (buttons & 0x01) {
      if (!isArmed) performArmingSequence();
      return;
    }

    if (!isArmed) return;

    // --- CONTROL LOGIC ---
    
    // --- JOYSTICK MODE LOGIC (Differential) ---
    if (isCenter) {
      targetLeft = neutralPulse;
      targetRight = neutralPulse;
    } else {
          // Calculate inputs based on Angle
          int angleDeg = angleByte * 2;
          
          // 8-ZONE LOGIC with NEW SPEEDS
          // Cardinal Fwd/Back (N, S) -> 40%
          // Cardinal Left/Right (E, W) -> One 40%, One 20% (Sharp Turn)
          // Intercardinal (NE, NW, SE, SW) -> One 40%, One 30% (Smooth Turn)
          
          if (angleDeg >= 338 || angleDeg < 23) {
            // --- EAST (Right) --- -> Turn Right
            // Left Motor Fast (40%), Right Motor Slow (20%)
            targetLeft = speed40Fwd;
            targetRight = speed20Fwd;
          } 
          else if (angleDeg >= 23 && angleDeg < 68) {
             // --- NORTH EAST --- -> Smooth Turn Right
             // Left Motor 40%, Right Motor 30%
             targetLeft = speed40Fwd;
             targetRight = speed30Fwd;
          }
          else if (angleDeg >= 68 && angleDeg < 113) {
             // --- NORTH (Up) --- -> Both 40%
             targetLeft = speed40Fwd;
             targetRight = speed40Fwd;
          }
          else if (angleDeg >= 113 && angleDeg < 158) {
             // --- NORTH WEST --- -> Smooth Turn Left
             // Right Motor 40%, Left Motor 30%
             targetLeft = speed30Fwd;
             targetRight = speed40Fwd;
          }
          else if (angleDeg >= 158 && angleDeg < 203) {
             // --- WEST (Left) --- -> Turn Left
             // Right Motor 40%, Left Motor 20%
             targetLeft = speed20Fwd;
             targetRight = speed40Fwd;
          }
          else if (angleDeg >= 203 && angleDeg < 248) {
             // --- SOUTH WEST --- -> Reverse Left Turn
             // Right Motor 40% Rev, Left Motor 30% Rev
             targetLeft = speed30Rev;
             targetRight = speed40Rev;
          }
          else if (angleDeg >= 248 && angleDeg < 293) {
             // --- SOUTH (Down) --- -> Both 40% Rev
             targetLeft = speed40Rev;
             targetRight = speed40Rev;
          }
          else if (angleDeg >= 293 && angleDeg < 338) {
             // --- SOUTH EAST --- -> Reverse Right Turn
             // Left Motor 40% Rev, Right Motor 30% Rev
             targetLeft = speed40Rev;
             targetRight = speed30Rev;
          }
    
          Serial.print("Joy | Ang: "); Serial.print(angleDeg);
          Serial.print(" L: "); Serial.print(targetLeft);
          Serial.print(" R: "); Serial.println(targetRight);
      }
    
  } else if (moduleId == 0x01) {
    // --- DIGITAL MODE SUPPORT (Real D-Pad) ---
    
    uint8_t actionButtons = data[5];
    uint8_t dpadButtons = data[6];
    
    if (actionButtons & 0x10) { 
      Serial.println("E-BRAKE"); emergencyStop(); return; 
    }
    if (actionButtons & 0x01 && !isArmed) {
      performArmingSequence(); return;
    }
    
    // Digital Drive with Diagonals
    if (isArmed) {
       // Reset first
       targetLeft = neutralPulse;
       targetRight = neutralPulse;

       // Check Diagonals First (Bit Combinations)
       // NE = Up (0x01) + Right (0x08) = 0x09
       // NW = Up (0x01) + Left (0x04) = 0x05
       // SE = Down (0x02) + Right (0x08) = 0x0A
       // SW = Down (0x02) + Left (0x04) = 0x06 -- Note: Dabble might handle multipress differently, ensuring logic works for bitmasks
       
       bool up = dpadButtons & 0x01;
       bool down = dpadButtons & 0x02;
       bool left = dpadButtons & 0x04;
       bool right = dpadButtons & 0x08;

       if (up && right) {
          // NE: Left 40%, Right 30%
          targetLeft = speed40Fwd; targetRight = speed30Fwd;
       } else if (up && left) {
          // NW: Right 40%, Left 30%
          targetLeft = speed30Fwd; targetRight = speed40Fwd;
       } else if (down && right) {
          // SE: Left 40% Rev, Right 30% Rev
          targetLeft = speed40Rev; targetRight = speed30Rev;
       } else if (down && left) {
          // SW: Right 40% Rev, Left 30% Rev
          targetLeft = speed30Rev; targetRight = speed40Rev;
       } 
       // Check Cardinals
       else if (up) {
          targetLeft = speed40Fwd; targetRight = speed40Fwd;
       } else if (down) {
          targetLeft = speed40Rev; targetRight = speed40Rev;
       } else if (left) {
          // Left Turn: Right 40%, Left 20%
          targetLeft = speed20Fwd; targetRight = speed40Fwd;
       } else if (right) {
          // Right Turn: Left 40%, Right 20%
          targetLeft = speed40Fwd; targetRight = speed20Fwd;
       }
    }
  }

  // --- SAFETY CHECK (Obstacle Avoidance) ---
  // Final safeguard: if blocked, prevent forward movement regardless of input source
  if (isBlocked) {
    if (targetLeft > neutralPulse) targetLeft = neutralPulse;
    if (targetRight > neutralPulse) targetRight = neutralPulse;
  }
}

// --- HELPER FUNCTIONS ---

// --- PUMP LOGIC ---
void updatePumpLogic() {
  // If Master Switch is OFF, ensure pump is OFF and return
  if (!isPumpSystemEnabled) {
    if (isPumpRunning) {
      digitalWrite(relayPin, LOW);
      isPumpRunning = false;
      Serial.println("Pump Stopped (System Disabled).");
    }
    return;
  }

  // Check if robot is moving
  // We consider "moving" if either target is NOT neutral.
  // Note: We use targetLeft/Right because they reflect the INTENTION to move.
  bool specificMoving = (targetLeft != neutralPulse || targetRight != neutralPulse);
  
  // Additional safety: If blocked, we are NOT moving (logic in loop forces targets to neutral, but good to be redundant)
  if (isBlocked) specificMoving = false;

  if (specificMoving) {
    // If just started moving, capture time
    if (movementStartTime == 0) {
      movementStartTime = millis();
    }
    
    // Check if 5 seconds have passed
    if (millis() - movementStartTime >= 5000) {
      if (!isPumpRunning) {
        digitalWrite(relayPin, HIGH); // START PUMP
        isPumpRunning = true;
        Serial.println("Pump Started (5s Delay Reached).");
      }
    }
  } else {
    // Robot Stopped
    movementStartTime = 0; // Reset timer
    if (isPumpRunning) {
      digitalWrite(relayPin, LOW); // STOP PUMP
      isPumpRunning = false;
      Serial.println("Pump Stopped (Robot Stopped).");
    }
  }
}

void emergencyStop() {
  targetLeft = neutralPulse;
  targetRight = neutralPulse;
  currentLeft = neutralPulse;
  currentRight = neutralPulse;
  
  leftEsc.writeMicroseconds(neutralPulse);
  rightEsc.writeMicroseconds(neutralPulse);
  
  // Turn off Pump
  relayState = false;
  digitalWrite(relayPin, LOW);
}

void setupMotors() {
  Serial.println("Attaching ESCs...");
  leftEsc.attach(leftEscPin, minPulse, maxPulse);
  rightEsc.attach(rightEscPin, minPulse, maxPulse);
  emergencyStop();
}

void performArmingSequence() {
  Serial.println("ARMING Sequence Started...");
  emergencyStop();
  matrix.loadSequence(LEDMATRIX_ANIMATION_LOCK);
  matrix.play(true);
  
  pinMode(LED_BUILTIN, OUTPUT);
  for(int i=5; i>0; i--) {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println(i);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
  
  isArmed = true;
  Serial.println("ARMED! Ready.");
  matrix.clear();
}
