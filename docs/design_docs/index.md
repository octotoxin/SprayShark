# Design Documentation

## 1. Purpose

The `design_docs/` directory documents the engineering design decisions, design approaches, and architectural choices made during the development of SprayShark.

Its purpose is to give developers and coding agents an understanding of why the project is designed a certain way, what alternatives were considered, and what constraints influenced each decision.

These documents serve as a reference when implementing new features, modifying existing functionality, or making changes that could affect the overall system design.

## 2. Directory Structure

The directory is organized into design documents and a dedicated `decisions/` subdirectory.

```text
design_docs/
├── index.md
├── communication-design.md
├── motor_control.md
├── raspberry-pi-esp32-responsibilities.md
├── safety-design.md
├── spraying-system-design.md
├── steering-design.md
└── decisions/
    └── format.md
```

### Design Documents

- **`communication-design.md`** — Describes the overall communication approach between SprayShark's components and the reasoning behind the selected communication methods.
- **`motor_control.md`** — Documents the motor-control design, including the selected approach, control strategy, and relevant engineering considerations.
- **`raspberry-pi-esp32-responsibilities.md`** — Defines the division of responsibilities between the Raspberry Pi and ESP32 and explains which functionality belongs on each platform.
- **`safety-design.md`** — Documents the safety design, including safety responsibilities, expected behavior, and relevant design considerations.
- **`spraying-system-design.md`** — Explains the spraying system design, including pump control, spraying behavior, and planned capabilities.
- **`steering-design.md`** — Documents the steering mechanism, actuator selection, steering geometry, and design considerations.

### Decisions Directory

The `decisions/` directory contains Architecture Decision Records (ADRs). These capture individual engineering decisions that have a significant impact on the project.

- **`format.md`** — Defines the standard template that must be followed when creating or updating decision records.

Each decision record should document:

- Context: What problem or requirement led to the decision?
- Decision: What option was selected?
- Alternatives considered: What other options were evaluated?
- Rationale: Why was the selected option preferred?
- Consequences: What are the benefits, limitations, and trade-offs?

## 3. How to Update Design Documentation

Coding agents and developers should follow these guidelines when updating this directory:

1. **Review existing documentation:** Before making a design change, read the relevant design document and existing decision records.
2. **Update the appropriate document:** Modify the existing document when an established design changes. Avoid creating duplicate documents for the same topic.
3. **Document significant decisions:** When making an important engineering decision, create a new ADR using the structure defined in `decisions/format.md`.
4. **Update decision status:** Use `Proposed`, `Accepted`, or `Superseded` to indicate the current status of a decision.
5. **Preserve the reasoning:** Document why a decision was made, including relevant alternatives and trade-offs. Do not record only the final choice.
6. **Keep documentation accurate:** Clearly distinguish between proposed designs, accepted decisions, and implemented functionality. Do not describe planned capabilities as existing features.
7. **Maintain this index:** When adding, removing, or renaming design documents or ADRs, update this `index.md` to reflect the directory structure.

## 4. When to Create a Design Document or ADR

Not every code change requires a design document or ADR.

- Update an existing design document when implementation details or the established design approach change.
- Create a new design document when introducing a substantial new subsystem or design area.
- Create an ADR when making a significant engineering decision that affects future development, architecture, hardware selection, or system behavior.
- Small bug fixes, minor code cleanup, and routine implementation changes generally do not require new design documentation.

## 5. Relationship to Other Documentation

The design documents complement the other documentation in the `docs/` directory:

- `architecture.md` describes how the software components fit together.
- `interfaces/` documents communication contracts and message formats.
- `hardware/` documents physical components, specifications, and wiring.
- `exec_plans/` documents plans for significant engineering tasks.
- `quality.md` defines the required software verification and quality checks.

The `design_docs/` directory focuses primarily on the reasoning behind engineering choices and the intended design of the system.