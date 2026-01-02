/**
 * CabinBoyDiscovery - UDP Multicast Discovery Implementation
 *
 * Arr, here be the code fer shoutin' "AHOY" across the network
 * and listenin' fer the MainDeck's "WELCOME ABOARD" response!
 */

#include "CabinBoyDiscovery.h"

CabinBoyDiscovery::CabinBoyDiscovery()
    : _discovered(false)
    , _debug(false)
{
    _multicastIP.fromString(DISCOVERY_MULTICAST_GROUP);
    _mainDeckIP = INADDR_NONE;
}

IPAddress CabinBoyDiscovery::discover(unsigned long timeoutMs) {
    if (_debug) {
        Serial.println("[CabinBoy] Starting MainDeck discovery...");
    }

    _discovered = false;
    _mainDeckIP = INADDR_NONE;

    // Start UDP
    if (!_udp.beginMulticast(_multicastIP, DISCOVERY_PORT)) {
        if (_debug) {
            Serial.println("[CabinBoy] Failed to start multicast UDP!");
        }
        return INADDR_NONE;
    }

    unsigned long startTime = millis();
    unsigned long lastShout = 0;

    while (millis() - startTime < timeoutMs) {
        // Send AHOY every DISCOVERY_RETRY_INTERVAL_MS
        if (millis() - lastShout >= DISCOVERY_RETRY_INTERVAL_MS) {
            if (!sendAhoy()) {
                if (_debug) {
                    Serial.println("[CabinBoy] Failed to send AHOY!");
                }
            } else if (_debug) {
                Serial.println("[CabinBoy] Sent AHOY to multicast group");
            }
            lastShout = millis();
        }

        // Listen for response
        if (listenForWelcome(100)) {
            _discovered = true;
            _udp.stop();
            if (_debug) {
                Serial.print("[CabinBoy] MainDeck found at: ");
                Serial.println(_mainDeckIP);
            }
            return _mainDeckIP;
        }

        yield();  // Allow other tasks to run
    }

    _udp.stop();
    if (_debug) {
        Serial.println("[CabinBoy] Discovery timeout - MainDeck not found");
    }
    return INADDR_NONE;
}

bool CabinBoyDiscovery::sendAhoy() {
    _udp.beginPacket(_multicastIP, DISCOVERY_PORT);
    size_t written = _udp.write((const uint8_t*)DISCOVERY_AHOY, strlen(DISCOVERY_AHOY));
    return _udp.endPacket() && written == strlen(DISCOVERY_AHOY);
}

bool CabinBoyDiscovery::listenForWelcome(unsigned long timeoutMs) {
    unsigned long startTime = millis();

    while (millis() - startTime < timeoutMs) {
        int packetSize = _udp.parsePacket();
        if (packetSize > 0) {
            char buffer[64];
            int len = _udp.read(buffer, sizeof(buffer) - 1);
            if (len > 0) {
                buffer[len] = '\0';

                if (_debug) {
                    Serial.print("[CabinBoy] Received: ");
                    Serial.print(buffer);
                    Serial.print(" from ");
                    Serial.println(_udp.remoteIP());
                }

                // Check for "WELCOME ABOARD" response
                if (strncmp(buffer, DISCOVERY_WELCOME, strlen(DISCOVERY_WELCOME)) == 0) {
                    _mainDeckIP = _udp.remoteIP();
                    return true;
                }
            }
        }
        delay(10);
    }

    return false;
}
