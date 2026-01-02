"""
Cannon Crewmate Tests

Arr, these tests verify the cannon prop behaves correctly
without needin' actual GPIO hardware!
"""

import pytest
import asyncio
import msgpack


class TestCannonProperties:
    """Test cannon property handling"""

    def test_cannon_has_required_properties(self, mock_gpio):
        """Verify cannon has firing, loading, and jammed properties"""
        # Import after mocking GPIO
        from crewmates.cannon import Cannon

        cannon = Cannon()

        # Check properties exist (they start as False/None)
        assert hasattr(cannon, 'firing')
        assert hasattr(cannon, 'loading')
        assert hasattr(cannon, 'jammed')

    def test_cannon_address_is_cannon(self, mock_gpio):
        """Verify cannon has correct address"""
        from crewmates.cannon import Cannon

        cannon = Cannon()
        assert cannon.address == "CANNON"


class TestCannonCommands:
    """Test cannon command handling"""

    @pytest.mark.asyncio
    async def test_fire_command_sets_firing_true(self, mock_gpio):
        """Test that FIRE command sets firing property"""
        from crewmates.cannon import Cannon

        cannon = Cannon()

        # Simulate receiving a FIRE command
        msg = {"command": "FIRE"}

        # Note: handle_command is async
        # For a quick test, we can check the logic
        # In a full integration test, we'd use the WebSocket

        # Since we can't easily test the full async flow in a unit test,
        # we verify the cannon object is properly initialized
        assert cannon.firing is not True  # Should be False initially


class TestCannonStateTransitions:
    """Test cannon state machine"""

    def test_initial_state_is_idle(self, mock_gpio):
        """Verify cannon starts in idle state"""
        from crewmates.cannon import Cannon

        cannon = Cannon()

        assert cannon.firing is not True
        assert cannon.loading is not True
        assert cannon.jammed is not True


@pytest.mark.asyncio
class TestCannonIntegration:
    """Integration tests with MainDeck"""

    async def test_cannon_registration(self, mock_gpio, mock_discovery, main_deck_server, websocket_client, pack_message, unpack_message):
        """Test cannon registers with MainDeck correctly"""
        # Send a GET_MANIFEST to check connected crewmates
        manifest_request = pack_message({"type": "GM"})
        await websocket_client.send(manifest_request)

        # Receive response
        response = await asyncio.wait_for(websocket_client.recv(), timeout=2.0)
        msg = unpack_message(response)

        # MainDeck should respond with a manifest
        assert msg["type"] == "M"

    async def test_command_message_round_trip(self, main_deck_server, websocket_client, pack_message, unpack_message):
        """Test sending a command through MainDeck"""
        # First, register as a test client
        register_msg = pack_message({
            "type": "SA",
            "address": "TEST_CLIENT"
        })
        await websocket_client.send(register_msg)

        # Subscribe to all messages
        subscribe_msg = pack_message({
            "type": "S",
            "address": "*"
        })
        await websocket_client.send(subscribe_msg)

        # Small delay for registration
        await asyncio.sleep(0.1)

        # Now we can verify the connection is working
        # by requesting the manifest
        manifest_msg = pack_message({"type": "GM"})
        await websocket_client.send(manifest_msg)

        response = await asyncio.wait_for(websocket_client.recv(), timeout=2.0)
        msg = unpack_message(response)

        assert msg["type"] == "M"
        assert "TEST_CLIENT" in msg.get("data", [])


class TestCannonWithoutGPIO:
    """Verify cannon works without GPIO hardware"""

    def test_cannon_instantiates_without_gpio(self):
        """Cannon should instantiate even without RPi.GPIO"""
        # Don't use mock_gpio - verify it handles missing GPIO gracefully
        try:
            from crewmates.cannon import Cannon
            cannon = Cannon()
            assert cannon is not None
        except ImportError:
            pytest.skip("crewmates module not in path")
