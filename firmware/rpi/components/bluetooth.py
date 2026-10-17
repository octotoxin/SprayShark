"""Bluetooth RFCOMM receiver and Dabble gamepad packet decoder."""

from __future__ import annotations

import time
from dataclasses import dataclass
from typing import Any, Callable


PACKET_PREFIX = b"\xff\x01\x01\x01\x02"
PACKET_MIN_LENGTH = len(PACKET_PREFIX) + 2
LEFT_MASK = 0x04
RIGHT_MASK = 0x08
DEFAULT_RFCOMM_CHANNEL = 1
DEFAULT_RECEIVE_TIMEOUT_S = 0.1


@dataclass(frozen=True)
class ButtonState:
    left: bool
    right: bool


class BluetoothConnectionError(ConnectionError):
    """Raised when the Bluetooth device disconnects or cannot be reached."""


class InvalidDabblePacket(ValueError):
    """Raised when received data cannot be decoded as a Dabble packet."""


def decode_dabble_packet(packet: bytes) -> ButtonState:
    """Decode the documented Dabble Gamepad header and directional byte."""
    if len(packet) < PACKET_MIN_LENGTH or not packet.startswith(PACKET_PREFIX):
        raise InvalidDabblePacket("Invalid Dabble Gamepad packet")

    directional_buttons = packet[6]
    return ButtonState(
        left=bool(directional_buttons & LEFT_MASK),
        right=bool(directional_buttons & RIGHT_MASK),
    )


class DabblePacketParser:
    """Extract Dabble packets from a byte stream that may split or combine packets."""

    def __init__(self) -> None:
        self._buffer = bytearray()

    def feed(self, data: bytes) -> list[ButtonState]:
        self._buffer.extend(data)
        states: list[ButtonState] = []

        while self._buffer:
            start = self._buffer.find(PACKET_PREFIX)
            if start < 0:
                keep = 0
                for length in range(1, min(len(self._buffer), len(PACKET_PREFIX) - 1) + 1):
                    if self._buffer[-length:] == PACKET_PREFIX[:length]:
                        keep = length
                if keep:
                    del self._buffer[:-keep]
                else:
                    self._buffer.clear()
                break

            if start:
                del self._buffer[:start]

            if len(self._buffer) < PACKET_MIN_LENGTH:
                break

            states.append(decode_dabble_packet(bytes(self._buffer[:PACKET_MIN_LENGTH])))
            del self._buffer[:PACKET_MIN_LENGTH]

        return states


class BluetoothReceiver:
    """Accept a Dabble phone's Bluetooth Classic RFCOMM connection."""

    def __init__(
        self,
        channel: int = DEFAULT_RFCOMM_CHANNEL,
        expected_address: str | None = None,
        socket_factory: Any | None = None,
        advertise_service: Any | None = None,
    ) -> None:
        self.channel = channel
        self.expected_address = (
            expected_address.upper() if expected_address is not None else None
        )
        self._socket_factory = socket_factory
        self._advertise_service = advertise_service
        self._server: Any | None = None
        self._socket: Any | None = None
        self._parser = DabblePacketParser()

    def connect(self, should_stop: Callable[[], bool] | None = None) -> None:
        if self._server is not None:
            return
        bluetooth_error: type[BaseException] = OSError
        try:
            if self._socket_factory is None:
                try:
                    import bluetooth
                except ImportError as exc:
                    raise BluetoothConnectionError(
                        "PyBluez is required; install the Raspberry Pi package "
                        "'python3-bluez'"
                    ) from exc
                bluetooth_error = bluetooth.BluetoothError
                advertiser = bluetooth.advertise_service
                server = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
            else:
                advertiser = self._advertise_service
                server = self._socket_factory()
            self._server = server
            server.bind(("", self.channel))
            server.listen(1)
            server.settimeout(1.0)
            if advertiser is None:
                raise BluetoothConnectionError(
                    "An RFCOMM service advertiser is required"
                )
            advertiser(
                server,
                "SprayShark Dabble",
                service_classes=["1101"],
                profiles=[("1101", 0x0100)],
            )
        except BluetoothConnectionError:
            raise
        except (OSError, bluetooth_error) as exc:
            raise BluetoothConnectionError(
                f"Could not start Bluetooth RFCOMM service on channel "
                f"{self.channel}: {exc}"
            ) from exc
        self._bluetooth_error = bluetooth_error
        try:
            while should_stop is None or not should_stop():
                try:
                    connection, peer = server.accept()
                except TimeoutError:
                    continue
                if (
                    self.expected_address is not None
                    and str(peer[0]).upper() != self.expected_address
                ):
                    connection.close()
                    continue
                self._socket = connection
                return
            return
        except (OSError, bluetooth_error) as exc:
            raise BluetoothConnectionError(
                f"Bluetooth RFCOMM accept failed: {exc}"
            ) from exc

    def read_buttons(
        self, timeout: float = DEFAULT_RECEIVE_TIMEOUT_S
    ) -> ButtonState | None:
        if self._socket is None:
            raise BluetoothConnectionError("Bluetooth receiver is not connected")
        if timeout < 0:
            raise ValueError("timeout must be non-negative")

        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return None
            try:
                self._socket.settimeout(remaining)
                data = self._socket.recv(256)
            except TimeoutError:
                return None
            except (OSError, getattr(self, "_bluetooth_error", OSError)) as exc:
                raise BluetoothConnectionError(
                    f"Bluetooth receive failed: {exc}"
                ) from exc

            if not data:
                raise BluetoothConnectionError("Bluetooth device disconnected")

            states = self._parser.feed(data)
            if states:
                return states[-1]

    def close(self) -> None:
        errors: list[BaseException] = []
        if self._socket is not None:
            connection, self._socket = self._socket, None
            try:
                connection.close()
            except (OSError, getattr(self, "_bluetooth_error", OSError)) as exc:
                errors.append(exc)
        if self._server is not None:
            server, self._server = self._server, None
            try:
                server.close()
            except (OSError, getattr(self, "_bluetooth_error", OSError)) as exc:
                errors.append(exc)
        if errors:
            raise BluetoothConnectionError(
                "Could not close Bluetooth connection: "
                + "; ".join(str(error) for error in errors)
            ) from errors[0]