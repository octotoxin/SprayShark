# SprayShark v3 — ESP32 & RoboClaw Pinout and Wiring Guide

This guide documents the hardware wiring, pin assignments, communication chain, and configuration settings for the **Raspberry Pi 5 ↔ ESP32-S3 ↔ BasicMicro RoboClaw 2x30A** motor test setup.

---

## 1. High-Level Architecture

```
┌─────────────────┐       USB Cable       ┌──────────────────┐      3-Wire UART      ┌─────────────────┐
│                 │ ────────────────────► │  Freenove ESP32  │ ────────────────────► │ RoboClaw 2x30A  │
│ Raspberry Pi 5  │  (Data + 5V Power)    │  S3 Dev Board    │  (TX / RX / GND)      │  Control Header │
│                 │ ◄──────────────────── │                  │ ◄──────────────────── │                 │
└─────────────────┘                       └──────────────────┘                       └─────────────────┘
```

* **Pi 5 ↔ ESP32:** USB Serial link (`/dev/ttyACM0` or `/dev/ttyUSB0`) running at **115,200 baud**.
* **ESP32 ↔ RoboClaw:** Hardware UART1 (`HardwareSerial roboclawSerial(1)`) running at **38,400 baud**.

---

## 2. Pinout & Connection Mapping

### A. ESP32-S3 ↔ RoboClaw 2x30A Header

| ESP32-S3 Pin | RoboClaw Pin | Signal Direction | Description |
| :--- | :--- | :--- | :--- |
| **GPIO 17** (TX1) | **S1** (RX) | ESP32 $\rightarrow$ RoboClaw | Packet serial command stream |
| **GPIO 16** (RX1) | **S2** (TX) | RoboClaw $\rightarrow$ ESP32 | Telemetry, status register, ACKs |
| **GND** | **GND (`-` pin)** | Shared Reference | **Mandatory** common ground |
| *DO NOT CONNECT* | **5V (`+` pin)** | N/A | Leave disconnected (ESP32 is powered by USB) |

> [!IMPORTANT]
> **Crossed TX/RX Wiring:**
> * ESP32 **TX** (GPIO 17) connects to RoboClaw **S1** (RoboClaw's RX).
> * ESP32 **RX** (GPIO 16) connects to RoboClaw **S2** (RoboClaw's TX).

---

### B. Raspberry Pi 5 ↔ ESP32-S3 (USB Connection)

* Connect a standard **data-capable USB-C cable** from a Raspberry Pi 5 USB port to the **Right USB port (UART)** on the Freenove ESP32-S3 board.

#### Freenove ESP32-S3 Dual Port Identification:
* **Right Port (UART):** Connected to the onboard **WCH CH343** USB-to-UART bridge (`1a86:55d3`). **Use this port.**
* **Left Port (USB):** Connected directly to the ESP32-S3 internal USB PHY (GPIO 19/20).

---

## 3. Electrical & Logic Level Details

1. **Common Ground:**
   * You **must** connect ESP32 `GND` to RoboClaw control header `GND` (`-`). Without a shared ground reference, UART signals will be corrupt.
2. **Logic Voltage (3.3V vs 5V):**
   * **ESP32 $\rightarrow$ RoboClaw (S1):** The ESP32 outputs 3.3V logic. RoboClaw TTL inputs recognize $\ge 2.0\text{V}$ as a valid HIGH, so direct connection works reliably.
   * **RoboClaw $\rightarrow$ ESP32 (S2):** RoboClaw outputs 5V TTL logic on S2. While short tests often tolerate 5V, using a simple resistor divider ($1\text{k}\Omega$ / $2\text{k}\Omega$) or a 3.3V/5V logic level shifter is recommended for protection.
3. **Power Isolation:**
   * **Do not** connect the middle `+` (5V BEC) pin of the RoboClaw control header to the ESP32 5V/VBUS pin when the ESP32 is plugged into the Pi's USB port.

---

## 4. RoboClaw DIP Switch Settings

Configure the DIP switches on the RoboClaw 2x30A for **Packet Serial Mode** at **38,400 baud** (Address `0x80` / `128`):

| Switch | Position | Function |
| :--- | :--- | :--- |
| **SW 1** | `ON` | Mode 7: Packet Serial |
| **SW 2** | `ON` | Mode 7: Packet Serial |
| **SW 3** | `ON` | Mode 7: Packet Serial |
| **SW 4** | `OFF` | 38,400 Baud |
| **SW 5** | `OFF` | 38,400 Baud |
| **SW 6** | `OFF` | Packet Serial Address: `0x80` (128) |
| **SW 7** | `OFF` | Packet Serial Address: `0x80` (128) |

---

## 5. Software & Execution Summary

1. **Flash ESP32 Firmware:**
   * Open `rpi_motor_test_esp32/rpi_motor_test_esp32.ino` in Arduino IDE.
   * Select Board: `ESP32S3 Dev Module`.
   * Upload using the **Right (UART) USB-C port**.

2. **Run Raspberry Pi Test:**
   * Plug the ESP32 into the Raspberry Pi 5.
   * Verify the device name: `ls /dev/ttyACM* /dev/ttyUSB*`
   * Run the test script:
     ```bash
     python3 rpi_motor_test_rpi.py
     ```
