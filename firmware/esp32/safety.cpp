
/*
 * SprayShark - Firmware Safety Monitoring
 * DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.
 *
 * Purpose:
 *     Implement local safety checks and protective actions on the ESP32
 *     so hardware can enter a safe state when valid commands stop arriving.
 *
 * Responsibilities:
 *     - Track the time of the most recent valid command or heartbeat.
 *     - Detect command timeouts and relevant firmware faults.
 *     - Request actuator and drive-motor stops when required.
 *     - Request pump shutdown when required.
 *     - Maintain and expose safety or fault status.
 *     - Support safe initialization and controlled fault recovery.
 *
 * Design:
 *     This module implements the safety interface declared in safety.h.
 *     It provides hardware-independent safety coordination by calling
 *     the appropriate control modules.
 *
 * Related modules:
 *     - safety.h: Declares the safety interface.
 *     - communication.cpp: Supplies valid command activity.
 *     - movement.cpp: Implements actuator stopping.
 *     - spraying.cpp: Implements pump shutdown.
 *     - esp32.ino: Runs periodic safety checks.
 *
 * Safety:
 *     Timeout behavior must be deterministic and must not depend on
 *     Raspberry Pi software remaining operational. Choose timeout values
 *     appropriate to the command protocol. A timeout should stop all
 *     affected outputs, and fault recovery must not restart them
 *     unexpectedly.
 */
