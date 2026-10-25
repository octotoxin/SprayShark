
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
