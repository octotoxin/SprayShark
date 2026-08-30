#include <Arduino.h>
#include <HardwareSerial.h>

// ── Pin assignments for RoboClaw UART link ──────────────────────────────────
static const int PIN_ROBOCLAW_RX = 16;  // ESP32 receives from RoboClaw (S2)
static const int PIN_ROBOCLAW_TX = 17;  // ESP32 transmits to  RoboClaw (S1)

// Optional onboard LED pin for activity blinking (GPIO 2 / 21 depending on board)
#ifdef LED_BUILTIN
  static const int PIN_STATUS_LED = LED_BUILTIN;
#else
  static const int PIN_STATUS_LED = 2;
#endif

// ── Baud rates ───────────────────────────────────────────────────────────────
static const uint32_t BAUD_RPI      = 115200;  // USB Serial       ↔ Raspberry Pi
static const uint32_t BAUD_ROBOCLAW =  38400;  // Hardware UART1   ↔ RoboClaw

// ── Hardware Serial instance ─────────────────────────────────────────────────
HardwareSerial roboclawSerial(1);

// ── Diagnostic Byte Counters ─────────────────────────────────────────────────
static unsigned long bytes_from_pi       = 0;
static unsigned long bytes_from_roboclaw = 0;

// ──────────────────────────────────────────────────────────────────────────────
void setup() {
  // 1. Hardware Startup Diagnostic: Blink Pin 17 LED 3 times
  // This visually proves to the user that Pin 17 and breakout board LEDs work.
  pinMode(PIN_ROBOCLAW_TX, OUTPUT);
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_ROBOCLAW_TX, HIGH);
    delay(100);
    digitalWrite(PIN_ROBOCLAW_TX, LOW);
    delay(100);
  }

  // 2. Initialize USB-CDC (Left Port) and CH343 Hardware UART0 (Right Port)
  Serial.begin(BAUD_RPI);
  Serial0.begin(BAUD_RPI);

  // 3. Hardware UART1 → RoboClaw
  roboclawSerial.begin(BAUD_ROBOCLAW, SERIAL_8N1, PIN_ROBOCLAW_RX, PIN_ROBOCLAW_TX);
}

// ──────────────────────────────────────────────────────────────────────────────
void loop() {
  // RPi → RoboClaw: forward from Native USB (Serial)
  while (Serial.available() > 0) {
    uint8_t c = Serial.read();
    roboclawSerial.write(c);
    bytes_from_pi++;
  }

  // RPi → RoboClaw: forward from CH343 UART (Serial0)
  while (Serial0.available() > 0) {
    uint8_t c = Serial0.read();
    roboclawSerial.write(c);
    bytes_from_pi++;
  }

  // RoboClaw → RPi: forward reply to BOTH USB streams
  while (roboclawSerial.available() > 0) {
    uint8_t b = roboclawSerial.read();
    Serial.write(b);
    Serial0.write(b);
    bytes_from_roboclaw++;
  }
}
