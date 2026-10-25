
/*
 * SprayShark - Serial Communication Handler
 * DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.
 *
 * Purpose:
 *     Receive, validate, parse, and dispatch commands from the Raspberry Pi
 *     to the appropriate ESP32 firmware modules.
 *
 * Responsibilities:
 *     - Initialize USB serial communication at the agreed baud rate.
 *     - Read newline-terminated commands from the Raspberry Pi.
 *     - Validate command names, formatting, and parameter values.
 *     - Dispatch actuator, drive-motor, and spraying commands.
 *     - Reject malformed, oversized, or unsupported commands.
 *     - Update command-freshness information for safety monitoring.
 *
 * Design:
 *     This module implements the serial communication interface declared
 *     in communication.h. It routes commands to hardware-control modules
 *     without implementing their low-level operations.
 *
 * Related modules:
 *     - communication.h: Declares the communication interface.
 *     - esp32.ino: Calls communication initialization and processing.
 *     - movement.cpp: Implements steering actuator control.
 *     - spraying.cpp: Implements pump control.
 *     - safety.cpp: Uses command-freshness information for safety checks.
 *
 * Safety:
 *     Never execute partially received or invalid commands. Ensure command
 *     buffers have defined size limits and that communication processing
 *     does not block safety checks indefinitely.
 */
