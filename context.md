# SprayBot / SprayShark Context: Electronics and Programming

*Last updated: 2026-10-14*

This document captures the electronics and software details discussed
for the SprayBot/SprayShark robotic lawn sprayer. Treat confirmed
details as facts, and treat explicitly marked possibilities or unknowns
as items to inspect before changing hardware or code.

## 0. Repository map and agent quick-reference

This repository is organized around an ESP32-controlled robotic lawn-sprayer prototype and several test sketches. The most relevant folders for new work are:

-   `main_projects/sprayshark_v2_multifile/` — the main multi-file Arduino project for the robot. It contains:
    -   `config.h` — shared pin assignments, motion/actuator constants, BLE UUIDs, and global state.
    -   `sprayshark_v2_multifile.ino` — main sketch that initializes BLE, motor control, pump logic, and arming sequence.
    -   `actuator.ino` — actuator driver, limit switches, and target-position logic.
    -   `motors.ino` — ESC ramping, emergency stop, and arming sequence.
    -   `bluetooth.ino` — Dabble packet parsing and robot command logic.
-   `main_projects/sprayshark_v2_singlefile/` — a single-file variant of the same project for comparison or legacy use.
-   `test_files/v3_actuator_test/` — the active current-sensed actuator travel-time test workspace. Contains `v3_actuator_test_for_esp32.ino` and `v3_actuator_test_for_rpi.py`.
-   `test_files/v3_motor_test/` — wiring documentation and motor test firmware. Relevant files include `PINOUT_AND_WIRING.md` and the `rpi_motor_test_esp32.ino` sketch.
-   `tests/actuator/esp32/` — current standalone actuator test sketches, including `auto_calibrate/auto_calibrate.ino`, `half_extend/half_extend.ino`, and `test_current_pin/test_current_pin.ino`.
-   `tests/actuator/rpi/` — Raspberry Pi actuator test scripts.
-   `test_files/ble_sniffer/` — BLE traffic sniffing / packet debugging experiments.
-   `test_files/v2_motor_ramping_demo/` — motor ramping demo code.
-   `other_info/dabble_codes.md` — Dabble button/packet mapping used by the Bluetooth packet parser.

Important repo-specific facts discovered from the code:

-   The active robot firmware uses a BLE/Dabble control model rather than a plain serial command protocol in the main project.
-   The main multi-file project sets `Serial.begin(115200)` and uses `BLE.begin()` with `SprayShark` as the device name.
-   The most relevant actuator pin map in `config.h` is:
    -   `RPWM_PIN = 2` (extend)
    -   `LPWM_PIN = 3` (retract)
    -   `R_EN_PIN = 4`
    -   `L_EN_PIN = 5`
    -   `R_IS_PIN = 12` and `L_IS_PIN = 11` in the main-project source;
        the current actuator-test wiring instead uses `R_IS_PIN = 6`
        and `L_IS_PIN = 7`.
    -   `LIMIT_SWITCH_RIGHT_PIN = A0`
    -   `LIMIT_SWITCH_LEFT_PIN = A1`
-   The main project also uses ESC pins `leftEscPin = 9` and `rightEscPin = 10` and a relay pin at `6` for the pump.
-   The `Dabble` packet format includes action buttons and d-pad buttons, with `0xFF 0x01 0x01 0x01 0x02 [ACTION_BTNS] [DPAD_BTNS] ...` as the packet frame described in `other_info/dabble_codes.md`.
-   The `main_projects/sprayshark_v2_multifile` code relies on `isOverturnLimitHit()` and a symmetric position model. This means actuator motion is governed by time and virtual position, not just raw duty-cycle commands.
-   The `v3_actuator_test` workspace measures limit-to-limit travel using BTS7960 current-sense outputs; it does not use the main project's limit switches or position model.

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

Inspect the current repository and wiring before assuming a particular
pin, relay, H-bridge, or protocol. Existing working code should be
reused rather than replaced without a reason.

## 2. Raspberry Pi

-   Board: **Raspberry Pi 5, 8 GB RAM**.
-   Raspberry Pi is used for high-level control and Python code.
-   The current Pi-to-ESP32 USB serial device previously observed is
    `/dev/ttyACM0`.
-   The Pi-to-ESP32 serial baud rate previously used is **115200**.
-   These serial settings should be checked against the currently
    running code before relying on them.
-   Raspberry Pi 5 accessories discussed include an active cooler, an
    NVMe M.2 HAT, a 128 GB NVMe SSD, and a Pi camera.
-   Camera note: a Raspberry Pi NoIR camera was previously not detected
    on the Pi 5; this was a separate troubleshooting issue.

## 3. ESP32 boards and development setup

### ESP32-S3 control board

-   Board/module discussed: **ESP32-S3-WROOM-1 N16R8**, used with a
    Freenove board/breakout.
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

## 4. Existing serial communication: Raspberry Pi ↔ ESP32-S3

The following arrangement was reported as working:

-   Raspberry Pi to ESP32-S3 over USB serial.
-   Pi serial device: `/dev/ttyACM0`.
-   Pi-to-ESP32 baud rate: **115200**.
-   ESP32 to RoboClaw motor-controller baud rate: **38400**.
-   RoboClaw address: **0x80**.
-   Commands from the Pi passed through the ESP32 to RoboClaw, and motor
    control was working.
-   A known issue was that RoboClaw telemetry/feedback was not being
    received even though motor commands worked.
-   A Python diagnostic script had been used and was referred to as
    **SPRAYSHARK v3**.

Before modifying the serial protocol:

1.  Inspect the current Python serial implementation and ESP32 command
    parser.
2.  Preserve existing framing, delimiters, command identifiers,
    acknowledgments, and telemetry format where possible.
3.  Avoid having multiple scripts or processes open the same serial port
    simultaneously.
4.  Distinguish the Pi↔ESP32 baud rate (115200) from the ESP32↔RoboClaw
    baud rate (38400).
5.  Do not assume that a working command path proves the
    feedback/telemetry path is correct.

## 5. Drive motors and RoboClaw

-   Motor controller discussed: **RoboClaw 2x15A**.
-   RoboClaw address: **0x80**.
-   ESP32-to-RoboClaw serial baud rate: **38400**.
-   Drive motor control was reported working through the chain Pi →
    ESP32-S3 → RoboClaw.
-   Encoder/closed-loop motor control was part of the design.
-   The reported outstanding issue was missing motor telemetry/feedback.

When debugging feedback, inspect the exact RoboClaw command/API being
used, packet or serial mode, address, baud rate, response parsing, and
whether the transmit/receive wiring and ground reference support
bidirectional communication. Do not change motor-control settings
blindly while the drive system is mechanically loaded.

## 6. Linear actuator test requested on 2026-10-11

The active test measures actuator travel time between physical endpoints
by detecting the current increase when the actuator reaches a limit.

Exactly two files are requested:

-   `v3_actuator_test_for_esp32.ino`
-   `v3_actuator_test_for_rpi.py`

After the Pi sends a start command, the ESP32 reverses at each sensed
limit, measures five traversals, and returns the average travel time to
the Pi. `START` assumes the actuator starts extended; `START_RETRACTED`
assumes it starts retracted.

### Raspberry Pi script requirements

-   Use the existing serial connection and protocol if possible.
-   Current known settings are `/dev/ttyACM0` at 115200 baud, but verify
    them against the current implementation.
-   Send one start command.
-   Display basic status messages from the ESP32.
-   Handle connection errors and communication timeouts.
-   Do not implement the actuator timing on the Pi; the ESP32 owns the
    sequence.
-   Exit cleanly after completion or report communication failure.

### ESP32 firmware requirements

-   Use the existing actuator driver and GPIO configuration after
    inspecting the project.
-   Start the complete sequence after receiving one valid start command.
-   Keep serial communication responsive during the sequence; prefer a
    non-blocking state machine over a long blocking delay.
-   Prevent a second test from starting while the first is active.
-   Stop actuator outputs at startup and after the sequence.
-   Send basic start/completion status messages.
-   Detect endpoints from R_IS/L_IS current sensing; do not use the
    main project's limit switches or virtual-position logic in this
    standalone test.
-   Do not modify existing project files; create only the two named
    files.
-   Provide build/flash instructions and the exact command to run the Pi
    script.

### Implementation notes

The two requested test files are in `test_files/v3_actuator_test/`. The
firmware uses BTS7960 direction pins GPIO 2 (extend) and GPIO 3
(retract), enable pins GPIO 4/5, and current-sense pins GPIO 6 (R_IS,
extend) and GPIO 7 (L_IS, retract). These current-sense assignments
match the actuator-test wiring and differ from the R_IS/L_IS assignments
in the main project's `config.h`. In particular, GPIO 6 is the pump
relay in the main project, so these configurations must not be treated
as interchangeable. The test sketch stops both direction outputs at
startup, detects a sustained current rise at each travel limit, and
reverses after a short dead time.

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

### Important interpretation

The test identifies endpoints from a sustained current increase, which
is intended to indicate that the actuator has reached a mechanical
limit. The configured current-rise threshold should be verified against
the installed driver and actuator before use. The test should be
supervised, with the linkage clear and the robot safely supported; stop
it if the mechanism binds or the driver overheats.

The test sketch's pin assignments and BTS7960 driver name reflect the
current actuator-test wiring, but the actuator's voltage/current
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
each leg's duration and the average extension and retraction durations
to both `Serial` and `Serial0`. `STOP` stops the actuator and aborts the
sequence. ESP32-S3 compilation was verified; hardware behavior was not
tested.

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
3.  Preserve working Pi↔ESP32↔RoboClaw communication.
4.  Make the smallest changes needed for the requested task.
5.  Do not refactor unrelated code or edit existing files when the task
    explicitly requests new files only.
6.  Include the exact filenames, build/flash commands, run commands, and
    known limitations in the handoff.
7.  Never claim hardware behavior was tested unless a real test was
    performed and its result observed.
