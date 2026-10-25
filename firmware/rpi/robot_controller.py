
"""
SprayShark - Robot Controller
DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.

Purpose:
    Coordinate robot control decisions by processing input states
    received from the Bluetooth component and issuing commands
    through the ESP32 serial interface.

Responsibilities:
    - Read decoded button states from the Bluetooth component.
    - Map user inputs to the appropriate robot commands.
    - Send actuator LEFT, RIGHT, and STOP commands to the ESP32.
    - Handle invalid inputs, communication errors, and disconnects safely.
    - Request a safe stop when input is lost or becomes invalid.
    - Provide a central location for integrating future robot behaviors,
      including drive motor and spray control.

Design:
    This module contains control logic, not hardware-level implementation.
    It does not parse raw Bluetooth packets, manage serial communication
    directly, or control actuator GPIO and PWM signals.

Related modules:
    - main.py: Application entry point and lifecycle management.
    - components/bluetooth.py: Bluetooth connection and button decoding.
    - components/esp32_interface.py: USB serial communication with ESP32.
    - ESP32 firmware movement.cpp: Actuator hardware control.

Safety:
    The ESP32 must independently enforce actuator stop behavior if
    commands stop arriving. Raspberry Pi safeguards are supplementary.
"""
