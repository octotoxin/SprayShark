/**
 * rpi_motor_test_esp32.ino
 *
 * Transparent byte-pipe serial relay for an ESP32-S3-DevKitC-1.
 *
 * Replaces the Arduino UNO (SoftwareSerial) relay with dual hardware UARTs
 * for lower latency and higher baud-rate headroom.
 *
 * Wiring
 * ──────
 *   USB (Serial)  ──►  Raspberry Pi
 *     • Connect ESP32 USB port → Pi USB port
 *     • ESP32 appears as /dev/ttyACM0 (or /dev/ttyUSB0) on the Pi
 *     • Baud: 115200
 *
 *   Hardware Serial2  (GPIO 16 RX / GPIO 17 TX)  ──►  RoboClaw 2x30A
 *     • GPIO 16 (RX2)  ←  RoboClaw S2 (TX)
 *     • GPIO 17 (TX2)  →  RoboClaw S1 (RX)
 *     • Baud: 38400
 *
 * NOTE: Cross your TX/RX wires between the ESP32 and RoboClaw.
 * NOTE: Disconnect from the Pi's serial port before uploading new firmware,
 *       or use a second USB port / SSH to trigger the upload.
 *
 * Board / IDE setup
 * ─────────────────
 *   • Board:  "ESP32S3 Dev Module"  (or your specific variant)
 *   • USB CDC On Boot: Enabled   (so Serial maps to the USB-CDC port)
 *   • Upload via USB or UART0 per your board's boot-mode jumpers
 */

#include <Arduino.h>
#include <HardwareSerial.h>

// ── Pin assignments for RoboClaw UART link ──────────────────────────────────
static const int PIN_ROBOCLAW_RX = 16;  // ESP32 receives from RoboClaw
static const int PIN_ROBOCLAW_TX = 17;  // ESP32 transmits to  RoboClaw

// ── Baud rates ───────────────────────────────────────────────────────────────
static const uint32_t BAUD_RPI      = 115200;  // USB Serial       ↔ Raspberry Pi
static const uint32_t BAUD_ROBOCLAW =  38400;  // Hardware UART1   ↔ RoboClaw

// ── Hardware Serial instance ─────────────────────────────────────────────────
// ESP32-S3 does not declare 'Serial2' by default. We instantiate UART1 directly.
HardwareSerial roboclawSerial(1);

// ── Relay buffer ─────────────────────────────────────────────────────────────
static const size_t BUF_SIZE = 256;
static uint8_t buf[BUF_SIZE];

// ──────────────────────────────────────────────────────────────────────────────
void setup() {
  // Test LED on Pin 17: blink 3 times on boot to prove Pin 17 is working
  pinMode(PIN_ROBOCLAW_TX, OUTPUT);
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_ROBOCLAW_TX, HIGH);
    delay(100);
    digitalWrite(PIN_ROBOCLAW_TX, LOW);
    delay(100);
  }

  // Initialize both USB-CDC (Left Port) and CH343 Hardware UART0 (Right Port)
  Serial.begin(BAUD_RPI);
  Serial0.begin(BAUD_RPI);

  // Hardware UART1 → RoboClaw
  roboclawSerial.begin(BAUD_ROBOCLAW, SERIAL_8N1, PIN_ROBOCLAW_RX, PIN_ROBOCLAW_TX);
}

// ──────────────────────────────────────────────────────────────────────────────
void loop() {
  // RPi → RoboClaw: forward from Native USB (Serial)
  while (Serial.available() > 0) {
    roboclawSerial.write(Serial.read());
  }

  // RPi → RoboClaw: forward from CH343 UART (Serial0)
  while (Serial0.available() > 0) {
    roboclawSerial.write(Serial0.read());
  }

  // RoboClaw → RPi: forward reply to BOTH USB streams
  while (roboclawSerial.available() > 0) {
    uint8_t b = roboclawSerial.read();
    Serial.write(b);
    Serial0.write(b);
  }
}
