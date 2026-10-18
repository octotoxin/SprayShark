"""Read USB keyboard events and translate them into actuator commands."""

from __future__ import annotations

import logging
import select
from dataclasses import dataclass
from typing import Callable, Protocol

LOGGER = logging.getLogger(__name__)
KEYBOARD_POLL_INTERVAL_S = 0.1
ACTUATOR_STOP = "ACTUATOR_STOP"
ACTUATOR_LEFT = "ACTUATOR_LEFT"
ACTUATOR_RIGHT = "ACTUATOR_RIGHT"


class KeyboardInputError(ConnectionError):
    """Raised when a usable evdev keyboard cannot be read."""


@dataclass(frozen=True)
class KeyEvent:
    key: str
    pressed: bool


class KeyboardInput(Protocol):
    def read_event(
        self, timeout: float = KEYBOARD_POLL_INTERVAL_S
    ) -> KeyEvent | None: ...


class ActuatorOutput(Protocol):
    def send_command(self, command: str) -> None: ...


class EvdevKeyboard:
    def __init__(self) -> None:
        try:
            import evdev
        except ImportError as exc:
            raise KeyboardInputError(
                "evdev is required; install it on the Raspberry Pi with "
                "'sudo apt install python3-evdev'"
            ) from exc

        self._evdev = evdev
        self._devices = []
        required_keys = {evdev.ecodes.KEY_A, evdev.ecodes.KEY_D, evdev.ecodes.KEY_Q}

        try:
            for path in evdev.list_devices():
                device = evdev.InputDevice(path)
                key_codes = set(device.capabilities().get(evdev.ecodes.EV_KEY, []))
                if required_keys.issubset(key_codes):
                    self._devices.append(device)
                else:
                    device.close()
        except OSError as exc:
            self.close()
            raise KeyboardInputError(f"Could not open an evdev keyboard: {exc}") from exc

        if not self._devices:
            raise KeyboardInputError(
                "No keyboard with A, D, and Q keys was found under /dev/input"
            )

        self._key_names = {
            evdev.ecodes.KEY_A: "A",
            evdev.ecodes.KEY_D: "D",
            evdev.ecodes.KEY_Q: "Q",
        }

    def read_event(
        self, timeout: float = KEYBOARD_POLL_INTERVAL_S
    ) -> KeyEvent | None:
        try:
            ready, _, _ = select.select(self._devices, [], [], timeout)
        except (OSError, ValueError) as exc:
            raise KeyboardInputError(f"Could not read an evdev keyboard: {exc}") from exc

        for device in ready:
            while True:
                event = device.read_one()
                if event is None:
                    break
                if event.type != self._evdev.ecodes.EV_KEY or event.value not in (0, 1):
                    continue
                key = self._key_names.get(event.code)
                if key is not None:
                    return KeyEvent(key, event.value == 1)
        return None

    def close(self) -> None:
        for device in self._devices:
            device.close()
        self._devices.clear()


class RobotController:
    def __init__(self, keyboard: KeyboardInput, esp32: ActuatorOutput) -> None:
        self.keyboard = keyboard
        self.esp32 = esp32
        self._command = ACTUATOR_STOP
        self._pressed_keys: set[str] = set()
        self._exit_requested = False

    def _send_command(self, command: str, force: bool = False) -> None:
        if force or command != self._command:
            self.esp32.send_command(command)
            self._command = command

    def turnActuatorLeft(self) -> None:
        self._send_command(ACTUATOR_LEFT)

    def turnActuatorRight(self) -> None:
        self._send_command(ACTUATOR_RIGHT)

    def stopActuator(self, force: bool = False) -> None:
        self._send_command(ACTUATOR_STOP, force=force)

    def start(self) -> None:
        self.stopActuator(force=True)

    def step(self) -> None:
        event = self.keyboard.read_event(KEYBOARD_POLL_INTERVAL_S)
        if event is None:
            return

        if event.key == "Q" and event.pressed:
            self._exit_requested = True
            self.stopActuator()
            return

        if event.key not in {"A", "D"}:
            return
        if event.pressed:
            self._pressed_keys.add(event.key)
        else:
            self._pressed_keys.discard(event.key)

        if self._pressed_keys == {"A"}:
            self.turnActuatorLeft()
        elif self._pressed_keys == {"D"}:
            self.turnActuatorRight()
        else:
            self.stopActuator()

    def run(self, should_stop: Callable[[], bool] | None = None) -> None:
        self.start()
        while not self._exit_requested and not (should_stop and should_stop()):
            self.step()

    def stop(self) -> None:
        self.stopActuator(force=True)