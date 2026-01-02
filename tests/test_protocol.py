"""
PirateHelm Protocol Tests

Arr, these tests verify the msgpack message format be compatible
across Python crewmates, JavaScript UIs, and ESP32 CabinBoys!
"""

import pytest
import msgpack

from crewmates.util import MessageTypes, MessageFields


class TestMessageFormat:
    """Test message structure and encoding"""

    def test_command_message_format(self):
        """Test COMMAND message structure"""
        msg = {
            MessageFields.TYPE: MessageTypes.COMMAND,
            MessageFields.ADDRESS: "CANNON",
            MessageFields.DATA: {"command": "FIRE"}
        }

        # Pack and unpack
        packed = msgpack.packb(msg, use_bin_type=True)
        unpacked = msgpack.unpackb(packed, raw=False)

        assert unpacked["type"] == "C"
        assert unpacked["address"] == "CANNON"
        assert unpacked["data"]["command"] == "FIRE"

    def test_notify_message_format(self):
        """Test NOTIFY message structure"""
        msg = {
            MessageFields.TYPE: MessageTypes.NOTIFY,
            MessageFields.ADDRESS: "CANNON",
            MessageFields.FROM: "CANNON",
            MessageFields.DATA: {"prop": "firing", "val": True}
        }

        packed = msgpack.packb(msg, use_bin_type=True)
        unpacked = msgpack.unpackb(packed, raw=False)

        assert unpacked["type"] == "N"
        assert unpacked["address"] == "CANNON"
        assert unpacked["data"]["prop"] == "firing"
        assert unpacked["data"]["val"] is True

    def test_subscribe_message_format(self):
        """Test SUBSCRIBE message structure"""
        msg = {
            MessageFields.TYPE: MessageTypes.SUBSCRIBE,
            MessageFields.ADDRESS: "CANNON"
        }

        packed = msgpack.packb(msg, use_bin_type=True)
        unpacked = msgpack.unpackb(packed, raw=False)

        assert unpacked["type"] == "S"
        assert unpacked["address"] == "CANNON"

    def test_set_address_message_format(self):
        """Test SET_ADDRESS message structure"""
        msg = {
            MessageFields.TYPE: MessageTypes.SET_ADDRESS,
            MessageFields.ADDRESS: "CANNON"
        }

        packed = msgpack.packb(msg, use_bin_type=True)
        unpacked = msgpack.unpackb(packed, raw=False)

        assert unpacked["type"] == "SA"
        assert unpacked["address"] == "CANNON"

    def test_get_manifest_message_format(self):
        """Test GET_MANIFEST message structure"""
        msg = {
            MessageFields.TYPE: MessageTypes.GET_MANIFEST
        }

        packed = msgpack.packb(msg, use_bin_type=True)
        unpacked = msgpack.unpackb(packed, raw=False)

        assert unpacked["type"] == "GM"


class TestPropertyValues:
    """Test different property value types"""

    def test_boolean_property(self):
        """Test boolean property values"""
        msg = {
            "type": "N",
            "address": "CANNON",
            "data": {"prop": "firing", "val": True}
        }

        packed = msgpack.packb(msg, use_bin_type=True)
        unpacked = msgpack.unpackb(packed, raw=False)

        assert unpacked["data"]["val"] is True

    def test_integer_property(self):
        """Test integer property values"""
        msg = {
            "type": "N",
            "address": "PARROT",
            "data": {"prop": "volume", "val": 75}
        }

        packed = msgpack.packb(msg, use_bin_type=True)
        unpacked = msgpack.unpackb(packed, raw=False)

        assert unpacked["data"]["val"] == 75

    def test_float_property(self):
        """Test float property values"""
        msg = {
            "type": "N",
            "address": "SENSOR",
            "data": {"prop": "temperature", "val": 23.5}
        }

        packed = msgpack.packb(msg, use_bin_type=True)
        unpacked = msgpack.unpackb(packed, raw=False)

        assert abs(unpacked["data"]["val"] - 23.5) < 0.001

    def test_string_property(self):
        """Test string property values"""
        msg = {
            "type": "N",
            "address": "PUMPKINS",
            "data": {"prop": "song", "val": "spooky_scary.mp3"}
        }

        packed = msgpack.packb(msg, use_bin_type=True)
        unpacked = msgpack.unpackb(packed, raw=False)

        assert unpacked["data"]["val"] == "spooky_scary.mp3"


class TestMessageSize:
    """Test message size efficiency"""

    def test_command_message_is_compact(self):
        """Verify COMMAND messages are reasonably compact"""
        msg = {
            "type": "C",
            "address": "CANNON",
            "data": {"command": "FIRE"}
        }

        packed = msgpack.packb(msg, use_bin_type=True)

        # Should be under 50 bytes for a simple command
        assert len(packed) < 50

    def test_notify_message_is_compact(self):
        """Verify NOTIFY messages are reasonably compact"""
        msg = {
            "type": "N",
            "address": "CANNON",
            "from": "CANNON",
            "data": {"prop": "firing", "val": True}
        }

        packed = msgpack.packb(msg, use_bin_type=True)

        # Should be under 60 bytes for a simple notification
        assert len(packed) < 60


class TestWildcardSubscription:
    """Test wildcard subscription handling"""

    def test_wildcard_address(self):
        """Test subscribing to all crewmates with '*'"""
        msg = {
            "type": "S",
            "address": "*"
        }

        packed = msgpack.packb(msg, use_bin_type=True)
        unpacked = msgpack.unpackb(packed, raw=False)

        assert unpacked["address"] == "*"


class TestSpecialCommands:
    """Test special command patterns"""

    def test_property_ping_command(self):
        """Test the ~P~ property ping command"""
        msg = {
            "type": "C",
            "address": "CANNON",
            "data": {"c": "~P~"}
        }

        packed = msgpack.packb(msg, use_bin_type=True)
        unpacked = msgpack.unpackb(packed, raw=False)

        assert unpacked["data"]["c"] == "~P~"
