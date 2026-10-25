
/*
 * SprayShark - Spray Pump Control
 * DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.
 *
 * Purpose:
 *     Implement low-level control of the robot's spray pump through
 *     its designated driver or relay.
 *
 * Responsibilities:
 *     - Configure the verified pump-control output.
 *     - Initialize the pump output to OFF.
 *     - Implement pump start and stop operations.
 *     - Report the requested or commanded pump state when required.
 *     - Provide the functions declared in spraying.h.
 *     - Support future spraying features when the hardware requirements
 *       and control protocol are defined.
 *
 * Design:
 *     This module owns pump-control hardware operations. It does not
 *     implement Bluetooth communication, serial command parsing, or
 *     steering actuator control.
 *
 * Related modules:
 *     - spraying.h: Declares the spray-control interface.
 *     - communication.cpp: Dispatches spraying commands.
 *     - safety.cpp: Requests pump shutdown during faults or timeouts.
 *     - esp32.ino: Initializes spraying control.
 *
 * Safety:
 *     Never drive the pump directly from an ESP32 GPIO pin. Use the
 *     correctly rated driver or relay and verify its active-high or
 *     active-low behavior. Ensure the pump remains OFF at startup and
 *     enters the defined safe state after communication loss or faults.
 */
