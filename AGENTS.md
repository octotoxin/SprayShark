# SprayShark — Agent Instructions

## 1. Mission

SprayShark is a robotic lawn-spraying project using embedded control and higher-level application software.

Make changes that improve the requested functionality while preserving system reliability, hardware safety, and compatibility between components.

Treat the repository as the source of truth. Inspect the current code and relevant documentation before making decisions.

## 2. Understand the Repository Before Making Changes

Before starting a task:

1. Inspect the relevant source files, configuration, tests, and build setup.
2. Read `context.md` for background, but verify important claims against current code and hardware documentation.
3. Read the relevant documents under `docs/` when they exist.
4. Identify the components affected by the change and any dependencies on other components.
5. Determine how the change will be tested.

Do not assume that historical notes, previous AI-generated suggestions, or proposed designs describe the current implementation.

If documentation conflicts with the source code, wiring records, or observed behavior, investigate the discrepancy. Do not silently choose one version.

Clearly distinguish confirmed facts, assumptions, proposed changes, and unverified hardware behavior.

## 3. Source of Truth

Use each source for its intended purpose:

- `README.md`: project introduction and getting-started information.
- `context.md`: project background and historical technical context.
- `docs/architecture.md`: component responsibilities and system architecture.
- `docs/hardware/`: verified hardware inventory, pin assignments, and wiring.
- `docs/interfaces/`: communication contracts and component interfaces.
- `docs/design-docs/`: durable engineering decisions and their rationale.
- `docs/exec-plans/`: implementation plans and completion records.
- `docs/quality.md`: testing and verification requirements, if present.

Code, configuration, and verified wiring must be inspected before relying on historical notes.

Do not create duplicate specifications when an authoritative document already exists. Update the existing source of truth when an approved design changes.

## 4. Component Ownership

Maintain clear separation of responsibilities.

- High-level application behavior belongs in the appropriate application layer.
- Embedded firmware is responsible for hardware-level control and independent safety behavior.
- Hardware drivers should be separated from higher-level behavior where appropriate.
- Communication interfaces should have explicit, documented ownership and responsibilities.

Inspect the actual repository to determine which components exist and where responsibilities currently reside. Do not assume a proposed architecture has already been implemented.

Avoid unrelated refactoring. Make the smallest coherent change that satisfies the task.

## 5. Cross-Component Compatibility

Changes affecting communication between components require special care.

Before changing an interface:

1. Locate its current implementation and specification.
2. Identify all producers and consumers of the interface.
3. Preserve existing behavior unless a change is explicitly required.
4. Update the authoritative interface specification when the contract changes.
5. Update affected implementations and tests, or clearly identify the work that remains.

Do not invent command names, message formats, pin assignments, baud rates, response formats, or timeout behavior.

Do not assume that two communication paths use the same protocol. For example, Bluetooth/Dabble control, USB serial commands, and ESP32-to-motor-controller communication must be treated as distinct interfaces unless the verified architecture states otherwise.

If a task affects another component, explain the dependency and required coordinated changes in the implementation plan or final handoff.

## 6. Hardware Safety

Safety takes priority over convenience and performance.

- Never drive motors, pumps, actuators, or other high-current loads directly from GPIO pins.
- Verify pin assignments, driver compatibility, voltage levels, and relevant electrical ratings before changing hardware control.
- Do not assume that pin assignments from one test setup apply to the main robot.
- Ensure that startup, reset, communication loss, and detected faults lead to an appropriate safe state.
- Preserve independent embedded safety mechanisms where required.
- Do not remove or weaken a safety check without an explicit engineering reason and an appropriate replacement.
- Treat emergency-stop behavior, actuator limits, motor outputs, and pump control as safety-sensitive.
- Never claim physical safety or successful hardware operation based only on code review or compilation.

If a critical hardware fact is unknown, document the uncertainty and avoid making an unsafe assumption.

## 7. Implementation Standards

Write maintainable code consistent with the language, framework, and style already used in the relevant component.

- Follow existing naming, formatting, and module conventions unless there is a clear reason to change them.
- Keep modules focused on well-defined responsibilities.
- Handle invalid input, communication errors, timeouts, and failure conditions.
- Avoid unnecessary dependencies and speculative abstractions.
- Preserve existing working functionality.
- Add or update comments when they explain important behavior, constraints, or non-obvious decisions.
- Do not claim code is complete when required implementation or dependent changes are still missing.

Do not introduce a new framework, library, architecture, or communication protocol without a clear need.

## 8. Plans and Engineering Decisions

For a small, isolated change, a brief task description and relevant tests may be sufficient.

For a change involving several components, safety behavior, hardware interfaces, or significant architectural decisions:

1. Create or update an execution plan under `docs/exec-plans/active/`.
2. Record the goal, scope, constraints, dependencies, and acceptance criteria.
3. Identify the relevant design documents and interfaces.
4. Implement and verify the change.
5. Record results, unresolved issues, and hardware-validation requirements.
6. Move the plan to `docs/exec-plans/completed/` when the work is complete, or explicitly record what remains unfinished.

Record durable architectural decisions in design documents rather than leaving them only in task conversations or execution plans.

Do not create documentation for its own sake. Documentation should resolve ambiguity, preserve a meaningful decision, or help verify implementation.

## 9. Verification

Before reporting a change as complete:

- Run the relevant automated tests.
- Run applicable static checks and compilation or build commands.
- Verify that affected interfaces still conform to their specifications.
- Inspect the final changes for unintended modifications.
- Report checks that failed, were skipped, or could not be executed.
- Explicitly identify any required physical hardware tests that remain outstanding.

Never invent test results or claim a test passed without evidence.

Distinguish clearly between code implemented, software checks passed, and physical hardware behavior verified.

## 10. Final Handoff

For each task, report:

1. **Changes made:** Files modified and the behavior implemented.
2. **Verification:** Tests, static checks, and build or compilation results.
3. **Interface impact:** Any contracts or dependent components affected.
4. **Limitations:** Unverified assumptions, unresolved issues, and physical tests still required.
5. **Documentation:** Specifications or plans updated as part of the change.

Keep the handoff factual and concise.

## 11. Improve the Development Environment

When recurring mistakes or workflow problems are discovered, consider improving the repository so they are easier to prevent.

Examples include:

- Adding tests for communication contracts.
- Automating firmware compilation.
- Adding reproducible test commands.
- Removing contradictory documentation.
- Recording an important design decision.
- Making hardware-specific assumptions explicit.

Prefer a repeatable check or a clear authoritative specification over repeatedly adding reminders to agent instructions.

Do not introduce broad tooling changes unrelated to the task.
