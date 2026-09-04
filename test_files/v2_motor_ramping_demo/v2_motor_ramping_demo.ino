#include <Servo.h>

Servo esc;
const int escPin = 9;

// ESC PULSE LIMITS (Bidirectional Maxynos 45A)
const int minPulse = 1000;      // Full Reverse 
const int neutralPulse = 1500;  // Zero/Neutral point - Full Stop with Brake
const int maxPulse = 2000;      // Full Forward

// Ramp Configuration
const int step = 10; 
const int rampDelayMs = 5; // Reduced delay for a quicker ramp

void setup() {
  Serial.begin(115200);
  while (!Serial); 

  Serial.println("--- BLDC SYSTEM STARTING ---");
  Serial.println("ESC is Bidirectional: 1500us is STOP, 2000us is MAX FORWARD, 1000us is MAX REVERSE.");
  
  // Attach ESC
  Serial.print("Attaching ESC to pin ");
  Serial.println(escPin);
  esc.attach(escPin, minPulse, maxPulse);
  
  // Arming Sequence - Send 1500us (Neutral/Stop)
  Serial.println("Sending NEUTRAL THROTTLE (1500us) to Arm...");
  esc.writeMicroseconds(neutralPulse);
  
  // Wait for ESC to arm (usually beeps)
  for(int i = 5; i > 0; i--) {
    Serial.print("Arming in ");
    Serial.println(i);
    delay(1000);
  }
  
  Serial.println("--- SYSTEM ARMED ---");
  delay(1000);
}

void loop() {
  
  // --- 1. FORWARD RAMP UP (1500us to 2000us) ---
  Serial.println("\n--- BEGIN CYCLE: FORWARD ---");
  Serial.println("Beginning Ramp UP (Forward Acceleration)");
  for (int pulse = neutralPulse; pulse <= maxPulse; pulse += step) {
    esc.writeMicroseconds(pulse);
    
    // Status Report: Map the forward range (1500-2000) to 0-100% throttle.
    Serial.print("Status: Accelerating | Throttle: ");
    Serial.print(map(pulse, neutralPulse, maxPulse, 0, 100));
    Serial.print("% Forward | Pulse: ");
    Serial.println(pulse);
    
    delay(rampDelayMs); 
  }

  Serial.println("!!! FULL FORWARD SPEED REACHED !!!");
  delay(2000); // Hold full speed

  // --- 2. FORWARD RAMP DOWN (2000us to 1500us) ---
  Serial.println("Beginning Ramp DOWN (Deceleration)");
  for (int pulse = maxPulse; pulse >= neutralPulse; pulse -= step) {
    esc.writeMicroseconds(pulse);
    
    // Status Report: Map the forward range (1500-2000) to 100-0% throttle.
    Serial.print("Status: Decelerating | Throttle: ");
    Serial.print(map(pulse, neutralPulse, maxPulse, 0, 100));
    Serial.print("% Forward | Pulse: ");
    Serial.println(pulse);
    
    delay(rampDelayMs); 
  }

  Serial.println("--- MOTOR STOPPED (Neutral 1500us) ---");
  // CRITICAL CHANGE: Increased delay to satisfy ESC's reverse lockout/deadband safety feature.
  // The ESC needs the motor to stop spinning before accepting a reverse signal.
  delay(3000); 

  // --- 3. REVERSE RAMP UP (1500us to 1000us) ---
  Serial.println("\n--- BEGIN CYCLE: REVERSE ---");
  Serial.println("Beginning Ramp UP (Reverse Acceleration)");
  for (int pulse = neutralPulse; pulse >= minPulse; pulse -= step) {
    esc.writeMicroseconds(pulse);
    
    // Status Report: Map the reverse range (1500-1000) to 0-100% throttle.
    Serial.print("Status: Accelerating | Throttle: ");
    Serial.print(map(pulse, minPulse, neutralPulse, 100, 0)); // Note: Mapping is reversed for reporting
    Serial.print("% Reverse | Pulse: ");
    Serial.println(pulse);
    
    delay(rampDelayMs); 
  }

  Serial.println("!!! FULL REVERSE SPEED REACHED !!!");
  delay(2000); // Hold full speed

  // --- 4. REVERSE RAMP DOWN (1000us to 1500us) ---
  Serial.println("Beginning Ramp DOWN (Deceleration)");
  for (int pulse = minPulse; pulse <= neutralPulse; pulse += step) {
    esc.writeMicroseconds(pulse);
    
    // Status Report: Map the reverse range (1500-1000) to 100-0% throttle.
    Serial.print("Status: Decelerating | Throttle: ");
    Serial.print(map(pulse, minPulse, neutralPulse, 100, 0)); // Note: Mapping is reversed for reporting
    Serial.print("% Reverse | Pulse: ");
    Serial.println(pulse);
    
    delay(rampDelayMs); 
  }

  Serial.println("--- MOTOR STOPPED (Neutral 1500us) ---");
  esc.writeMicroseconds(neutralPulse); // Ensure motor is stopped at neutral
  delay(3000); // Wait before restarting entire cycle
}