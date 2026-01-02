"""
MainDeck (Message Broker) Tests

Arr, these tests verify the MainDeck correctly routes messages
between crewmates!
"""

import pytest
import asyncio
import msgpack


@pytest.mark.asyncio
class TestMainDeckServer:
    """Test MainDeck WebSocket server functionality"""

    async def test_server_accepts_connections(self, main_deck_server, websocket_client):
        """Test that MainDeck accepts WebSocket connections"""
        # If we got here, connection succeeded
        assert websocket_client.open

    async def test_server_responds_to_manifest_request(self, main_deck_server, websocket_client, pack_message, unpack_message):
        """Test GET_MANIFEST returns a manifest"""
        # Request manifest
        msg = pack_message({"type": "GM"})
        await websocket_client.send(msg)

        # Receive response
        response = await asyncio.wait_for(websocket_client.recv(), timeout=2.0)
        manifest = unpack_message(response)

        assert manifest["type"] == "M"
        assert "data" in manifest

    async def test_client_registration(self, main_deck_server, websocket_client, pack_message, unpack_message):
        """Test clients can register with SET_ADDRESS"""
        # Register
        register_msg = pack_message({
            "type": "SA",
            "address": "TEST_CREWMATE"
        })
        await websocket_client.send(register_msg)

        # Small delay for processing
        await asyncio.sleep(0.1)

        # Check manifest
        manifest_msg = pack_message({"type": "GM"})
        await websocket_client.send(manifest_msg)

        response = await asyncio.wait_for(websocket_client.recv(), timeout=2.0)
        manifest = unpack_message(response)

        assert "TEST_CREWMATE" in manifest.get("data", [])


@pytest.mark.asyncio
class TestMessageRouting:
    """Test message routing between clients"""

    async def test_notification_broadcast(self, main_deck_server, pack_message, unpack_message):
        """Test NOTIFY messages are broadcast to subscribers"""
        import websockets

        # Create two clients
        client1 = await websockets.connect("ws://127.0.0.1:31337")
        client2 = await websockets.connect("ws://127.0.0.1:31337")

        try:
            # Client 1 registers as CANNON
            await client1.send(pack_message({
                "type": "SA",
                "address": "CANNON"
            }))

            # Client 2 registers and subscribes to CANNON
            await client2.send(pack_message({
                "type": "SA",
                "address": "UI_CLIENT"
            }))
            await client2.send(pack_message({
                "type": "S",
                "address": "CANNON"
            }))

            await asyncio.sleep(0.1)

            # Client 1 sends a NOTIFY
            await client1.send(pack_message({
                "type": "N",
                "address": "CANNON",
                "from": "CANNON",
                "data": {"prop": "firing", "val": True}
            }))

            # Client 2 should receive it
            response = await asyncio.wait_for(client2.recv(), timeout=2.0)
            msg = unpack_message(response)

            assert msg["type"] == "N"
            assert msg["data"]["prop"] == "firing"
            assert msg["data"]["val"] is True

        finally:
            await client1.close()
            await client2.close()

    async def test_command_routing(self, main_deck_server, pack_message, unpack_message):
        """Test COMMAND messages route to correct crewmate"""
        import websockets

        # Create cannon and commander
        cannon = await websockets.connect("ws://127.0.0.1:31337")
        commander = await websockets.connect("ws://127.0.0.1:31337")

        try:
            # Cannon registers and subscribes
            await cannon.send(pack_message({
                "type": "SA",
                "address": "CANNON"
            }))
            await cannon.send(pack_message({
                "type": "S",
                "address": "CANNON"
            }))

            # Commander registers
            await commander.send(pack_message({
                "type": "SA",
                "address": "COMMANDER"
            }))

            await asyncio.sleep(0.1)

            # Commander sends FIRE command to cannon
            await commander.send(pack_message({
                "type": "C",
                "address": "CANNON",
                "from": "COMMANDER",
                "data": {"command": "FIRE"}
            }))

            # Cannon should receive the command
            response = await asyncio.wait_for(cannon.recv(), timeout=2.0)
            msg = unpack_message(response)

            assert msg["type"] == "C"
            assert msg["data"]["command"] == "FIRE"

        finally:
            await cannon.close()
            await commander.close()


@pytest.mark.asyncio
class TestWildcardSubscription:
    """Test wildcard subscription behavior"""

    async def test_wildcard_receives_all_notifications(self, main_deck_server, pack_message, unpack_message):
        """Test * subscription receives all notifications"""
        import websockets

        # Create sender and wildcard listener
        sender = await websockets.connect("ws://127.0.0.1:31337")
        listener = await websockets.connect("ws://127.0.0.1:31337")

        try:
            # Sender registers
            await sender.send(pack_message({
                "type": "SA",
                "address": "SOME_PROP"
            }))

            # Listener subscribes to everything
            await listener.send(pack_message({
                "type": "SA",
                "address": "LISTENER"
            }))
            await listener.send(pack_message({
                "type": "S",
                "address": "*"
            }))

            await asyncio.sleep(0.1)

            # Sender notifies
            await sender.send(pack_message({
                "type": "N",
                "address": "SOME_PROP",
                "data": {"prop": "status", "val": "active"}
            }))

            # Listener should receive it
            response = await asyncio.wait_for(listener.recv(), timeout=2.0)
            msg = unpack_message(response)

            assert msg["type"] == "N"
            assert msg["address"] == "SOME_PROP"

        finally:
            await sender.close()
            await listener.close()
