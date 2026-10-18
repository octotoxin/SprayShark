"""Run the Raspberry Pi USB-keyboard-to-ESP32 actuator controller."""

from __future__ import annotations

import logging
import signal
import sys
import threading
from pathlib import Path

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
    from firmware.rpi.components.esp32_interface import (
        Esp32CommunicationError,
        Esp32Interface,
    )
    from firmware.rpi.robot_controller import (
        EvdevKeyboard,
        KeyboardInputError,
        RobotController,
    )
else:
    from .components.esp32_interface import Esp32CommunicationError, Esp32Interface
    from .robot_controller import EvdevKeyboard, KeyboardInputError, RobotController


LOGGER = logging.getLogger("sprayshark")


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
    shutdown = threading.Event()
    signal.signal(signal.SIGTERM, lambda *_: shutdown.set())

    esp32 = None
    keyboard = None
    controller = None
    return_code = 0
    try:
        esp32 = Esp32Interface("/dev/ttyACM0")
        keyboard = EvdevKeyboard()
        controller = RobotController(keyboard, esp32)
        controller.run(shutdown.is_set)
    except KeyboardInterrupt:
        LOGGER.info("Shutdown requested")
    except (Esp32CommunicationError, KeyboardInputError) as exc:
        LOGGER.error("%s", exc)
        return_code = 1
    finally:
        if controller is not None:
            try:
                controller.stop()
            except Esp32CommunicationError as exc:
                LOGGER.error("Could not send actuator STOP during shutdown: %s", exc)
                return_code = 1
        elif esp32 is not None:
            try:
                esp32.send_command("ACTUATOR_STOP")
            except Esp32CommunicationError as exc:
                LOGGER.error("Could not send actuator STOP during shutdown: %s", exc)
                return_code = 1
        if keyboard is not None:
            try:
                keyboard.close()
            except OSError as exc:
                LOGGER.error("Could not close keyboard input: %s", exc)
                return_code = 1
        if esp32 is not None:
            try:
                esp32.close()
            except Esp32CommunicationError as exc:
                LOGGER.error("%s", exc)
                return_code = 1

    return return_code


if __name__ == "__main__":
    sys.exit(main())