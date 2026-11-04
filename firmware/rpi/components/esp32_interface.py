
"""
SprayShark - ESP32 Serial Interface
DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.

Purpose:
    Manage USB serial communication between the Raspberry Pi and
    the ESP32, providing a consistent interface for sending robot
    control commands.

Responsibilities:
    - Establish and manage the USB serial connection to the ESP32.
    - Configure the serial port and baud rate (115200 baud).
    - Send newline-terminated commands using the agreed serial protocol.
    - Support actuator commands such as ACTUATOR LEFT, ACTUATOR RIGHT,
      and ACTUATOR STOP.
    - Validate commands before transmission.
    - Detect and report connection failures and serial communication errors.
    - Provide connection status and resource cleanup functions.
    - Attempt to send a STOP command during shutdown when communication
      is still available.

Design:
    This module handles serial transport only. It does not decide robot
    behavior, decode Bluetooth packets, parse commands on the ESP32,
    or control actuator hardware.

Related modules:
    - main.py: Application entry point and lifecycle management.
    - robot_controller.py: Determines which robot commands to send.
    - components/bluetooth.py: Receives and decodes phone input.
    - ESP32 communication.cpp: Receives and parses serial commands.
    - ESP32 movement.cpp: Implements actuator hardware control.

Safety:
    Sending STOP during shutdown is best-effort only. The ESP32 must
    independently enforce a command timeout and stop the actuator
    if valid commands stop arriving.
"""

import serial


class ESP32Interface:
    """Manage the Raspberry Pi's USB serial connection to the ESP32."""

    BAUD_RATE = 115200
    SERIAL_TIMEOUT_SECONDS = 1.0

    def __init__(self, port: str) -> None:
        if not port:
            raise ValueError("A serial port must be specified for the ESP32.")

        self._serial = serial.Serial(
            port=port,
            baudrate=self.BAUD_RATE,
            timeout=self.SERIAL_TIMEOUT_SECONDS,
            write_timeout=self.SERIAL_TIMEOUT_SECONDS,
        )

    def ping(self) -> None:
        """Send PING and require the matching PONG response."""
        self._serial.reset_input_buffer()
        self._serial.write(b"PING\n")
        self._serial.flush()

        response = self._serial.readline().rstrip(b"\r\n")
        if not response:
            raise TimeoutError("Timed out waiting for the ESP32 PONG response.")
        if response != b"PONG":
            raise RuntimeError(
                f"Unexpected response to ESP32 PING: {response!r}"
            )

    def close(self) -> None:
        self._serial.close()

    def __enter__(self) -> "ESP32Interface":
        return self

    def __exit__(self, exc_type: object, exc_value: object, traceback: object) -> None:
        self.close()
