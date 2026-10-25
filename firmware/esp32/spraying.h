
/*
 * SprayShark - Spray Control Interface
 * DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.
 *
 * Purpose:
 *     Declare the interface for controlling the robot's spray pump
 *     and related spraying hardware.
 *
 * Responsibilities:
 *     - Declare spraying initialization functions.
 *     - Declare pump start, stop, and status functions.
 *     - Provide a consistent interface for communication and safety modules.
 *     - Support future spraying features when their requirements are defined.
 *
 * Design:
 *     This header contains declarations and shared definitions only.
 *     Pump-driver GPIO operations and spraying implementation are
 *     contained in spraying.cpp.
 *
 * Related modules:
 *     - spraying.cpp: Implements pump control.
 *     - communication.cpp: Dispatches spraying commands.
 *     - safety.cpp: Requests pump shutdown when required.
 *     - esp32.ino: Initializes spraying control.
 *
 * Safety:
 *     The pump must default to OFF during initialization. Pump power must
 *     be switched through an appropriately rated driver or relay, never
 *     directly through an ESP32 GPIO pin.
 */
