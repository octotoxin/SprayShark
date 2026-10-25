
/*
 * SprayShark - ESP32 Firmware Entry Point
 * DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.
 *
 * Purpose:
 *     Initialize and coordinate the ESP32 firmware modules responsible
 *     for robot movement, communication, safety, encoders, and spraying.
 *
 * Responsibilities:
 *     - Initialize serial communication and required hardware modules.
 *     - Call each module's initialization function.
 *     - Process incoming commands during the main loop.
 *     - Run periodic safety checks and module updates.
 *     - Coordinate module execution without implementing their internal logic.
 *
 * Design:
 *     This file is the firmware entry point. It coordinates the application
 *     lifecycle but does not implement serial parsing, actuator control,
 *     encoder processing, or pump-driver operations.
 *
 * Related modules:
 *     - communication.cpp: Receives and dispatches commands.
 *     - movement.cpp: Controls the steering actuator.
 *     - safety.cpp: Implements firmware safety monitoring.
 *     - encoders.cpp: Reads encoder feedback.
 *     - spraying.cpp: Controls the spray pump.
 *
 * Safety:
 *     Initialize outputs to safe states before accepting movement or spray
 *     commands. Ensure periodic safety processing cannot be skipped during
 *     normal firmware operation.
 */
