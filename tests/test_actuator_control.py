import unittest

from firmware.rpi.components.bluetooth import (
    BluetoothReceiver,
    DabblePacketParser,
    InvalidDabblePacket,
    decode_dabble_packet,
)
from firmware.rpi.components.esp32_interface import Esp32Interface
from firmware.rpi.robot_controller import RobotController, command_for_buttons
from firmware.rpi.components.bluetooth import ButtonState


class FakeBluetooth:
    def __init__(self, states):
        self.states = iter(states)

    def read_buttons(self, timeout=0.1):
        return next(self.states)


class FakeEsp32:
    def __init__(self):
        self.commands = []

    def send_command(self, command):
        self.commands.append(command)


class FakeSerial:
    def __init__(self, **kwargs):
        self.options = kwargs
        self.writes = []

    def write(self, data):
        self.writes.append(data)

    def flush(self):
        pass

    def close(self):
        pass


class FakeBluetoothSocket:
    def __init__(self, peer="AA:BB:CC:DD:EE:FF"):
        self.peer = peer
        self.bound = None
        self.listening = False

    def bind(self, address):
        self.bound = address

    def listen(self, backlog):
        self.listening = True

    def settimeout(self, timeout):
        pass

    def accept(self):
        return FakeBluetoothConnection(), (self.peer, 1)

    def close(self):
        pass


class FakeBluetoothConnection:
    def close(self):
        pass


class DabblePacketTests(unittest.TestCase):
    def test_decodes_left_and_right_direction_bits(self):
        header = b"\xff\x01\x01\x01\x02\x00"
        self.assertEqual(decode_dabble_packet(header + b"\x04"), ButtonState(True, False))
        self.assertEqual(decode_dabble_packet(header + b"\x08"), ButtonState(False, True))
        self.assertEqual(decode_dabble_packet(header + b"\x0c"), ButtonState(True, True))

    def test_parser_handles_fragmented_and_combined_packets(self):
        packet_left = b"\xff\x01\x01\x01\x02\x00\x04"
        packet_right = b"\xff\x01\x01\x01\x02\x00\x08"
        parser = DabblePacketParser()
        self.assertEqual(parser.feed(packet_left[:3]), [])
        self.assertEqual(parser.feed(packet_left[3:] + packet_right), [
            ButtonState(True, False),
            ButtonState(False, True),
        ])

    def test_rejects_malformed_packet(self):
        with self.assertRaises(InvalidDabblePacket):
            decode_dabble_packet(b"\xff\x01\x01\x01\x03\x00\x04")
        self.assertEqual(DabblePacketParser().feed(b"\xff\x01\x01\x01\x03\x00\x04"), [])

    def test_receiver_advertises_and_accepts_rfcomm_connection(self):
        server = FakeBluetoothSocket()
        advertised = []
        receiver = BluetoothReceiver(
            socket_factory=lambda: server,
            advertise_service=lambda *args, **kwargs: advertised.append((args, kwargs)),
        )
        receiver.connect()
        self.assertEqual(server.bound, ("", 1))
        self.assertTrue(server.listening)
        self.assertEqual(advertised[0][0][1], "SprayShark Dabble")
        receiver.close()


class ControllerTests(unittest.TestCase):
    def test_maps_button_combinations_to_commands(self):
        self.assertEqual(command_for_buttons(ButtonState(True, False)), "ACTUATOR LEFT")
        self.assertEqual(command_for_buttons(ButtonState(False, True)), "ACTUATOR RIGHT")
        self.assertEqual(command_for_buttons(ButtonState(False, False)), "ACTUATOR STOP")
        self.assertEqual(command_for_buttons(ButtonState(True, True)), "ACTUATOR STOP")

    def test_sends_stop_and_direction_changes(self):
        bluetooth = FakeBluetooth([
            ButtonState(True, False),
            ButtonState(False, True),
            ButtonState(True, True),
        ])
        esp32 = FakeEsp32()
        controller = RobotController(bluetooth, esp32)
        controller.start()
        controller.step()
        controller.step()
        controller.step()
        self.assertEqual(
            esp32.commands,
            ["ACTUATOR STOP", "ACTUATOR LEFT", "ACTUATOR RIGHT", "ACTUATOR STOP"],
        )

    def test_input_timeout_stops_actuator(self):
        now = [0.0]
        bluetooth = FakeBluetooth([ButtonState(True, False), None])
        esp32 = FakeEsp32()
        controller = RobotController(
            bluetooth, esp32, input_timeout=0.5, clock=lambda: now[0]
        )
        controller.start()
        controller.step()
        now[0] = 0.6
        controller.step()
        self.assertEqual(esp32.commands[-1], "ACTUATOR STOP")


class Esp32InterfaceTests(unittest.TestCase):
    def test_sends_newline_terminated_commands_at_115200(self):
        connections = []
        interface = Esp32Interface(
            "/dev/ttyACM0",
            serial_factory=lambda **kwargs: connections.append(
                FakeSerial(**kwargs)
            )
            or connections[-1],
        )
        connection = connections[0]
        interface.send_command("ACTUATOR LEFT")
        interface.send_command("ACTUATOR STOP")
        self.assertEqual(connection.options["baudrate"], 115200)
        self.assertEqual(connection.writes, [b"ACTUATOR LEFT\n", b"ACTUATOR STOP\n"])

    def test_rejects_unsupported_commands(self):
        interface = Esp32Interface("/dev/ttyACM0", serial_factory=FakeSerial)
        with self.assertRaises(ValueError):
            interface.send_command("LEFT")


if __name__ == "__main__":
    unittest.main()
