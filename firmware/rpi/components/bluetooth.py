
"""
SprayShark - Bluetooth Input Handler
DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.

Purpose:
    Manage Bluetooth communication between the Raspberry Pi and the
    phone running the Dabble app, providing decoded button states
    to the robot controller.

Responsibilities:
    - Establish and manage the Bluetooth connection.
    - Receive incoming Bluetooth data from the phone.
    - Validate and parse Dabble packets according to the actual protocol.
    - Decode directional button states, including Left and Right.
    - Expose a clean interface for robot_controller.py to retrieve
      the latest valid button states.
    - Detect disconnections, malformed packets, and stale input.
    - Report communication failures so the controller can respond safely.

Design:
    This module handles Bluetooth communication and input decoding only.
    It does not decide robot behavior, send commands to the ESP32,
    or control actuator hardware.

Related modules:
    - main.py: Application entry point and lifecycle management.
    - robot_controller.py: Interprets button states and selects actions.
    - components/esp32_interface.py: Communicates with the ESP32.

Safety:
    Invalid, stale, or unavailable input must not be treated as a valid
    movement command. The robot controller and ESP32 must independently
    enforce safe stopping behavior when input or communication is lost.

Implementation note:
    Verify the Bluetooth connection mode supported by the Dabble app
    and Raspberry Pi before implementing packet reception and decoding.
"""
