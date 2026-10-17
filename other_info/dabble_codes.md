# Dabble Gamepad Button Codes (HEX)

## Action Buttons (Byte 5)
| Button | Hex Code | Bitmask |
| :--- | :--- | :--- |
| **Start** | `0x01` | `0000 0001` |
| **Select** | `0x02` | `0000 0010` |
| **Triangle** | `0x04` | `0000 0100` |
| **Circle** | `0x08` | `0000 1000` |
| **Cross (X)** | `0x10` | `0001 0000` |
| **Square** | `0x20` | `0010 0000` |

## Directional Buttons (Byte 6)
| Button | Hex Code | Bitmask |
| :--- | :--- | :--- |
| **Up** | `0x01` | `0000 0001` |
| **Down** | `0x02` | `0000 0010` |
| **Left** | `0x04` | `0000 0100` |
| **Right** | `0x08` | `0000 1000` |

## Packet Structure
`0xFF 0x01 0x01 0x01 0x02 [ACTION_BTNS] [DPAD_BTNS] ...`

For the documented Gamepad packet, the five-byte prefix is
`FF 01 01 01 02`; the action byte is at offset 5 and the directional-button
byte is at offset 6 (zero-based). Bluetooth reads are a byte stream: a read may
contain a partial packet or multiple packets, so use the prefix to synchronize
and wait until at least seven bytes are available before reading the buttons.
The bytes after the directional-button byte are not specified here.

The Raspberry Pi hosts an RFCOMM Serial Port Profile service on channel 1 for
the Dabble phone to connect to. Pair the phone with the Pi and make the Pi
discoverable before starting the application. The ESP32 does not run Dabble;
it receives only the newline-terminated `ACTUATOR LEFT`, `ACTUATOR RIGHT`, and
`ACTUATOR STOP` commands over USB serial.
