/**
 * CabinBoy Cannon Example
 *
 * Arr, this be a proof-of-concept implementation of the Cannon crewmate
 * using the CabinBoy library on an ESP32 board!
 *
 * This example demonstrates:
 * - Connecting to WiFi and discovering the MainDeck
 * - Registering as a crewmate with the "CANNON" address
 * - Handling FIRE commands from the UI or other crewmates
 * - Broadcasting property changes (firing, loading, jammed)
 * - Controlling GPIO pins for light flash and smoke effects
 *
 * GPIO Pins (configurable in platformio.ini):
 * - LIGHT_PIN (default: GPIO5) - Controls the flash/strobe light
 * - SMOKE_PIN (default: GPIO18) - Controls the smoke machine momentary switch
 */

#include <Arduino.h>
#include <CabinBoy.h>

// WiFi credentials (set in platformio.ini build_flags)
#ifndef WIFI_SSID
#define WIFI_SSID "YourWiFiSSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YourWiFiPassword"
#endif

// GPIO pins (set in platformio.ini build_flags)
#ifndef LIGHT_PIN
#define LIGHT_PIN 5
#endif

#ifndef SMOKE_PIN
#define SMOKE_PIN 18
#endif

// Debug mode
#ifndef CABIN_BOY_DEBUG
#define CABIN_BOY_DEBUG 0
#endif

// Create the CabinBoy instance with address "CANNON"
CabinBoy cannon("CANNON");

// State variables
volatile bool firing = false;
volatile bool loading = false;
volatile bool jammed = false;

// Timing constants (matching Python cannon.py)
const unsigned long SMOKE_PULSE_MS = 250;
const unsigned long LOAD_DELAY_MS = 2000;
const unsigned long FIRE_FLASH_MS = 3000;
const unsigned long FIRE_COOLDOWN_MS = 500;
const unsigned long JAM_DURATION_MS = 250;

/**
 * Pulse the smoke machine (momentary toggle)
 * ON for SMOKE_PULSE_MS, then OFF
 */
void pulseSmoke() {
    digitalWrite(SMOKE_PIN, HIGH);
    delay(SMOKE_PULSE_MS);
    digitalWrite(SMOKE_PIN, LOW);
}

/**
 * Load the cannon sequence (smoke effects)
 */
void loadCannon() {
    loading = true;
    cannon.setProperty("loading", true);
    Serial.println("[Cannon] Loading...");

    // First smoke pulse
    pulseSmoke();

    // Wait for loading
    delay(LOAD_DELAY_MS);

    // Second smoke pulse
    pulseSmoke();

    loading = false;
    cannon.setProperty("loading", false);
    Serial.println("[Cannon] Loaded!");
}

/**
 * Fire the cannon sequence (light flash)
 */
void fireCannon() {
    Serial.println("[Cannon] **BOOM**");

    firing = true;
    cannon.setProperty("firing", true);

    // Light flash ON
    digitalWrite(LIGHT_PIN, HIGH);

    // Hold flash
    delay(FIRE_FLASH_MS);

    // Light flash OFF
    digitalWrite(LIGHT_PIN, LOW);

    // Cooldown
    delay(FIRE_COOLDOWN_MS);

    firing = false;
    cannon.setProperty("firing", false);
    Serial.println("[Cannon] Ready");
}

/**
 * Trigger jam sequence (when fire requested while busy)
 */
void triggerJam() {
    Serial.println("[Cannon] JAMMED!");

    jammed = true;
    cannon.setProperty("jammed", true);

    delay(JAM_DURATION_MS);

    jammed = false;
    cannon.setProperty("jammed", false);
}

/**
 * Complete firing sequence: load then fire
 */
void startFiringSequence() {
    loadCannon();
    fireCannon();
}

/**
 * Command handler callback
 * Called when a command message is received
 */
void handleCommand(const char* command, MsgPack::Unpacker& data) {
    Serial.print("[Cannon] Received command: ");
    Serial.println(command);

    if (strcmp(command, "FIRE") == 0) {
        // Check if we're busy
        if (firing || loading || jammed) {
            triggerJam();
            return;
        }

        // Start firing sequence
        startFiringSequence();
    }
}

/**
 * Connection state callback
 * Called when WebSocket connects/disconnects
 */
void handleConnection(bool connected) {
    if (connected) {
        Serial.println("[Cannon] Connected to MainDeck!");
        // Flash light briefly to indicate connection
        digitalWrite(LIGHT_PIN, HIGH);
        delay(100);
        digitalWrite(LIGHT_PIN, LOW);
    } else {
        Serial.println("[Cannon] Disconnected from MainDeck");
    }
}

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(1000);  // Wait for serial to stabilize

    Serial.println();
    Serial.println("=================================");
    Serial.println("  CabinBoy Cannon - ESP32");
    Serial.println("=================================");
    Serial.println();

    // Initialize GPIO pins
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(SMOKE_PIN, OUTPUT);
    digitalWrite(LIGHT_PIN, LOW);
    digitalWrite(SMOKE_PIN, LOW);

    Serial.print("Light PIN: GPIO");
    Serial.println(LIGHT_PIN);
    Serial.print("Smoke PIN: GPIO");
    Serial.println(SMOKE_PIN);
    Serial.println();

    // Enable debug output if configured
    #if CABIN_BOY_DEBUG
    cannon.setDebug(true);
    #endif

    // Register callbacks
    cannon.onCommand(handleCommand);
    cannon.onConnection(handleConnection);

    // Connect to WiFi, discover MainDeck, and establish WebSocket
    Serial.println("Connecting to PirateHelm fleet...");
    if (cannon.begin(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("Successfully joined the crew!");
        Serial.print("MainDeck IP: ");
        Serial.println(cannon.getMainDeckIP());
    } else {
        Serial.println("Failed to join the crew!");
        Serial.println("Check WiFi credentials and MainDeck availability");
        Serial.println("Restarting in 10 seconds...");
        delay(10000);
        ESP.restart();
    }
}

void loop() {
    // Process WebSocket messages
    cannon.loop();

    // Check for serial commands (for testing)
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'f' || c == 'F') {
            Serial.println("[Serial] Manual FIRE command");
            if (!firing && !loading && !jammed) {
                startFiringSequence();
            } else {
                triggerJam();
            }
        } else if (c == 's' || c == 'S') {
            // Print status
            Serial.println();
            Serial.println("=== Cannon Status ===");
            Serial.print("Connected: ");
            Serial.println(cannon.isConnected() ? "Yes" : "No");
            Serial.print("Firing: ");
            Serial.println(firing ? "Yes" : "No");
            Serial.print("Loading: ");
            Serial.println(loading ? "Yes" : "No");
            Serial.print("Jammed: ");
            Serial.println(jammed ? "Yes" : "No");
            Serial.println("====================");
            Serial.println();
        }
    }
}
