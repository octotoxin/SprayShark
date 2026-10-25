# Documentation Index

The `docs/` directory serves as a central map of project knowledge relevant to the SprayShark codebase. It helps developers and coding agents find hardware specifications, software architecture, communication contracts, engineering decisions, execution plans, and code quality requirements.

## Quality Checks

[`quality.md`](quality.md) documents the quality checks expected for code changes. It includes instructions for running Python tests, compiling firmware with Arduino CLI, and performing other applicable verification steps.

## Hardware

The [`hardware/`](hardware/) directory documents the physical components and wiring required for SprayShark.

- [`hardware/overview.md`](hardware/overview.md) describes the robot's hardware, including batteries, motors, motor controllers, steering components, pump, and other relevant specifications.
- [`hardware/pinout_and_wiring.md`](hardware/pinout_and_wiring.md) documents ESP32 pin assignments and hardware connections to help ensure correct wiring.

## Software Architecture

[`architecture.md`](architecture.md) explains how SprayShark's software components fit together, what each component is responsible for, and how they interact. It provides a high-level view of the software architecture, including the Raspberry Pi and ESP32 responsibilities.

## Execution Plans

The [`exec_plans/`](exec_plans/) directory stores plans for significant engineering tasks. These plans define goals, scope, constraints, implementation steps, and acceptance criteria.

- [`exec_plans/active/`](exec_plans/active/) contains plans for work in progress.
- [`exec_plans/completed/`](exec_plans/completed/) contains completed plans and records of implementation and verification results.
- [`exec_plans/tech_debt.md`](exec_plans/tech_debt.md) tracks known issues and improvements deferred for future work.

## Communication Interfaces

The [`interfaces/`](interfaces/) directory documents the communication contracts between system components.

- [`interfaces/dabble_bluetooth.md`](interfaces/dabble_bluetooth.md) describes Bluetooth communication through the Dabble control app, including supported commands, button mappings, and expected behavior.
- [`interfaces/pi_esp32.md`](interfaces/pi_esp32.md) describes the Raspberry Pi–ESP32 serial interface, including message formats, baud rate, commands, responses, and error handling.
- [`interfaces/esp32-roboclaw.md`](interfaces/esp32-roboclaw.md) documents communication between the ESP32 and RoboClaw motor controller, including motor commands, controller addresses, communication settings, and error handling.

## Design Documents

The [`design_docs/`](design_docs/) directory documents important engineering decisions and design approaches. Topics include communication design, motor control, Raspberry Pi and ESP32 responsibilities, safety mechanisms, spraying system behavior, and steering design.

The [`design_docs/decisions/`](design_docs/decisions/) directory contains Architecture Decision Records (ADRs), which document significant decisions, their rationale, alternatives considered, and consequences.

## Documentation Guidelines

Use this index to locate the relevant documentation before making changes. Keep technical details in their appropriate documents and update the documentation when implementation or accepted design decisions change. Treat design documents as descriptions of intended behavior and verify actual implementation against the source code and hardware.