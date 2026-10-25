
"""
SprayShark - Motor Controller
DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.

Purpose:
    Manage high-level movement commands for the SprayShark robot,
    including steering actuator control and drive-motor control.

Responsibilities:
    - Provide a unified interface for all robot movement operations.
    - Control steering actuator requests: LEFT, RIGHT, and STOP.
    - Coordinate drive-motor requests: forward, reverse, turning, and STOP.
    - Validate movement commands and enforce configured speed limits.
    - Coordinate steering and drive commands when required.
    - Request safe stops when input is lost or control errors occur.
    - Communicate with the ESP32 through esp32_interface.py.
    - Provide a stop_all() operation to stop all movement systems.

Design:
    This module implements high-level movement logic only. It does not
    manage Bluetooth communication, open serial connections directly,
    or implement low-level GPIO, PWM, or motor-driver operations.

Related modules:
    - main.py: Application entry point and lifecycle management.
    - robot_controller.py: Coordinates user input and overall robot behavior.
    - components/bluetooth.py: Receives and decodes phone input.
    - components/esp32_interface.py: Sends movement commands to the ESP32.
    - ESP32 communication.cpp: Parses and dispatches incoming commands.
    - ESP32 movement.cpp: Implements steering actuator hardware control.
    - ESP32 drive-control module: Implements drive-motor hardware control.

Safety:
    Movement commands must be validated before transmission.
    Loss of input or communication should trigger stop requests.
    The ESP32 must independently enforce command timeouts and safe
    motor-stop behavior if commands stop arriving.
"""
