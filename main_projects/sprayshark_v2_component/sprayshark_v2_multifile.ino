/*
  SprayShark_Bluetooth_Joystick_Demo.ino

  Purpose:
  Control two BLDC motors (Left/Right) via Bluetooth using the "Dabble" App (Joystick Module).
  
  Features:
  - Custom Packet Parser for Uno R4 WiFi Native BLE
  - Smooth Ramping (Acceleration/Deceleration)
  - Emergency Brake (Cross/X Button)
  - LED Matrix Feedback: Shows DANGER icon on E-Brake or Obstacle
  
  The Hardware:
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

#include "config.h"

// --- OBJECTS ---
Servo leftEsc;
Servo rightEsc;
ArduinoLEDMatrix matrix; // Matrix Object

BLEService uartService(serviceUuid);
BLECharacteristic rxCharacteristic(rxCharUuid, BLEWrite | BLEWriteWithoutResponse, 20);
BLECharacteristic txCharacteristic(txCharUuid, BLENotify, 20);

// --- STATE ---
int lastStateCLK;
bool isManualOverride = false;

bool isArmed = false;
unsigned long dangerDisplayStartTime = 0; // Timer for E-Brake display

bool isDangerDisplayActive = false;

// Ramp State (Unified for Rear Wheels)
float currentDrivePulse = neutralPulse;
int targetDrivePulse = neutralPulse;
unsigned long lastRampTime = 0;

unsigned long POS_MIDDLE_MS = 3000;  // 25mm (Updated live by rotary encoder)

// --- ACTUATOR STATE TRACKING ---
unsigned long currentActuatorPosMs = POS_MIDDLE_MS; // Start at middle
unsigned long targetActuatorPosMs = POS_MIDDLE_MS;
bool isActuatorMoving = false;
unsigned long actuatorMoveStartMs = 0;
unsigned long actuatorStartPosMs = POS_MIDDLE_MS;

// Real-Time Position Tracking
float currentActuatorPosMM = 0.0; // 0.0 is center

// --- PUMP CONTROL STATE ---
bool isPumpRunning = false;          
bool triangleWasPressed = false;     
unsigned long movementStartTime = 0; 
bool autoPumpTriggered = false;      

// --- PUMP HELPERS ---
void setPump(bool targetState) {
  isPumpRunning = targetState;
  digitalWrite(relayPin, isPumpRunning ? HIGH : LOW);
  
  // Visual Feedback
  if (isPumpRunning) {
    matrix.loadFrame(LEDMATRIX_PUMP_ON);
  } else {
    matrix.loadFrame(LEDMATRIX_PUMP_OFF);
  }
  dangerDisplayStartTime = millis(); 
  isDangerDisplayActive = true; 
}

void updatePumpLogic() {
  bool isMoving = (targetDrivePulse != neutralPulse);

  if (isMoving) {
    // 1. Mark movement start if first bit of movement
    if (movementStartTime == 0) {
      movementStartTime = millis();
      autoPumpTriggered = false; // Reset for this movement
    }

    // 2. Auto-On after 3 seconds
    if (!autoPumpTriggered && (millis() - movementStartTime > 3000)) {
      if (!isPumpRunning) {
         Serial.println("Auto-Pump: ON (3s delay reached)");
         setPump(true);
      }
      autoPumpTriggered = true; // Only trigger auto-on once per movement
    }
  } 
  else {
    // 3. Auto-Off when stopped — but NOT if manually toggled
    if (isPumpRunning && !autoPumpTriggered) {
       Serial.println("Auto-Pump: OFF (Robot Stopped)");
       setPump(false);
    }
    movementStartTime = 0;
    // Don't reset autoPumpTriggered here — it guards manual toggles while stationary
  }
}

void setup() {
  Serial.begin(115200);
  matrix.begin(); // Start Matrix
  
  Serial.println("--- SprayShark Ready ---");

  setupMotors();
  setupActuator();
  
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); 

  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  BLE.setDeviceName("SprayShark"); 
  BLE.setLocalName("SprayShark");
  
  uartService.addCharacteristic(rxCharacteristic);
  uartService.addCharacteristic(txCharacteristic);
  BLE.addService(uartService);
  BLE.setAdvertisedService(uartService);
  BLE.advertise();

  performArmingSequence();
}

void loop() {
  BLEDevice central = BLE.central();

  if (isDangerDisplayActive && millis() - dangerDisplayStartTime > 1000) {
    matrix.clear();
    isDangerDisplayActive = false;
  }
  
  updateActuator();
  processRotaryEncoder();
  updateMotorRamp();
  updatePumpLogic(); // <--- NEW AUTO LOGIC

  if (central) {
    if (central.connected()) {
      if (rxCharacteristic.written()) {
        int len = rxCharacteristic.valueLength();
        const uint8_t* data = rxCharacteristic.value();
        processDabblePacket(data, len);
      }
    } else {
      targetDrivePulse = neutralPulse;
      targetActuatorPosMs = POS_MIDDLE_MS;
      matrix.clear();
    }
  }
}
