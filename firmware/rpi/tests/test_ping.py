import pytest

from firmware.rpi import main as rpi_main
from firmware.rpi.components.esp32_interface import ESP32Interface
from firmware.rpi.robot_controller import RobotController


class FakeSerial:
    def __init__(self, response: bytes) -> None:
        self.response = response
        self.writes = []
        self.closed = False

    def reset_input_buffer(self) -> None:
        pass

    def write(self, data: bytes) -> int:
        self.writes.append(data)
        return len(data)

    def flush(self) -> None:
        pass

    def readline(self) -> bytes:
        return self.response

    def close(self) -> None:
        self.closed = True


def test_ping_sends_newline_terminated_command_and_accepts_pong(monkeypatch) -> None:
    connection = FakeSerial(b"PONG\r\n")
    monkeypatch.setattr("firmware.rpi.components.esp32_interface.serial.Serial",
                        lambda **kwargs: connection)

    with ESP32Interface("/dev/ttyUSB0") as interface:
        interface.ping()

    assert connection.writes == [b"PING\n"]
    assert connection.closed


def test_ping_reports_timeout(monkeypatch) -> None:
    connection = FakeSerial(b"")
    monkeypatch.setattr("firmware.rpi.components.esp32_interface.serial.Serial",
                        lambda **kwargs: connection)
    interface = ESP32Interface("/dev/ttyUSB0")

    with pytest.raises(TimeoutError, match="PONG"):
        interface.ping()


def test_ping_rejects_unexpected_response(monkeypatch) -> None:
    connection = FakeSerial(b"NOT-PONG\n")
    monkeypatch.setattr("firmware.rpi.components.esp32_interface.serial.Serial",
                        lambda **kwargs: connection)
    interface = ESP32Interface("/dev/ttyUSB0")

    with pytest.raises(RuntimeError, match="Unexpected response"):
        interface.ping()


def test_robot_controller_initialization_pings_esp32() -> None:
    class FakeESP32:
        pinged = False

        def ping(self) -> None:
            self.pinged = True

    esp32 = FakeESP32()

    RobotController(esp32).initialize()

    assert esp32.pinged


def test_main_defaults_to_expected_esp32_port(monkeypatch) -> None:
    opened_ports = []

    class FakeInterface:
        def __init__(self, port: str) -> None:
            opened_ports.append(port)

        def __enter__(self):
            return self

        def __exit__(self, exc_type, exc_value, traceback) -> None:
            pass

    class FakeController:
        def __init__(self, esp32) -> None:
            pass

        def initialize(self) -> None:
            pass

    monkeypatch.setattr("sys.argv", ["main.py"])
    monkeypatch.delenv("SPRAYSHARK_SERIAL_PORT", raising=False)
    monkeypatch.setattr(rpi_main, "ESP32Interface", FakeInterface)
    monkeypatch.setattr(rpi_main, "RobotController", FakeController)

    assert rpi_main.main() == 0
    assert opened_ports == ["/dev/ttyACM0"]
