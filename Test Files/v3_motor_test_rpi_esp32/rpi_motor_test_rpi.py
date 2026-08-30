#!/usr/bin/env python3
"""
rpi_motor_test_rpi.py  (ESP32 relay edition)
=============================================
Test script for driving a single DC motor on Channel M1 of a
BasicMicro RoboClaw 2x30A motor controller via a transparent serial relay.

Communication chain:
    Raspberry Pi 5  -->  ESP32-S3 (USB/Serial2 relay)  -->  RoboClaw 2x30A
    (this script)        /dev/ttyACM0 @ 115200 baud         UART @ 38400 baud

NOTE ON BAUD RATE:
    The serial port opened here uses 115200 baud because that is the rate at
    which the ESP32's USB Serial link talks to the Pi.  The RoboClaw itself
    is configured at 38400 baud; the ESP32 bridges the two using hardware
    Serial2.
    DO NOT change 115200 here to 38400 — that would break the Pi↔ESP32 link.

PROOF-OF-CONCEPT NOTE:
    This is a demo-only test script for SprayShark v3.  It validates the
    Pi ↔ ESP32 ↔ RoboClaw communication chain with the new brusher motors.

Dependencies:
    pip install basicmicro

Usage:
    python3 rpi_motor_test_rpi.py
"""

import time
import sys

# --------------------------------------------------------------------------- #
# Configuration — edit these to match your hardware setup                     #
# --------------------------------------------------------------------------- #

SERIAL_PORT   = "/dev/ttyACM0"   # ESP32's USB serial device on the Pi
BAUD_RATE     = 115200            # Pi↔ESP32 baud rate (NOT the RoboClaw rate)
ROBOCLAW_ADDR = 0x80             # Default RoboClaw packet-serial address

# Open-loop duty cycle range: basicmicro uses –32767 … +32767
# 50 % forward  = +16383,  50 % reverse = –16383
DUTY_FULL_SCALE = 32767
DUTY_50_PCT     = int(DUTY_FULL_SCALE * 0.50)   # ≈ 16383

# --------------------------------------------------------------------------- #
# Helpers                                                                      #
# --------------------------------------------------------------------------- #

def duty_percent_to_raw(pct: float) -> int:
    """Convert a duty-cycle percentage (–100 … +100) to a raw duty value."""
    raw = int((pct / 100.0) * DUTY_FULL_SCALE)
    return max(-DUTY_FULL_SCALE, min(DUTY_FULL_SCALE, raw))


def set_duty(rc, addr: int, raw_duty: int) -> None:
    """
    Send an open-loop duty command to M1.
    Raises RuntimeError if the command is not acknowledged.
    """
    success = rc.DutyM1(addr, raw_duty)
    if not success:
        raise RuntimeError(f"DutyM1 command failed (raw_duty={raw_duty})")


def check_errors(rc, addr: int, context: str = "") -> None:
    """
    Read the RoboClaw error/status register and print any active fault flags.
    Returns silently if no faults are set.
    """
    result = rc.ReadError(addr)
    # result is a tuple: (status_int, valid_bool)
    if not result[1]:
        print(f"  [WARN] ReadError() returned invalid data{' — ' + context if context else ''}")
        return

    status = result[0]
    if status == 0:
        return  # No faults

    # Decode the BasicMicro fault-flag bitmask (datasheet §5 / Python library)
    fault_flags = {
        0x000001: "E-STOP",
        0x000002: "Temperature Warning",
        0x000004: "Temperature2 Warning",
        0x000008: "Main Battery High",
        0x000010: "Logic Battery High",
        0x000020: "Logic Battery Low",
        0x000040: "M1 Driver Fault",
        0x000080: "M2 Driver Fault",
        0x000100: "Main Battery High Warning",
        0x000200: "Main Battery Low Warning",
        0x000400: "Temperature Error",
        0x000800: "Temperature2 Error",
        0x001000: "M1 Home",
        0x002000: "M2 Home",
        0x010000: "Overcurrent M1",
        0x020000: "Overcurrent M2",
    }
    active = [label for mask, label in fault_flags.items() if status & mask]
    tag = f" [{context}]" if context else ""
    print(f"  [FAULT{tag}] RoboClaw status=0x{status:06X}  →  {', '.join(active)}")


def ramp_duty(rc, addr: int, start_pct: float, end_pct: float,
              duration_s: float, steps: int = 50) -> None:
    """
    Linearly ramp the M1 duty cycle from start_pct to end_pct over duration_s.
    Performs `steps` incremental DutyM1 commands with equal sleep intervals.
    """
    step_delay = duration_s / steps
    for i in range(steps + 1):
        pct = start_pct + (end_pct - start_pct) * (i / steps)
        raw = duty_percent_to_raw(pct)
        set_duty(rc, addr, raw)
        time.sleep(step_delay)


# --------------------------------------------------------------------------- #
# Main test sequence                                                           #
# --------------------------------------------------------------------------- #

def run_test(rc, addr: int) -> None:
    """Execute the full motor test sequence on M1."""

    print("\n" + "=" * 60)
    print("RoboClaw M1 Open-Loop Test Sequence  (ESP32 relay)")
    print("=" * 60)

    # ------------------------------------------------------------------ #
    # Stage 1 – Ramp 0 % → +50 % forward over 2 s                        #
    # ------------------------------------------------------------------ #
    print("\n[Stage 1] Ramping M1 forward: 0 % → +50 % over 2 s ...")
    ramp_duty(rc, addr, start_pct=0, end_pct=50, duration_s=2.0)
    check_errors(rc, addr, context="after forward ramp-up")

    # ------------------------------------------------------------------ #
    # Stage 2 – Hold at +50 % for 2 s                                     #
    # ------------------------------------------------------------------ #
    print("[Stage 2] Holding M1 at +50 % for 2 s ...")
    set_duty(rc, addr, DUTY_50_PCT)
    time.sleep(1.0)
    check_errors(rc, addr, context="mid-hold forward")
    time.sleep(1.0)
    check_errors(rc, addr, context="end-hold forward")

    # ------------------------------------------------------------------ #
    # Stage 3 – Ramp +50 % → 0 % over 1 s, then pause 1 s               #
    # ------------------------------------------------------------------ #
    print("[Stage 3] Ramping M1 forward: +50 % → 0 % over 1 s ...")
    ramp_duty(rc, addr, start_pct=50, end_pct=0, duration_s=1.0)
    check_errors(rc, addr, context="after forward ramp-down")
    print("          Pausing 1 s ...")
    time.sleep(1.0)

    # ------------------------------------------------------------------ #
    # Stage 4 – Ramp 0 % → -50 % reverse over 2 s                        #
    # ------------------------------------------------------------------ #
    print("[Stage 4] Ramping M1 reverse: 0 % → -50 % over 2 s ...")
    ramp_duty(rc, addr, start_pct=0, end_pct=-50, duration_s=2.0)
    check_errors(rc, addr, context="after reverse ramp-up")

    # ------------------------------------------------------------------ #
    # Stage 5 – Hold at -50 % for 2 s                                     #
    # ------------------------------------------------------------------ #
    print("[Stage 5] Holding M1 at -50 % for 2 s ...")
    set_duty(rc, addr, -DUTY_50_PCT)
    time.sleep(1.0)
    check_errors(rc, addr, context="mid-hold reverse")
    time.sleep(1.0)
    check_errors(rc, addr, context="end-hold reverse")

    # ------------------------------------------------------------------ #
    # Stage 6 – Ramp -50 % → 0 % and stop                                #
    # ------------------------------------------------------------------ #
    print("[Stage 6] Ramping M1 reverse: -50 % → 0 % over 1 s ...")
    ramp_duty(rc, addr, start_pct=-50, end_pct=0, duration_s=1.0)
    check_errors(rc, addr, context="after reverse ramp-down")

    print("\n[Done]  Test sequence complete.")


def main() -> None:
    # Import basicmicro / roboclaw library with fallback support
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
        print("ERROR: 'basicmicro' package not found or could not be loaded.")
        print("       Run:  pip install basicmicro --break-system-packages")
        sys.exit(1)

    print(f"Opening serial connection to ESP32 relay on {SERIAL_PORT} @ {BAUD_RATE} baud ...")
    print("  (Relay forwards to RoboClaw @ 38400 baud — do not change 115200 here)")

    rc = Roboclaw(SERIAL_PORT, BAUD_RATE)

    try:
        rc.Open()
    except Exception as exc:
        print(f"ERROR: Could not open {SERIAL_PORT}: {exc}")
        sys.exit(1)

    print(f"Connected.  RoboClaw address: 0x{ROBOCLAW_ADDR:02X}")

    # ------------------------------------------------------------------ #
    # Comms sanity check — read main battery voltage                       #
    # ------------------------------------------------------------------ #
    print("\n[Sanity check] Reading main battery voltage ...")
    volt_result = rc.ReadMainBatteryVoltage(ROBOCLAW_ADDR)
    if volt_result[1]:                     # tuple: (value, valid_bool)
        voltage_v = volt_result[0] / 10.0  # RoboClaw returns tenths of a volt
        print(f"  Main battery voltage: {voltage_v:.1f} V")
    else:
        print("  WARNING: Could not read battery voltage — check wiring/relay.")

    # Initial error check before moving anything
    print("[Sanity check] Reading RoboClaw error register ...")
    check_errors(rc, ROBOCLAW_ADDR, context="startup")

    # ------------------------------------------------------------------ #
    # Motor test — always stop motor on exit                               #
    # ------------------------------------------------------------------ #
    try:
        run_test(rc, ROBOCLAW_ADDR)

    except KeyboardInterrupt:
        print("\n\n[Interrupted] Ctrl+C detected — stopping motor ...")

    except Exception as exc:
        print(f"\n[ERROR] Unexpected exception: {exc}")
        raise

    finally:
        # Safety stop — runs on normal exit, Ctrl+C, and any exception
        print("[Safety] Sending DutyM1 = 0 to stop motor ...")
        try:
            rc.DutyM1(ROBOCLAW_ADDR, 0)
        except Exception as stop_exc:
            print(f"  WARNING: Could not send stop command: {stop_exc}")
        print("[Safety] Motor stop command sent.  Script exiting.")


if __name__ == "__main__":
    main()
