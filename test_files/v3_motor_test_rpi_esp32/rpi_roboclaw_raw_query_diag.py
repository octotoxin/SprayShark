#!/usr/bin/env python3
"""
Raw RoboClaw packet-serial query diagnostic for SprayShark v3.

This bypasses the BasicMicro/roboclaw Python library so we can tell whether
the ESP32 relay and RoboClaw are actually returning bytes for specific query
commands. It does not run the motor; the only write command is DutyM1 = 0.
"""

import argparse
import glob
import os
import sys
import time

try:
    import serial
except ImportError:
    print("ERROR: pyserial is not installed. Try: pip install pyserial --break-system-packages")
    sys.exit(1)


DEFAULT_PORT = "/dev/ttyACM0"
DEFAULT_BAUD = 115200
DEFAULT_ADDRESS = 0x80


def crc16(data: bytes) -> int:
    """RoboClaw CRC16/CCITT, initial value 0."""
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def auto_detect_serial_port() -> str:
    if os.path.exists(DEFAULT_PORT):
        return DEFAULT_PORT

    candidates = (
        glob.glob("/dev/serial/by-id/*QinHeng*")
        + glob.glob("/dev/serial/by-id/*Espressif*")
        + glob.glob("/dev/serial/by-id/*")
        + glob.glob("/dev/ttyACM*")
        + glob.glob("/dev/ttyUSB*")
    )
    if candidates:
        return os.path.realpath(candidates[0])

    return DEFAULT_PORT


def hex_bytes(data: bytes) -> str:
    if not data:
        return "<none>"
    return " ".join(f"0x{byte:02X}" for byte in data)


def packet(address: int, command: int, payload: bytes = b"") -> bytes:
    body = bytes([address, command]) + payload
    crc = crc16(body)
    return body + bytes([(crc >> 8) & 0xFF, crc & 0xFF])


def read_request(address: int, command: int) -> bytes:
    # RoboClaw read commands send only address + command. The response carries
    # the CRC; adding request CRC bytes here pollutes the next packet.
    return bytes([address, command])


def read_response(port: serial.Serial, expected_len: int, timeout_s: float) -> bytes:
    deadline = time.monotonic() + timeout_s
    response = bytearray()

    while len(response) < expected_len and time.monotonic() < deadline:
        waiting = port.in_waiting
        if waiting:
            response.extend(port.read(min(waiting, expected_len - len(response))))
        else:
            time.sleep(0.002)

    return bytes(response)


def response_crc_ok(address: int, command: int, response: bytes) -> bool:
    if len(response) < 3:
        return False

    data = response[:-2]
    received_crc = (response[-2] << 8) | response[-1]
    expected_crc = crc16(bytes([address, command]) + data)
    return received_crc == expected_crc


def run_query(port: serial.Serial, address: int, command: int, expected_len: int, label: str) -> bytes:
    port.reset_input_buffer()
    port.reset_output_buffer()

    request = read_request(address, command)
    print(f"\n{label}")
    print(f"  TX: {hex_bytes(request)}")

    port.write(request)
    port.flush()
    response = read_response(port, expected_len, timeout_s=0.5)

    print(f"  RX: {hex_bytes(response)} ({len(response)}/{expected_len} bytes)")
    if len(response) >= 3:
        data = response[:-2]
        received_crc = (response[-2] << 8) | response[-1]
        expected_crc = crc16(bytes([address, command]) + data)
        crc_status = "OK" if response_crc_ok(address, command, response) else "BAD"
        print(f"  CRC: {crc_status} received=0x{received_crc:04X} expected=0x{expected_crc:04X}")
    else:
        print("  CRC: not enough bytes to check")

    return response


def run_write_ack(port: serial.Serial, address: int) -> None:
    port.reset_input_buffer()
    port.reset_output_buffer()

    # Command 32 / 0x20 is DutyM1. Payload 0x0000 is a safe stop command.
    request = packet(address, 0x20, b"\x00\x00")
    print("\nSafe write ACK check: DutyM1 = 0")
    print(f"  TX: {hex_bytes(request)}")

    port.write(request)
    port.flush()
    response = read_response(port, 1, timeout_s=0.5)
    print(f"  RX: {hex_bytes(response)} ({len(response)}/1 bytes)")
    print("  ACK:", "OK" if response == b"\xFF" else "MISSING/UNEXPECTED")


def parse_queries(address: int, responses: dict) -> None:
    print("\nInterpretation")

    main = responses.get(0x18, b"")
    if len(main) == 4 and response_crc_ok(address, 0x18, main):
        value = (main[0] << 8) | main[1]
        print(f"  0x18 main battery: {value / 10.0:.1f} V")
    else:
        print("  0x18 main battery: no valid CRC response")

    currents = responses.get(0x31, b"")
    if len(currents) == 6 and response_crc_ok(address, 0x31, currents):
        m1 = int.from_bytes(currents[0:2], byteorder="big", signed=True) / 100.0
        m2 = int.from_bytes(currents[2:4], byteorder="big", signed=True) / 100.0
        print(f"  0x31 motor currents: M1={m1:.2f} A, M2={m2:.2f} A")
    else:
        print("  0x31 motor currents: no valid CRC response")

    status = responses.get(0x5A, b"")
    if len(status) == 6 and response_crc_ok(address, 0x5A, status):
        value = int.from_bytes(status[0:4], byteorder="big", signed=False)
        print(f"  0x5A status: 0x{value:08X}")
    else:
        print("  0x5A status: no valid CRC response")

    if all(len(responses.get(cmd, b"")) == 0 for cmd in (0x18, 0x31, 0x5A)):
        print("  No query returned bytes. The RoboClaw is ACKing writes but not answering read packets.")
    elif response_crc_ok(address, 0x18, responses.get(0x18, b"")) and not response_crc_ok(address, 0x31, responses.get(0x31, b"")):
        print("  Basic reads work, but current/status reads do not. That points at command support or firmware/library mismatch.")
    elif response_crc_ok(address, 0x31, responses.get(0x31, b"")) or response_crc_ok(address, 0x5A, responses.get(0x5A, b"")):
        print("  Raw replies exist. If the main test still fails, the Python RoboClaw library is parsing/expecting the wrong response.")


def main() -> None:
    parser = argparse.ArgumentParser(description="Raw RoboClaw query diagnostic")
    parser.add_argument("--port", default=auto_detect_serial_port(), help="Pi serial port connected to ESP32")
    parser.add_argument("--baud", default=DEFAULT_BAUD, type=int, help="Pi to ESP32 baud rate")
    parser.add_argument("--address", default=DEFAULT_ADDRESS, type=lambda value: int(value, 0), help="RoboClaw address")
    args = parser.parse_args()

    print("Raw RoboClaw query diagnostic")
    print(f"  Port:    {args.port}")
    print(f"  Baud:    {args.baud}")
    print(f"  Address: 0x{args.address:02X}")

    try:
        with serial.Serial(args.port, args.baud, timeout=0) as port:
            time.sleep(0.25)

            responses = {}
            responses[0x18] = run_query(port, args.address, 0x18, 4, "ReadMainBatteryVoltage: command 0x18")
            responses[0x31] = run_query(port, args.address, 0x31, 6, "ReadCurrents: command 0x31")
            responses[0x5A] = run_query(port, args.address, 0x5A, 6, "ReadStatus/ReadError: command 0x5A")
            run_write_ack(port, args.address)
            parse_queries(args.address, responses)
    except serial.SerialException as exc:
        print(f"ERROR: Could not open/use {args.port}: {exc}")
        sys.exit(1)


if __name__ == "__main__":
    main()
