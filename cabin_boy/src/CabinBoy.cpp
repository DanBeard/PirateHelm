/**
 * CabinBoy - ESP32 Crewmate Library Implementation
 *
 * Arr, this be the heart of the CabinBoy library, handlin' all the
 * WebSocket communication and msgpack message passin'!
 */

#include "CabinBoy.h"

// Static instance pointer for callback routing
CabinBoy* CabinBoy::_instance = nullptr;

CabinBoy::CabinBoy(const char* address)
    : _address(address)
    , _commandCallback(nullptr)
    , _notifyCallback(nullptr)
    , _connectionCallback(nullptr)
    , _connected(false)
    , _registered(false)
    , _debug(false)
    , _lastReconnectAttempt(0)
{
    _instance = this;
}

bool CabinBoy::begin(const char* ssid, const char* password, unsigned long timeoutMs) {
    // Connect to WiFi
    if (_debug) {
        Serial.print("[CabinBoy] Connecting to WiFi: ");
        Serial.println(ssid);
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > timeoutMs) {
            if (_debug) {
                Serial.println("[CabinBoy] WiFi connection timeout!");
            }
            return false;
        }
        delay(500);
        if (_debug) {
            Serial.print(".");
        }
    }

    if (_debug) {
        Serial.println();
        Serial.print("[CabinBoy] WiFi connected! IP: ");
        Serial.println(WiFi.localIP());
    }

    // Discover MainDeck
    _discovery.setDebug(_debug);
    _mainDeckIP = _discovery.discover(timeoutMs);

    if (_mainDeckIP == INADDR_NONE) {
        if (_debug) {
            Serial.println("[CabinBoy] Failed to discover MainDeck!");
        }
        return false;
    }

    // Connect WebSocket
    if (_debug) {
        Serial.print("[CabinBoy] Connecting WebSocket to ");
        Serial.print(_mainDeckIP);
        Serial.print(":");
        Serial.println(CABIN_BOY_WS_PORT);
    }

    _webSocket.begin(_mainDeckIP.toString().c_str(), CABIN_BOY_WS_PORT, "/");
    _webSocket.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
        if (_instance) {
            _instance->handleWebSocketEvent(type, payload, length);
        }
    });
    _webSocket.setReconnectInterval(5000);

    // Wait for connection
    startTime = millis();
    while (!_connected && millis() - startTime < timeoutMs) {
        _webSocket.loop();
        delay(10);
    }

    return _connected;
}

void CabinBoy::loop() {
    _webSocket.loop();
}

bool CabinBoy::isConnected() {
    return _connected;
}

void CabinBoy::onCommand(CommandCallback callback) {
    _commandCallback = callback;
}

void CabinBoy::onNotify(NotifyCallback callback) {
    _notifyCallback = callback;
}

void CabinBoy::onConnection(ConnectionCallback callback) {
    _connectionCallback = callback;
}

void CabinBoy::handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            if (_debug) {
                Serial.println("[CabinBoy] WebSocket disconnected");
            }
            _connected = false;
            _registered = false;
            if (_connectionCallback) {
                _connectionCallback(false);
            }
            break;

        case WStype_CONNECTED:
            if (_debug) {
                Serial.println("[CabinBoy] WebSocket connected!");
            }
            _connected = true;
            sendRegistration();
            if (_connectionCallback) {
                _connectionCallback(true);
            }
            break;

        case WStype_BIN:
            // Binary data - msgpack message
            handleMessage(payload, length);
            break;

        case WStype_TEXT:
            // Shouldn't receive text with msgpack, but handle gracefully
            if (_debug) {
                Serial.println("[CabinBoy] Warning: Received text instead of binary");
            }
            break;

        case WStype_ERROR:
            if (_debug) {
                Serial.println("[CabinBoy] WebSocket error");
            }
            break;

        default:
            break;
    }
}

void CabinBoy::sendRegistration() {
    if (_debug) {
        Serial.print("[CabinBoy] Registering as: ");
        Serial.println(_address);
    }

    // Send SET_ADDRESS message
    MsgPack::Packer packer;
    packer.packMap(2);
    packer.packString(MSG_FIELD_TYPE);
    packer.packString(MSG_TYPE_SET_ADDRESS);
    packer.packString(MSG_FIELD_ADDRESS);
    packer.packString(_address);

    _webSocket.sendBIN(packer.data(), packer.size());

    delay(50);  // Small delay between messages

    // Subscribe to our own address
    sendSubscribe();

    _registered = true;
}

void CabinBoy::sendSubscribe() {
    MsgPack::Packer packer;
    packer.packMap(2);
    packer.packString(MSG_FIELD_TYPE);
    packer.packString(MSG_TYPE_SUBSCRIBE);
    packer.packString(MSG_FIELD_ADDRESS);
    packer.packString(_address);

    _webSocket.sendBIN(packer.data(), packer.size());

    if (_debug) {
        Serial.print("[CabinBoy] Subscribed to: ");
        Serial.println(_address);
    }
}

void CabinBoy::subscribe(const char* target) {
    MsgPack::Packer packer;
    packer.packMap(2);
    packer.packString(MSG_FIELD_TYPE);
    packer.packString(MSG_TYPE_SUBSCRIBE);
    packer.packString(MSG_FIELD_ADDRESS);
    packer.packString(target);

    _webSocket.sendBIN(packer.data(), packer.size());

    if (_debug) {
        Serial.print("[CabinBoy] Subscribed to: ");
        Serial.println(target);
    }
}

void CabinBoy::handleMessage(uint8_t* payload, size_t length) {
    MsgPack::Unpacker unpacker;
    unpacker.feed(payload, length);

    // Parse as map
    size_t mapSize = 0;
    if (!unpacker.isMap()) {
        if (_debug) {
            Serial.println("[CabinBoy] Error: Message is not a map");
        }
        return;
    }
    unpacker.unpackMapSize(mapSize);

    String msgType;
    String msgAddress;
    String msgFrom;
    bool hasData = false;
    size_t dataOffset = 0;

    // Parse message fields
    for (size_t i = 0; i < mapSize; i++) {
        String key;
        unpacker.unpackString(key);

        if (key == MSG_FIELD_TYPE) {
            unpacker.unpackString(msgType);
        } else if (key == MSG_FIELD_ADDRESS) {
            unpacker.unpackString(msgAddress);
        } else if (key == MSG_FIELD_FROM) {
            unpacker.unpackString(msgFrom);
        } else if (key == MSG_FIELD_DATA) {
            hasData = true;
            dataOffset = unpacker.index();
            unpacker.skip();  // Skip data for now, we'll parse it in handler
        } else {
            unpacker.skip();
        }
    }

    if (_debug) {
        Serial.print("[CabinBoy] Message: type=");
        Serial.print(msgType);
        Serial.print(" address=");
        Serial.println(msgAddress);
    }

    // Route message based on type
    if (msgType == MSG_TYPE_COMMAND && _commandCallback) {
        // Handle command
        if (hasData) {
            MsgPack::Unpacker dataUnpacker;
            dataUnpacker.feed(payload + dataOffset, length - dataOffset);

            // Parse data map to find command
            if (dataUnpacker.isMap()) {
                size_t dataMapSize;
                dataUnpacker.unpackMapSize(dataMapSize);

                for (size_t i = 0; i < dataMapSize; i++) {
                    String dataKey;
                    dataUnpacker.unpackString(dataKey);

                    if (dataKey == "command" || dataKey == "c") {
                        String command;
                        dataUnpacker.unpackString(command);

                        // Create new unpacker for callback with remaining data
                        MsgPack::Unpacker callbackUnpacker;
                        callbackUnpacker.feed(payload, length);
                        _commandCallback(command.c_str(), callbackUnpacker);
                        return;
                    } else {
                        dataUnpacker.skip();
                    }
                }
            }
        }
    } else if (msgType == MSG_TYPE_NOTIFY && _notifyCallback) {
        // Handle notification
        if (hasData) {
            MsgPack::Unpacker dataUnpacker;
            dataUnpacker.feed(payload + dataOffset, length - dataOffset);

            if (dataUnpacker.isMap()) {
                size_t dataMapSize;
                dataUnpacker.unpackMapSize(dataMapSize);

                String propName;
                bool hasProp = false;

                for (size_t i = 0; i < dataMapSize; i++) {
                    String dataKey;
                    dataUnpacker.unpackString(dataKey);

                    if (dataKey == "prop") {
                        dataUnpacker.unpackString(propName);
                        hasProp = true;
                    } else if (dataKey == "val" && hasProp) {
                        // Create unpacker positioned at value
                        MsgPack::Unpacker valueUnpacker;
                        valueUnpacker.feed(payload + dataUnpacker.index() - 1, length - dataUnpacker.index() + 1);
                        _notifyCallback(msgFrom.c_str(), propName.c_str(), valueUnpacker);
                        return;
                    } else {
                        dataUnpacker.skip();
                    }
                }
            }
        }
    }
}

void CabinBoy::sendCommand(const char* target, const char* command) {
    MsgPack::Packer packer;
    packer.packMap(4);

    packer.packString(MSG_FIELD_TYPE);
    packer.packString(MSG_TYPE_COMMAND);

    packer.packString(MSG_FIELD_ADDRESS);
    packer.packString(target);

    packer.packString(MSG_FIELD_FROM);
    packer.packString(_address);

    packer.packString(MSG_FIELD_DATA);
    packer.packMap(1);
    packer.packString("command");
    packer.packString(command);

    _webSocket.sendBIN(packer.data(), packer.size());

    if (_debug) {
        Serial.print("[CabinBoy] Sent command ");
        Serial.print(command);
        Serial.print(" to ");
        Serial.println(target);
    }
}

void CabinBoy::sendCommand(const char* target, const char* command, const char* key, const char* value) {
    MsgPack::Packer packer;
    packer.packMap(4);

    packer.packString(MSG_FIELD_TYPE);
    packer.packString(MSG_TYPE_COMMAND);

    packer.packString(MSG_FIELD_ADDRESS);
    packer.packString(target);

    packer.packString(MSG_FIELD_FROM);
    packer.packString(_address);

    packer.packString(MSG_FIELD_DATA);
    packer.packMap(2);
    packer.packString("command");
    packer.packString(command);
    packer.packString(key);
    packer.packString(value);

    _webSocket.sendBIN(packer.data(), packer.size());
}

void CabinBoy::setProperty(const char* name, bool value) {
    MsgPack::Packer packer;
    packer.packMap(4);

    packer.packString(MSG_FIELD_TYPE);
    packer.packString(MSG_TYPE_NOTIFY);

    packer.packString(MSG_FIELD_ADDRESS);
    packer.packString(_address);

    packer.packString(MSG_FIELD_FROM);
    packer.packString(_address);

    packer.packString(MSG_FIELD_DATA);
    packer.packMap(2);
    packer.packString("prop");
    packer.packString(name);
    packer.packString("val");
    packer.packBool(value);

    _webSocket.sendBIN(packer.data(), packer.size());

    if (_debug) {
        Serial.print("[CabinBoy] Property ");
        Serial.print(name);
        Serial.print(" = ");
        Serial.println(value ? "true" : "false");
    }
}

void CabinBoy::setProperty(const char* name, int value) {
    MsgPack::Packer packer;
    packer.packMap(4);

    packer.packString(MSG_FIELD_TYPE);
    packer.packString(MSG_TYPE_NOTIFY);

    packer.packString(MSG_FIELD_ADDRESS);
    packer.packString(_address);

    packer.packString(MSG_FIELD_FROM);
    packer.packString(_address);

    packer.packString(MSG_FIELD_DATA);
    packer.packMap(2);
    packer.packString("prop");
    packer.packString(name);
    packer.packString("val");
    packer.packInt32(value);

    _webSocket.sendBIN(packer.data(), packer.size());

    if (_debug) {
        Serial.print("[CabinBoy] Property ");
        Serial.print(name);
        Serial.print(" = ");
        Serial.println(value);
    }
}

void CabinBoy::setProperty(const char* name, float value) {
    MsgPack::Packer packer;
    packer.packMap(4);

    packer.packString(MSG_FIELD_TYPE);
    packer.packString(MSG_TYPE_NOTIFY);

    packer.packString(MSG_FIELD_ADDRESS);
    packer.packString(_address);

    packer.packString(MSG_FIELD_FROM);
    packer.packString(_address);

    packer.packString(MSG_FIELD_DATA);
    packer.packMap(2);
    packer.packString("prop");
    packer.packString(name);
    packer.packString("val");
    packer.packFloat32(value);

    _webSocket.sendBIN(packer.data(), packer.size());

    if (_debug) {
        Serial.print("[CabinBoy] Property ");
        Serial.print(name);
        Serial.print(" = ");
        Serial.println(value);
    }
}

void CabinBoy::setProperty(const char* name, const char* value) {
    MsgPack::Packer packer;
    packer.packMap(4);

    packer.packString(MSG_FIELD_TYPE);
    packer.packString(MSG_TYPE_NOTIFY);

    packer.packString(MSG_FIELD_ADDRESS);
    packer.packString(_address);

    packer.packString(MSG_FIELD_FROM);
    packer.packString(_address);

    packer.packString(MSG_FIELD_DATA);
    packer.packMap(2);
    packer.packString("prop");
    packer.packString(name);
    packer.packString("val");
    packer.packString(value);

    _webSocket.sendBIN(packer.data(), packer.size());

    if (_debug) {
        Serial.print("[CabinBoy] Property ");
        Serial.print(name);
        Serial.print(" = ");
        Serial.println(value);
    }
}
