# SprayBot / SprayShark Context: Electronics and Programming

*Last updated: 2026-10-17*

This document captures the electronics and software details discussed
for the SprayBot/SprayShark robotic lawn sprayer. Treat confirmed
details as facts, and treat explicitly marked possibilities or unknowns
as items to inspect before changing hardware or code.

## 0. Repository map and agent quick-reference

The repository has a `firmware/` tree for the Raspberry Pi and ESP32 application, plus separate hardware-test programs:

-   `firmware/rpi/` — Raspberry Pi entry point, robot controller, and component modules.
-   `firmware/esp32/` — ESP32 sketch and communication, encoder, movement, safety, and spraying modules.
-   `tests/actuator/` — standalone actuator test sketches and Pi scripts.
-   `tests/motor/` — RoboClaw wiring notes, Pi diagnostics, and an ESP32 serial forwarder.
-   `tests/bluetooth/rpi/` — Dabble code-finder utility.
-   `other_info/dabble_codes.md` — recorded Dabble button/packet mapping.
-   `final_wiring.md` — wiring reference; verify it against the hardware as currently assembled.

**Firmware rebuild status (2026-10-17):** the Raspberry Pi actuator-control path and the ESP32 serial-command/movement path have been implemented in the files described below. Other firmware modules and declaration headers may still be empty or incomplete. Do not assume unrelated historical architecture, protocols, pin assignments, or behavior are present or working just because they are described elsewhere in this document. The files under `tests/` are separate test programs and are not the application firmware.

The following are historical project notes, not facts verified by the current firmware source:

-   BLE/Dabble control, USB serial communication, and RoboClaw packet serial have all been discussed or used in earlier experiments. Confirm which interface is intended for each new step rather than combining protocols by assumption.
-   Historical pin assignments and Dabble framing are recorded below and in `other_info/dabble_codes.md`; verify them against the actual wiring and the specific test before use.
-   Standalone actuator tests may use different pins and control logic from the eventual application firmware. Do not transfer a test's pin map into the application without checking the wiring.

## 1. Project purpose and architecture

SprayBot is a mobile robot intended to automate lawn herbicide spraying,
with a target coverage area of up to approximately 10,000 sq ft.

The general control architecture discussed is:

-   **Raspberry Pi 5 (8 GB):** high-level application, Python scripts,
    and potential camera/image-processing tasks.
-   **ESP32-S3:** real-time low-level controller and serial command
    endpoint.
-   **RoboClaw 2x15A:** motor controller used for drive motors, with
    encoder/closed-loop control capabilities.
-   **Actuator and pump controls:** separate hardware outputs for
    steering/linear-actuator operation and the spray pump.
-   **Power separation:** motor, pump, and actuator power should be kept
    separate from low-voltage logic power, with a common
    reference/ground where required by the chosen driver and wiring
    design.

The application firmware is being rebuilt incrementally. Implement only
the requested step, inspect the repository and wiring before choosing
pins, drivers, or protocols, and verify each step before building on it.
Hardware and architecture notes elsewhere in this document are historical
references until confirmed for the current build.

## 2. Raspberry Pi

-   Board: **Raspberry Pi 5, 8 GB RAM**.
-   Raspberry Pi is used for high-level control and Python code.
-   `firmware/rpi/main.py` runs the actuator controller when invoked
    directly on the Pi (for example, `python3 firmware/rpi/main.py`).
-   `firmware/rpi/robot_controller.py` reads physical USB keyboard
    key-down/key-up events through `evdev`; `A` retracts, `D` extends,
    releasing either direction stops, and `Q` stops and exits. Holding
    both A and D stops as a safety behavior.
-   The controller sends `ACTUATOR_LEFT`, `ACTUATOR_RIGHT`, and
    `ACTUATOR_STOP` through `firmware/rpi/components/esp32_interface.py`.
-   `firmware/rpi/components/esp32_interface.py` opens `/dev/ttyACM0`
    at 115200 baud and writes each command followed by a newline.
-   On the Pi, install the runtime dependencies with
    `sudo apt install python3-evdev python3-serial`. The evdev device
    permissions must allow the running user to read the keyboard.
-   Raspberry Pi 5 accessories discussed include an active cooler, an
    NVMe M.2 HAT, a 128 GB NVMe SSD, and a Pi camera.
-   Camera note: a Raspberry Pi NoIR camera was previously not detected
    on the Pi 5; this was a separate troubleshooting issue.

## 3. ESP32 boards and development setup

### ESP32-S3 control board

-   Board/module discussed: **ESP32-S3-WROOM-1 N16R8**, used with a
    Freenove board/breakout.
-   `firmware/esp32/esp32.ino` initializes the actuator hardware, starts
    USB serial at 115200 baud, and processes serial input in the loop.
-   `firmware/esp32/communication.cpp` parses newline-terminated
    `ACTUATOR_LEFT`, `ACTUATOR_RIGHT`, and `ACTUATOR_STOP` commands and
    dispatches them to the matching actuator movement functions.
-   `firmware/esp32/movement.cpp` maps left to retract and right to
    extend; STOP disables both actuator directions.
-   The ESP32 sketch compiled for the generic ESP32 target with Arduino
    CLI. Hardware flashing and physical actuator behavior have not been
    verified.
-   Configuration discussed: **16 MB flash and 8 MB PSRAM**, 40 MHz
    crystal.
-   A Freenove breakout/protoboard was selected as a practical way to
    mount the ESP32 and make wiring connections.
-   GPIO screw terminals and secured stranded wiring were considered for
    connections to motor drivers, steering/actuator controls, pump
    relay, encoders, sensors, and limit/emergency switches.
-   Use appropriately rated interface circuitry where needed. Do not
    drive a motor, pump, or actuator directly from an ESP32 GPIO.

### ESP32-C6 development

A separate ESP32-C6 / Matter experiment was also discussed. Keep it
separate from the SprayBot ESP32-S3 control firmware unless the
repository explicitly combines them.

Recorded ESP-IDF environment details from that experiment:

-   ESP-IDF version: **v6.0.3**.
-   Python version observed: **3.13.3**.
-   ESP-IDF project built successfully at least once, producing
    `sample_project.bin`.
-   Monitor was run on **COM9 at 115200 baud**.
-   Boot log identified an ESP32-C6 with 2 MB flash and chip revision
    v0.2.
-   Windows and WSL setup had earlier problems involving Virtual Machine
    Platform/WSL2, and an ESP-IDF PowerShell export script referenced a
    missing Python virtual environment.
-   The ESP32-C6 Matter fan project is a separate development effort; do
    not assume its environment or code applies to SprayBot.

## 4. Serial communication: Raspberry Pi ↔ ESP32-S3

### Current actuator command path (2026-10-17)

-   Connection: Raspberry Pi 5 to ESP32 over USB serial at
    `/dev/ttyACM0`, **115200 baud**.
-   Framing: ASCII command text terminated by newline (`\n`). The
      ESP32 also accepts CRLF by removing a trailing carriage return.
-   Supported commands: `ACTUATOR_LEFT`, `ACTUATOR_RIGHT`, and
      `ACTUATOR_STOP`. The underscore spelling is intentional and must
      match exactly on both sides.
-   `ACTUATOR_LEFT` calls `actuatorLeft()` (retract); `ACTUATOR_RIGHT`
      calls `actuatorRight()` (extend); `ACTUATOR_STOP` calls
      `actuatorStop()`.
-   The ESP32 does not apply a movement heartbeat/timeout. The Pi emits
      a direction command on key-down and an explicit STOP on key-up, so
      timing out an otherwise-held key would stop motion prematurely.
-   Unknown or oversized command lines cause the ESP32 to stop the
      actuator as a fail-safe. Startup also initializes the actuator
      stopped.
-   The Pi handles the `Q` exit path by stopping the actuator before
      shutdown.
-   Rationale: use evdev's real key-down/key-up events for momentary
      actuator control, rather than relying on repeated polling of a
      gamepad/Bluetooth state; underscore-separated tokens make the wire
      commands explicit and consistent across Pi and ESP32.
-   Arduino CLI compilation succeeded for `esp32:esp32:esp32`; this
      verifies compilation only, not USB serial behavior or actuator
      operation on hardware.

### Historical Pi ↔ RoboClaw serial arrangement

The following settings and behavior are from an earlier version and are
not implemented by the current actuator command path:

-   ESP32 to RoboClaw motor-controller baud rate: **38400**.
-   RoboClaw address: **0x80**.
-   Commands from the Pi passed through the ESP32 to RoboClaw, and motor
    control was working.
-   A known issue was that RoboClaw telemetry/feedback was not being
    received even though motor commands worked.
-   A Python diagnostic script had been used and was referred to as
    **SPRAYSHARK v3**.

When adding or changing serial protocols:

1.  Keep the Pi and ESP32 command names, framing, and baud rate in sync.
2.  Avoid having multiple scripts or processes open the same serial port
    simultaneously.
3.  Treat the ESP32↔RoboClaw baud rate (38400) separately from the
    Pi↔ESP32 baud rate (115200).
4.  Do not assume that a working command path proves the
    feedback/telemetry path is correct.

## 5. Drive motors and RoboClaw

-   Motor controller discussed: **RoboClaw 2x15A**.
-   RoboClaw address: **0x80**.
-   ESP32-to-RoboClaw serial baud rate: **38400**.
-   Drive motor control was reported working through the chain Pi →
    ESP32-S3 → RoboClaw in an earlier implementation; that code has
    since been cleared.
-   Encoder/closed-loop motor control was part of the design.
-   The reported outstanding issue was missing motor telemetry/feedback.

When debugging feedback, inspect the exact RoboClaw command/API being
used, packet or serial mode, address, baud rate, response parsing, and
whether the transmit/receive wiring and ground reference support
bidirectional communication. Do not change motor-control settings
blindly while the drive system is mechanically loaded.

## 6. Historical linear actuator test request (2026-10-11)

This section records requirements and implementation notes from an
earlier standalone test request. The described `v3_actuator_test` files
are not present in the current repository; treat these details as
historical reference, not as an available test or application feature.

The original request was for exactly two files:

-   `v3_actuator_test_for_esp32.ino`
-   `v3_actuator_test_for_rpi.py`

After the Pi sends a start command, the ESP32 reverses at each sensed
limit, measures five traversals, and returns the average travel time to
the Pi. `START` assumes the actuator starts extended; `START_RETRACTED`
assumes it starts retracted.

### Raspberry Pi script requirements

-   Define a serial connection and protocol for the standalone test;
    there is no application protocol to reuse.
-   The historical settings were `/dev/ttyACM0` at 115200 baud; verify
    the device and baud rate for the test setup.
-   Send one start command.
-   Display basic status messages from the ESP32.
-   Handle connection errors and communication timeouts.
-   Do not implement the actuator timing on the Pi; the ESP32 owns the
    sequence.
-   Exit cleanly after completion or report communication failure.

### ESP32 firmware requirements

-   Select the actuator driver and GPIO configuration only after
    inspecting the hardware and wiring.
-   Start the complete sequence after receiving one valid start command.
-   Keep serial communication responsive during the sequence; prefer a
    non-blocking state machine over a long blocking delay.
-   Prevent a second test from starting while the first is active.
-   Stop actuator outputs at startup and after the sequence.
-   Send basic start/completion status messages.
-   Detect endpoints from R_IS/L_IS current sensing; do not assume
    application limit switches or virtual-position logic apply to this
    standalone test.
-   Do not modify existing project files; create only the two named
    files.
-   Provide build/flash instructions and the exact command to run the Pi
    script.

### Historical implementation notes (not present in the current tree)

The two requested test files were intended for
`test_files/v3_actuator_test/`; that directory is absent from the current
repository. The former proposed firmware used BTS7960 direction pins
GPIO 2 (extend) and GPIO 3 (retract), enable pins GPIO 4/5, and
current-sense pins GPIO 6 (R_IS, extend) and GPIO 7 (L_IS, retract).
These assignments were recorded for the standalone test and differ from
an older application pin map. Historical notes assigned GPIO 6 to the
pump relay in that application map, so the configurations must not be
treated as interchangeable. The test sketch stopped both direction
outputs at startup, detected a sustained current rise at each travel
limit, and reversed after a short dead time.

The standalone test protocol accepts newline-terminated `START` or
`START_RETRACTED` over USB serial at 115200 baud. `START` assumes the
actuator starts extended and initially retracts; `START_RETRACTED`
assumes it starts retracted and initially extends. The non-blocking
state machine measures five consecutive limit-to-limit traversals and
reports each time plus `AVERAGE_TRAVEL_MS=<milliseconds>`. It reports a
travel timeout if current-based limit detection does not occur within
20 seconds. On ESP32-S3, the sketch reads from both `Serial` and
`Serial0` and mirrors status lines to both. The Pi script defaults to
`/dev/ttyACM0`, supports `--start-position extended|retracted`,
displays the reported average, and waits up to 120 seconds for
completion. It uses pyserial, consistent with existing Pi scripts.
To run the Pi script from its directory with the actuator starting
retracted, use:

```bash
python3 v3_actuator_test_for_rpi.py --start-position retracted
```

Arduino CLI requires the sketch's main `.ino` filename to match its
sketch directory. These required filenames do not match the existing
`v3_actuator_test` directory name, so compile/upload using a temporary
Arduino sketch directory named `v3_actuator_test_for_esp32` containing
`v3_actuator_test_for_esp32.ino`. Compilation was verified for
`esp32:esp32:esp32s3` that way; hardware flashing and actuator operation
were not tested.

### Safety notes for actuator tests

The historical test identified endpoints from a sustained current
increase, intended to indicate that the actuator has reached a
mechanical limit. Any current-rise threshold must be verified against
the installed driver and actuator before use. Supervise actuator tests,
keep the linkage clear, and stop if the mechanism binds or the driver
overheats.

The standalone test's pin assignments and BTS7960 driver name reflect
the wiring recorded for that test, but the actuator's voltage/current
ratings must still be verified against the installed hardware before
operation. The standalone serial protocol is specific to this test
sketch; it is not the main project's BLE/Dabble control protocol or the
RoboClaw packet-serial protocol.

### Current actuator calibration experiment (2026-10-14)

The current ESP32-S3 calibration sketch is
`tests/actuator/esp32/auto_calibrate/auto_calibrate.ino`. It uses the
actuator-test pin map: RPWM/extend GPIO 2, LPWM/retract GPIO 3, enables
GPIO 4/5, and current-sense inputs R_IS GPIO 6 and L_IS GPIO 7. This
test uses the BTS7960 current-sense outputs as limit indicators; it is
separate from the earlier current-rise test described above.

At 115200 baud, newline-terminated `START` begins a non-blocking
sequence: retract until L_IS reads exactly zero continuously for at
least 10 ms, then perform three extend/retract traversals. R_IS > L_IS
confirms extension and L_IS > R_IS confirms retraction before a zero
reading is accepted as that leg's endpoint. Any nonzero sample resets
the 10 ms confirmation interval. Each leg's duration is measured from
the first zero sample in its confirmed interval. The sketch reports
each leg's duration and the average of all six travel durations to both
`Serial` and `Serial0`, then extends for half of that average duration
to finish at the estimated midpoint. `STOP` stops the actuator and
aborts the sequence. ESP32-S3 compilation was verified; hardware
behavior was not tested.

The related `tests/actuator/esp32/half_extend/half_extend.ino` sketch
is a fixed-duration test: it retracts for 4 seconds, pauses for 2
seconds, extends for 1.61 seconds, then pauses for 2 seconds. A
newline-terminated `START` begins the sequence; it reports status on
both serial ports at 115200 baud.

## 7. Other electronics discussed

-   **Spray pump:** SEAFLO 21-Series diaphragm pump, 12 V DC,
    approximately 1.2 GPM, adjustable pressure switch up to 35 PSI.
-   **Tank:** 3-gallon tank.
-   **Filtering:** 50-mesh filter between tank and pump; 80-mesh filter
    between pump and nozzle.
-   **Battery configurations discussed at different stages:** two 12 V
    LiFePO4 20 Ah batteries were mentioned earlier; a 12 V 40 Ah battery
    with a 40 A BMS was also mentioned later. Confirm the current
    installed configuration rather than assuming these are
    interchangeable or simultaneously installed.
-   **Motor drivers/relays considered:** BTS7960 43 A H-bridge module
    and SRD-05VDC-SL-C relay modules. These were discussed as
    components, but do not assume either is the currently installed
    actuator driver without checking the project.
-   **Other controller:** an Arduino was part of an earlier electronics
    configuration. Confirm whether it remains in the current
    architecture before adding a new communication path.
-   An earlier issue involved a cable burning between an ESP32 and
    Arduino. Inspect wiring, current paths, driver ratings, and grounds
    before reconnecting or reusing that setup.

## 8. Wiring, safety, and packaging principles

-   Do not power actuators, pumps, or drive motors from ESP32 GPIO pins.
-   Use a motor driver/relay rated for the load and appropriate for the
    motor's direction-control requirements.
-   Keep high-current actuator, pump, and drive-motor power paths
    separate from low-voltage logic power.
-   Protect electronics from water, herbicide, mud, and debris; mount
    them above the lowest chassis plane where practical.
-   Use strain relief and secured wiring. The Freenove board was
    considered useful for organized GPIO connections.
-   Verify logic voltage compatibility and use level shifting or
    interface circuitry when required.
-   Ensure outputs default to a safe, stopped state after reset or
    startup.
-   For actuator testing, keep hands clear of the moving linkage and
    stop the test if the mechanism binds or the driver overheats.

## 9. Programming and tooling notes

-   Python is used on the Raspberry Pi for higher-level control and
    serial commands.
-   Arduino-style `.ino` firmware is used for ESP32 work in the
    immediate actuator-test request.
-   ESP-IDF v6.0.3 was used for a separate ESP32-C6 project; do not
    assume the SprayBot ESP32-S3 firmware is built with ESP-IDF unless
    the repository confirms it.
-   Pylance is Microsoft's Python language support for VS Code. It can
    provide Python completion, type checking, diagnostics, and
    navigation. It does not control hardware, flash ESP32 firmware, or
    test the actuator.
-   A Pylance MCP integration, if installed, may expose some
    code-analysis capabilities to an AI agent; exact capabilities depend
    on the specific implementation.

## 10. Agent workflow and constraints

When working on SprayBot electronics/programming:

1.  Inspect the actual repository and existing code before choosing GPIO
    pins, command formats, drivers, or libraries.
2.  Clearly distinguish verified facts from assumptions.
3.  Rebuild and verify Pi↔ESP32↔RoboClaw communication step by step;
    do not assume the historical connection still works.
4.  Make the smallest changes needed for the requested task.
5.  Do not refactor unrelated code or edit existing files when the task
    explicitly requests new files only.
6.  Include the exact filenames, build/flash commands, run commands, and
    known limitations in the handoff.
7.  Never claim hardware behavior was tested unless a real test was
    performed and its result observed.
