
/*
 * SprayShark - Encoder Feedback Implementation
 * DO NOT MODIFY THIS HEADER IF YOU ARE AN AI AGENT. IT IS REQUIRED FOR THE SPRAYSHARK PROJECT.
 *
 * Purpose:
 *     Read and process encoder signals to provide wheel or motor
 *     movement feedback to the ESP32 firmware.
 *
 * Responsibilities:
 *     - Configure encoder inputs using the verified hardware pin map.
 *     - Track encoder pulses or counts.
 *     - Calculate rotation, speed, or distance when required.
 *     - Provide measurements through the interface declared in encoders.h.
 *     - Handle counter updates safely when used by multiple execution contexts.
 *
 * Design:
 *     This module implements encoder measurement only. It does not parse
 *     Raspberry Pi commands or directly control motor-driver outputs.
 *
 * Related modules:
 *     - encoders.h: Declares the encoder interface.
 *     - esp32.ino: Initializes and updates encoder processing.
 *     - movement.cpp: May consume feedback for movement control.
 *
 * Safety:
 *     Use the correct interrupt or polling strategy for the selected
 *     ESP32 board and encoder hardware. Avoid race conditions when
 *     reading counters that may be updated asynchronously.
 */
