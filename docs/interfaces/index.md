# Interface Documentation Index

## Purpose

This directory contains the authoritative specifications for communication between SprayShark components.

Before changing a communication interface, inspect its specification and current implementation. If the interface changes, update the specification and all affected implementations and tests.

Do not assume that different communication paths use the same protocol.

## Communication Interfaces

| Interface | Connected components | Specification | Status |
|---|---|---|---|
| Bluetooth / Dabble | Operator application ↔ ESP32 | `bluetooth-dabble.md` | Existing behavior to document and verify |
| USB serial | Raspberry Pi ↔ ESP32 | `pi-esp32-serial.md` | Verify the protocol used by the current application |
| Motor-controller serial | ESP32 ↔ RoboClaw | `esp32-roboclaw.md` | Existing configuration and behavior to document and verify |

The listed specifications may be created incrementally. Their contents must be based on the actual implementation and verified hardware configuration, not assumed behavior.

## What Each Specification Must Define

Each interface specification should document the relevant items below:

- **Purpose:** What the interface is used for.
- **Endpoints:** Which components send and receive information.
- **Transport and configuration:** Bluetooth mode, serial connection, baud rate, device address, or other relevant settings.
- **Message format:** Packet structure, delimiters, encoding, and field definitions.
- **Commands and data:** Supported commands, parameters, valid ranges, and expected behavior.
- **Responses and errors:** Acknowledgments, status messages, error handling, and invalid-message behavior.
- **Timing and failure handling:** Timeouts, stale commands, connection loss, and recovery behavior where applicable.
- **Examples:** Representative valid messages and expected outcomes.
- **Verification:** Tests or procedures used to check compatibility.
- **Implementation references:** Source files that implement the interface.

Only include requirements relevant to the specific interface.

## Change Rules

1. Inspect the current implementation before changing an interface.
2. Treat the corresponding specification as the shared contract for all participating components.
3. Update the specification whenever an approved change alters the contract.
4. Update all affected senders, receivers, and tests.
5. Do not silently introduce incompatible message formats or command names.
6. Distinguish verified behavior from proposed changes and unverified assumptions.
7. Record physical hardware testing separately from software tests and compilation results.

## Important Distinction

The main project's Bluetooth/Dabble control, standalone USB-serial test protocols, and ESP32-to-RoboClaw communication are separate interfaces.

Do not combine them into a single protocol specification. Create a separate document for each interface that actually exists in the relevant implementation.

## Adding an Interface

When a new communication interface is introduced:

1. Add a specification for it.
2. Add its entry to this index.
3. Define the contract before implementing dependent components where practical.
4. Add suitable tests or verification procedures.
