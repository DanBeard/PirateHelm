/**
 * CabinBoy - ESP32 Crewmate Library for PirateHelm
 *
 * Arr, this be a lightweight embedded version of the Python Crewmate class,
 * allowin' ESP32 boards to join the PirateHelm fleet!
 *
 * Features:
 * - UDP multicast discovery (AHOY protocol)
 * - WebSocket connection to MainDeck
 * - msgpack binary serialization
 * - Property change notifications
 * - Command handling
 */

#ifndef CABIN_BOY_H
#define CABIN_BOY_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <MsgPack.h>
#include "CabinBoyDiscovery.h"

// Protocol constants (matching Python util.py)
#define CABIN_BOY_WS_PORT 31337
#define CABIN_BOY_UDP_PORT 31338
#define CABIN_BOY_MULTICAST_GROUP "224.1.33.7"

// Message types
#define MSG_TYPE_COMMAND      "C"
#define MSG_TYPE_NOTIFY       "N"
#define MSG_TYPE_SUBSCRIBE    "S"
#define MSG_TYPE_SET_ADDRESS  "SA"
#define MSG_TYPE_GET_MANIFEST "GM"
#define MSG_TYPE_MANIFEST     "M"
#define MSG_TYPE_LOG          "L"

// Message field keys
#define MSG_FIELD_TYPE    "type"
#define MSG_FIELD_ADDRESS "address"
#define MSG_FIELD_FROM    "from"
#define MSG_FIELD_DATA    "data"

// Callback types
typedef void (*CommandCallback)(const char* command, MsgPack::Unpacker& data);
typedef void (*NotifyCallback)(const char* from, const char* prop, MsgPack::Unpacker& value);
typedef void (*ConnectionCallback)(bool connected);

/**
 * CabinBoy - Main class for ESP32 crewmates
 */
class CabinBoy {
public:
    /**
     * Create a new CabinBoy crewmate
     * @param address Unique address for this crewmate (e.g., "CANNON", "PARROT")
     */
    CabinBoy(const char* address);

    /**
     * Initialize WiFi, discover MainDeck, and connect WebSocket
     * @param ssid WiFi network name
     * @param password WiFi password
     * @param timeoutMs Connection timeout in milliseconds (default 30s)
     * @return true if connected successfully
     */
    bool begin(const char* ssid, const char* password, unsigned long timeoutMs = 30000);

    /**
     * Process incoming messages - call this in your loop()
     */
    void loop();

    /**
     * Check if connected to MainDeck
     */
    bool isConnected();

    /**
     * Register a callback for incoming commands
     * @param callback Function to call when a command is received
     */
    void onCommand(CommandCallback callback);

    /**
     * Register a callback for incoming notifications
     * @param callback Function to call when a notification is received
     */
    void onNotify(NotifyCallback callback);

    /**
     * Register a callback for connection state changes
     * @param callback Function to call when connection state changes
     */
    void onConnection(ConnectionCallback callback);

    /**
     * Send a command to another crewmate
     * @param target Target crewmate address
     * @param command Command name
     */
    void sendCommand(const char* target, const char* command);

    /**
     * Send a command with additional data
     * @param target Target crewmate address
     * @param command Command name
     * @param key Additional data key
     * @param value Additional data value
     */
    void sendCommand(const char* target, const char* command, const char* key, const char* value);

    /**
     * Subscribe to notifications from another crewmate
     * @param target Target crewmate address (use "*" for all)
     */
    void subscribe(const char* target);

    /**
     * Set a property and broadcast notification
     * @param name Property name
     * @param value Property value
     */
    void setProperty(const char* name, bool value);
    void setProperty(const char* name, int value);
    void setProperty(const char* name, float value);
    void setProperty(const char* name, const char* value);

    /**
     * Get the crewmate's address
     */
    const char* getAddress() const { return _address; }

    /**
     * Get the MainDeck IP address (after discovery)
     */
    IPAddress getMainDeckIP() const { return _mainDeckIP; }

    /**
     * Enable/disable debug output
     */
    void setDebug(bool enabled) { _debug = enabled; }

private:
    const char* _address;
    IPAddress _mainDeckIP;
    WebSocketsClient _webSocket;
    CabinBoyDiscovery _discovery;

    CommandCallback _commandCallback;
    NotifyCallback _notifyCallback;
    ConnectionCallback _connectionCallback;

    bool _connected;
    bool _registered;
    bool _debug;
    unsigned long _lastReconnectAttempt;

    // Internal methods
    void handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    void handleMessage(uint8_t* payload, size_t length);
    void sendRegistration();
    void sendSubscribe();

    // msgpack helpers
    void sendMessage(const char* type, const char* address, MsgPack::Packer& data);

    // Static callback wrapper for WebSocket library
    static void webSocketEventCallback(WStype_t type, uint8_t* payload, size_t length);
    static CabinBoy* _instance;  // For static callback routing
};

#endif // CABIN_BOY_H
