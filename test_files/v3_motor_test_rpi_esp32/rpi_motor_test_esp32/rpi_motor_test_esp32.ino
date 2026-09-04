#include <Arduino.h>
#include <HardwareSerial.h>

// Target board: Freenove ESP32-S3 WROOM (dual USB-port development board).

// ── Pin assignments for RoboClaw UART link ──────────────────────────────────
// RoboClaw Multi-Unit Mode must remain OFF so S2 provides a normal UART TX
// signal for replies and command acknowledgements back to GPIO 16.
static const int PIN_ROBOCLAW_RX = 16;  // ESP32 receives from RoboClaw (S2)
static const int PIN_ROBOCLAW_TX = 17;  // ESP32 transmits to  RoboClaw (S1)

// Status LED: flashes only when a byte arrives from the RoboClaw on S2.
// This is intentionally separate from the USB serial streams so it cannot
// corrupt the packet-serial replies forwarded to the Raspberry Pi.
#ifdef LED_BUILTIN
  static const int PIN_STATUS_LED = LED_BUILTIN;
#else
  static const int PIN_STATUS_LED = 2;
#endif

static const uint32_t STATUS_LED_PULSE_MS = 50;
static unsigned long status_led_off_at = 0;

// ── Baud rates ───────────────────────────────────────────────────────────────
static const uint32_t BAUD_RPI      = 115200;  // USB Serial       ↔ Raspberry Pi
static const uint32_t BAUD_ROBOCLAW =  38400;  // Hardware UART1   ↔ RoboClaw

// Set true only while diagnosing RoboClaw read responses. With tracing enabled,
// connect a second USB cable to the LEFT/native USB port and open its serial
// monitor at 115200 baud. Replies remain binary-clean on Serial0 (the RIGHT
// CH343 port connected to the Raspberry Pi) and are printed as hex on Serial.
static const bool ENABLE_ROBOCLAW_RX_TRACE = false;

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

  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, LOW);

  // 3. Hardware UART1 → RoboClaw
  roboclawSerial.begin(BAUD_ROBOCLAW, SERIAL_8N1, PIN_ROBOCLAW_RX, PIN_ROBOCLAW_TX);
}

// ──────────────────────────────────────────────────────────────────────────────
void loop() {
  // End a previously scheduled reply-activity pulse without blocking UART work.
  if (status_led_off_at != 0 && millis() >= status_led_off_at) {
    digitalWrite(PIN_STATUS_LED, LOW);
    status_led_off_at = 0;
  }

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
  bool received_roboclaw_reply = false;
  uint8_t reply_bytes[64];
  size_t reply_length = 0;
  while (roboclawSerial.available() > 0) {
    uint8_t b = roboclawSerial.read();
    Serial0.write(b);
    if (ENABLE_ROBOCLAW_RX_TRACE) {
      if (reply_length < sizeof(reply_bytes)) {
        reply_bytes[reply_length++] = b;
      }
    } else {
      Serial.write(b);
    }
    bytes_from_roboclaw++;
    received_roboclaw_reply = true;
  }

  if (ENABLE_ROBOCLAW_RX_TRACE && reply_length > 0) {
    Serial.print("[RoboClaw RX]");
    for (size_t i = 0; i < reply_length; ++i) {
      Serial.print(" 0x");
      if (reply_bytes[i] < 0x10) Serial.print('0');
      Serial.print(reply_bytes[i], HEX);
    }
    Serial.println();
  }

  // If this LED never flashes during a Pi-side read request, GPIO 16 has not
  // received a reply from RoboClaw S2. If it flashes but Python still times
  // out, the issue is downstream in the ESP32-to-Pi USB serial path.
  if (received_roboclaw_reply) {
    digitalWrite(PIN_STATUS_LED, HIGH);
    status_led_off_at = millis() + STATUS_LED_PULSE_MS;
  }
}
