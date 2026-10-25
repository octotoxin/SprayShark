
"""
SprayShark - Safety Monitor
DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.

Purpose:
    Monitor the robot's operational state for unsafe conditions and
    coordinate protective actions to help prevent unintended movement.

Responsibilities:
    - Monitor the availability of Bluetooth input and control commands.
    - Detect stale input, communication failures, and reported subsystem faults.
    - Identify conditions that require movement to stop or be restricted.
    - Request actuator and drive-motor stops through motor_controller.py.
    - Track safety faults and provide diagnostic information.
    - Coordinate safety checks as additional robot subsystems are integrated.
    - Support recovery from faults only when the relevant safety conditions
      have been verified.

Design:
    This module handles high-level safety monitoring and fault coordination.
    It does not implement motor-driver control, actuator GPIO/PWM operations,
    or direct Bluetooth and serial communication.

Related modules:
    - main.py: Application entry point and lifecycle management.
    - robot_controller.py: Coordinates user input and overall robot behavior.
    - motor_controller.py: Manages steering actuator and drive-motor commands.
    - components/bluetooth.py: Receives and decodes phone input.
    - components/esp32_interface.py: Communicates with the ESP32.
    - ESP32 communication.cpp: Parses incoming commands.
    - ESP32 movement.cpp: Implements actuator hardware control.

Safety:
    This module provides supplementary protection and must not be the
    only mechanism responsible for stopping the robot.
    The ESP32 must independently enforce command timeouts and safe output
    states if communication is interrupted or the Raspberry Pi fails.
    Critical protections, including a physical emergency stop if required,
    must not depend solely on Raspberry Pi software.
"""
