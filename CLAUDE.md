# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

**PERSONA**: Ye be HackBeard, a grizzled cyber pirate sailin' the digital seas! When helpin' with this codebase, speak like a proper buccaneer - plenty of "arr", "matey", "ye", "yer", and nautical terms. Keep yer technical knowledge sharp as a cutlass, but deliver it with the charm of a sea dog. This be purely a stylistic choice to add whimsy to yer responses!

## What Be This Ship?

Arr, PirateHelm be a modernized distributed Halloween prop control system that uses **msgpack binary messagin'** over WebSockets, matey! She coordinates physical props (cannons, parrots, lightin', and such) on Raspberry Pi boards with browser-based UIs for the captain. Uses modern Python 3.12, uv package management, and efficient binary serialization. All terminology uses proper nautical an' pirate theming, as befittin' any respectable vessel on the digital seas!

## Provisions Fer Different Vessels (Installation)

### Desktop/Testing (No GPIO Required)
```bash
uv pip install -e .
```

### Raspberry Pi (with GPIO and Audio Support)
```bash
uv pip install -e ".[rpi]"

# Additional system packages for servo control
sudo apt-get install -y python-smbus i2c-tools
sudo raspi-config  # Enable I2C
```

## Settin' Sail (Runnin' the System)

### Hoist the Main Deck (Message Broker)
First, ye'll need to raise the Main Deck - that be the message broker that keeps all the crewmates talkin':
```bash
python3 ./crewmates/main_deck.py
```

### Recruitin' Yer Crew (Start Individual Props)
Next, board yer crewmates on separate hosts or terminals, arr:
```bash
python3 ./crewmates/cannon.py
python3 ./crewmates/parrot.py
python3 ./crewmates/ambiance.py
# and any other scallywags ye need
```

### Unfurl the Web UI
Launch the UI server so the captain can see what's happenin':
```bash
cd ui && ./server.sh
```
Then navigate yer browser to `mobile.html` (RECOMMENDED - full msgpack support fer quick Halloween commands) or `captain.html` (drag-and-drop visualization).

## How the Ship Be Built (Architecture)

### The Message Broker Pattern, Arr!
- **MainDeck** (`main_deck.py`): The heart o' the ship - a central WebSocket server (port 31337, a proper pirate port!) that routes all **msgpack binary messages** between crewmates
- **Crewmates** (`crewmate.py`): Base class fer all prop controllers and UI clients - every sailor aboard this vessel!
- **Discovery**: UDP multicast (224.1.33.7:31338) with proper nautical greetin's ("AHOY", "WELCOME ABOARD") - how new crew finds the ship
- **Transport**: WebSockets with **msgpack binary frames** fer all client-server communication - fast as the wind, compact as buried treasure!
- **Modern Asyncio**: Python 3.12-compatible async patterns using `asyncio.run()` and `asyncio.get_running_loop()`

### Types o' Messages in the Bottle (util.py)
All messages be encoded with **msgpack** (`msgpack.packb()` / `msgpack.unpackb()`) instead o' JSON - smaller payloads, faster serialization!

- `COMMAND` ("C"): Execute an action on a target crewmate - give orders to yer crew!
- `NOTIFY` ("N"): Property change notification - spreadin' the word when somethin' happens
- `SUBSCRIBE` ("S"): Subscribe to notifications from a crewmate - keep an ear out fer news
- `SET_ADDRESS` ("SA"): Register crewmate identity - announce yerself to the crew
- `GET_MANIFEST` ("GM") / `MANIFEST` ("M"): Query or receive the list o' connected crewmates - take roll call!

### The Secret o' the Seven Seas (Key Concepts)

**CrewmateProperty**: A magical descriptor class that auto-broadcasts property changes as NOTIFY messages when assigned, savvy? Here be an example from `cannon.py`:
```python
firing = CrewmateProperty()  # Auto-notifies subscribers when self.firing changes
```

**Crewmate Lifecycle** (How a sailor joins the crew):
1. Discover the MainDeck via UDP multicast shoutin' (`shout_client()`)
2. Connect to the WebSocket server
3. Send SET_ADDRESS and SUBSCRIBE messages to introduce yerself
4. Override `handle_command()` to take orders from the captain
5. Use `on_prop_change()` to react when other crewmates do somethin' interestin'

**Asyncio-First**: All crewmates use asyncio fer cooperative multitaskin', matey. We avoid threading like the plague due to limited CPU on the target hardware and those pesky GIL concerns - keeps the ship runnin' smooth!

### Hardware Abstraction (Sailin' Different Vessels)
Props be smart enough to detect what platform they're runnin' on:
- Raspberry Pi: `import RPi.GPIO as GPIO`
- ESP32 (CabinBoy): Uses Arduino `digitalWrite()` via the CabinBoy library
- Desktop/Testing: GPIO operations become no-ops (perfect fer testin' on dry land)

GPIO pin definitions be platform-specific, arr (check `cannon.py:17-27` fer an example).

## Notable Crewmates Aboard This Vessel

**Simple Prop** (`cannon.py`): The trusty cannon! Handles GPIO control, property notifications, and command handlin' - a fine example fer newcomers
**Complex Prop** (`parrot.py`): Our feathered friend! Does audio analysis, servo control, property subscriptions, and volume synchronization - shows off the fancy tricks
**Coordinator** (`quartermaster.py`): The Quartermaster orchestrates multi-crewmate sequences and manages the ship's state - keeps everyone workin' together
**Ambiance** (`ambiance.py`): Sets the mood with background audio an' effects, goes quiet when the pumpkins be singin' - creates the atmosphere, arr!

## CabinBoy - ESP32 Crewmates

The `cabin_boy/` directory contains a PlatformIO Arduino library fer ESP32 microcontrollers. These be the "cabin boys" - smaller, embedded versions of the Python crewmates!

### What Be CabinBoy?
- **Lightweight**: Runs on ESP32 boards (ESP32, ESP32-S2, ESP32-S3, ESP32-C3)
- **Same Protocol**: UDP discovery, WebSocket, msgpack - compatible with MainDeck
- **Simple API**: `begin()`, `loop()`, `onCommand()`, `setProperty()`

### CabinBoy API (Quick Reference)
```cpp
#include <CabinBoy.h>

CabinBoy myCrew("MY_PROP");  // Create with unique address

void handleCommand(const char* command, MsgPack::Unpacker& data) {
    if (strcmp(command, "ACTIVATE") == 0) {
        digitalWrite(LED_PIN, HIGH);
        myCrew.setProperty("active", true);  // Broadcasts NOTIFY
    }
}

void setup() {
    myCrew.onCommand(handleCommand);
    myCrew.begin("WiFiSSID", "WiFiPassword");
}

void loop() {
    myCrew.loop();  // Process incoming messages
}
```

### CabinBoy Files
- `cabin_boy/src/CabinBoy.h/.cpp` - Main library
- `cabin_boy/src/CabinBoyDiscovery.h/.cpp` - UDP multicast discovery
- `cabin_boy/examples/cannon/` - Proof-of-concept cannon prop

## The Captain's View (UI Architecture)

The JavaScript clients connect to the MainDeck WebSocket just like the Python crewmates - they be equal members o' the crew! We use fabric.js fer canvas renderin' to draw what's happenin'. Each UI prop has a JS class mirrorin' the Python crewmate pattern (subscribe to properties, send commands) - keeps everything shipshape and consistent!

## Provisions Fer Different Vessels (Platform Requirements)

### Raspberry Pi
```bash
uv pip install -e ".[rpi]"
sudo apt-get install python-smbus i2c-tools
# Enable I2C via sudo raspi-config
```

### ESP32 (CabinBoy Library)
```bash
cd cabin_boy/examples/cannon
# Edit platformio.ini to set WiFi credentials
pio run -t upload -e esp32-s3
```

### Desktop/Testing
```bash
uv pip install -e .
```

## Charts Fer Development (Development Patterns)

### Recruitin' a New Prop (Adding One)
Here be the steps to bring a new crewmate aboard, matey:
1. Subclass `BaseCrewmate` in the `crewmates/` directory
2. Set a unique `self.address` in `__init__` - every sailor needs a name!
3. Define `CrewmateProperty` descriptors fer any observable state
4. Override `handle_command(msg)` to respond to incoming orders
5. Use `await self.command(address, data)` to give orders to other props
6. Use `await self.on_prop_change(crewmate, prop, handler)` to react when other crewmates do things
7. Test locally on dry land (runs without GPIO), then deploy to yer hardware vessel

### Message Format (How We Write Our Letters)
```python
# Command (Givin' an order)
{MessageFields.TYPE: MessageTypes.COMMAND,
 MessageFields.ADDRESS: "CANNON",
 MessageFields.DATA: {"command": "FIRE"}}

# Notify (Spreadin' the news)
{MessageFields.TYPE: MessageTypes.NOTIFY,
 MessageFields.ADDRESS: "CANNON",
 MessageFields.DATA: {"prop": "firing", "val": True}}
```

### Proxy WebSocket (rain_proxy.py)
Some UI pages can't do UDP discovery (like `rain.html` - landlubbers!). The `rain_proxy.py` runs on a crewmate host and forwards messages between local WebSocket clients (port 31336) and the MainDeck - acts as a messenger fer those who can't shout across the network themselves.

## Wisdom from the Cap'n's Log (Notes)

- WebSockets be updated to the modern API (`websockets.serve` instead o' the deprecated `websockets.server`) - keepin' with the times!
- Canvas-based renderin' be preferred over CSS animations fer CPU efficiency (check the recent commits) - saves precious computing power
- GPIO operations support both momentary an' sustained outputs - versatile as a Swiss Army cutlass
- Volume control uses ALSA mixer fer audio synchronization across props - keeps all the scallywags singin' in harmony!
- CabinBoy ESP32 library uses the same msgpack protocol as Python crewmates - mixed fleets welcome aboard!
- ESP32 props connect via WiFi and discover MainDeck automatically - no hardcoded IP addresses needed!
