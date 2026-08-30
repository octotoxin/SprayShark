#include "config.h"

//errors found march 2025

// --- ROTARY ENCODER LOGIC ---
void processRotaryEncoder() {
  int currentStateCLK = digitalRead(encoderCLK);
  
  // If state has changed and it's 1 (rising edge)
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
    if (digitalRead(encoderDT) != currentStateCLK) {
      // "+1" Extend Action (assuming CW -> Extend)
      unsigned long newTarget = targetActuatorPosMs + 100;
      setActuatorTarget(newTarget, false); // FALSE = Do not enforce bounds
      isManualOverride = true;
      POS_MIDDLE_MS = newTarget; // Redefine center to this new position
      Serial.print("Encoder: Extend +100ms | New Center: "); Serial.println(POS_MIDDLE_MS);
    } else {
      // "-1" Retract Action (CCW -> Retract)
      if (targetActuatorPosMs >= 100) {
        unsigned long newTarget = targetActuatorPosMs - 100;
        setActuatorTarget(newTarget, false); // FALSE = Do not enforce bounds
        isManualOverride = true;
        POS_MIDDLE_MS = newTarget; // Redefine center to this new position
        Serial.print("Encoder: Retract -100ms | New Center: "); Serial.println(POS_MIDDLE_MS);
      }
    }
  }
  lastStateCLK = currentStateCLK;
  
  // Button press = Stop
  if (digitalRead(encoderSW) == LOW) {
    targetActuatorPosMs = currentActuatorPosMs; // Erase memory of active position
    isManualOverride = false;
    stopActuator();
    Serial.println("Encoder: Button STOP");
    delay(200); // Simple debounce
  }
}
