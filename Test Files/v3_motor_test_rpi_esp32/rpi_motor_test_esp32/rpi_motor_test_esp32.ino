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
  // USB Serial → Raspberry Pi
  Serial.begin(BAUD_RPI);

  // Hardware UART1 → RoboClaw (assigning custom RX/TX GPIOs via ESP32 Matrix)
  roboclawSerial.begin(BAUD_ROBOCLAW, SERIAL_8N1, PIN_ROBOCLAW_RX, PIN_ROBOCLAW_TX);
}

// ──────────────────────────────────────────────────────────────────────────────
void loop() {
  // RPi → RoboClaw: forward any bytes arriving from the Raspberry Pi
  int avail = Serial.available();
  if (avail > 0) {
    size_t toRead = (avail < (int)BUF_SIZE) ? (size_t)avail : BUF_SIZE;
    size_t n = Serial.readBytes((char*)buf, toRead);
    roboclawSerial.write(buf, n);
  }

  // RoboClaw → RPi: forward any bytes arriving from the RoboClaw
  avail = roboclawSerial.available();
  if (avail > 0) {
    size_t toRead = (avail < (int)BUF_SIZE) ? (size_t)avail : BUF_SIZE;
    size_t n = roboclawSerial.readBytes((char*)buf, toRead);
    Serial.write(buf, n);
  }
}
