#include "config.h"

//errors found march 2025

// --- PACKET PARSER ---
void processDabblePacket(const uint8_t* data, int len) {
  // Debug Print (Keep this to find Buttons!)
  Serial.print("PKT: ");
  for(int i=0; i<len; i++) {
    Serial.print(data[i], HEX); Serial.print(" ");
  }
  Serial.println();

  if (len < 7 || data[0] != 0xFF || data[1] != 0x01) return;

  uint8_t moduleId = data[2]; 
  uint8_t buttons = data[5]; // Byte 5 holds Buttons in both Joystick and Digital modes
  
  // --- 1. COMMON BUTTON LOGIC ---

  // E-BRAKE (Cross = 0x10)
  if (buttons & 0x10) {
    Serial.println("!!! E-BRAKE TRIGGERED !!!");
    emergencyStop();
    matrix.loadFrame(LEDMATRIX_DANGER);
    dangerDisplayStartTime = millis();
    isDangerDisplayActive = true;
    return;
  }

  // TRIANGLE (0x04) - Pump Toggle (Single Press)
  // Reads pin directly and flips state, so it always works regardless of tracked state
  bool trianglePressed = buttons & 0x04;
  if (trianglePressed && !triangleWasPressed) {
     bool relayIsOn = digitalRead(relayPin) == HIGH;
     setPump(!relayIsOn);
     autoPumpTriggered = true; // Prevents auto-logic from immediately overriding
     Serial.print("Manual Pump: "); 
     Serial.println(isPumpRunning ? "ON" : "OFF");
  }
  triangleWasPressed = trianglePressed;

  // ARMING (Start = 0x01)
  if (buttons & 0x01) {
    if (!isArmed) performArmingSequence();
    return;
  }

  if (!isArmed) return;

  // --- 2. MOVEMENT LOGIC ---
  
  if (moduleId == 0x02) {
    // --- JOYSTICK MODE ---
    uint8_t angleByte = data[6]; 
    bool isCenter = (angleByte == 0 && data[5] == 0); 

    if (isCenter) {
      targetDrivePulse = neutralPulse;
      // We will leave the joystick resetting the wheels to center if let go
      setActuatorTarget(POS_MIDDLE_MS);
    } else {
      int angleDeg = angleByte * 2;
      unsigned long travelMs = travelDistMM * MS_PER_MM;
      unsigned long posLeftMs = (POS_MIDDLE_MS > travelMs) ? (POS_MIDDLE_MS - travelMs) : 0;  
      unsigned long posRightMs = POS_MIDDLE_MS + travelMs; 

      if (angleDeg >= 338 || angleDeg < 23) {
        targetDrivePulse = neutralPulse; setActuatorTarget(posRightMs);
      } else if (angleDeg >= 23 && angleDeg < 68) {
        targetDrivePulse = speed60Fwd; setActuatorTarget(posRightMs);
      } else if (angleDeg >= 68 && angleDeg < 113) {
        targetDrivePulse = speed60Fwd; setActuatorTarget(POS_MIDDLE_MS);
      } else if (angleDeg >= 113 && angleDeg < 158) {
        targetDrivePulse = speed60Fwd; setActuatorTarget(posLeftMs);
      } else if (angleDeg >= 158 && angleDeg < 203) {
        targetDrivePulse = neutralPulse; setActuatorTarget(posLeftMs);
      } else if (angleDeg >= 203 && angleDeg < 248) {
        targetDrivePulse = speed60Rev; setActuatorTarget(posLeftMs);
      } else if (angleDeg >= 248 && angleDeg < 293) {
        targetDrivePulse = speed60Rev; setActuatorTarget(POS_MIDDLE_MS);
      } else if (angleDeg >= 293 && angleDeg < 338) {
        targetDrivePulse = speed60Rev; setActuatorTarget(posRightMs);
      }
      
      Serial.print("Joy | Ang: "); Serial.print(angleDeg);
      Serial.print(" Drive: "); Serial.println(targetDrivePulse);
    }
    
  } else if (moduleId == 0x01) {
    // --- DIGITAL MODE SUPPORT (D-Pad) ---
    uint8_t dpadButtons = data[6];
    
    // Reset Drive Only
    targetDrivePulse = neutralPulse;

    bool up = dpadButtons & 0x01;
    bool down = dpadButtons & 0x02;
    bool left = (dpadButtons & 0x04) || (buttons & 0x20);   // D-Pad Left OR Square
    bool right = (dpadButtons & 0x08) || (buttons & 0x08);  // D-Pad Right OR Circle

    unsigned long travelMs = travelDistMM * MS_PER_MM;
    unsigned long posLeftMs = (POS_MIDDLE_MS > travelMs) ? (POS_MIDDLE_MS - travelMs) : 0;
    unsigned long posRightMs = POS_MIDDLE_MS + travelMs;

    // --- STEERING LOGIC ---
    if (left) {
      setActuatorTarget(posLeftMs);
    } else if (right) {
      setActuatorTarget(posRightMs);
    } else {
      // If neither is pressed, keep wheels exactly at their current angle
      setActuatorTarget(currentActuatorPosMs);
    }

    // --- DRIVE LOGIC ---
    if (up) {
      targetDrivePulse = speed60Fwd;
    } else if (down) {
      targetDrivePulse = speed60Rev;
    }
  }
}
