"""USB serial interface for the actuator command protocol."""

from __future__ import annotations

from typing import Any


BAUD_RATE = 115200
SUPPORTED_COMMANDS = frozenset(
    {"ACTUATOR_LEFT", "ACTUATOR_RIGHT", "ACTUATOR_STOP"}
)


class Esp32CommunicationError(ConnectionError):
    """Raised when an actuator command cannot be sent to the ESP32."""


class Esp32Interface:
    def __init__(
        self,
        port: str,
        baud_rate: int = BAUD_RATE,
        serial_factory: Any | None = None,
    ) -> None:
        if serial_factory is None:
            try:
                import serial
            except ImportError as exc:
                raise Esp32CommunicationError(
                    "pyserial is required; install the Raspberry Pi package "
                    "'python3-serial'"
                ) from exc
            serial_factory = serial.Serial

        try:
            self._connection = serial_factory(
                port=port, baudrate=baud_rate, timeout=0.1, write_timeout=1.0
            )
        except OSError as exc:
            raise Esp32CommunicationError(
                f"Could not open ESP32 serial port {port}: {exc}"
            ) from exc

    def send_command(self, command: str) -> None:
        if command not in SUPPORTED_COMMANDS:
            raise ValueError(f"Unsupported actuator command: {command!r}")
        try:
            self._connection.write((command + "\n").encode("ascii"))
            self._connection.flush()
        except OSError as exc:
            raise Esp32CommunicationError(
                f"Could not send command to ESP32: {exc}"
            ) from exc

    def close(self) -> None:
        try:
            self._connection.close()
        except OSError as exc:
            raise Esp32CommunicationError(
                f"Could not close ESP32 serial connection: {exc}"
            ) from exc