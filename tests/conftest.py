"""
PirateHelm Test Fixtures

Arr, this module provides shared fixtures fer testin' the PirateHelm fleet
without needin' physical hardware!

Fixtures:
- mock_gpio: Mocks RPi.GPIO for testing without Raspberry Pi
- mock_discovery: Mocks UDP discovery to return localhost
- main_deck_server: Starts a real MainDeck server for integration tests
- websocket_client: Provides a WebSocket client for sending test messages
"""

import pytest
import asyncio
import sys
from unittest.mock import MagicMock, patch, AsyncMock
import msgpack


# ============================================
# GPIO Mocking
# ============================================

@pytest.fixture
def mock_gpio():
    """
    Mock the RPi.GPIO module for testing without hardware.

    This allows crewmates like cannon.py to run on any system
    without the actual GPIO library installed.
    """
    mock_gpio_module = MagicMock()
    mock_gpio_module.BCM = 11
    mock_gpio_module.BOARD = 10
    mock_gpio_module.OUT = 0
    mock_gpio_module.IN = 1
    mock_gpio_module.HIGH = 1
    mock_gpio_module.LOW = 0
    mock_gpio_module.setmode = MagicMock()
    mock_gpio_module.setup = MagicMock()
    mock_gpio_module.output = MagicMock()
    mock_gpio_module.cleanup = MagicMock()

    with patch.dict(sys.modules, {'RPi': MagicMock(), 'RPi.GPIO': mock_gpio_module}):
        yield mock_gpio_module


# ============================================
# Network Discovery Mocking
# ============================================

@pytest.fixture
def mock_discovery(monkeypatch):
    """
    Mock UDP multicast discovery to return localhost.

    This allows crewmates to "discover" the MainDeck without
    needing actual network multicast support.
    """
    async def mock_shout_client():
        """Return localhost instead of doing real UDP discovery"""
        return "127.0.0.1"

    # Patch the discovery function
    monkeypatch.setattr("crewmates.util.shout_client", mock_shout_client)

    yield "127.0.0.1"


# ============================================
# MainDeck Server Fixture
# ============================================

@pytest.fixture
async def main_deck_server():
    """
    Start a MainDeck WebSocket server for integration testing.

    This provides a real message broker that crewmates can connect to.
    The server runs on localhost:31337.
    """
    import websockets
    from crewmates.main_deck import MainDeck

    md = MainDeck()

    # Start the server
    server = await websockets.serve(md.start, "127.0.0.1", 31337)

    yield md

    # Cleanup
    server.close()
    await server.wait_closed()


# ============================================
# WebSocket Client Fixture
# ============================================

@pytest.fixture
async def websocket_client(main_deck_server):
    """
    Provide a WebSocket client connected to the MainDeck.

    This can be used to send commands and receive notifications
    during integration tests.
    """
    import websockets

    # Wait a moment for server to be ready
    await asyncio.sleep(0.1)

    # Connect to MainDeck
    ws = await websockets.connect("ws://127.0.0.1:31337")

    yield ws

    # Cleanup
    await ws.close()


# ============================================
# Message Helpers
# ============================================

@pytest.fixture
def pack_message():
    """
    Helper fixture for packing msgpack messages.

    Returns a function that packs a message dict to binary.
    """
    def _pack(msg_dict):
        return msgpack.packb(msg_dict, use_bin_type=True)
    return _pack


@pytest.fixture
def unpack_message():
    """
    Helper fixture for unpacking msgpack messages.

    Returns a function that unpacks binary to a message dict.
    """
    def _unpack(data):
        return msgpack.unpackb(data, raw=False)
    return _unpack


# ============================================
# Audio Mocking (for crewmates that use playsound)
# ============================================

@pytest.fixture
def mock_audio():
    """
    Mock audio playback for testing without speakers.

    This prevents actual sound from playing during tests.
    """
    with patch("playsound.playsound") as mock_play:
        yield mock_play


# ============================================
# ALSA Mixer Mocking (for volume control)
# ============================================

@pytest.fixture
def mock_alsa():
    """
    Mock ALSA mixer for testing without audio hardware.

    This allows crewmates like ambiance.py and parrot.py
    to run without the alsaaudio library.
    """
    mock_mixer = MagicMock()
    mock_mixer.getvolume = MagicMock(return_value=[50])
    mock_mixer.setvolume = MagicMock()

    mock_alsaaudio = MagicMock()
    mock_alsaaudio.Mixer = MagicMock(return_value=mock_mixer)

    with patch.dict(sys.modules, {'alsaaudio': mock_alsaaudio}):
        yield mock_alsaaudio
