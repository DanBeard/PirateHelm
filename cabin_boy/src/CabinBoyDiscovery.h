/**
 * CabinBoyDiscovery - UDP Multicast Discovery for PirateHelm
 *
 * Arr, this module handles the "AHOY" protocol fer findin' the MainDeck
 * on the local network using UDP multicast!
 */

#ifndef CABIN_BOY_DISCOVERY_H
#define CABIN_BOY_DISCOVERY_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// Discovery protocol constants
#define DISCOVERY_MULTICAST_GROUP "224.1.33.7"
#define DISCOVERY_PORT 31338
#define DISCOVERY_AHOY "AHOY"
#define DISCOVERY_WELCOME "WELCOME ABOARD"
#define DISCOVERY_TIMEOUT_MS 5000
#define DISCOVERY_RETRY_INTERVAL_MS 1000

/**
 * CabinBoyDiscovery - Handles UDP multicast discovery
 */
class CabinBoyDiscovery {
public:
    CabinBoyDiscovery();

    /**
     * Discover the MainDeck on the network
     * Sends "AHOY" via multicast and listens for "WELCOME ABOARD" response
     *
     * @param timeoutMs Maximum time to wait for response
     * @return MainDeck IP address, or INADDR_NONE if not found
     */
    IPAddress discover(unsigned long timeoutMs = DISCOVERY_TIMEOUT_MS);

    /**
     * Check if discovery found the MainDeck
     */
    bool isDiscovered() const { return _discovered; }

    /**
     * Get the discovered MainDeck IP
     */
    IPAddress getMainDeckIP() const { return _mainDeckIP; }

    /**
     * Enable/disable debug output
     */
    void setDebug(bool enabled) { _debug = enabled; }

private:
    WiFiUDP _udp;
    IPAddress _mainDeckIP;
    IPAddress _multicastIP;
    bool _discovered;
    bool _debug;

    bool sendAhoy();
    bool listenForWelcome(unsigned long timeoutMs);
};

#endif // CABIN_BOY_DISCOVERY_H
