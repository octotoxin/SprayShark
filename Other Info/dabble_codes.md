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
