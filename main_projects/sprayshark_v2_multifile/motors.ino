#include "config.h"

// --- STATE FOR KICKSTART ---
bool isKickstarting = false;
unsigned long kickstartStartTime = 0;

// --- MOTOR CONTROL & RAMPING LOGIC ---
void updateMotorRamp() {
  unsigned long now = millis();

  // 1. Kickstart Logic: Override target if starting from stationary
  int effectiveTarget = targetDrivePulse;
  bool isLinearPhase = false;

  if (targetDrivePulse != neutralPulse) {
    // If robot is not moving (stopped or just started), and a signal is issued
    if (currentDrivePulse == (float)neutralPulse && !isKickstarting) {
      isKickstarting = true;
      kickstartStartTime = now;
      Serial.println("Kickstart: Initiated (1s Ramp-Up to 80%)");
    }

    if (isKickstarting) {
      unsigned long elapsed = now - kickstartStartTime;
      
      if (elapsed < kickstartRampDuration) {
        // Phase 1: Linear Ramp Up to 80% (1000ms)
        float progress = (float)elapsed / kickstartRampDuration;
        float startPulse = (float)neutralPulse;
        float endPulse = (targetDrivePulse < neutralPulse) ? (float)speed80Fwd : (float)speed80Rev;
        
        currentDrivePulse = startPulse + (endPulse - startPulse) * progress;
        isLinearPhase = true; 
      } 
      else if (elapsed < (kickstartRampDuration + kickstartHoldDuration)) {
        // Phase 2: Hold at 80% (500ms)
        effectiveTarget = (targetDrivePulse < neutralPulse) ? speed80Fwd : speed80Rev;
      } 
      else {
        // Phase 3: Settle at 35% (Normal ramping logic will handle the gradual move from 80% to 35%)
        effectiveTarget = (targetDrivePulse < neutralPulse) ? speed35Fwd : speed35Rev;
      }
    }
  } else {
    // Reset if target is neutral (stopped)
    isKickstarting = false;
  }

  // 2. Output Logic
  if (isLinearPhase) {
    // Write smoothly during high-speed ramp-up
    leftEsc.writeMicroseconds((int)currentDrivePulse);
    rightEsc.writeMicroseconds((int)currentDrivePulse);
    lastRampTime = now; // Keep ramping timer updated
  } 
  else if (now - lastRampTime >= rampInterval) {
    lastRampTime = now;

    // Gradual Ramping towards Effective Target (Handles 80% -> 40% transition)
    if (currentDrivePulse < effectiveTarget) {
      currentDrivePulse += rampStep;
      if (currentDrivePulse > effectiveTarget) currentDrivePulse = (float)effectiveTarget;
    } else if (currentDrivePulse > effectiveTarget) {
      currentDrivePulse -= rampStep;
      if (currentDrivePulse < effectiveTarget) currentDrivePulse = (float)effectiveTarget;
    }

    // Write pulses to ESCs
    leftEsc.writeMicroseconds((int)currentDrivePulse);
    rightEsc.writeMicroseconds((int)currentDrivePulse);
  }
}

void emergencyStop() {
  targetDrivePulse = neutralPulse;
  currentDrivePulse = neutralPulse;
  
  leftEsc.writeMicroseconds(neutralPulse);
  rightEsc.writeMicroseconds(neutralPulse);

  isKickstarting = false;

  // Hard stop actuator and forget targets instantly
  isManualOverride = false;
  targetActuatorPosMs = currentActuatorPosMs;
  stopActuator();
  
  // Turn off Pump
  isPumpRunning = false;
  digitalWrite(relayPin, LOW); // NO: LOW = pump off
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
  
  for(int i=5; i>0; i--) {
    Serial.println(i);
    delay(1000);
  }
  
  isArmed = true;
  Serial.println("ARMED! Ready.");
  matrix.clear();
}
