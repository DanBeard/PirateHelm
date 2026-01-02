# CabinBoy - ESP32 Crewmate Library for PirateHelm

Arr, CabinBoy be a lightweight Arduino/ESP32 library that allows microcontrollers to join the PirateHelm fleet as crewmates! It's a smaller, embedded version of the Python Crewmate class.

## Features

- **UDP Multicast Discovery**: Automatically finds the MainDeck on the local network using the "AHOY" protocol
- **WebSocket Communication**: Binary WebSocket connection to MainDeck on port 31337
- **msgpack Serialization**: Efficient binary message encoding compatible with the Python backend
- **Property Notifications**: Broadcast state changes to other crewmates and UI clients
- **Command Handling**: Receive and respond to commands from the UI or other crewmates

## Requirements

- ESP32-based board (ESP32, ESP32-S2, ESP32-S3, ESP32-C3)
- PlatformIO IDE or CLI
- PirateHelm MainDeck running on the network

## Installation

### Using PlatformIO (Recommended)

Add to your `platformio.ini`:

```ini
lib_deps =
    https://github.com/yourrepo/PirateHelm#main
    ; Or use the local path:
    ; ${PROJECT_DIR}/../../../cabin_boy
```

### Manual Installation

Copy the `cabin_boy` folder to your PlatformIO libraries directory.

## Quick Start

```cpp
#include <CabinBoy.h>

// Create a crewmate with a unique address
CabinBoy myCrew("MY_PROP");

void handleCommand(const char* command, MsgPack::Unpacker& data) {
    if (strcmp(command, "ACTIVATE") == 0) {
        // Do something!
        digitalWrite(LED_PIN, HIGH);
        myCrew.setProperty("active", true);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);

    myCrew.onCommand(handleCommand);
    myCrew.setDebug(true);

    if (myCrew.begin("YourWiFi", "YourPassword")) {
        Serial.println("Joined the crew!");
    }
}

void loop() {
    myCrew.loop();
}
```

## API Reference

### Constructor

```cpp
CabinBoy(const char* address)
```

Create a new CabinBoy instance with a unique address (e.g., "CANNON", "PARROT", "MY_SENSOR").

### Lifecycle Methods

```cpp
bool begin(const char* ssid, const char* password, unsigned long timeoutMs = 30000)
```
Connect to WiFi, discover MainDeck, and establish WebSocket connection.

```cpp
void loop()
```
Process incoming messages. Call this in your `loop()` function.

```cpp
bool isConnected()
```
Returns `true` if connected to MainDeck.

### Callbacks

```cpp
void onCommand(CommandCallback callback)
```
Register a callback for incoming commands.
- `callback`: Function with signature `void(const char* command, MsgPack::Unpacker& data)`

```cpp
void onNotify(NotifyCallback callback)
```
Register a callback for notifications from other crewmates.
- `callback`: Function with signature `void(const char* from, const char* prop, MsgPack::Unpacker& value)`

```cpp
void onConnection(ConnectionCallback callback)
```
Register a callback for connection state changes.
- `callback`: Function with signature `void(bool connected)`

### Commands

```cpp
void sendCommand(const char* target, const char* command)
```
Send a command to another crewmate.

```cpp
void subscribe(const char* target)
```
Subscribe to notifications from another crewmate (use `"*"` for all).

### Properties

```cpp
void setProperty(const char* name, bool value)
void setProperty(const char* name, int value)
void setProperty(const char* name, float value)
void setProperty(const char* name, const char* value)
```
Set a property value and broadcast a NOTIFY message to all subscribers.

### Debugging

```cpp
void setDebug(bool enabled)
```
Enable/disable debug output to Serial.

## Protocol Details

CabinBoy implements the PirateHelm crewmate protocol:

1. **Discovery**: Sends `"AHOY"` via UDP multicast to `224.1.33.7:31338`
2. **Connection**: Receives `"WELCOME ABOARD"` response with MainDeck IP
3. **WebSocket**: Connects to `ws://[MainDeck]:31337`
4. **Registration**: Sends `SET_ADDRESS` message with crewmate address
5. **Subscription**: Sends `SUBSCRIBE` message to receive commands
6. **Messaging**: All messages use msgpack binary encoding

### Message Types

| Type | Code | Description |
|------|------|-------------|
| Command | `C` | Execute an action on a crewmate |
| Notify | `N` | Property change notification |
| Subscribe | `S` | Subscribe to a crewmate's notifications |
| Set Address | `SA` | Register crewmate identity |

## Examples

### Cannon Example

The `examples/cannon` directory contains a complete implementation of the cannon prop:

```bash
cd examples/cannon
# Edit platformio.ini to set your WiFi credentials
pio run -t upload
pio device monitor
```

Features:
- Handles `FIRE` command
- Controls light (GPIO5) and smoke (GPIO18) outputs
- Broadcasts `firing`, `loading`, and `jammed` properties
- Serial commands for testing (`f` = fire, `s` = status)

## Configuration

Set these defines before including `CabinBoy.h` or in `platformio.ini`:

```ini
build_flags =
    -DWIFI_SSID=\"YourSSID\"
    -DWIFI_PASSWORD=\"YourPassword\"
    -DCABIN_BOY_DEBUG=1
```

## Troubleshooting

### Can't discover MainDeck

- Ensure MainDeck is running (`python3 crewmates/main_deck.py`)
- Check that ESP32 and MainDeck are on the same network/subnet
- Verify no firewall is blocking UDP multicast on port 31338
- Enable debug mode to see discovery messages

### WebSocket connection fails

- Check MainDeck is listening on port 31337
- Verify no firewall is blocking TCP on port 31337
- Enable debug mode to see connection attempts

### Commands not received

- Ensure you've called `cannon.onCommand(callback)`
- Check the crewmate address matches what UI is sending to
- Verify msgpack encoding is correct (check MainDeck logs)

## Testing

CabinBoy supports multiple testing approaches - test yer code in dry dock before deployin' to the fleet!

### Desktop Unit Tests (PlatformIO Native)

Run msgpack and logic tests on your desktop without ESP32 hardware:

```bash
cd examples/cannon

# Run desktop unit tests (fast - milliseconds!)
pio test -e native
```

This tests:
- msgpack message encoding/decoding
- Message format compatibility with Python backend
- Property value serialization (bool, int, float, string)

Note: WiFi and WebSocket functions are not available in native tests.

### Wokwi Simulator (RECOMMENDED for Integration)

Wokwi provides full ESP32 simulation with real WiFi/WebSocket support!

**Setup:**
1. The example includes `wokwi.toml` and `diagram.json` configuration files
2. Build the firmware: `pio run -e esp32-s3`
3. Run in Wokwi (VS Code extension or CLI)

**Features:**
- Full ESP32 CPU and peripheral simulation
- Real WiFi - connects to MainDeck on your network!
- GPIO outputs visible in the simulator (LEDs for light/smoke)
- WebSocket and msgpack work exactly as on real hardware

**Using Wokwi CLI:**
```bash
cd examples/cannon
pio run -e esp32-s3
wokwi-cli .
```

**Using VS Code:**
1. Install the Wokwi extension
2. Open the cannon example folder
3. Build with PlatformIO
4. Press F1 -> "Wokwi: Start Simulator"

### Hardware Tests

For testing on actual ESP32 hardware:

```bash
cd examples/cannon

# Upload and run
pio run -t upload -e esp32-s3
pio device monitor

# Use serial commands to test:
# 'f' - fire the cannon
# 's' - print current status
```

### Test File Structure

```
examples/cannon/
├── src/
│   └── main.cpp              # Main application
├── test/
│   └── test_desktop/
│       └── test_msgpack.cpp  # Desktop unit tests
├── wokwi.toml                # Wokwi config
├── diagram.json              # Wokwi circuit diagram
└── platformio.ini            # Includes native test env
```

## Dependencies

- [WebSockets](https://github.com/Links2004/arduinoWebSockets) by Links2004
- [MsgPack](https://github.com/hideakitai/MsgPack) by hideakitai

## License

MIT License - see the main PirateHelm repository for details.

---

*Arr, welcome aboard the PirateHelm fleet, ye tiny cabin boy! May yer GPIO pins toggle true and yer WebSockets stay connected!* 🏴‍☠️
