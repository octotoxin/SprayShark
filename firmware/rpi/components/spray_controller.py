
"""
SprayShark - Spray Controller
DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.

Purpose:
    Manage high-level spraying operations for the SprayShark robot,
    including pump activation, shutdown, and spray-operation logic.

Responsibilities:
    - Start and stop spraying in response to authorized commands.
    - Track the requested and current spraying state.
    - Prevent spraying when required operating or safety conditions
      are not satisfied.
    - Coordinate spray operations with robot movement when required.
    - Request pump shutdown when input is lost, communication fails,
      or a relevant safety fault occurs.
    - Provide a foundation for future features such as spray-duration
      control, coverage planning, and flow monitoring.
    - Communicate with the ESP32 through esp32_interface.py.

Design:
    This module implements high-level spraying logic only. It does not
    manage Bluetooth communication, open serial connections directly,
    or implement low-level pump-driver and GPIO operations.

Related modules:
    - main.py: Application entry point and lifecycle management.
    - robot_controller.py: Coordinates user input and overall robot behavior.
    - motor_controller.py: Manages steering actuator and drive-motor commands.
    - components/bluetooth.py: Receives and decodes phone input.
    - components/esp32_interface.py: Sends commands to the ESP32.
    - ESP32 communication.cpp: Parses and dispatches incoming commands.
    - ESP32 pump-control module: Implements pump-driver hardware control.

Safety:
    The pump must default to OFF during startup and enter a safe state
    when valid commands or communication are lost.
    Pump power must be controlled through an appropriately rated driver
    or relay, never directly from a Raspberry Pi or ESP32 GPIO pin.
    Critical shutdown protections must not depend solely on Raspberry
    Pi software.
"""
