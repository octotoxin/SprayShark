
"""
SprayShark main.py - Raspberry Pi Application Entry Point
DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.

Purpose:
    Initialize and coordinate the Raspberry Pi components required
    to control the robot.

Responsibilities:
    - Initialize the Bluetooth receiver and ESP32 serial interface.
    - Start and manage the robot controller (robot_controller.py).
    - Keep the application running until shutdown.
    - Handle graceful shutdown and request actuator STOP when possible.

Design:
    This file coordinates the application lifecycle. It does not
    implement Bluetooth packet decoding, robot control decisions,
    ESP32 serial protocol handling, or actuator hardware control.

Related modules:
    - robot_controller.py: Coordinates robot control logic.
    - components/bluetooth.py: Receives and decodes phone input.
    - components/esp32_interface.py: Communicates with the ESP32.

Hardware control is implemented in the ESP32 firmware, in the esp32 directory. 
The Raspberry Pi application sends commands to the ESP32, which then controls the actuators.
"""

import argparse
import os

if __package__:
    from .components.esp32_interface import ESP32Interface
    from .robot_controller import RobotController
else:
    from components.esp32_interface import ESP32Interface
    from robot_controller import RobotController


def main() -> int:
    parser = argparse.ArgumentParser(description="Start the SprayShark Pi controller.")
    parser.add_argument(
        "--port",
        default=os.environ.get("SPRAYSHARK_SERIAL_PORT", "/dev/ttyACM0"),
        help=(
            "ESP32 serial device (default: /dev/ttyACM0; "
            "overridden by SPRAYSHARK_SERIAL_PORT)"
        ),
    )
    args = parser.parse_args()

    try:
        with ESP32Interface(args.port) as esp32:
            controller = RobotController(esp32)
            controller.initialize()
            print("ESP32 ping succeeded.")
            print("Enter LEFT, RIGHT, STOP, or QUIT.")

            while True:
                try:
                    command = input("> ").strip().upper()
                except EOFError:
                    print()
                    break

                if command in ("QUIT", "EXIT"):
                    break
                if command not in ("LEFT", "RIGHT", "STOP"):
                    print("Invalid command. Enter LEFT, RIGHT, STOP, or QUIT.")
                    continue

                try:
                    response = controller.set_actuator(command)
                except (OSError, TimeoutError, RuntimeError) as error:
                    print(f"ESP32 command failed: {error}")
                    continue
                print(f"ESP32: {response}")
    except (OSError, TimeoutError, RuntimeError) as error:
        parser.exit(1, f"ESP32 initialization failed: {error}\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
