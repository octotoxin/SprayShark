import pytest

from firmware.rpi import main as rpi_main
from firmware.rpi.components.esp32_interface import ESP32Interface
from firmware.rpi.robot_controller import RobotController


class FakeSerial:
    def __init__(self, *responses: bytes) -> None:
        self.responses = list(responses)
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
        return self.responses.pop(0)

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


@pytest.mark.parametrize(
    ("direction", "expected_command", "response"),
    [
        ("LEFT", b"ACTUATOR,LEFT\n", b"MOCK_ACTUATOR,MOVING_LEFT\n"),
        ("RIGHT", b"ACTUATOR,RIGHT\n", b"MOCK_ACTUATOR,MOVING_RIGHT\n"),
        ("STOP", b"ACTUATOR,STOP\n", b"MOCK_ACTUATOR,STOPPED\n"),
    ],
)
def test_actuator_command_returns_esp32_confirmation(
    monkeypatch, direction, expected_command, response
) -> None:
    connection = FakeSerial(response)
    monkeypatch.setattr(
        "firmware.rpi.components.esp32_interface.serial.Serial",
        lambda **kwargs: connection,
    )
    interface = ESP32Interface("/dev/ttyACM0")

    assert interface.set_actuator(direction) == response.decode().strip()
    assert connection.writes == [expected_command]


def test_actuator_command_rejects_invalid_direction(monkeypatch) -> None:
    connection = FakeSerial()
    monkeypatch.setattr(
        "firmware.rpi.components.esp32_interface.serial.Serial",
        lambda **kwargs: connection,
    )
    interface = ESP32Interface("/dev/ttyACM0")

    with pytest.raises(ValueError, match="Unsupported actuator command"):
        interface.set_actuator("FORWARD")
    assert connection.writes == []


def test_actuator_command_rejects_unexpected_confirmation(monkeypatch) -> None:
    connection = FakeSerial(b"MOCK_ACTUATOR,STOPPED\n")
    monkeypatch.setattr(
        "firmware.rpi.components.esp32_interface.serial.Serial",
        lambda **kwargs: connection,
    )
    interface = ESP32Interface("/dev/ttyACM0")

    with pytest.raises(RuntimeError, match="Unexpected response"):
        interface.set_actuator("LEFT")


def test_robot_controller_initialization_pings_esp32() -> None:
    class FakeESP32:
        pinged = False

        def ping(self) -> None:
            self.pinged = True

    esp32 = FakeESP32()

    RobotController(esp32).initialize()

    assert esp32.pinged


def test_robot_controller_forwards_actuator_command() -> None:
    class FakeESP32:
        def set_actuator(self, direction: str) -> str:
            return f"MOCK_ACTUATOR,{direction}"

    controller = RobotController(FakeESP32())

    assert controller.set_actuator("LEFT") == "MOCK_ACTUATOR,LEFT"


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
    monkeypatch.setattr("builtins.input", lambda prompt: "quit")
    monkeypatch.setattr(rpi_main, "ESP32Interface", FakeInterface)
    monkeypatch.setattr(rpi_main, "RobotController", FakeController)

    assert rpi_main.main() == 0
    assert opened_ports == ["/dev/ttyACM0"]


def test_main_sends_terminal_commands_and_reports_responses(
    monkeypatch, capsys
) -> None:
    connection = FakeSerial(
        b"PONG\n",
        b"MOCK_ACTUATOR,MOVING_LEFT\n",
        b"MOCK_ACTUATOR,MOVING_RIGHT\n",
        b"MOCK_ACTUATOR,STOPPED\n",
    )
    monkeypatch.setattr(
        "firmware.rpi.components.esp32_interface.serial.Serial",
        lambda **kwargs: connection,
    )
    monkeypatch.setattr("sys.argv", ["main.py"])
    monkeypatch.delenv("SPRAYSHARK_SERIAL_PORT", raising=False)
    commands = iter(["left", "RIGHT", "stop", "quit"])
    monkeypatch.setattr("builtins.input", lambda prompt: next(commands))

    assert rpi_main.main() == 0

    assert connection.writes == [
        b"PING\n",
        b"ACTUATOR,LEFT\n",
        b"ACTUATOR,RIGHT\n",
        b"ACTUATOR,STOP\n",
    ]
    assert connection.closed
    output = capsys.readouterr().out
    assert "ESP32: MOCK_ACTUATOR,MOVING_LEFT" in output
    assert "ESP32: MOCK_ACTUATOR,MOVING_RIGHT" in output
    assert "ESP32: MOCK_ACTUATOR,STOPPED" in output
