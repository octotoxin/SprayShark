# Code Quality and Verification

Coding agents must run the applicable quality checks after modifying code. Report the commands executed and their actual results. Never claim a check passed unless it was run successfully.

## 1. Raspberry Pi Python Tests

Use `pytest` to test the Python code in `firmware/rpi/`.

### Run tests

From the repository root, run:

```powershell
python -m pytest firmware/rpi/
```

This instructs pytest to search the Raspberry Pi project directory recursively for test files. By default, pytest discovers files named `test_*.py` or `*_test.py` and functions named `test_*`.

To run a specific test file:

```powershell
python -m pytest firmware/rpi/tests/test_robot_controller.py
```

Replace the example path with the actual test file path.

To display more detailed test results:

```powershell
python -m pytest firmware/rpi/ -v
```

### Requirements

- After modifying Python code, run the relevant existing tests. Run the full Raspberry Pi test suite when practical.
- When adding new functionality or fixing a bug, add or update tests to cover the changed behavior.
- Test normal inputs, invalid inputs, error handling, and safety-related logic where applicable.
- Mock hardware dependencies, Bluetooth connections, serial communication, motors, and relays when tests should run without physical hardware.
- Fix failures caused by the changes before completing the task. Do not modify or remove tests merely to make them pass without addressing the underlying issue.
- If no tests are discovered, report that fact. Do not describe an empty test run as successful test coverage.
- If a test fails because of an unrelated pre-existing issue, report the failure and explain why it appears unrelated.
- If pytest is unavailable, report the issue rather than silently skipping verification.

## 2. ESP32 Firmware Compilation

Use Arduino CLI to compile the ESP32-S3 firmware after modifying ESP32 source files, including `.ino`, `.cpp`, and `.h` files.

### Compile the main sketch

Run the following command from the repository root, replacing the example directory with the actual directory containing the main `esp32.ino` sketch:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32s3 firmware/esp32
```

**Important:** Arduino CLI expects the sketch directory. The directory must contain the main `.ino` file, whose name normally matches the sketch directory name. If the actual sketch directory or `.ino` filename differs, use the repository's real path and ensure the sketch layout is valid for Arduino CLI.

### Requirements

- Compile the main SprayShark ESP32 sketch after modifying firmware code.
- Resolve compilation errors introduced by the changes before completing the task.
- If compilation fails because Arduino CLI, the ESP32 board platform, or a required library is missing, report the specific error.
- Do not substitute a successful compilation of an unrelated test sketch for compilation of the main firmware.
- Compilation verifies that the firmware builds; it does not prove that motor control, wiring, communication, or safety behavior works on physical hardware.

## 3. Final Verification Report

Before completing a coding task, provide a concise verification report containing:

- **Files changed:** List the modified files.
- **Python tests:** State the exact command, number of tests passed or failed, and whether any tests were skipped or not discovered.
- **ESP32 compilation:** State the exact command and whether compilation succeeded or failed, when applicable.
- **Unverified behavior:** Identify any tests that require physical hardware or could not be performed.
- **Remaining issues:** List unresolved failures and relevant limitations.

Run only the checks applicable to the changes, but explain any omitted check. Clearly distinguish automated test results, successful compilation, and physical hardware verification.