/**
 * roboclaw_relay.ino
 *
 * Transparent byte-pipe serial relay for a Lafvin Uno (ATmega328P).
 *
 * Wiring
 * ──────
 *   USB (Serial)  ──►  Raspberry Pi
 *     • Connect Arduino USB port → Pi USB port
 *     • Arduino appears as /dev/ttyACM0 (or /dev/ttyUSB0) on the Pi
 *     • Baud: 9600
 *
 *   SoftwareSerial   (pins 10 / 11) ──►  RoboClaw 2x30A
 *     • Pin 10 (RX)  ←  RoboClaw S2 (TX)
 *     • Pin 11 (TX)  →  RoboClaw S1 (RX)
 *     • Baud: 38400
 *
 * NOTE: Cross your TX/RX wires between the Arduino and RoboClaw.
 * NOTE: Disconnect from the Pi's serial port before uploading new firmware,
 *       or use a second USB port / SSH to trigger the upload.
 */

#include <SoftwareSerial.h>

// ── Pin assignments ────────────────────────────────────────────────────────────
static const uint8_t PIN_ROBOCLAW_RX = 10;  // Arduino receives from RoboClaw
static const uint8_t PIN_ROBOCLAW_TX = 11;  // Arduino transmits to  RoboClaw

// ── Baud rates ─────────────────────────────────────────────────────────────────
static const uint32_t BAUD_RPI      =  9600;  // Hardware Serial  ↔ Raspberry Pi
static const uint32_t BAUD_ROBOCLAW = 38400;  // SoftwareSerial   ↔ RoboClaw

// ── SoftwareSerial instance (RoboClaw link) ────────────────────────────────────
SoftwareSerial roboclawSerial(PIN_ROBOCLAW_RX, PIN_ROBOCLAW_TX);

// ──────────────────────────────────────────────────────────────────────────────
void setup() {
  // Hardware Serial → Raspberry Pi
  Serial.begin(BAUD_RPI);

  // SoftwareSerial → RoboClaw
  roboclawSerial.begin(BAUD_ROBOCLAW);
}

// ──────────────────────────────────────────────────────────────────────────────
void loop() {
  // RPi → RoboClaw: forward any byte arriving from the Raspberry Pi
  if (Serial.available()) {
    roboclawSerial.write(Serial.read());
  }

  // RoboClaw → RPi: forward any byte arriving from the RoboClaw
  if (roboclawSerial.available()) {
    Serial.write(roboclawSerial.read());
  }
}
