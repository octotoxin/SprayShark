
/*
 * SprayShark - Communication Interface
 * DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.
 *
 * Purpose:
 *     Declare the interface for receiving and processing commands sent
 *     from the Raspberry Pi to the ESP32 over USB serial.
 *
 * Responsibilities:
 *     - Declare communication initialization and processing functions.
 *     - Define the supported command interface.
 *     - Provide a consistent interface for command handling.
 *     - Expose communication status when required by other modules.
 *
 * Design:
 *     This header contains declarations and shared definitions only.
 *     Serial reception, command parsing, validation, and dispatch logic
 *     are implemented in communication.cpp.
 *
 * Related modules:
 *     - esp32.ino: Initializes and processes communication.
 *     - communication.cpp: Implements the communication interface.
 *     - movement.h: Declares movement and actuator functions.
 *     - spraying.h: Declares spray-control functions.
 *     - safety.h: Declares safety-related interfaces.
 *
 * Safety:
 *     Commands must be validated before execution. Malformed, incomplete,
 *     or unsupported commands must not activate hardware.
 */
