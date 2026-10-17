"""Advertise an RFCOMM service and display raw data received from a host."""

from __future__ import annotations

import argparse
import sys
from typing import Sequence


DEFAULT_RFCOMM_CHANNEL = 1
DEFAULT_SERVICE_NAME = "SprayShark Code Receiver"
SERIAL_PORT_PROFILE_UUID = "00001101-0000-1000-8000-00805F9B34FB"


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--channel",
        type=int,
        default=DEFAULT_RFCOMM_CHANNEL,
        help="Bluetooth Classic RFCOMM channel (default: 1)",
    )
    parser.add_argument(
        "--service-name",
        default=DEFAULT_SERVICE_NAME,
        help="Bluetooth service name shown to connecting hosts",
    )
    args = parser.parse_args(argv)

    try:
        import bluetooth
    except ImportError:
        print(
            "PyBluez is required. On Raspberry Pi OS, install it with "
            "'sudo apt install python3-bluez'.",
            file=sys.stderr,
        )
        return 1

    server = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
    try:
        server.bind(("", args.channel))
        server.listen(1)
        bluetooth.advertise_service(
            server,
            args.service_name,
            service_id=SERIAL_PORT_PROFILE_UUID,
            service_classes=[SERIAL_PORT_PROFILE_UUID, bluetooth.SERIAL_PORT_CLASS],
            profiles=[bluetooth.SERIAL_PORT_PROFILE],
        )
        print(
            f"Advertising Bluetooth Classic service "
            f"{args.service_name!r} on RFCOMM channel {args.channel}.",
            flush=True,
        )

        while True:
            connection, peer = server.accept()
            print(f"Host connected: {peer[0]} (RFCOMM channel {peer[1]})", flush=True)
            try:
                while True:
                    data = connection.recv(1024)
                    if not data:
                        print(f"Host disconnected: {peer[0]}", flush=True)
                        break

                    print(
                        f"RX {peer[0]} ({len(data)} bytes): "
                        f"{data!r} | hex: {data.hex(' ')}",
                        flush=True,
                    )
            finally:
                connection.close()
    except KeyboardInterrupt:
        print("\nBluetooth receiver stopped.", flush=True)
    except (OSError, bluetooth.BluetoothError) as exc:
        print(f"Bluetooth receiver failed: {exc}", file=sys.stderr, flush=True)
        return 1
    finally:
        server.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())