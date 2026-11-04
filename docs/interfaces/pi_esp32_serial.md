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

## 4. Mock Actuator Commands

The terminal interface on the Raspberry Pi accepts `LEFT`, `RIGHT`, and
`STOP` and sends the corresponding command to the ESP32. These commands are
mock-only: the current ESP32 communication handler does not call actuator or
motor hardware functions.

| Pi terminal input | Pi-to-ESP32 command | ESP32 response |
|---|---|---|
| `LEFT` | `ACTUATOR,LEFT` | `MOCK_ACTUATOR,MOVING_LEFT` |
| `RIGHT` | `ACTUATOR,RIGHT` | `MOCK_ACTUATOR,MOVING_RIGHT` |
| `STOP` | `ACTUATOR,STOP` | `MOCK_ACTUATOR,STOPPED` |

The Raspberry Pi displays the matching ESP32 response in the terminal.
Commands are case-insensitive at the terminal. Use `QUIT` or `EXIT` to leave
the terminal control loop.

Each actuator request uses the following newline-terminated format:

```text
ACTUATOR,LEFT
ACTUATOR,RIGHT
ACTUATOR,STOP
```

Each message ends with a newline character (`\n`).

## 5. Protocol Compatibility

The Raspberry Pi and ESP32 must use matching command names and message formats.

Any changes to the protocol must be reflected in both codebases.

## 6. Implementation References

- Raspberry Pi entry point: `firmware/rpi/main.py`
- Raspberry Pi serial transport: `firmware/rpi/components/esp32_interface.py`
- ESP32 serial receiver and ping responder: `firmware/esp32/communication.cpp`
- ESP32 entry point: `firmware/esp32/esp32.ino`
