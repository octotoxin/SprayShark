
/*
 * SprayShark - Steering Actuator Control
 * DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.
 *
 * Purpose:
 *     Implement low-level control of the steering actuator through the
 *     IBT-2 motor driver.
 *
 * Responsibilities:
 *     - Configure the verified actuator GPIO and PWM pins.
 *     - Initialize outputs to safe states.
 *     - Drive the actuator left or right on request.
 *     - Stop actuator drive outputs when STOP is requested.
 *     - Apply validated PWM values and direction transitions.
 *     - Provide the functions declared in movement.h.
 *
 * Design:
 *     This module owns steering actuator hardware control. It does not
 *     parse serial commands, decode Bluetooth packets, or control the
 *     spray pump. Drive-motor control should remain in a dedicated module
 *     if the robot's drive hardware requires separate control logic.
 *
 * Related modules:
 *     - movement.h: Declares the movement interface.
 *     - communication.cpp: Dispatches actuator commands.
 *     - safety.cpp: Coordinates timeout and fault-related stops.
 *     - esp32.ino: Initializes movement control.
 *
 * Safety:
 *     Verify pin assignments against the actual ESP32 board and IBT-2 wiring.
 *     Prevent unsafe direction changes and ensure STOP sets drive PWM
 *     outputs to zero. Hardware enable states must follow the driver's
 *     requirements. Do not assume GPIO assignments are electrically safe
 *     without verifying board-specific constraints.
 */
