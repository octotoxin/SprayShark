# SprayShark Software Architecture

## 1. Overview

SprayShark consists of two separate firmware codebases located under the `firmware/` directory: Raspberry Pi software and ESP32 firmware. Each runs on different hardware and must be deployed independently. The two codebases communicate over a serial connection and must remain compatible, particularly in their command formats and communication protocols.

The Raspberry Pi handles high-level application logic, receives operator commands, coordinates software components, and sends commands to the ESP32. The ESP32 handles low-level robot control, hardware interaction, sensor feedback, and time-sensitive safety behavior.

## 2. Raspberry Pi Software

The Raspberry Pi codebase is organized around an entry point, a central controller, and individual components.

### Core Files

- **`main.py`** — Entry point for the Raspberry Pi application. It initializes and starts the program.
- **`robot_controller.py`** — Coordinates the Raspberry Pi components and connects their functionality to the overall robot control workflow.

### Components

- **`bluetooth.py`** — Connects to the phone or control application over Bluetooth and receives operator commands for robot operation.
- **`esp32_interface.py`** — Handles serial communication between the Raspberry Pi and ESP32, providing the interface for sending commands and receiving responses or telemetry.
- **`motor_controller.py`** — Coordinates motor-control commands sent to the ESP32 and manages actuator-related commands as implemented by the current codebase.
- **`safety_monitor.py`** — Monitors software-level safety conditions and initiates appropriate responses when configured safety conditions are violated. Its capabilities depend on available sensor data and implemented checks.
- **`spray_controller.py`** — Controls the spray pump through a relay. It provides the software interface for spray operation and can support future higher-level spraying logic, such as coordinating spraying with robot movement and weed-treatment decisions.

The Raspberry Pi primarily coordinates robot behavior and issues commands. Hardware-level actions and time-sensitive control are delegated to the ESP32 where implemented.

## 3. ESP32 Firmware

The ESP32 codebase is responsible for low-level robot control, direct hardware interaction, feedback processing, and embedded safety behavior. It is built and flashed independently of the Raspberry Pi software.

### Entry Point

- **`esp32.ino`** — Main firmware entry point. It initializes the hardware and software components and runs the main control loop.

### Firmware Components

Each component has a corresponding header file (`.h`) that declares its functions and interfaces.

- **`communication.cpp` / `communication.h`** — Handles serial communication with the Raspberry Pi, including receiving commands and returning responses or telemetry.
- **`movement.cpp` / `movement.h`** — Implements low-level movement control by translating commands into the appropriate motor-control actions.
- **`encoders.cpp` / `encoders.h`** — Reads and processes encoder feedback to measure wheel or motor movement. This feedback can support motion estimation and future localization capabilities alongside GPS or other sensors.
- **`safety.cpp` / `safety.h`** — Implements embedded safety checks and responses to configured fault conditions. This provides a safety layer close to the hardware.
- **`spraying.cpp` / `spraying.h`** — Controls the spraying hardware at the ESP32 level, including relay operation where assigned by the current implementation.

The exact division of hardware ownership between the Pi and ESP32 should follow the current implementation, particularly for actuator and spray-pump control.

## 4. Communication and Control Flow

The overall control flow is:

1. The operator sends commands through the phone's Bluetooth control application.
2. The Raspberry Pi's Bluetooth component receives the commands.
3. `robot_controller.py` coordinates the requested behavior and invokes the relevant components.
4. `esp32_interface.py` sends supported commands to the ESP32 over serial.
5. `communication.cpp` receives and interprets the commands.
6. The appropriate ESP32 component performs the requested operation, such as movement or spraying.
7. The ESP32 can return status information, feedback, or telemetry to the Raspberry Pi, as supported by the protocol.

The Raspberry Pi and ESP32 must use compatible communication formats. Changes to command definitions, responses, or protocol behavior may require coordinated changes to both codebases.

## 5. Safety Architecture

Safety responsibilities are divided between the Raspberry Pi and ESP32.

The Raspberry Pi safety monitor handles software-level monitoring and coordination. The ESP32 safety component handles embedded checks that require timely responses near the hardware. The two layers complement each other, but the design must not assume that both are always available or that the Pi can detect every physical hazard.

Safety behavior should explicitly define responses to conditions such as lost communication, invalid commands, controller faults, and emergency-stop activation where supported. Actual detection capabilities depend on installed sensors, electrical protection, and implemented logic. Safety-critical stopping behavior should not rely exclusively on the Raspberry Pi or its communication link.

## 6. Design Principles

- **Separation of responsibilities:** Keep high-level coordination on the Raspberry Pi and low-level, time-sensitive control on the ESP32 where appropriate.
- **Clear interfaces:** Components should communicate through defined functions and documented interfaces rather than duplicating responsibilities.
- **Protocol compatibility:** Keep the Raspberry Pi and ESP32 command formats synchronized.
- **Independent deployment:** Each codebase must be built, verified, and deployed to its own hardware.
- **Layered safety:** Implement appropriate checks at both the application and embedded-control levels.
- **Incremental development:** Future capabilities, including more advanced spraying decisions and localization, should be introduced without unnecessarily coupling unrelated components.

## 7. Documentation and Implementation

This document describes the high-level software architecture and the intended responsibilities of its components.

For detailed communication contracts, refer to `docs/interfaces/`. For hardware specifications and wiring, refer to `docs/hardware/`. For engineering decisions, refer to `docs/design_docs/`.

When the implementation changes, update this document as needed. Verify component responsibilities and actual behavior against the source code rather than assuming that every planned capability is already implemented.