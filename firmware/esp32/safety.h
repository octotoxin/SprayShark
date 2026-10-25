
/*
 * SprayShark - Firmware Safety Interface
 * DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.
 *
 * Purpose:
 *     Declare the interface for ESP32-side safety monitoring and
 *     protective actions.
 *
 * Responsibilities:
 *     - Declare safety initialization and periodic update functions.
 *     - Define interfaces for tracking valid command activity.
 *     - Declare safe-stop and fault-status functions.
 *     - Provide a consistent interface for safety-related checks.
 *
 * Design:
 *     This header contains declarations and shared definitions only.
 *     Timeout monitoring, fault handling, and protective actions are
 *     implemented in safety.cpp.
 *
 * Related modules:
 *     - safety.cpp: Implements safety monitoring.
 *     - communication.cpp: Reports valid command activity.
 *     - movement.cpp: Provides actuator stop operations.
 *     - spraying.cpp: Provides pump shutdown operations.
 *     - esp32.ino: Initializes and periodically updates safety monitoring.
 *
 * Safety:
 *     Safety interfaces must allow critical protective actions to occur
 *     independently of the Raspberry Pi application.
 */
