#!/usr/bin/env python3
"""Run the fixed actuator retract-then-extend sequence on the ESP32."""

import argparse
import sys
import time

try:
    import serial
except ImportError:
    print(
        "Missing pyserial. Install it with: sudo apt install python3-serial",
        file=sys.stderr,
    )
    sys.exit(1)


DEFAULT_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
SERIAL_READ_TIMEOUT_S = 0.5
BOARD_STARTUP_DELAY_S = 2.0
TEST_TIMEOUT_S = 15.0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default=DEFAULT_PORT, help="ESP32 serial device")
    args = parser.parse_args()

    try:
        with serial.Serial(
            args.port,
            BAUD_RATE,
            timeout=SERIAL_READ_TIMEOUT_S,
            write_timeout=2,
        ) as connection:
            time.sleep(BOARD_STARTUP_DELAY_S)
            connection.reset_input_buffer()
            connection.write(b"START\n")
            connection.flush()
            print(
                f"Started on {args.port}: retract 4 s, stop 2 s, "
                "extend 1.61 s, stop 2 s."
            )

            deadline = time.monotonic() + TEST_TIMEOUT_S
            while time.monotonic() < deadline:
                response = connection.readline()
                if not response:
                    continue

                message = response.decode("utf-8", errors="replace").strip()
                if not message:
                    continue

                print(f"ESP32: {message}", flush=True)
                if message == "TEST_COMPLETE":
                    return 0
                if message == "BUSY":
                    print(
                        "The ESP32 is already running the actuator sequence.",
                        file=sys.stderr,
                    )
                    return 1
                if message == "UNKNOWN_COMMAND":
                    print(
                        "The ESP32 did not recognize START. "
                        "Flash the matching actuator sketch.",
                        file=sys.stderr,
                    )
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
