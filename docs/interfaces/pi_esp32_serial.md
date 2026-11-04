# Raspberry Pi – ESP32 Communication Protocol

## 1. Purpose

This document defines the communication protocol between the Raspberry Pi and ESP32 in SprayShark.

Both devices communicate over serial. The Raspberry Pi sends commands to the ESP32, and the ESP32 can send responses or telemetry back to the Raspberry Pi.

Both firmware codebases must follow the same command names and message formats.

## 2. Communication Configuration

| Setting | Value |
|---|---|
| Communication method | Serial |
| Raspberry Pi device | `/dev/ttyACM0` by default |
| Baud rate | 115200 |
| Data format | UTF-8/ASCII text commands and responses |
| Message terminator | Newline (`\n`) |
| Communication direction | Bidirectional |

The Raspberry Pi application may select another serial device with `--port`
or the `SPRAYSHARK_SERIAL_PORT` environment variable. The command-line option
takes precedence over the environment variable.

## 3. Startup Ping

At startup, the Raspberry Pi sends `PING` and expects `PONG` from the ESP32.
The response must arrive within one second. A timeout or any other response
causes Pi initialization to fail.

```text
Pi -> ESP32: PING\n
ESP32 -> Pi: PONG\n
```

The ESP32 ignores unsupported commands. Oversized serial lines are discarded
until the next newline and do not produce a response.

## 4. Actuator Commands

The following commands control steering actuator movement.

| Command | Description |
|---|---|
| `LEFT` | Move the steering actuator left |
| `RIGHT` | Move the steering actuator right |
| `STOP` | Stop actuator movement |

## 5. Message Format

Actuator commands use the following format:

```text
ACTUATOR,LEFT
ACTUATOR,RIGHT
ACTUATOR,STOP
```

Each message ends with a newline character (`\n`).

## 6. Protocol Compatibility

The Raspberry Pi and ESP32 must use matching command names and message formats.

Any changes to the protocol must be reflected in both codebases.

## 7. Implementation References

- Raspberry Pi entry point: `firmware/rpi/main.py`
- Raspberry Pi serial transport: `firmware/rpi/components/esp32_interface.py`
- ESP32 serial receiver and ping responder: `firmware/esp32/communication.cpp`
- ESP32 entry point: `firmware/esp32/esp32.ino`
