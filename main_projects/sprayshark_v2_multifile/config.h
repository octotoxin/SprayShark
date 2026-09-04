#ifndef CONFIG_H
#define CONFIG_H

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
const int relayPin = 6;

// Actuator Pins (BTS7960)
const int RPWM_PIN = 2; // Extend
const int LPWM_PIN = 3; // Retract
const int R_EN_PIN = 4;
const int L_EN_PIN = 5;
const int R_IS_PIN = 12; // Current Sense Right (Forward/Extend)
const int L_IS_PIN = 11; // Current Sense Left (Reverse/Retract)

// Rotary Encoder Pins
const int encoderCLK = 7;
const int encoderDT = 8;
const int encoderSW = 13;

// Overturn Limit Switches (NC to GND, uses INPUT_PULLUP)
// When triggered (HIGH = switch opened), block steering in that direction
const int LIMIT_SWITCH_RIGHT_PIN = A0; // Blocks rightward (extend) steering
const int LIMIT_SWITCH_LEFT_PIN  = A1; // Blocks leftward (retract) steering

// Motor Pulse Settings
const int minPulse = 1000;      // Max Reverse
const int neutralPulse = 1500;  // Stop
const int maxPulse = 2000;      // Max Forward

// Speed Targets (Static friction kickstart logic)
const int speed80Fwd = 1100;      // 80% Forward
const int speed80Rev = 1900;      // 80% Reverse
const int speed35Fwd = 1325;      // 35% Forward
const int speed35Rev = 1675;      // 35% Reverse
const unsigned long kickstartRampDuration = 1000; // 1s ramp to 80%
const unsigned long kickstartHoldDuration = 500;  // 0.5s hold at 80%

// Speed Targets (Original 60% - kept for compatibility)
const int speed60Fwd = 1200;      // 60% Forward - Swapped
const int speed60Rev = 1800;      // 60% Reverse - Swapped

// Ramping Settings (Adjusted for 2x faster transition)
const int rampStep = 2;         // Increased from 1 to double speed
const unsigned long rampInterval = 25; // Interval to update

// --- ACTUATOR TIMING CONSTANTS ---
const unsigned long FULL_TRAVEL_MS = 6000; 
const int MS_PER_MM = 120; // 60ms / 50mm = 120ms/mm
const int travelDistMM = 12; // Distance to move the actuatorleft/right from center

// BLE UUIDs
const char* serviceUuid    = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* rxCharUuid     = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; 
const char* txCharUuid     = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; 

// --- OBJECTS ---
extern Servo leftEsc;
extern Servo rightEsc;
extern ArduinoLEDMatrix matrix; // Matrix Object
extern BLEService uartService;
extern BLECharacteristic rxCharacteristic;
extern BLECharacteristic txCharacteristic;

// --- SHARED GLOBAL STATE VARIABLES ---
extern int lastStateCLK;
extern bool isManualOverride;

extern bool isArmed;
extern unsigned long dangerDisplayStartTime; // Timer for E-Brake display
extern bool isDangerDisplayActive;

// Ramp State (Unified for Rear Wheels)
extern float currentDrivePulse;
extern int targetDrivePulse;
extern unsigned long lastRampTime;

// Kickstart State
extern bool isKickstarting;
extern unsigned long kickstartStartTime;

extern unsigned long POS_MIDDLE_MS;  // 25mm (Updated live by rotary encoder)

// --- ACTUATOR STATE TRACKING ---
extern unsigned long currentActuatorPosMs; // Start at middle
extern unsigned long targetActuatorPosMs;
extern bool isActuatorMoving;
extern unsigned long actuatorMoveStartMs;
extern unsigned long actuatorStartPosMs;

// Real-Time Position Tracking
extern float currentActuatorPosMM; // 0.0 is center

// --- PUMP CONTROL STATE ---
extern bool isPumpRunning;          // Pump on/off (toggled by Triangle)
extern bool triangleWasPressed;     // Track state for rising edge
extern unsigned long movementStartTime; // Track when robot started moving
extern bool autoPumpTriggered;      // Ensure auto-on only fires once per movement

// --- FUNCTION PROTOTYPES ---
void setupMotors();
void updateMotorRamp();
void emergencyStop();
void performArmingSequence();

void setupActuator();
void updateActuator();
void stopActuator();
void setActuatorTarget(unsigned long targetMs, bool enforceBounds = true);

void setPump(bool targetState);
void updatePumpLogic();
void processRotaryEncoder();
void processDabblePacket(const uint8_t* data, int len);
bool isOverturnLimitHit(bool movingRight);

#endif // CONFIG_H
