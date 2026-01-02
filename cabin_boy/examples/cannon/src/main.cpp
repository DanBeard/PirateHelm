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

// State variables (no volatile needed - not using interrupts)
bool firing = false;
bool loading = false;
bool jammed = false;

// Timing constants (matching Python cannon.py)
const unsigned long SMOKE_PULSE_MS = 250;
const unsigned long LOAD_DELAY_MS = 2000;
const unsigned long FIRE_FLASH_MS = 3000;
const unsigned long FIRE_COOLDOWN_MS = 500;
const unsigned long JAM_DURATION_MS = 250;
const unsigned long CONNECT_FLASH_MS = 100;

/**
 * State Machine States
 *
 * Instead of using blocking delay() calls that freeze the entire loop,
 * we use a state machine that tracks what phase we're in and when we
 * entered it. Each loop iteration checks if enough time has passed to
 * transition to the next state. This keeps cannon.loop() running so
 * WebSocket messages are always processed!
 *
 * State flow for FIRE command:
 *
 *   IDLE ──[FIRE cmd]──> LOAD_SMOKE1_ON ──(250ms)──> LOAD_WAITING
 *                                                         │
 *                        ┌────────────────(2000ms)────────┘
 *                        v
 *                  LOAD_SMOKE2_ON ──(250ms)──> FIRE_FLASH_ON
 *                                                    │
 *                        ┌─────────────(3000ms)──────┘
 *                        v
 *                  FIRE_COOLDOWN ──(500ms)──> IDLE
 *
 *   IDLE ──[FIRE while busy]──> JAM_ACTIVE ──(250ms)──> IDLE
 */
enum class CannonState {
    IDLE,              // Ready to receive commands - waiting for orders
    LOAD_SMOKE1_ON,    // First smoke pulse - SMOKE_PIN is HIGH
    LOAD_WAITING,      // Pause between smoke pulses - simulating "loading"
    LOAD_SMOKE2_ON,    // Second smoke pulse - SMOKE_PIN is HIGH again
    FIRE_FLASH_ON,     // Main event! LIGHT_PIN is HIGH for the flash
    FIRE_COOLDOWN,     // Brief pause after flash before accepting new commands
    JAM_ACTIVE,        // Cannon jammed - occurs when FIRE received while busy
    CONNECT_FLASH      // Brief light flash to indicate MainDeck connection
};

/**
 * State Machine Variables
 *
 * currentState: Which state we're currently in
 * stateEntryTime: millis() timestamp when we entered the current state
 *
 * To check if it's time to transition: (millis() - stateEntryTime) >= DURATION
 */
CannonState currentState = CannonState::IDLE;
unsigned long stateEntryTime = 0;

/**
 * Transition to a new state and record entry time
 *
 * This is the key to non-blocking timing! By recording when we entered
 * a state, updateStateMachine() can check how long we've been there
 * without blocking.
 */
void enterState(CannonState newState) {
    currentState = newState;
    stateEntryTime = millis();  // Record when we entered this state
}

/**
 * Check if cannon is busy (in any non-idle state)
 *
 * Used to determine if we should accept a new FIRE command or trigger
 * a jam. Any state other than IDLE means we're in the middle of something.
 */
bool isBusy() {
    return currentState != CannonState::IDLE;
}

/**
 * Trigger jam sequence (non-blocking)
 *
 * Called when a FIRE command is received while already busy.
 * Sets the jammed flag and notifies subscribers, then enters JAM_ACTIVE
 * state which will auto-clear after JAM_DURATION_MS.
 */
void triggerJam() {
    if (jammed) return;  // Already jammed, don't re-trigger

    Serial.println("[Cannon] JAMMED!");
    jammed = true;
    cannon.setProperty("jammed", true);  // Notify UI/subscribers
    enterState(CannonState::JAM_ACTIVE);
}

/**
 * Start firing sequence (non-blocking)
 *
 * This function returns IMMEDIATELY! It just:
 * 1. Checks if we're busy (triggers jam if so)
 * 2. Sets up initial state (loading=true, smoke ON)
 * 3. Enters the first state of the firing sequence
 *
 * The actual timing and GPIO control happens in updateStateMachine()
 * which gets called every loop iteration.
 */
void startFiring() {
    // Can't fire if we're already doing something
    if (isBusy()) {
        triggerJam();
        return;
    }

    // Begin the loading phase
    Serial.println("[Cannon] Loading...");
    loading = true;
    cannon.setProperty("loading", true);  // Notify UI/subscribers

    // Turn on smoke and enter first loading state
    // The state machine will handle the timing from here
    digitalWrite(SMOKE_PIN, HIGH);
    enterState(CannonState::LOAD_SMOKE1_ON);
}

/**
 * Update the state machine - call every loop iteration
 *
 * This is the heart of the non-blocking pattern! Instead of:
 *   delay(3000);  // BLOCKS everything for 3 seconds!
 *
 * We do:
 *   if (millis() - stateEntryTime >= 3000) { transition to next state }
 *
 * This function returns IMMEDIATELY every time it's called - it just
 * checks if enough time has passed and transitions if so. This allows
 * cannon.loop() to keep processing WebSocket messages even during a
 * 6+ second firing sequence!
 */
void updateStateMachine() {
    // Calculate how long we've been in the current state
    unsigned long elapsed = millis() - stateEntryTime;

    switch (currentState) {

        //--------------------------------------------------------------
        // IDLE: Waiting for commands. Nothing to do here - state
        // transitions are triggered by startFiring() or triggerJam()
        //--------------------------------------------------------------
        case CannonState::IDLE:
            break;

        //--------------------------------------------------------------
        // LOADING PHASE: Two smoke pulses with a delay between them
        //--------------------------------------------------------------

        case CannonState::LOAD_SMOKE1_ON:
            // First smoke pulse is active (SMOKE_PIN went HIGH in startFiring)
            // Wait for pulse duration, then turn off smoke and start waiting
            if (elapsed >= SMOKE_PULSE_MS) {
                digitalWrite(SMOKE_PIN, LOW);   // End first smoke pulse
                enterState(CannonState::LOAD_WAITING);
            }
            break;

        case CannonState::LOAD_WAITING:
            // Waiting between smoke pulses - this is the "loading" time
            // After the delay, trigger the second smoke pulse
            if (elapsed >= LOAD_DELAY_MS) {
                digitalWrite(SMOKE_PIN, HIGH);  // Start second smoke pulse
                enterState(CannonState::LOAD_SMOKE2_ON);
            }
            break;

        case CannonState::LOAD_SMOKE2_ON:
            // Second smoke pulse is active
            // When done, loading is complete - transition directly to firing!
            if (elapsed >= SMOKE_PULSE_MS) {
                digitalWrite(SMOKE_PIN, LOW);   // End second smoke pulse

                // Loading phase complete - notify subscribers
                loading = false;
                cannon.setProperty("loading", false);
                Serial.println("[Cannon] Loaded!");

                // Immediately begin firing phase - no gap between load and fire
                Serial.println("[Cannon] **BOOM**");
                firing = true;
                cannon.setProperty("firing", true);
                digitalWrite(LIGHT_PIN, HIGH);  // FLASH ON - the main event!
                enterState(CannonState::FIRE_FLASH_ON);
            }
            break;

        //--------------------------------------------------------------
        // FIRING PHASE: Light flash followed by cooldown
        //--------------------------------------------------------------

        case CannonState::FIRE_FLASH_ON:
            // Light is ON - this is the visible "boom" effect
            // Hold for the flash duration, then turn off
            if (elapsed >= FIRE_FLASH_MS) {
                digitalWrite(LIGHT_PIN, LOW);   // Flash OFF
                enterState(CannonState::FIRE_COOLDOWN);
            }
            break;

        case CannonState::FIRE_COOLDOWN:
            // Brief cooldown period after the flash
            // Prevents rapid re-firing and gives a realistic "reset" feel
            if (elapsed >= FIRE_COOLDOWN_MS) {
                firing = false;
                cannon.setProperty("firing", false);
                Serial.println("[Cannon] Ready");
                enterState(CannonState::IDLE);  // Ready for next command!
            }
            break;

        //--------------------------------------------------------------
        // JAM STATE: Brief notification when FIRE received while busy
        //--------------------------------------------------------------

        case CannonState::JAM_ACTIVE:
            // Jam notification is active - just wait for duration
            // This gives UI time to show the jam indicator
            if (elapsed >= JAM_DURATION_MS) {
                jammed = false;
                cannon.setProperty("jammed", false);
                enterState(CannonState::IDLE);  // Clear jam, ready again
            }
            break;

        //--------------------------------------------------------------
        // CONNECTION FLASH: Brief indicator when WebSocket connects
        //--------------------------------------------------------------

        case CannonState::CONNECT_FLASH:
            // Quick flash to show we've joined the fleet
            if (elapsed >= CONNECT_FLASH_MS) {
                digitalWrite(LIGHT_PIN, LOW);
                enterState(CannonState::IDLE);
            }
            break;
    }
}

/**
 * Command handler callback
 * Called when a command message is received
 */
void handleCommand(const char* command, MsgPack::Unpacker& data) {
    Serial.print("[Cannon] Received command: ");
    Serial.println(command);

    if (strcmp(command, "FIRE") == 0) {
        startFiring();  // Handles busy check internally
    }
}

/**
 * Connection state callback
 * Called when WebSocket connects/disconnects
 */
void handleConnection(bool connected) {
    if (connected) {
        Serial.println("[Cannon] Connected to MainDeck!");
        // Flash light briefly to indicate connection (skip if busy)
        if (!isBusy()) {
            digitalWrite(LIGHT_PIN, HIGH);
            enterState(CannonState::CONNECT_FLASH);
        }
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

/**
 * Main Loop - runs continuously
 *
 * THE KEY INSIGHT: Both cannon.loop() and updateStateMachine() return
 * immediately! This means:
 *
 * - WebSocket messages are processed every iteration (never missed!)
 * - State transitions happen based on elapsed time, not blocking delays
 * - The loop runs hundreds/thousands of times per second
 * - Even during a 6-second firing sequence, we stay responsive
 *
 * Compare to the OLD blocking approach:
 *   loop() {
 *     cannon.loop();
 *     if (fireCommand) {
 *       delay(2000);  // DEAF for 2 seconds!
 *       delay(3000);  // DEAF for 3 more seconds!
 *     }
 *   }
 */
void loop() {
    // Process WebSocket messages - ALWAYS runs, NEVER blocked!
    // This is how we receive commands from MainDeck
    cannon.loop();

    // Check state machine for time-based transitions
    // This handles all the timing that used to be delay() calls
    updateStateMachine();

    // Check for serial commands (for testing)
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'f' || c == 'F') {
            Serial.println("[Serial] Manual FIRE command");
            startFiring();  // Handles busy check internally
        } else if (c == 's' || c == 'S') {
            // Print status
            Serial.println();
            Serial.println("=== Cannon Status ===");
            Serial.print("Connected: ");
            Serial.println(cannon.isConnected() ? "Yes" : "No");
            Serial.print("State: ");
            Serial.println(static_cast<int>(currentState));
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
