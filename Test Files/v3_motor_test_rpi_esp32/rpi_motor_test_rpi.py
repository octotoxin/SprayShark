#!/usr/bin/env python3
"""
rpi_motor_test_rpi.py — SprayShark v3 Motor Diagnostic & Test Suite
====================================================================
Advanced diagnostic and motor test utility for driving DC motors (Channel M1)
on a BasicMicro RoboClaw 2x30A controller via an ESP32-S3 hardware serial relay.

Architecture:
    Raspberry Pi 5  ──►  ESP32-S3 (USB/UART Relay)  ──►  RoboClaw 2x30A
    (Python Script)      /dev/ttyACM0 @ 115200 baud       Hardware UART @ 38400 baud

Features:
    • Automatic serial port discovery across /dev/serial/by-id, /dev/ttyACM*, /dev/ttyUSB*
    • Comprehensive hardware & communication pre-flight diagnostic probes
    • Real-time telemetry monitoring (Battery voltage, motor currents, board temperature)
    • Detailed fault bitmask decoding with human-readable error descriptions
    • Graceful multi-stage open-loop ramping with safety failsafe cutoffs
"""

import sys
import os
import glob
import time

# --------------------------------------------------------------------------- #
# Configuration                                                               #
# --------------------------------------------------------------------------- #

DEFAULT_PORT  = "/dev/ttyACM0"   # Default device node on the Raspberry Pi
BAUD_RATE     = 115200            # Pi ↔ ESP32 baud rate (NOT RoboClaw rate)
ROBOCLAW_ADDR = 0x80             # Default RoboClaw Packet Serial Address (128)

# Duty scale: BasicMicro packet serial uses -32767 ... +32767
DUTY_FULL_SCALE = 32767
DUTY_50_PCT     = int(DUTY_FULL_SCALE * 0.50)  # ≈ 16383

# --------------------------------------------------------------------------- #
# Error Register Bitmask Definitions                                          #
# --------------------------------------------------------------------------- #

FAULT_FLAGS = {
    0x000001: ("E-STOP", "Emergency Stop input is active / triggered"),
    0x000002: ("Temperature Warning", "Board temperature exceeds warning threshold (> 85°C)"),
    0x000004: ("Temperature2 Warning", "Secondary sensor temperature warning"),
    0x000008: ("Main Battery High", "Main battery voltage exceeds maximum safe cutoff (> 34V)"),
    0x000010: ("Logic Battery High", "Logic battery voltage exceeds maximum safe level"),
    0x000020: ("Logic Battery Low", "Logic battery voltage is below minimum operating level (< 6V)"),
    0x000040: ("M1 Driver Fault", "Motor 1 MOSFET driver / gate fault detected"),
    0x000080: ("M2 Driver Fault", "Motor 2 MOSFET driver / gate fault detected"),
    0x000100: ("Main Battery High Warning", "Main battery voltage approaching high limit"),
    0x000200: ("Main Battery Low Warning", "Main battery voltage approaching low limit (< 11V)"),
    0x000400: ("Temperature Error", "Board over-temperature thermal shutdown (> 100°C)"),
    0x000800: ("Temperature2 Error", "Secondary sensor over-temperature shutdown"),
    0x001000: ("M1 Home", "Motor 1 Home switch / limit active"),
    0x002000: ("M2 Home", "Motor 2 Home switch / limit active"),
    0x010000: ("Overcurrent M1", "Motor 1 current exceeded maximum current limit"),
    0x020000: ("Overcurrent M2", "Motor 2 current exceeded maximum current limit"),
}

# --------------------------------------------------------------------------- #
# Helper Functions                                                            #
# --------------------------------------------------------------------------- #

def print_header(title: str) -> None:
    """Print a visually distinct section header."""
    print("\n" + "═" * 70)
    print(f"  {title}")
    print("═" * 70)


def print_box(lines: list, title: str = "") -> None:
    """Print an alert/info box for formatted diagnostic output."""
    width = 68
    print("┌" + "─" * (width) + "┐")
    if title:
        print(f"│  {title:<{width-2}}│")
        print("├" + "─" * (width) + "┤")
    for line in lines:
        print(f"│  {line:<{width-2}}│")
    print("└" + "─" * (width) + "┘")


def duty_percent_to_raw(pct: float) -> int:
    """Convert duty cycle percentage (-100% ... +100%) to raw integer (-32767 ... +32767)."""
    raw = int((pct / 100.0) * DUTY_FULL_SCALE)
    return max(-DUTY_FULL_SCALE, min(DUTY_FULL_SCALE, raw))


def auto_detect_serial_port() -> str:
    """Scan and return the most probable serial port for the ESP32 relay."""
    if os.path.exists(DEFAULT_PORT):
        return DEFAULT_PORT

    # Priority search: by-id hardware names, then ACM nodes, then USB nodes
    candidates = (
        glob.glob("/dev/serial/by-id/*QinHeng*") +
        glob.glob("/dev/serial/by-id/*Espressif*") +
        glob.glob("/dev/serial/by-id/*") +
        glob.glob("/dev/ttyACM*") +
        glob.glob("/dev/ttyUSB*")
    )
    if candidates:
        detected = os.path.realpath(candidates[0])
        print(f"  [AUTO-DETECT] '{DEFAULT_PORT}' not found. Resolved active port: {detected}")
        return detected

    return DEFAULT_PORT


# --------------------------------------------------------------------------- #
# Diagnostic Probes                                                           #
# --------------------------------------------------------------------------- #

def run_preflight_diagnostics(rc, addr: int) -> bool:
    """
    Run comprehensive health and connectivity probes against the RoboClaw.
    Returns True if communication is healthy, False if critical comms fail.
    """
    print_header("PRE-FLIGHT DIAGNOSTIC PROBES")
    version=rc.ReadVersion(addr)
    print("DEBUG Firmware Version=",repr(version))
    # ── Probe 1: Main Battery Voltage ───────────────────────────────────────
    print("• [Probe 1/4] Reading Main Battery Voltage ...", end=" ", flush=True)
    try:
        volt_result = rc.ReadMainBatteryVoltage(addr)

        if volt_result and volt_result[1]:
            voltage_v = volt_result[1] / 10.0
            status_tag = "✅ NORMAL" if voltage_v >= 11.0 else "⚠️ LOW VOLTAGE WARNING"
            print(f"✅ OK\n  → Main Battery: {voltage_v:.1f} V ({status_tag})")

            if voltage_v < 6.0:
                print("  [CRITICAL] Battery voltage < 6.0V! Motor drive logic will not operate.")
        else:
            print("⚠️ INVALID DATA")
    except Exception as exc:
        print(f"⚠️ FAILED ({exc})")

    # ── Probe 2: Logic Battery Voltage ──────────────────────────────────────
    print("• [Probe 2/4] Reading Logic Voltage ...", end=" ", flush=True)
    try:
        logic_result = rc.ReadLogicBatteryVoltage(addr)
        print("DEBUG logic_result", repr(logic_result))
        if logic_result and logic_result[1]:
            logic_v = logic_result[1] / 10.0
            print(f"✅ OK\n  → Logic Rail: {logic_v:.1f} V")
        else:
            print("ℹ️ Shared with Main Battery")
    except Exception:
        print("ℹ️ Skipped (Standard unified rail)")

    # ── Probe 3: Board Temperature ──────────────────────────────────────────
    print("• [Probe 3/4] Reading Board Temperature ...", end=" ", flush=True)
    try:
        temp_result = rc.ReadTemperature(addr)
        if temp_result and temp_result[1]:
            temp_c = temp_result[1] / 10.0
            temp_f = (temp_c * 9/5) + 32
            print(f"✅ OK\n  → Temp: {temp_c:.1f}°C / {temp_f:.1f}°F")
        else:
            print("⚠️ Unavailable")
    except Exception:
        print("⚠️ Skipped")

    # ── Probe 4: RoboClaw Status Register ───────────────────────────────────
    print("• [Probe 4/4] Checking RoboClaw Status Register ...", end=" ", flush=True)
    has_faults = check_and_report_status(rc, addr, context="pre-flight check")
    if not has_faults:
        print("✅ CLEAN (No active faults)")

    print("\n  [Summary] Pre-flight communication verified successfully.")
    return True


def check_and_report_status(rc, addr: int, context: str = "") -> bool:
    """Read and decode the RoboClaw 32-bit status register."""
    try:
        result = rc.ReadError(addr)

        if not result or not result[0]:
            print(f"  [WARN] Unable to read RoboClaw status"
                  + (f" - {context}" if context else ""))
            return False

        status = result[1]

        print(f"\nRoboClaw Status{f' [{context}]' if context else ''}: "
              f"0x{status:08X}")

        if status == 0:
            print("  ✓ No active status flags")
            return False

        fault_flags = {
            0x00000001: ("E-STOP",
                         "Emergency Stop is active"),
            0x00000002: ("TEMPERATURE",
                         "Temperature fault"),
            0x00000004: ("TEMPERATURE 2",
                         "Second temperature fault"),
            0x00000040: ("M1 MOTOR FAULT",
                         "M1 driver fault"),
            0x00000080: ("M2 MOTOR FAULT",
                         "M2 driver fault"),
        }

        warning_flags = {
            0x01000000: ("SPEED ERROR WARNING",
                         "Speed error limit warning"),
            0x02000000: ("POSITION ERROR WARNING",
                         "Position error limit warning"),
        }

        faults = []
        warnings = 0

        for mask, (name, description) in fault_flags.items():
            if status & mask:
                faults.append(
                    f"• {name} (0x{mask:08X}): {description}"
                )

        for mask, (name, description) in warning_flags.items():
            if status & mask:
                warnings += 1
                print(
                    f"  ⚠ {name} (0x{mask:08X}): {description}"
                )

        if faults:
            print("  ❌ ACTIVE FAULTS:")
            for fault in faults:
                print(f"     {fault}")

        if not faults and not warnings:
            print("  ⚠ Unknown status bits are set")

        return bool(faults)

    except Exception as exc:
        print(f"  [WARN] Status check failed: {exc}")
        return False


def print_communication_troubleshooting() -> None:
    """Print a troubleshooting guide when RoboClaw does not answer."""
    lines = [
        "RoboClaw did not respond to Packet Serial command (Timeout).",
        "",
        "HARDWARE CHECKLIST:",
        "1. TX/RX Crossing: ESP32 Pin 17 (TX) → S1 (RX) | Pin 16 (RX) ← S2 (TX).",
        "2. Breakout Board: Ensure wires are on the OUTER row (ESP32-S3).",
        "3. Common Ground: ESP32 GND ('G') MUST connect to RoboClaw GND ('-').",
        "4. Power: Main battery MUST be connected to RoboClaw heavy terminals.",
        "5. RoboClaw Status: Verify Green STAT1 LED is on/blinking.",
        "6. Mode Config: Mode 7 (Packet Serial), 38400 baud, Address 0x80 (128).",
        "   (On v6e boards, check with Mode/Set buttons or Motion Studio).",
    ]
    print_box(lines, title="COMMUNICATION DIAGNOSTIC FAILURE")


# --------------------------------------------------------------------------- #
# Motor Control Routines                                                      #
# --------------------------------------------------------------------------- #

def set_duty_safe(rc, addr: int, raw_duty: int, label: str = "") -> None:
    """Send an open-loop duty command with error trapping and feedback."""
    pct = (raw_duty / DUTY_FULL_SCALE) * 100.0
    success = rc.DutyM1(addr, raw_duty)
    if not success:
        raise RuntimeError(f"DutyM1 command failed (Duty: {pct:+.1f}%, Raw: {raw_duty}) — {label}")


def ramp_duty_verbose(rc, addr: int, start_pct: float, end_pct: float,
                       duration_s: float, stage_name: str, steps: int = 40) -> None:
    """
    Ramp motor duty cycle linearly while streaming live telemetry diagnostics.
    """
    step_delay = duration_s / steps
    print(f"\n[{stage_name}] Ramping M1: {start_pct:+.1f}% → {end_pct:+.1f}% over {duration_s:.1f}s ...")

    for i in range(steps + 1):
        pct = start_pct + (end_pct - start_pct) * (i / steps)
        raw = duty_percent_to_raw(pct)
        set_duty_safe(rc, addr, raw, label=stage_name)

        # Print progress line every 10 steps
        if i % 10 == 0 or i == steps:
            # Query active motor current
            current_str = ""
            try:
                cur = rc.ReadCurrents(addr)
                if cur and cur[1]:
                    i_m1 = cur[1] / 100.0  # Returns 10mA units
                    current_str = f" | Current: {i_m1:.2f} A"
            except Exception:
                pass

            print(f"  → [{i*100//steps:3d}%] Duty: {pct:+6.1f}% (Raw: {raw:+6d}){current_str}")

        time.sleep(step_delay)


# --------------------------------------------------------------------------- #
# Main Motor Test Sequence                                                    #
# --------------------------------------------------------------------------- #

def execute_motor_test_sequence(rc, addr: int) -> None:
    """Execute the complete 6-stage motor test sequence."""
    print_header("STARTING M1 MOTOR TEST SEQUENCE")

    print("\n[Encoder Check] Capturing M1 encoder change during low-speed movement ...")
    before = rc.ReadEncoderM1(addr)

    # Command a controlled low-speed movement before the full test sequence.
    set_duty_safe(rc, addr, duty_percent_to_raw(10), label="Encoder check")
    time.sleep(2)
    set_duty_safe(rc, addr, 0, label="Encoder check stop")

    after = rc.ReadEncoderM1(addr)

    print(f"M1 Encoder before: {before}")
    print(f"M1 Encoder after : {after}")

    # ── Stage 1: Forward Ramp (0% → +50%) ──────────────────────────────────
    ramp_duty_verbose(rc, addr, start_pct=0, end_pct=50, duration_s=2.0, stage_name="Stage 1: Forward Ramp-Up")
    check_and_report_status(rc, addr, context="post forward ramp")

    # ── Stage 2: Forward Hold (+50% for 2s) ────────────────────────────────
    print("\n[Stage 2: Forward Hold] Holding M1 at +50% for 2.0s ...")
    set_duty_safe(rc, addr, DUTY_50_PCT, label="Hold +50%")
    for sec in range(1, 3):
        time.sleep(1.0)
        check_and_report_status(rc, addr, context=f"forward hold second {sec}")

    # ── Stage 3: Forward Ramp-Down (+50% → 0%) ──────────────────────────────
    ramp_duty_verbose(rc, addr, start_pct=50, end_pct=0, duration_s=1.0, stage_name="Stage 3: Forward Ramp-Down")
    check_and_report_status(rc, addr, context="post forward ramp-down")
    print("  → Pausing 1.0s in neutral ...")
    time.sleep(1.0)

    # ── Stage 4: Reverse Ramp (0% → -50%) ──────────────────────────────────
    ramp_duty_verbose(rc, addr, start_pct=0, end_pct=-50, duration_s=2.0, stage_name="Stage 4: Reverse Ramp-Up")
    check_and_report_status(rc, addr, context="post reverse ramp")

    # ── Stage 5: Reverse Hold (-50% for 2s) ────────────────────────────────
    print("\n[Stage 5: Reverse Hold] Holding M1 at -50% for 2.0s ...")
    set_duty_safe(rc, addr, -DUTY_50_PCT, label="Hold -50%")
    for sec in range(1, 3):
        time.sleep(1.0)
        check_and_report_status(rc, addr, context=f"reverse hold second {sec}")

    # ── Stage 6: Reverse Ramp-Down (-50% → 0%) ─────────────────────────────
    ramp_duty_verbose(rc, addr, start_pct=-50, end_pct=0, duration_s=1.0, stage_name="Stage 6: Reverse Ramp-Down")
    check_and_report_status(rc, addr, context="test completion")

    print_header("TEST SEQUENCE COMPLETED SUCCESSFULLY")


# --------------------------------------------------------------------------- #
# Main Entry Point                                                            #
# --------------------------------------------------------------------------- #

def main() -> None:
    print_header("SPRAYSHARK v3 — MOTOR TEST & DIAGNOSTICS")

    # 1. Verify BasicMicro Library Installation
    Roboclaw = None
    try:
        from basicmicro import Basicmicro as Roboclaw
    except ImportError:
        try:
            from basicmicro import Roboclaw
        except ImportError:
            try:
                from roboclaw_3 import Roboclaw
            except ImportError:
                try:
                    from roboclaw import Roboclaw
                except ImportError:
                    pass

    if Roboclaw is None:
        print("\n❌ [ERROR] 'basicmicro' Python library is not installed.")
        print("   Install it using:  pip install basicmicro --break-system-packages\n")
        sys.exit(1)

    # 2. Resolve Serial Port
    target_port = auto_detect_serial_port()

    print(f"• Serial Port:   {target_port}")
    print(f"• Pi ↔ ESP32:    {BAUD_RATE} Baud (USB Serial)")
    print(f"• ESP32 ↔ Motor: 38,400 Baud (Hardware UART1)")
    print(f"• Target Node:   RoboClaw Address 0x{ROBOCLAW_ADDR:02X} ({ROBOCLAW_ADDR})")

    # 3. Open Serial Connection
    print(f"\nOpening connection to {target_port} ...", end=" ", flush=True)
    rc = Roboclaw(target_port, BAUD_RATE)

    try:
        opened = rc.Open()
    except Exception as exc:
        print(f"❌ FAILED\n[ERROR] Exception opening {target_port}: {exc}")
        sys.exit(1)

    if opened is False:
        print(f"❌ FAILED\n[ERROR] Could not open {target_port}.")
        print("   1. Verify your user is in the dialout group: sudo usermod -a -G dialout $USER")
        print("   2. Make sure no other process (e.g. minicom or screen) is using the port.")
        sys.exit(1)

    print("✅ CONNECTED")

    # 4. Run Pre-flight Diagnostic Probes
    healthy = run_preflight_diagnostics(rc, ROBOCLAW_ADDR)
    if not healthy:
        print("\n❌ [ABORT] Pre-flight checks failed. Resolving communication errors above before running motors.")
        sys.exit(1)

    # 5. Execute Motor Test Sequence
    try:
        execute_motor_test_sequence(rc, ROBOCLAW_ADDR)

    except KeyboardInterrupt:
        print("\n\n⚠️  [INTERRUPTED] User aborted test with Ctrl+C!")

    except Exception as exc:
        print(f"\n❌ [ERROR] Runtime failure during test execution: {exc}")
        raise

    finally:
        # Failsafe Emergency Stop
        print("\n[Safety Failsafe] Sending DutyM1 = 0 & DutyM2 = 0 stop command ...", end=" ", flush=True)
        try:
            rc.DutyM1(ROBOCLAW_ADDR, 0)
            rc.DutyM2(ROBOCLAW_ADDR, 0)
            print("✅ MOTORS STOPPED")
        except Exception as stop_exc:
            print(f"⚠️  WARNING: Could not send stop command: {stop_exc}")

        print("Test session closed cleanly.\n")


if __name__ == "__main__":
    main()
