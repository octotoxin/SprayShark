# Raspberry Pi – ESP32 Communication Protocol

## 1. Purpose

This document defines the communication protocol between the Raspberry Pi and ESP32 in SprayShark.

Both devices communicate over serial. The Raspberry Pi sends commands to the ESP32, and the ESP32 can send responses or telemetry back to the Raspberry Pi.

Both firmware codebases must follow the same command names and message formats.

## 2. Communication Configuration

| Setting | Value |
|---|---|
| Communication method | Serial |
| Baud rate | 115200 |
| Data format | Text-based, comma-separated messages |
| Message terminator | Newline (`\n`) |
| Communication direction | Bidirectional |

## 3. Actuator Commands

The following commands control steering actuator movement.

| Command | Description |
|---|---|
| `LEFT` | Move the steering actuator left |
| `RIGHT` | Move the steering actuator right |
| `STOP` | Stop actuator movement |

## 4. Message Format

Actuator commands use the following format:

```text
ACTUATOR,LEFT
ACTUATOR,RIGHT
ACTUATOR,STOP
```

Each message ends with a newline character (`\n`).

## 5. Protocol Compatibility

The Raspberry Pi and ESP32 must use matching command names and message formats.

Any changes to the protocol must be reflected in both codebases.