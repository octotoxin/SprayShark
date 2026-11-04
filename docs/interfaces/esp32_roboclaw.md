# ESP32 – RoboClaw Communication

## 1. Purpose

This document defines how the ESP32-S3 communicates with the RoboClaw motor controller in SprayShark.

The ESP32 uses the official Basicmicro Arduino library to control the RoboClaw through Packet Serial communication.

## 2. Library

- **Library:** Basicmicro Arduino Library
- **Header:** `Basicmicro.h`
- **Repository:** https://github.com/basicmicro/basicmicro_arduino
- **Installation:** `arduino-cli lib install "Basicmicro"`

## 3. Communication Configuration

| Setting | Value |
|---|---|
| Communication method | Hardware UART |
| Protocol | RoboClaw Packet Serial |
| Library | Basicmicro |
| Controller address | `0x80` (example; verify in Motion Studio) |
| Baud rate | `38400` (example; verify in Motion Studio) |
| Serial format | 8N1 |
| ESP32 UART | `Serial2` (proposed) |
| RX/TX GPIO | To be confirmed from hardware wiring |

The ESP32 UART configuration must match the RoboClaw settings configured in Motion Studio.

## 4. Motor Control

The Basicmicro library provides functions for controlling the RoboClaw's two motor channels.

| Function | Purpose |
|---|---|
| `DutyM1()` | Control motor channel 1 |
| `DutyM2()` | Control motor channel 2 |
| `DutyM1M2()` | Control both motor channels |
| `SpeedM1()` | Encoder-based speed control for motor 1 |
| `SpeedM2()` | Encoder-based speed control for motor 2 |

Duty-cycle commands use signed values from approximately -32767 to +32767, with zero representing stopped output.

## 5. Encoder Feedback

The Basicmicro library supports reading encoder values and motor speeds from the RoboClaw.

Encoder feedback may be used for monitoring movement and implementing closed-loop motor control.

## 6. Implementation Requirements

- Use the Basicmicro Arduino library for RoboClaw communication.
- Use a dedicated hardware UART on the ESP32-S3.
- Keep RoboClaw communication logic within the appropriate ESP32 motor-control component.
- Use the actual configured controller address, baud rate, and GPIO assignments.
- Verify compilation using the Arduino CLI instructions in `docs/quality.md`.