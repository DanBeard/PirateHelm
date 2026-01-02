# Pirate Automation

This system is used to coordinate and control Halloween pirate props using modern Python and efficient binary messaging.

## Requirements
### Software
- Python 3.8 or higher (tested with Python 3.12)
- `uv` package manager (installs automatically, see below)

### Hardware
- Ethernet (wifi or wired)
- Raspberry Pi (for GPIO control - optional for testing)
- ESP32 boards (optional - for lightweight embedded props via CabinBoy library)

The loosely coupled architecture allows testing on any system and deploying to Raspberry Pi or ESP32 for actual prop control.

## Installing

### Quick Start (Desktop/Testing)
```bash
# Install with uv (uv will be installed automatically if needed)
uv pip install -e .
```

### Raspberry Pi (with GPIO support)
```bash
# Install with GPIO and audio support
uv pip install -e ".[rpi]"

# Additional system packages for servo control
sudo apt-get install -y python-smbus i2c-tools
sudo raspi-config  # Enable I2C interface
```

### Optional Audio Support
If you need audio playback (for ambiance.py and parrot.py):
```bash
# Note: playsound has compatibility issues with Python 3.12+
uv pip install -e ".[audio]"
```

### ESP32 (CabinBoy Library)
For lightweight embedded props using ESP32 microcontrollers:
```bash
cd cabin_boy/examples/cannon

# Edit platformio.ini to set WiFi credentials, then:
pio run -t upload -e esp32-s3
pio device monitor
```

See `cabin_boy/README.md` for full documentation of the CabinBoy Arduino/ESP32 library.

## Running
`python3 ./crewmates/main_deck.py`

Then on the same local network (can even be the same host, especially for testing), run a prop script directly
i.e.:
`python3 ./crewmates/cannon.py`

the main deck log should look something like this

```commandline
AVAST! NEW SHIP IN HARBOR ->  192.168.1.176
AHOY ->  192.168.1.176
('192.168.1.176', 55097) Permission to board?
{'type': 'SA', 'address': 'CANNON'}
{'type': 'S', 'address': 'CANNON'}
```

and the log from the crewmate should look something like:
```commandline
registering firing
registering jammed
registering loading
~LFC~
AHOY ->  192.168.1.128
WELCOME ABOARD ->  192.168.1.176
Boarding the good ship: ws://192.168.1.176:31337
```

## User Interface
Run server.sh to launch a simple HTTP server that hosts the user interface and other assets.

**mobile.html** (RECOMMENDED) - Simplified command menu with full msgpack support for quick actions during Halloween
![mobile.html](./docs/images/mobile.html.gif)

**captain.html** - Drag-and-droppable UI to visualize prop placement and status (note: needs msgpack JS updates for full functionality)
![captain.html](./docs/images/captain.html.gif)

## Architecture
The architecture is standard Pub/Sub message passing with all classes using nautically themed names:
- Broker: "Main Deck"
- Clients: "Crewmates"
- Discovery: UDP multicast with "AHOY" greetings

### Key Features
- **msgpack Binary Serialization**: Efficient binary encoding over WebSockets (replaces JSON for smaller, faster messages)
- **UDP Multicast Discovery**: Crewmates automatically find the Main Deck on port 31338
- **Modern asyncio**: Python 3.12-compatible async patterns for cooperative multitasking
- **Dynamic Properties**: `CrewmateProperty()` descriptors auto-notify subscribers when values change
- **Platform Detection**: Gracefully handles GPIO availability (Raspberry Pi vs desktop testing)

Each prop has its own Python class that discovers the main deck using UDP multicast and establishes a WebSocket connection. Commands are handled via `handle_command()` method, and dynamic properties automatically broadcast changes to subscribers.

The user interface uses JavaScript with WebSockets to connect to the main deck (same protocol as Python crewmates) and fabric.js for canvas rendering.

### ESP32 Support (CabinBoy)
The `cabin_boy/` directory contains a PlatformIO-compatible Arduino library for ESP32 microcontrollers. CabinBoy implements the same protocol as Python crewmates:
- UDP multicast discovery ("AHOY" / "WELCOME ABOARD")
- WebSocket connection to MainDeck on port 31337
- msgpack binary message serialization
- Property notifications and command handling

This allows lightweight embedded props to join the fleet alongside Raspberry Pi and browser-based crewmates.

### Message Types
See `util.py` for message type constants and field definitions. All messages use msgpack binary encoding. 


## Testing

PirateHelm supports testing without physical hardware - ye can test in dry dock before takin' the ship to sea!

### Python Crewmate Tests

Run the test suite with pytest:

```bash
# Install dev dependencies
uv pip install -e ".[dev]"

# Run all tests
pytest

# Run with verbose output
pytest -v

# Run specific test file
pytest tests/test_cannon.py

# Run only unit tests (fast, no network)
pytest tests/test_protocol.py

# Run integration tests (requires MainDeck)
pytest tests/test_main_deck.py -v
```

Tests include:
- **test_protocol.py**: msgpack message format verification
- **test_cannon.py**: Cannon crewmate behavior (mocked GPIO)
- **test_main_deck.py**: WebSocket server and message routing

### ESP32 CabinBoy Tests

For ESP32 unit tests (runs on desktop without hardware):

```bash
cd cabin_boy/examples/cannon

# Run desktop unit tests (msgpack only, no WiFi)
pio test -e native

# Run on real ESP32 hardware
pio test -e esp32-s3
```

For full integration testing with the Wokwi simulator:

```bash
# Install Wokwi CLI (or use VS Code extension)
# Build the firmware
pio run -e esp32-s3

# Run in Wokwi (connects to real MainDeck on your network!)
wokwi-cli .
```

See `cabin_boy/README.md` for more details on ESP32 testing options.

## Tips
### Run the script on boot
use `sudo crontab -e` for example:
```
# m h  dom mon dow   command
@reboot python3 /home/debian/Pirates/Helm/crewmates/cannon.py &
```
Note: This not work for media based props (e.g. projectors)

If that doesn't work for projection/video you can also try adding a .desktop file to ~/.config/autostart
for example the following file is /home/pi/.config/autostart/pumpkins.desktop
```
[Desktop Entry]
Type=Application
Name=Pumpkins
Comment=
Exec=vlc --extraintf=http --http-host 0.0.0.0 --http-port 8080 --http-password x --play-and-pause --fullscreen
Terminal=false
Hidden=false
```

### RPI servo hat requirements

If you're running the sero hat you'll need install the following:

```
sudo apt-get install -y python-smbus
sudo apt-get install -y i2c-tools
sudo pip3 install adafruit-circuitpython-servokit
```

You will also need to enable i2c via `sudo raspi-config` and reboot
see [the tutorial](https://learn.adafruit.com/adafruit-16-channel-pwm-servo-hat-for-raspberry-pi/overview) for more info

### ValueError: Namespace Gst not available

`sudo apt install python3-gst-1.0`