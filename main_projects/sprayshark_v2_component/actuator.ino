#include "config.h"

//errors found march 2025

// --- ACTUATOR LOGIC (NON-BLOCKING) ---
void setupActuator() {
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  pinMode(R_EN_PIN, OUTPUT);
  pinMode(L_EN_PIN, OUTPUT);
  pinMode(R_IS_PIN, INPUT);
  pinMode(L_IS_PIN, INPUT);

  // Rotary Encoder Pins
  pinMode(encoderCLK, INPUT);
  pinMode(encoderDT, INPUT);
  pinMode(encoderSW, INPUT_PULLUP);
  lastStateCLK = digitalRead(encoderCLK);

  // Overturn Limit Switches (NC switch to GND — LOW = safe, HIGH = triggered)
  pinMode(LIMIT_SWITCH_RIGHT_PIN, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_LEFT_PIN,  INPUT_PULLUP);

  // Enable Driver
  digitalWrite(R_EN_PIN, HIGH);
  digitalWrite(L_EN_PIN, HIGH);
  
  stopActuator();

  // --- LIMIT SWITCH DIAGNOSTICS ---
  Serial.print("Limit Switch RIGHT (A0): ");
  Serial.println(digitalRead(LIMIT_SWITCH_RIGHT_PIN) == HIGH ? "TRIGGERED (HIGH)" : "SAFE (LOW)");
  Serial.print("Limit Switch LEFT  (A1): ");
  Serial.println(digitalRead(LIMIT_SWITCH_LEFT_PIN)  == HIGH ? "TRIGGERED (HIGH)" : "SAFE (LOW)");
  
  // As per config, assume we start at the middle
  currentActuatorPosMs = POS_MIDDLE_MS;
  targetActuatorPosMs = POS_MIDDLE_MS;
}

// --- OVERTURN LIMIT SWITCH HELPER ---
// Returns true if the limit switch for the given direction is triggered.
// NC switches: LOW = safe (closed to GND), HIGH = triggered (switch opened)
// movingRight = true  → check A0 (right-tilt limit)
// movingRight = false → check A1 (left-tilt limit)
bool isOverturnLimitHit(bool movingRight) {
  if (movingRight) {
    return (digitalRead(LIMIT_SWITCH_RIGHT_PIN) == HIGH);
  } else {
    return (digitalRead(LIMIT_SWITCH_LEFT_PIN) == HIGH);
  }
}

void setActuatorTarget(unsigned long targetMs, bool enforceBounds) {
  // Bound check (limit to +/- travelDistMM from center)
  unsigned long travelMs = travelDistMM * MS_PER_MM;
  unsigned long maxPos = POS_MIDDLE_MS + travelMs;
  unsigned long minPos = (POS_MIDDLE_MS > travelMs) ? (POS_MIDDLE_MS - travelMs) : 0;
  
  // Guard against massive integer underflow from calculations beforehand
  if (targetMs > 50000) targetMs = 0; 
  
  if (enforceBounds) {
    if (targetMs > maxPos) targetMs = maxPos;
    if (targetMs < minPos) targetMs = minPos;
  } else {
    // Hard physical limits to prevent integer underflow/overflow on the 6s stroke
    if (targetMs > FULL_TRAVEL_MS) targetMs = FULL_TRAVEL_MS;
  }
  
  if (targetMs != targetActuatorPosMs) {
    // If the actuator is currently moving, we MUST calculate its final position based on the 
    // OLD target before updating to the NEW target. This prevents position tracking corruption.
    if (isActuatorMoving) {
      // One last position calculation before stopping
      unsigned long elapsed = millis() - actuatorMoveStartMs;
      
      // We use the OLD targetActuatorPosMs to mathematically determine which way it WAS moving
      if (targetActuatorPosMs > actuatorStartPosMs) {
        currentActuatorPosMs = actuatorStartPosMs + elapsed;
        if (currentActuatorPosMs > targetActuatorPosMs) currentActuatorPosMs = targetActuatorPosMs; // Prevent overshoot
      } else {
        if (elapsed > actuatorStartPosMs) currentActuatorPosMs = 0;
        else currentActuatorPosMs = actuatorStartPosMs - elapsed;
        if (currentActuatorPosMs < targetActuatorPosMs) currentActuatorPosMs = targetActuatorPosMs; // Prevent overshoot
      }
      stopActuator();
    }
    
    // Now it is safe to assign the new intended target
    targetActuatorPosMs = targetMs;
  }
}

void stopActuator() {
  digitalWrite(RPWM_PIN, LOW);
  digitalWrite(LPWM_PIN, LOW);
  isActuatorMoving = false;
  
  // Update real-time position logic
  currentActuatorPosMM = ((float)currentActuatorPosMs - POS_MIDDLE_MS) / (float)MS_PER_MM;
  
  // Note: we purposefully do NOT lock targetActuatorPosMs to current here,
  // so that when movement is allowed again, it can resume seeking its target.
}

void updateActuator() {
  if (isActuatorMoving) {
    // --- OVERTURN LIMIT CHECK (while moving) ---
    // Determine which direction we are currently moving
    bool movingRight = (targetActuatorPosMs > actuatorStartPosMs);
    if (isOverturnLimitHit(movingRight)) {
      // Limit switch triggered mid-move — hard stop immediately
      unsigned long elapsed = millis() - actuatorMoveStartMs;
      if (movingRight) {
        currentActuatorPosMs = actuatorStartPosMs + elapsed;
      } else {
        if (elapsed > actuatorStartPosMs) currentActuatorPosMs = 0;
        else currentActuatorPosMs = actuatorStartPosMs - elapsed;
      }
      targetActuatorPosMs = currentActuatorPosMs; // Cancel the target
      stopActuator();
      Serial.println("OVERTURN LIMIT HIT — Steering blocked!");
      return;
    }

    // Update our simulated position constantly while moving
    unsigned long elapsed = millis() - actuatorMoveStartMs;
    if (targetActuatorPosMs > actuatorStartPosMs) {
      currentActuatorPosMs = actuatorStartPosMs + elapsed;
      if (currentActuatorPosMs >= targetActuatorPosMs) {
        currentActuatorPosMs = targetActuatorPosMs;
        stopActuator();
      }
    } else {
      if (elapsed > actuatorStartPosMs) currentActuatorPosMs = 0;
      else currentActuatorPosMs = actuatorStartPosMs - elapsed;
      
      if (currentActuatorPosMs <= targetActuatorPosMs || elapsed >= (actuatorStartPosMs - targetActuatorPosMs)) {
        currentActuatorPosMs = targetActuatorPosMs;
        stopActuator();
      }
    }
  }

  // We are allowed to move. Check if we need to start targeting an unreached position.
  if (!isActuatorMoving && targetActuatorPosMs != currentActuatorPosMs) {
    // --- OVERTURN LIMIT CHECK (before starting) ---
    bool wantsRight = (targetActuatorPosMs > currentActuatorPosMs);
    if (isOverturnLimitHit(wantsRight)) {
      // Block movement in this direction — cancel target
      targetActuatorPosMs = currentActuatorPosMs;
      Serial.println("OVERTURN LIMIT — Cannot steer in that direction!");
      return;
    }

    actuatorMoveStartMs = millis();
    actuatorStartPosMs = currentActuatorPosMs;
    isActuatorMoving = true;
    
    if (targetActuatorPosMs > currentActuatorPosMs) {
      digitalWrite(LPWM_PIN, LOW);
      digitalWrite(RPWM_PIN, HIGH);
    } else {
      digitalWrite(RPWM_PIN, LOW);
      digitalWrite(LPWM_PIN, HIGH);
    }
  }

  // Only calculate real-time theoretical MM position when the position actually changes
  static unsigned long lastActuatorPosMs = 0xFFFFFFFF;
  if (currentActuatorPosMs != lastActuatorPosMs) {
    currentActuatorPosMM = ((float)currentActuatorPosMs - POS_MIDDLE_MS) / (float)MS_PER_MM;
    lastActuatorPosMs = currentActuatorPosMs;
  }
}
