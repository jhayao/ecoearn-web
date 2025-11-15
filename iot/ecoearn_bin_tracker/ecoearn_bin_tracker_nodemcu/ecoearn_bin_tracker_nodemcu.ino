/*
 * EcoEarn Smart Bin Controller - NodeMCU ESP8266 Version
 *
 * This Arduino sketch controls a smart trash bin with:
 * - User detection with Sharp IR sensor
 * - Capacity monitoring with 2 ultrasonic sensors
 * - Compartment control with 3 servo motors
 *
 * Hardware Required:
 * - NodeMCU ESP8266 Development Board
 * - ESP32-CAM AI Thinker (for material identification)
 * - 1x Sharp 2Y0A21 IR Distance Sensor
 * - 2x HC-SR04 Ultrasonic Sensors
 * - 3x Servo Motors (MG90S or similar)
 *
 * Pin Connections (NodeMCU):
 * ESP32-CAM Communication (SoftwareSerial):
 *   - NodeMCU D1 (GPIO 5) → ESP32-CAM RX (GPIO 3)
 *   - NodeMCU D2 (GPIO 4) → ESP32-CAM TX (GPIO 1)
 *   - Common GND
 *
 * Sensors:
 *   - Sharp IR Sensor OUT → NodeMCU A0 (Trash drop detection)
 *   - Ultrasonic 1 TRIG → D7 (GPIO 13), ECHO → D8 (GPIO 15) (Plastic capacity)
 *   - Ultrasonic 2 TRIG → D0 (GPIO 16), ECHO → D5 (GPIO 14) (Tin capacity)
 *
 * Servos:
 *   - Bin Lid Servo → D3 (GPIO 0)
 *   - Dropper Servo → D4 (GPIO 2)
 *   - Rotator Servo → D6 (GPIO 12)
 *
 * Status LED → D4 (GPIO 2) (built-in)
 *
 * Libraries Required:
 * - ESP8266WiFi (for WiFi)
 * - ESP8266HTTPClient (for HTTP requests)
 * - Servo (for servo control)
 * - ArduinoJson (for JSON parsing)
 * - SoftwareSerial (for ESP32-CAM communication)
 *
 * ============================================
 * SERIAL MONITOR TEST COMMANDS
 * ============================================
 * Type these commands in Serial Monitor (115200 baud) to test:
 *
 * Bin Control:
 *   activate   - Activate bin without QR scan (bypass for testing)
 *   deactivate - Deactivate bin and stop monitoring
 *   status     - Show current system status
 *   help       - Show all available commands
 *
 * Servo Testing (works anytime):
 *   lid-open        - Open bin lid
 *   lid-close       - Close bin lid
 *   drop-open       - Open dropper (release trash)
 *   drop-close      - Close dropper (hold position)
 *   rotate-plastic  - Rotate platform to plastic compartment
 *   rotate-tin      - Rotate platform to tin compartment
 *   rotate-reject   - Rotate platform to reject compartment
 *   test-all-servos - Run complete servo test sequence
 *
 * Material Testing (only when object is detected):
 *   plastic    - Simulate plastic bottle detection
 *   tin        - Simulate tin can detection
 *   reject     - Simulate rejected item
 *
 * Quick Servo Test Flow:
 *   1. Type "test-all-servos" to test all servos at once
 *   OR test individually:
 *   2. Type "lid-open" then "lid-close"
 *   3. Type "rotate-plastic", "rotate-tin", "rotate-reject"
 *   4. Type "drop-open" then "drop-close"
 *
 * Quick Detection Test Flow:
 *   1. Type "activate" to bypass QR scan
 *   2. Place object near Sharp IR sensor
 *   3. Type "plastic", "tin", or "reject" to test sorting
 *   4. Type "deactivate" when done
 * ============================================
 */

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Servo.h>
#include <ArduinoJson.h>

// ============================================
// CONFIGURATION - EDIT THESE VALUES
// ============================================

// WiFi credentials
// const char* WIFI_SSID = "FTTx-807a60";
// const char* WIFI_PASSWORD = "mark0523";

const char* WIFI_SSID = "Xiaomi_53DE";
const char* WIFI_PASSWORD = "hayao1014";

// API Configuration
const char* API_KEY = "BIN_MHSEHCF4_MH8NQIUUXEQVFVP30VU2M";  // Replace with your bin's API key
const char* BIN_ID = "BIN_MHSEHCF4_MH8NQIUUXEQVFVP30VU2M";    // Bin ID (same as API key for now)
const char* LOCATION_URL = "http://192.168.31.196:3000/api/iot/update-location";  // Location updates
const char* CAPACITY_URL = "http://192.168.31.196:3000/api/iot/update-capacity";  // Capacity updates
const char* HEARTBEAT_URL = "http://192.168.31.196:3000/api/iot/heartbeat";      // Heartbeat updates
const char* GET_COMMAND_URL = "http://192.168.31.196:3000/api/iot/get-command";  // Poll for commands
const char* DEACTIVATE_BIN_URL = "http://192.168.31.196:3000/api/bins/deactivate"; // Deactivate bin with session data

// Update intervals (in milliseconds)
const unsigned long CAPACITY_UPDATE_INTERVAL = 30000;  // 30 seconds for capacity
const unsigned long HEARTBEAT_INTERVAL = 30000;        // 30 seconds for heartbeat
const unsigned long COMMAND_POLL_INTERVAL = 5000;      // 5 seconds for command polling
const unsigned long USER_CHECK_INTERVAL = 500;         // 500ms for user presence detection
const unsigned long STANDBY_TIMEOUT = 30000;           // 30 seconds timeout to standby mode

// ============================================
// PIN CONFIGURATION (NodeMCU ESP8266)
// ============================================

// ESP32-CAM Serial Communication pins (SoftwareSerial)
const int CAM_RX_PIN = D2;  // GPIO4 - Receive from ESP32-CAM TX
const int CAM_TX_PIN = D1;  // GPIO5 - Transmit to ESP32-CAM RX

// User Presence Detection Sensor (Sharp 2Y0A21 IR Distance Sensor)
// NOW USED FOR: Trash drop detection
const int TRASH_DETECTION_PIN = A0;  // Analog pin A0

// Compartment 1 Capacity Sensor (Plastic)
const int COMP1_TRIG_PIN = D7;  // GPIO13
const int COMP1_ECHO_PIN = D8;  // GPIO15

// Compartment 2 Capacity Sensor (Tin)
const int COMP2_TRIG_PIN = D0;  // GPIO16
const int COMP2_ECHO_PIN = D5;  // GPIO14

// Servo Motor Pins (PWM capable)
// Rotating platform design: 1 bin lid + 1 dropper + 1 rotator
const int BIN_LID_SERVO_PIN = D3;   // GPIO0 - MG90S: Opens/closes main bin lid
const int DROPPER_SERVO_PIN = D4;   // GPIO2 - MG90S: Drops trash into compartment
const int ROTATOR_SERVO_PIN = D6;   // GPIO12 - MG995: Rotates platform to compartment

// Status LED
const int LED_PIN = D4;  // GPIO2 (Built-in LED on NodeMCU)

// ============================================
// BIN CONFIGURATION
// ============================================

// Ultrasonic sensor distances (in cm)
const float BIN_HEIGHT = 30.0;  // Total height of bin compartments

// Sharp 2Y0A21 IR Distance Sensor configuration
// NOW USED FOR: Detecting when trash is dropped into bin
// ESP8266 has 10-bit ADC (0-1023)
const int TRASH_DETECTION_THRESHOLD = 400;  // ADC threshold for trash detection (ESP8266 10-bit ADC: 0-1023)
const int TRASH_DETECTION_MIN_ADC = 120;     // Minimum valid reading (80cm distance)
const int TRASH_DETECTION_MAX_ADC = 860;     // Maximum valid reading (10cm distance)
// Detection range: ~10-80cm, optimal 20-50cm
// Note: ESP8266 has 10-bit ADC (0-1023) vs ESP32 12-bit (0-4095)

// Servo angles
// Bin Lid Servo (MG90S)
const int LID_CLOSED = 0;           // Lid closed
const int LID_OPEN = 90;            // Lid open

// Dropper Servo (MG90S)
const int DROPPER_HOLD = 90;     // Holding trash (starts at 90 degrees)
const int DROPPER_RELEASE = 0;   // Releasing trash (moves to 0 degrees)

// Rotator Servo (MG995)
const int ROTATE_PLASTIC = 0;       // Position for plastic compartment
const int ROTATE_TIN = 90;          // Position for tin compartment
const int ROTATE_REJECTED = 180;    // Position for rejected compartment

// Serial Communication with ESP32-CAM
// ESP32-CAM uses SoftwareSerial (GPIO 4/5)
SoftwareSerial ESP32_CAM_SERIAL(CAM_RX_PIN, CAM_TX_PIN);  // RX, TX
#define ESP32_BAUD 9600

// Compartment control commands from ESP32-CAM
#define CMD_OPEN_PLASTIC "OPEN_PLASTIC"
#define CMD_OPEN_TIN "OPEN_TIN"
#define CMD_OPEN_REJECTED "OPEN_REJECTED"
#define CMD_CLOSE_ALL "CLOSE_ALL"
#define CMD_STATUS "STATUS"
#define CMD_CHECK_USER "CHECK_USER"
#define CMD_ACTIVATE_BIN "ACTIVATE_BIN:"      // NEW: Format "ACTIVATE_BIN:userId:sessionId"
#define CMD_DEACTIVATE_BIN "DEACTIVATE_BIN"   // NEW: Deactivate bin after use

// System state
bool binActivatedByQR = false;        // NEW: Only true when QR code is scanned
String currentUserId = "";            // NEW: Store current user ID from QR scan
String currentSessionId = "";         // NEW: Store session ID from QR activation
bool userPresent = false;             // Not used anymore
bool systemActive = false;
bool monitoringForTrash = false;      // NEW: ESP8266 focuses on trash detection
bool trashBeingProcessed = false;     // NEW: Prevent multiple detections of same item
unsigned long trashMonitorStartTime = 0;
unsigned long lastUserDetectionTime = 0;
unsigned long lastUserCheckTime = 0;
const unsigned long TRASH_MONITOR_TIMEOUT = 30000;     // 30 seconds to drop trash

// Session counters for batch point calculation
int sessionPlasticCount = 0;          // Count of plastic bottles in current session
int sessionTinCount = 0;              // Count of tin cans in current session
int sessionRejectedCount = 0;         // Count of rejected items in current session

// Servo objects (Rotating platform design)
Servo binLidServo;    // MG90S - Main bin lid
Servo dropperServo;   // MG90S - Trash dropper
Servo rotatorServo;   // MG995 - Platform rotator

// Timing variables
unsigned long lastCapacityUpdateTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long lastCommandPollTime = 0;
unsigned long trashDetectionTime = 0;  // NEW: Track when trash was first detected
const unsigned long TRASH_PROCESSING_TIMEOUT = 60000;  // NEW: 60 seconds timeout for ESP32-CAM response

// Compartment state (controlled by ESP32-CAM)
bool binLidOpen = false;
bool platformRotated = false;
int currentPlatformPosition = ROTATE_PLASTIC;  // Track rotator position
unsigned long compartmentOpenTime = 0;
const unsigned long COMPARTMENT_OPEN_DURATION = 5000;  // 5 seconds auto-close

// Capacity readings
float plasticCapacity = 0.0;  // 0-100%
float tinCapacity = 0.0;      // 0-100%

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n=================================");
  Serial.println("EcoEarn Smart Bin Controller");
  Serial.println("NodeMCU ESP8266 Version");
  Serial.println("GPS-FREE with SoftwareSerial");
  Serial.println("TRASH DROP DETECTION ENABLED");
  Serial.println("=================================\n");
  
  // Initialize SoftwareSerial for ESP32-CAM communication
  ESP32_CAM_SERIAL.begin(ESP32_BAUD);
  Serial.println("ESP32-CAM SoftwareSerial initialized on pins D1(D5)/D2(D4)");
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize trash detection sensor (Sharp 2Y0A21 - analog input)
  pinMode(TRASH_DETECTION_PIN, INPUT);
  
  // Initialize ultrasonic sensor pins (only for capacity monitoring)
  pinMode(COMP1_TRIG_PIN, OUTPUT);
  pinMode(COMP1_ECHO_PIN, INPUT);
  pinMode(COMP2_TRIG_PIN, OUTPUT);
  pinMode(COMP2_ECHO_PIN, INPUT);
  
  // Initialize servos for rotating platform design
  binLidServo.attach(BIN_LID_SERVO_PIN);
  dropperServo.attach(DROPPER_SERVO_PIN);
  rotatorServo.attach(ROTATOR_SERVO_PIN);
  
  // Set initial servo positions
  binLidServo.write(LID_CLOSED);        // Lid closed
  dropperServo.write(DROPPER_HOLD);     // Holding position
  rotatorServo.write(ROTATE_PLASTIC);   // Default to plastic position
  
  // Connect to WiFi
  connectToWiFi();
  
  // Test basic connectivity
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   NETWORK DIAGNOSTICS                 ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.print("├─ ESP8266 IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("├─ Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("├─ DNS: ");
    Serial.println(WiFi.dnsIP());
    Serial.print("├─ Server URL: ");
    Serial.println(GET_COMMAND_URL);
    Serial.println("└─ Attempting connection test...\n");
    
    // Do an immediate command poll on startup to check for pending activations
    Serial.println("Checking for pending commands...");
    pollForCommands();
  } else {
    Serial.println("⚠️  WiFi not connected - cannot poll for commands");
  }
  
  Serial.println("\nSystem ready!");
  Serial.println("GPS: DISABLED - Operating without location tracking");
  Serial.println("Waiting for QR scan activation...");
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   🧪 TEST MODE AVAILABLE              ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("Commands:");
  Serial.println("  activate   - Bypass QR scan for testing");
  Serial.println("  deactivate - End test session\n");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Poll for commands from server
  // When bin is active: poll FASTER for disconnect commands (every 2 seconds)
  // When bin is inactive: poll normally (every 5 seconds)
  unsigned long pollInterval = binActivatedByQR ? 2000 : COMMAND_POLL_INTERVAL;  // 2s when active, 5s when inactive
  
  if (currentTime - lastCommandPollTime >= pollInterval) {
    pollForCommands();
    lastCommandPollTime = currentTime;
  }
  
  // Check for TEST/BYPASS commands from Serial Monitor
  checkSerialBypassCommands();
  
  // Check for bin activation/deactivation commands from ESP32-CAM (via SoftwareSerial)
  checkESP32Commands();
  
  // ONLY proceed with operations if bin is activated by QR scan
  if (!binActivatedByQR) {
    // Bin is LOCKED - do nothing, wait for QR activation
    digitalWrite(LED_PIN, LOW);  // LED off when inactive
    delay(100);
    return;  // Skip all other operations
  }
  
  // Bin is activated - blink LED to show active status
  if (currentTime % 2000 < 100) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  
  // Monitor for trash drop when in monitoring mode
  if (monitoringForTrash) {
    checkForTrashDrop();
    // NO TIMEOUT - Bin stays active until trash is detected or user deactivates manually
  }
  
  // Check for ESP32-CAM processing timeout
  if (trashBeingProcessed && (currentTime - trashDetectionTime >= TRASH_PROCESSING_TIMEOUT)) {
    Serial.println("\n⏰ ESP32-CAM TIMEOUT - No response within 60 seconds");
    Serial.println("🔄 Resetting trash detection system...");
    
    // Reset processing state
    trashBeingProcessed = false;
    monitoringForTrash = true;
    trashDetectionTime = 0;
    
    Serial.println("✅ Trash detection reset - ready for new item");
    Serial.println("────────────────────────────────────────\n");
  }
  
  // Update system active state
  updateSystemState(currentTime);
  
  // Update capacity readings periodically
  if (currentTime - lastCapacityUpdateTime >= CAPACITY_UPDATE_INTERVAL) {
    updateCapacityReadings();
    updateCapacityToServer();
    lastCapacityUpdateTime = currentTime;
  }
  
  // Send heartbeat to server periodically (ONLY when bin is NOT activated)
  // Skip heartbeat when user is active to avoid interfering with trash detection
  if (!binActivatedByQR && currentTime - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeatTime = currentTime;
  }
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    connectToWiFi();
  }
  
  delay(50);  // Small delay to prevent overwhelming the serial
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Blink LED 3 times to indicate successful connection
    blinkLED(3, 200);
  } else {
    Serial.println("\nFailed to connect to WiFi!");
  }
}

void updateCapacityToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  WiFi not connected. Cannot update capacity.");
    return;
  }
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   📤 SENDING CAPACITY TO SERVER       ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  WiFiClient client;
  HTTPClient http;
  
  // Configure HTTP request
  http.begin(client, CAPACITY_URL);
  http.addHeader("Content-Type", "application/json");
  
  // Create JSON payload
  String payload = "{";
  payload += "\"apiKey\":\"" + String(API_KEY) + "\",";
  payload += "\"comp1Capacity\":" + String(plasticCapacity, 1) + ",";
  payload += "\"comp2Capacity\":" + String(tinCapacity, 1);
  payload += "}";
  
  Serial.println("Target URL: " + String(CAPACITY_URL));
  Serial.println("Payload: " + payload);
  Serial.println("");
  
  // Send POST request
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("✓ HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response: ");
    Serial.println(response);
    
    if (httpResponseCode == 200) {
      Serial.println("✅ Capacity updated successfully in admin dashboard!");
      blinkLED(3, 100);
    } else {
      Serial.println("✗ Server returned an error!");
    }
  } else {
    Serial.print("❌ Error sending capacity request: ");
    Serial.println(httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
  }
  
  http.end();
  Serial.println("════════════════════════════════════════\n");
}

void sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, HEARTBEAT_URL);
  http.addHeader("Content-Type", "application/json");
  
  String payload = "{\"apiKey\":\"" + String(API_KEY) + "\"}";
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    Serial.println("💓 Heartbeat sent");
  }
  
  http.end();
}

// Ultrasonic sensor functions
float getUltrasonicDistance(int trigPin, int echoPin) {
  // Send trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read echo pulse (30ms timeout)
  long duration = pulseIn(echoPin, HIGH, 30000);
  
  // Calculate distance in cm
  float distance = duration * 0.034 / 2;
  
  // Filter out invalid readings
  if (distance <= 0 || distance > 400) {
    return -1;  // Invalid reading
  }
  
  return distance;
}

void updateCapacityReadings() {
  // Read plastic compartment capacity
  float plasticDistance = getUltrasonicDistance(COMP1_TRIG_PIN, COMP1_ECHO_PIN);
  if (plasticDistance > 0) {
    plasticCapacity = ((BIN_HEIGHT - plasticDistance) / BIN_HEIGHT) * 100.0;
    plasticCapacity = constrain(plasticCapacity, 0, 100);
  }
  
  // Read tin compartment capacity
  float tinDistance = getUltrasonicDistance(COMP2_TRIG_PIN, COMP2_ECHO_PIN);
  if (tinDistance > 0) {
    tinCapacity = ((BIN_HEIGHT - tinDistance) / BIN_HEIGHT) * 100.0;
    tinCapacity = constrain(tinCapacity, 0, 100);
  }
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   📊 CAPACITY SENSOR READINGS         ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("Bin Height: " + String(BIN_HEIGHT) + " cm");
  Serial.println("");
  Serial.println("PLASTIC Compartment (Comp1):");
  Serial.print("  ├─ Distance measured: ");
  Serial.print(plasticDistance);
  Serial.println(" cm");
  Serial.print("  ├─ Fill level: ");
  Serial.print(BIN_HEIGHT - plasticDistance);
  Serial.println(" cm");
  Serial.print("  └─ Capacity: ");
  Serial.print(plasticCapacity, 1);
  Serial.println("%");
  Serial.println("");
  Serial.println("TIN Compartment (Comp2):");
  Serial.print("  ├─ Distance measured: ");
  Serial.print(tinDistance);
  Serial.println(" cm");
  Serial.print("  ├─ Fill level: ");
  Serial.print(BIN_HEIGHT - tinDistance);
  Serial.println(" cm");
  Serial.print("  └─ Capacity: ");
  Serial.print(tinCapacity, 1);
  Serial.println("%");
  Serial.println("════════════════════════════════════════\n");
}

void checkSerialBypassCommands() {
  // Check for bypass/test commands from Serial Monitor
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    // BYPASS COMMAND - Activate bin without QR scan
    if (command == "activate" || command == "test" || command == "bypass") {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║   🔓 TEST MODE ACTIVATED              ║");
      Serial.println("╚════════════════════════════════════════╝");
      Serial.println("Bin activated for testing (bypassing QR scan)");
      Serial.println("You can now test trash detection!");
      Serial.println("Type 'deactivate' or 'stop' to end test mode\n");
      
      binActivatedByQR = true;
      currentUserId = "TEST_USER";
      currentSessionId = "TEST_SESSION_" + String(millis());
      activateBin();
      return;
    }
    
    // DEACTIVATE COMMAND
    if (command == "deactivate" || command == "stop" || command == "end") {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║   🔒 TEST MODE DEACTIVATED            ║");
      Serial.println("╚════════════════════════════════════════╝");
      Serial.println("Bin deactivated - test mode ended\n");
      
      deactivateBin();
      return;
    }
  }
}

void checkForTrashDrop() {
  // If trash is already being processed, don't detect again
  if (trashBeingProcessed) {
    return;
  }

  // Read analog value from Sharp 2Y0A21 IR sensor
  int adcValue = analogRead(TRASH_DETECTION_PIN);

  // Debug: Show IR sensor reading
  static int debugCounter = 0;
  debugCounter++;

  if (debugCounter >= 20) {
    Serial.print("📊 Trash Sensor ADC: ");
    Serial.print(adcValue);
    Serial.print(" (Threshold: ");
    Serial.print(TRASH_DETECTION_THRESHOLD);
    Serial.println(")");

    debugCounter = 0;
  }

  // Check if trash is detected (object close to sensor)
  if (adcValue >= TRASH_DETECTION_THRESHOLD && adcValue <= TRASH_DETECTION_MAX_ADC) {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   🗑️  TRASH DETECTED!                ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.print("ADC Value: ");
    Serial.println(adcValue);

    // SET PROCESSING FLAG - Prevent multiple detections
    trashBeingProcessed = true;
    trashDetectionTime = millis();  // Record detection time for timeout

    // WAIT FOR USER TO REMOVE HAND
    Serial.println("\n⏳ Waiting 2 seconds for user to remove hand...");
    delay(2000);  // 2 second delay
    Serial.println("✓ Delay complete - proceeding with capture\n");

    Serial.println("📸 TRIGGERING ESP32-CAM:");
    Serial.println("├─ Sending command: TRASH_DETECTED");
    Serial.println("├─ ESP32-CAM will capture 5 images");
    Serial.println("├─ Backend API: http://213.35.114.162:5001/identify/material");
    Serial.println("└─ Waiting for classification result...\n");

    // Stop monitoring until this item is processed
    monitoringForTrash = false;

    // Notify ESP32-CAM to capture and identify
    ESP32_CAM_SERIAL.println("TRASH_DETECTED");

    Serial.println("✅ Command sent to ESP32-CAM via SoftwareSerial");
    Serial.println("⏳ ESP32-CAM should respond within 30 seconds");
    Serial.println("🔒 Trash detection LOCKED until item is processed");
    Serial.println("────────────────────────────────────────\n");
  }
}

void updateSystemState(unsigned long currentTime) {
  bool shouldBeActive = binActivatedByQR;
  
  if (shouldBeActive != systemActive) {
    systemActive = shouldBeActive;
    
    if (systemActive) {
      Serial.println("🔋 System ACTIVE - Bin ready for use");
    } else {
      Serial.println("💤 System STANDBY - Bin locked");
      digitalWrite(LED_PIN, LOW);
      monitoringForTrash = false;
      trashBeingProcessed = false;  // Reset processing flag
      trashDetectionTime = 0;       // Reset detection time
    }
  }
}

void checkESP32Commands() {
  if (ESP32_CAM_SERIAL.available()) {
    String command = ESP32_CAM_SERIAL.readStringUntil('\n');
    command.trim();
    
    // Ignore debug messages from ESP32-CAM
    if (command.startsWith("Capturing") || 
        command.startsWith("Image captured") ||
        command.startsWith("Sending") ||
        command.startsWith("HTTP") ||
        command.startsWith("Response") ||
        command.startsWith("├─ Confidence") ||  // Filter confidence messages
        command.startsWith("├─ Action") ||      // Filter action messages
        command.startsWith("Material") ||
        command.startsWith("Confidence") ||
        command.startsWith("Action") ||
        command.startsWith("Camera") ||
        command.startsWith("WiFi") ||
        command.startsWith("Connecting") ||
        command.startsWith("IP address") ||
        command.startsWith("ERROR") ||
        command.startsWith(".") ||
        command.length() == 0) {
      return;  // Ignore debug output
    }
    
    Serial.println("Received command from ESP32-CAM: " + command);
    
    // HIGHEST PRIORITY: Check for bin activation command
    if (command.startsWith(CMD_ACTIVATE_BIN)) {
      // Format: ACTIVATE_BIN:userId:sessionId
      int firstColon = command.indexOf(':', 13);  // After "ACTIVATE_BIN:"
      int secondColon = command.indexOf(':', firstColon + 1);
      
      if (firstColon > 0 && secondColon > 0) {
        currentUserId = command.substring(firstColon + 1, secondColon);
        currentSessionId = command.substring(secondColon + 1);
        activateBin();
      } else {
        Serial.println("⚠️  Invalid ACTIVATE_BIN command format");
        Serial.println("ACTIVATION_FAILED");
        ESP32_CAM_SERIAL.println("ACTIVATION_FAILED");
      }
      return;
    }
    
    // Check for deactivation command
    if (command == CMD_DEACTIVATE_BIN) {
      deactivateBin();
      return;
    }
    
    // ALL OTHER COMMANDS REQUIRE BIN TO BE ACTIVATED FIRST
    if (!binActivatedByQR) {
      Serial.println("⚠️  Bin not activated - scan QR code first!");
      Serial.println("BIN_LOCKED");
      ESP32_CAM_SERIAL.println("BIN_LOCKED");
      return;
    }
    
    // Process compartment commands - NOW JUST COUNTING INSTEAD OF PROCESSING
    if (command == CMD_OPEN_PLASTIC) {
      sessionPlasticCount++;
      Serial.println("📦 Plastic bottle detected - Count: " + String(sessionPlasticCount));
      // Still need to physically sort the item
      sortPlasticItem();
    } else if (command == CMD_OPEN_TIN) {
      sessionTinCount++;
      Serial.println("🥫 Tin can detected - Count: " + String(sessionTinCount));
      // Still need to physically sort the item
      sortTinItem();
    } else if (command == CMD_OPEN_REJECTED) {
      sessionRejectedCount++;
      Serial.println("❌ Rejected item detected - Count: " + String(sessionRejectedCount));
      // Still need to physically sort the item
      sortRejectedItem();
    } else if (command == CMD_CLOSE_ALL) {
      closeAllCompartments();
    } else if (command == CMD_STATUS) {
      sendStatusToESP32();
    } else if (command == "PONG") {
      // Response to ping test
      Serial.println("📩 Received PONG from ESP32-CAM");
    } else if (command.indexOf("├─ Material:") >= 0) {
      // NEW: Parse ESP32-CAM material classification results
      // Format: "├─ Material: plastic" or similar
      int colonPos = command.indexOf("├─ Material:");
      if (colonPos >= 0) {
        String material = command.substring(colonPos + 12);  // Skip "├─ Material:"
        material.trim();
        material.toLowerCase();
        
        Serial.println("🎯 ESP32-CAM Material Classification: " + material);
        
        // Trigger appropriate sorting based on material
        if (material == "plastic" || material == "bottle") {
          sessionPlasticCount++;
          Serial.println("📦 Plastic bottle detected - Count: " + String(sessionPlasticCount));
          sortPlasticItem();
        } else if (material == "tin" || material == "can" || material == "metal") {
          sessionTinCount++;
          Serial.println("🥫 Tin can detected - Count: " + String(sessionTinCount));
          sortTinItem();
        } else if (material == "rejected" || material == "other" || material == "unknown") {
          sessionRejectedCount++;
          Serial.println("❌ Rejected item detected - Count: " + String(sessionRejectedCount));
          sortRejectedItem();
        } else {
          // Unknown material - treat as rejected
          sessionRejectedCount++;
          Serial.println("❓ Unknown material '" + material + "' - treating as rejected");
          Serial.println("❌ Rejected item detected - Count: " + String(sessionRejectedCount));
          sortRejectedItem();
        }
      }
    } else {
      Serial.println("Unknown command: " + command);
    }
  }
}

void sortPlasticItem() {
  Serial.println("Sorting plastic item...");
  
  // Step 1: Rotate platform to plastic position
  rotatorServo.write(ROTATE_PLASTIC);
  currentPlatformPosition = ROTATE_PLASTIC;
  platformRotated = true;
  delay(1000);  // Wait for rotation
  
  // Step 2: Drop trash
  dropperServo.write(DROPPER_RELEASE);
  delay(1000);
  dropperServo.write(DROPPER_HOLD);
  delay(500);
  
  // Step 3: Return rotator to default position (ready for next item)
  rotatorServo.write(ROTATE_PLASTIC);
  currentPlatformPosition = ROTATE_PLASTIC;
  delay(1000);
  
  Serial.println("✓ Plastic item sorted");
  
  // Update capacity readings and send to server
  updateCapacityReadings();
  updateCapacityToServer();
  
  // Re-enable trash monitoring for next item
  monitoringForTrash = true;
  trashBeingProcessed = false;  // Reset processing flag
  trashDetectionTime = 0;       // Reset detection time
  
  Serial.println("🔓 Trash detection UNLOCKED - ready for next item");
  Serial.println("PLASTIC_SORTED");
  ESP32_CAM_SERIAL.println("PLASTIC_SORTED");
}

void sortTinItem() {
  Serial.println("Sorting tin item...");
  
  // Step 1: Rotate platform to tin position
  rotatorServo.write(ROTATE_TIN);
  currentPlatformPosition = ROTATE_TIN;
  platformRotated = true;
  delay(1500);  // Wait for rotation to 90 degrees
  
  // Step 2: Drop trash
  dropperServo.write(DROPPER_RELEASE);
  delay(1000);
  dropperServo.write(DROPPER_HOLD);
  delay(500);
  
  // Step 3: Return rotator to default position (ready for next item)
  rotatorServo.write(ROTATE_PLASTIC);
  currentPlatformPosition = ROTATE_PLASTIC;
  delay(1500);  // Wait for rotation back to 0 degrees
  
  Serial.println("✓ Tin item sorted");
  
  // Update capacity readings and send to server
  updateCapacityReadings();
  updateCapacityToServer();
  
  // Re-enable trash monitoring for next item
  monitoringForTrash = true;
  trashBeingProcessed = false;  // Reset processing flag
  trashDetectionTime = 0;       // Reset detection time
  
  Serial.println("🔓 Trash detection UNLOCKED - ready for next item");
  Serial.println("TIN_SORTED");
  ESP32_CAM_SERIAL.println("TIN_SORTED");
}

void sortRejectedItem() {
  Serial.println("Sorting rejected item...");
  
  // Step 1: Rotate platform to rejected position (180 degrees)
  rotatorServo.write(ROTATE_REJECTED);
  currentPlatformPosition = ROTATE_REJECTED;
  platformRotated = true;
  delay(2000);  // Longer wait for 180° rotation
  
  // Step 2: Drop trash
  dropperServo.write(DROPPER_RELEASE);
  delay(1000);
  dropperServo.write(DROPPER_HOLD);
  delay(500);
  
  // Step 3: Return rotator to default position (ready for next item)
  rotatorServo.write(ROTATE_PLASTIC);
  currentPlatformPosition = ROTATE_PLASTIC;
  delay(2000);  // Longer wait for 180° rotation back to 0
  
  Serial.println("✓ Rejected item sorted");
  
  // Update capacity readings and send to server
  updateCapacityReadings();
  updateCapacityToServer();
  
  // Re-enable trash monitoring for next item
  monitoringForTrash = true;
  trashBeingProcessed = false;  // Reset processing flag
  trashDetectionTime = 0;       // Reset detection time
  
  Serial.println("🔓 Trash detection UNLOCKED - ready for next item");
  Serial.println("REJECTED_SORTED");
  ESP32_CAM_SERIAL.println("REJECTED_SORTED");
}

void closeAllCompartments() {
  binLidServo.write(LID_CLOSED);
  dropperServo.write(DROPPER_HOLD);
  rotatorServo.write(ROTATE_PLASTIC);  // Return to default position
  
  binLidOpen = false;
  platformRotated = false;
  currentPlatformPosition = ROTATE_PLASTIC;
  
  Serial.println("All servos returned to default position");
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("ALL_CLOSED");
  ESP32_CAM_SERIAL.println("ALL_CLOSED");
}

void sendStatusToESP32() {
  String status = "STATUS:Lid=" + String(binLidOpen ? "OPEN" : "CLOSED");
  status += ",Platform=" + String(currentPlatformPosition);
  status += ",Position=";
  
  if (currentPlatformPosition == ROTATE_PLASTIC) {
    status += "PLASTIC";
  } else if (currentPlatformPosition == ROTATE_TIN) {
    status += "TIN";
  } else if (currentPlatformPosition == ROTATE_REJECTED) {
    status += "REJECTED";
  }
  
  status += ",Active=" + String(binActivatedByQR ? "YES" : "NO");
  status += ",User=" + currentUserId;
  status += ",Session=" + currentSessionId;
  
  Serial.println(status);
  ESP32_CAM_SERIAL.println(status);
}

void activateBin() {
  binActivatedByQR = true;
  
  Serial.println("========================================");
  Serial.println("🔓 BIN ACTIVATED!");
  Serial.println("User ID: " + currentUserId);
  Serial.println("Session ID: " + currentSessionId);
  Serial.println("");
  Serial.println("⚡ PERFORMANCE MODE ENABLED:");
  Serial.println("  ✓ Command polling: ACTIVE (2s interval for disconnect detection)");
  Serial.println("  ✓ Heartbeat: PAUSED (prevents interference)");
  Serial.println("  ✓ Trash detection: ACTIVE & PRIORITIZED");
  Serial.println("========================================\n");
  
  // AUTOMATICALLY OPEN BIN LID
  Serial.println("🚪 AUTO-OPENING bin lid...");
  binLidServo.write(LID_OPEN);
  binLidOpen = true;
  digitalWrite(LED_PIN, HIGH);
  Serial.println("✓ Bin lid OPENED - User can throw trash now\n");
  
  // START TRASH MONITORING - NO TIMEOUT
  Serial.println("📡 Starting trash monitoring...");
  monitoringForTrash = true;
  Serial.println("⏰ No timeout - bin will stay active until trash is detected");
  Serial.println("💡 Type 'deactivate' to manually stop monitoring\n");
  
  Serial.println("BIN_ACTIVATED");
  ESP32_CAM_SERIAL.println("BIN_ACTIVATED");
}

void deactivateBin() {
  Serial.println("========================================");
  Serial.println("🔒 BIN DEACTIVATED");
  Serial.println("Session ended for user: " + currentUserId);
  Serial.println("");
  Serial.println("⚡ NORMAL MODE RESUMED:");
  Serial.println("  ✓ Heartbeat: ACTIVE");
  Serial.println("  ✓ Command polling: ACTIVE");
  Serial.println("  ✓ Trash detection: STANDBY");
  Serial.println("========================================");
  
  // SEND SESSION DATA TO SERVER BEFORE CLOSING
  Serial.println("\n📊 Sending session totals to server...");
  sendSessionDataToServer();
  
  // CLOSE LID FIRST - User disconnected, bin should close
  Serial.println("\n🚪 Closing bin lid...");
  binLidServo.write(LID_CLOSED);
  binLidOpen = false;
  delay(1000);  // Wait for lid to close
  Serial.println("✓ Lid closed\n");
  
  // Reset all servos to default positions
  dropperServo.write(DROPPER_HOLD);
  rotatorServo.write(ROTATE_PLASTIC);
  currentPlatformPosition = ROTATE_PLASTIC;
  platformRotated = false;
  
  // Reset state
  binActivatedByQR = false;
  currentUserId = "";
  currentSessionId = "";
  userPresent = false;
  systemActive = false;
  monitoringForTrash = false;
  trashBeingProcessed = false;  // Reset processing flag
  trashDetectionTime = 0;       // Reset detection time
  
  // RESET SESSION COUNTERS
  sessionPlasticCount = 0;
  sessionTinCount = 0;
  sessionRejectedCount = 0;
  
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("✓ All systems reset to standby mode\n");
  
  Serial.println("BIN_DEACTIVATED");
  ESP32_CAM_SERIAL.println("BIN_DEACTIVATED");
}

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}

void sendSessionDataToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  WiFi not connected. Skipping session data send.");
    return;
  }
  
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, DEACTIVATE_BIN_URL);
  http.addHeader("Content-Type", "application/json");
  
  // Create JSON payload with session data
  String payload = "{";
  payload += "\"apiKey\":\"" + String(API_KEY) + "\",";
  payload += "\"userId\":\"" + currentUserId + "\",";
  payload += "\"userEmail\":\"" + currentUserId + "\",";
  payload += "\"sessionData\":{";
  payload += "\"plasticCount\":" + String(sessionPlasticCount) + ",";
  payload += "\"tinCount\":" + String(sessionTinCount) + ",";
  payload += "\"rejectedCount\":" + String(sessionRejectedCount);
  payload += "}";
  payload += "}";
  
  Serial.println("=== Sending Session Data to Server ===");
  Serial.println("Plastic bottles: " + String(sessionPlasticCount));
  Serial.println("Tin cans: " + String(sessionTinCount));
  Serial.println("Rejected items: " + String(sessionRejectedCount));
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode == 200) {
    Serial.println("✓ Session data sent successfully");
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.print("✗ Error sending session data: ");
    Serial.println(httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
  }
  
  http.end();
  Serial.println();
}

void pollForCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  WiFiClient client;
  HTTPClient http;
  
  // Set timeout to avoid hanging
  http.setTimeout(5000);  // 5 second timeout
  
  // Begin connection with URL
  if (!http.begin(client, GET_COMMAND_URL)) {
    return;
  }
  
  http.addHeader("Content-Type", "application/json");

  // Prepare JSON payload with API key
  String payload = "{\"apiKey\":\"" + String(API_KEY) + "\"}";

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("✓ Response code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response: ");
    Serial.println(response);

    // Parse JSON response
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      bool hasCommand = doc["hasCommand"];
      
      if (hasCommand) {
        String command = doc["command"].as<String>();
        Serial.println("\n╔════════════════════════════════════════╗");
        Serial.println("║   📥 NEW COMMAND FROM SERVER          ║");
        Serial.println("╚════════════════════════════════════════╝");
        Serial.println("Command: " + command);
        
        // Process the command
        processServerCommand(command);
      } else {
        Serial.println("✅ No pending commands");
      }
    } else {
      Serial.print("❌ Failed to parse JSON response: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("❌ HTTP Error: ");
    Serial.println(httpResponseCode);
    Serial.print("Error description: ");
    Serial.println(http.errorToString(httpResponseCode));
  }

  http.end();
  Serial.println("────────────────────────────────────────\n");
}

void processServerCommand(String command) {
  // Check for activation command
  if (command.startsWith("ACTIVATE_BIN:")) {
    // Format: ACTIVATE_BIN:userId:sessionId
    // Remove the "ACTIVATE_BIN:" prefix first
    String params = command.substring(13);  // Skip "ACTIVATE_BIN:"
    
    int colonPos = params.indexOf(':');
    
    if (colonPos > 0) {
      currentUserId = params.substring(0, colonPos);
      currentSessionId = params.substring(colonPos + 1);
      
      Serial.println("\n🔓 ACTIVATION COMMAND RECEIVED FROM SERVER");
      Serial.println("├─ User ID: " + currentUserId);
      Serial.println("└─ Session: " + currentSessionId);
      
      activateBin();
    } else {
      Serial.println("⚠️  Invalid ACTIVATE_BIN format");
      Serial.println("Expected: ACTIVATE_BIN:userId:sessionId");
      Serial.println("Got: " + command);
    }
  } 
  else if (command == "DEACTIVATE_BIN") {
    Serial.println("\n🔒 DEACTIVATION COMMAND FROM SERVER");
    deactivateBin();
  }
  else {
    Serial.println("⚠️  Unknown server command: " + command);
  }
}
