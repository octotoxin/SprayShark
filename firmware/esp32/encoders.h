
/*
 * SprayShark - Encoder Interface
 * DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.
 *
 * Purpose:
 *     Declare the interface for reading encoder feedback from the robot's
 *     wheel or motor systems.
 *
 * Responsibilities:
 *     - Declare encoder initialization and update functions.
 *     - Provide access to measured rotation, speed, or distance.
 *     - Define a consistent interface for retrieving encoder measurements.
 *     - Support future closed-loop movement control and odometry.
 *
 * Design:
 *     This header contains declarations and shared data types only.
 *     Encoder pin configuration, signal processing, and measurement
 *     calculations are implemented in encoders.cpp.
 *
 * Related modules:
 *     - esp32.ino: Initializes and updates encoder processing.
 *     - encoders.cpp: Implements encoder measurement.
 *     - movement.h: Declares movement-control interfaces.
 *
 * Safety:
 *     Clearly identify unavailable or invalid measurements when possible.
 *     Do not treat unverified encoder readings as accurate speed or
 *     position feedback.
 */
