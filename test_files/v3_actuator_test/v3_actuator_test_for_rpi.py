#!/usr/bin/env python3
"""Measure actuator travel time using the ESP32's current-sense limits."""

import argparse
import sys
import time

try:
    import serial
except ImportError:
    print("Missing pyserial. Install it with: sudo apt install python3-serial", file=sys.stderr)
    sys.exit(1)


DEFAULT_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
SERIAL_READ_TIMEOUT_S = 0.5
BOARD_STARTUP_DELAY_S = 2.0
TEST_TIMEOUT_S = 120.0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default=DEFAULT_PORT, help="ESP32 serial device")
    parser.add_argument(
        "--start-position",
        choices=("extended", "retracted"),
        default="extended",
        help="actuator's position before the test starts (default: extended)",
    )
    args = parser.parse_args()
    print(f"Ensure the actuator is fully {args.start_position} before starting.")

    try:
        with serial.Serial(
            args.port,
            BAUD_RATE,
            timeout=SERIAL_READ_TIMEOUT_S,
            write_timeout=2,
        ) as connection:
            # Opening the serial port may reset the ESP32.
            time.sleep(BOARD_STARTUP_DELAY_S)
            connection.reset_input_buffer()
            command = (
                "START_RETRACTED"
                if args.start_position == "retracted"
                else "START"
            )
            connection.write(f"{command}\n".encode("ascii"))
            connection.flush()
            print(f"Started actuator test on {args.port}.")

            deadline = time.monotonic() + TEST_TIMEOUT_S
            average_travel_ms = None
            while time.monotonic() < deadline:
                response = connection.readline()
                if not response:
                    continue

                message = response.decode("utf-8", errors="replace").strip()
                if not message:
                    continue

                print(f"ESP32: {message}")
                if message.startswith("AVERAGE_TRAVEL_MS="):
                    value = message.partition("=")[2]
                    try:
                        average_travel_ms = int(value)
                    except ValueError:
                        print(
                            f"Invalid average travel time from ESP32: {message}",
                            file=sys.stderr,
                        )
                        return 1
                    if average_travel_ms <= 0:
                        print(
                            f"Invalid average travel time from ESP32: {message}",
                            file=sys.stderr,
                        )
                        return 1
                    print(
                        "Average limit-to-limit travel time: "
                        f"{average_travel_ms} ms "
                        f"({average_travel_ms / 1000:.2f} s)"
                    )
                    continue
                if message.startswith("ERROR,"):
                    print(f"Actuator test failed: {message}", file=sys.stderr)
                    return 1
                if message == "TEST_COMPLETE":
                    if average_travel_ms is None:
                        print(
                            "ESP32 completed without reporting an average "
                            "travel time.",
                            file=sys.stderr,
                        )
                        return 1
                    return 0
                if message == "BUSY":
                    print("The ESP32 is already running an actuator test.", file=sys.stderr)
                    return 1

            print(
                f"Communication timeout: no TEST_COMPLETE status within "
                f"{TEST_TIMEOUT_S:.0f} seconds.",
                file=sys.stderr,
            )
            return 1
    except (serial.SerialException, OSError) as exc:
        print(f"Serial communication failed on {args.port}: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())