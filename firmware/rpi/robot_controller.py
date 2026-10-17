"""Map Dabble gamepad states to safe actuator commands."""

from __future__ import annotations

import logging
import time
from typing import Callable, Protocol

from .components.bluetooth import (
    BluetoothConnectionError,
    ButtonState,
    InvalidDabblePacket,
)
from .components.esp32_interface import Esp32CommunicationError


LOGGER = logging.getLogger(__name__)
HEARTBEAT_INTERVAL_S = 0.2
INPUT_TIMEOUT_S = 0.5
BLUETOOTH_POLL_TIMEOUT_S = 0.1


class BluetoothInput(Protocol):
    def read_buttons(self, timeout: float = BLUETOOTH_POLL_TIMEOUT_S) -> ButtonState | None: ...


class ActuatorOutput(Protocol):
    def send_command(self, command: str) -> None: ...


def command_for_buttons(buttons: ButtonState) -> str:
    if buttons.left == buttons.right:
        return "ACTUATOR STOP"
    return "ACTUATOR LEFT" if buttons.left else "ACTUATOR RIGHT"


class RobotController:
    def __init__(
        self,
        bluetooth: BluetoothInput,
        esp32: ActuatorOutput,
        *,
        heartbeat_interval: float = HEARTBEAT_INTERVAL_S,
        input_timeout: float = INPUT_TIMEOUT_S,
        clock=time.monotonic,
    ) -> None:
        if heartbeat_interval <= 0 or input_timeout <= 0:
            raise ValueError("heartbeat_interval and input_timeout must be positive")
        self.bluetooth = bluetooth
        self.esp32 = esp32
        self.heartbeat_interval = heartbeat_interval
        self.input_timeout = input_timeout
        self._clock = clock
        self._command = "ACTUATOR STOP"
        self._last_command_at: float | None = None
        self._last_input_at: float | None = None

    def _send_if_needed(self, command: str, now: float, force: bool = False) -> None:
        heartbeat_due = (
            self._last_command_at is None
            or now - self._last_command_at >= self.heartbeat_interval
        )
        if force or command != self._command or heartbeat_due:
            self.esp32.send_command(command)
            self._command = command
            self._last_command_at = now

    def start(self) -> None:
        self._send_if_needed("ACTUATOR STOP", self._clock(), force=True)

    def step(self) -> None:
        now = self._clock()
        try:
            buttons = self.bluetooth.read_buttons(BLUETOOTH_POLL_TIMEOUT_S)
        except InvalidDabblePacket:
            LOGGER.warning("Invalid Dabble packet; stopping actuator")
            self._last_input_at = None
            self._send_if_needed("ACTUATOR STOP", self._clock())
            return
        except BluetoothConnectionError:
            self._send_if_needed("ACTUATOR STOP", self._clock(), force=True)
            raise

        now = self._clock()
        if buttons is not None:
            self._last_input_at = now
            command = command_for_buttons(buttons)
        elif (
            self._last_input_at is None
            or now - self._last_input_at >= self.input_timeout
        ):
            command = "ACTUATOR STOP"
        else:
            command = self._command

        self._send_if_needed(command, now)

    def run(self, should_stop: Callable[[], bool]) -> None:
        self.start()
        while not should_stop():
            self.step()

    def stop(self) -> None:
        self._send_if_needed("ACTUATOR STOP", self._clock(), force=True)
