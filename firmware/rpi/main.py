"""Run the Raspberry Pi Bluetooth-to-ESP32 actuator controller."""

from __future__ import annotations

import argparse
import logging
import os
import signal
import sys
import threading

from .components.bluetooth import BluetoothConnectionError, BluetoothReceiver
from .components.esp32_interface import Esp32CommunicationError, Esp32Interface
from .robot_controller import RobotController


LOGGER = logging.getLogger("sprayshark")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--bluetooth-address",
        default=os.environ.get("DABBLE_BLUETOOTH_ADDRESS"),
        help="Optional Dabble phone MAC address filter (or DABBLE_BLUETOOTH_ADDRESS)",
    )
    parser.add_argument(
        "--serial-port",
        default="/dev/ttyACM0",
        help="ESP32 USB serial device (default: /dev/ttyACM0)",
    )
    parser.add_argument("--rfcomm-channel", type=int, default=1)
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")

    receiver = BluetoothReceiver(
        channel=args.rfcomm_channel, expected_address=args.bluetooth_address
    )
    esp32 = None
    controller = None
    return_code = 0
    shutdown = threading.Event()
    signal.signal(signal.SIGTERM, lambda _signum, _frame: shutdown.set())
    try:
        receiver.connect(shutdown.is_set)
        if not shutdown.is_set():
            esp32 = Esp32Interface(args.serial_port)
            controller = RobotController(receiver, esp32)
            controller.run(shutdown.is_set)
    except KeyboardInterrupt:
        LOGGER.info("Shutdown requested")
    except BluetoothConnectionError as exc:
        LOGGER.error("%s", exc)
        return_code = 1
    except Esp32CommunicationError as exc:
        LOGGER.error("%s", exc)
        return_code = 1
    else:
        return_code = 0
    finally:
        if controller is not None:
            try:
                controller.stop()
            except Esp32CommunicationError as exc:
                LOGGER.error("Could not send actuator STOP during shutdown: %s", exc)
                return_code = 1
        try:
            receiver.close()
        except BluetoothConnectionError as exc:
            LOGGER.error("Could not close Bluetooth connection: %s", exc)
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