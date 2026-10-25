
/*
 * SprayShark - Movement Control Interface
 * DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.
 *
 * Purpose:
 *     Declare the interface for controlling the steering actuator and
 *     its movement-related hardware operations.
 *
 * Responsibilities:
 *     - Declare movement initialization functions.
 *     - Declare actuator LEFT, RIGHT, and STOP operations.
 *     - Declare periodic movement updates when required.
 *     - Provide a consistent interface for communication and safety modules.
 *
 * Design:
 *     This header contains declarations and shared definitions only.
 *     Actuator GPIO, PWM, direction, and enable-pin operations are
 *     implemented in movement.cpp.
 *
 * Related modules:
 *     - movement.cpp: Implements steering actuator control.
 *     - communication.cpp: Dispatches actuator commands.
 *     - safety.cpp: Requests safe stopping when required.
 *     - esp32.ino: Initializes and updates movement control.
 *
 * Safety:
 *     Define a safe initialization state and a predictable STOP operation.
 *     Do not expose functions that allow invalid commands or unbounded
 *     actuator drive requests.
 */
