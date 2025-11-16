/*
 * EcoEarn Smart Bin Controller
 * 
 * This Arduino sketch controls a smart trash bin with:
 * - GPS tracking (GY-GPS6MV2 module)
 * - User detection with Sharp IR sensor
 * - Capacity monitoring with 2 ultrasonic sensors
 * - Compartment control with 3 servo motors
 * 
 * Hardware Required:
 * - ESP32 38-pin Development Board
 * - GY-GPS6MV2 GPS Module
 * - ESP32-CAM AI Thinker (for material identification)
 * - 1x Sharp 2Y0A21 IR Distance Sensor
 * - 2x HC-SR04 Ultrasonic Sensors
 * - 3x Servo Motors (MG90S or similar)
 * 
 * Pin Connections:
 * GPS Module:
 *   - GPS RX → ESP32 GPIO 17 (TX2)
 *   - GPS TX → ESP32 GPIO 16 (RX2)
 * 
 * ESP32-CAM Communication (Hardware Serial1):
 *   - ESP32 GPIO 33 (RX1) ← ESP32-CAM U0T/GPIO 1 (TX)
 *   - ESP32 GPIO 32 (TX1) → ESP32-CAM U0R/GPIO 3 (RX)
 *   - Common GND
 *   Note: ESP32-CAM Serial Monitor will NOT work while connected
 *   GPIO 32/33 are SAFE - won't cause boot issues
 * 
 * Sensors:
 *   - Sharp IR Sensor OUT → ESP32 GPIO 34 (ADC) - Trash drop detection
 *   - Ultrasonic 1 TRIG → GPIO 25, ECHO → GPIO 26 (Plastic capacity)
 *   - Ultrasonic 2 TRIG → GPIO 27, ECHO → GPIO 14 (Tin capacity)
 * 
 * Servos:
 *   - Bin Lid Servo → GPIO 12
 *   - Dropper Servo → GPIO 13
 *   - Rotator Servo → GPIO 15
 * 
 * Status LED → GPIO 2 (built-in)
 * 
 * Libraries Required:
 * - TinyGPS++ (for GPS parsing)
 * - WiFi (ESP32)
 * - HTTPClient (ESP32)
 * - ESP32Servo (for servo control)
 * - ArduinoJson (for JSON parsing)
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

#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>  // For parsing server responses

// ============================================
// CONFIGURATION - EDIT THESE VALUES
// ============================================

// WiFi credentials
// const char* WIFI_SSID = "FTTx-ZTE_1294C5";
// const char* WIFI_PASSWORD = "mark0523";

const char* WIFI_SSID = "ZTE_1294C5";
const char* WIFI_PASSWORD = "1234567890";

// API Configuration - UPDATE THESE FOR YOUR NETWORK
const char* API_KEY = "BIN_MHSEHCF4_MH8NQIUUXEQVFVP30VU2M";  // Replace with your bin's API key
const char* BIN_ID = "BIN_MHSEHCF4_MH8NQIUUXEQVFVP30VU2M";    // Bin ID (same as API key for now)

// SERVER IP: Update this to your computer's local IP address (run 'ipconfig' on Windows or 'ifconfig' on Linux/Mac)
// Example: If your computer IP is 192.168.1.100, use "192.168.1.100"
// DO NOT use localhost/127.0.0.1 - ESP32 cannot connect to that
const char* SERVER_IP = "192.168.0.17";  // ← CHANGE THIS TO YOUR COMPUTER'S IP
const int SERVER_PORT = 3000;

// Construct URLs at runtime (can't concatenate const char* at compile time)
String LOCATION_URL;
String CAPACITY_URL;
String HEARTBEAT_URL;
String GET_COMMAND_URL;
String RECYCLE_URL;
String DEACTIVATE_BIN_URL;
String SESSION_DATA_URL;

// Update intervals (in milliseconds)
const unsigned long GPS_UPDATE_INTERVAL = 300000;  // 5 minutes for GPS
const unsigned long CAPACITY_UPDATE_INTERVAL = 30000;  // 30 seconds for capacity
const unsigned long HEARTBEAT_INTERVAL = 30000;        // 30 seconds for heartbeat
const unsigned long COMMAND_POLL_INTERVAL = 5000;      // 5 seconds for command polling
const unsigned long USER_CHECK_INTERVAL = 500;         // 500ms for user presence detection
const unsigned long STANDBY_TIMEOUT = 30000;           // 30 seconds timeout to standby mode

// ============================================
// PIN CONFIGURATION (ESP32 38-pin)
// ============================================

// GPS Serial pins (Hardware Serial 2)
const int GPS_RX_PIN = 16;  // GPIO16 (RX2)
const int GPS_TX_PIN = 17;  // GPIO17 (TX2)

// ESP32-CAM Serial Communication pins (GPIO 32/33 - SAFE)
const int CAM_RX_PIN = 33;  // GPIO33 - Receive from ESP32-CAM TX
const int CAM_TX_PIN = 32;  // GPIO32 - Transmit to ESP32-CAM RX

// User Presence Detection Sensor (Sharp 2Y0A21 IR Distance Sensor)
// NOW USED FOR: Trash drop detection
const int TRASH_DETECTION_PIN = 34;  // GPIO34 (ADC1_CH6 - input only)

// Compartment 1 Capacity Sensor (Plastic)
const int COMP1_TRIG_PIN = 25;  // GPIO25
const int COMP1_ECHO_PIN = 26;  // GPIO26

// Compartment 2 Capacity Sensor (Tin)
const int COMP2_TRIG_PIN = 27;  // GPIO27
const int COMP2_ECHO_PIN = 14;  // GPIO14

// Servo Motor Pins (PWM capable)
// Rotating platform design: 1 bin lid + 1 dropper + 1 rotator
const int BIN_LID_SERVO_PIN = 12;   // GPIO12 - MG90S: Opens/closes main bin lid
const int DROPPER_SERVO_PIN = 13;   // GPIO13 - MG90S: Drops trash into compartment
const int ROTATOR_SERVO_PIN = 15;   // GPIO15 - MG995: Rotates platform to compartment

// Status LED
const int LED_PIN = 2;  // GPIO2 (Built-in LED on most ESP32 boards)

// ============================================
// BIN CONFIGURATION
// ============================================

// Ultrasonic sensor distances (in cm)
const float BIN_HEIGHT = 30.0;  // Total height of bin compartments

// Sharp 2Y0A21 IR Distance Sensor configuration
// NOW USED FOR: Detecting when trash is dropped into bin
const int TRASH_DETECTION_THRESHOLD = 2600;  // ADC threshold for trash detection (ESP32 12-bit ADC: 0-4095)
const int TRASH_DETECTION_MIN_ADC = 500;     // Minimum valid reading (80cm distance)
const int TRASH_DETECTION_MAX_ADC = 3500;    // Maximum valid reading (10cm distance)
// Detection range: ~10-80cm, optimal 20-50cm
// Note: ESP32 has 12-bit ADC (0-4095) vs ESP8266 10-bit (0-1023)

// Servo angles
// Bin Lid Servo (MG90S)
const int LID_CLOSED = 0;           // Lid closed
const int LID_OPEN = 90;            // Lid open

// Dropper Servo (MG90S)
const int DROPPER_HOLD = 90;         // Holding trash
const int DROPPER_RELEASE = 0;     // Releasing trash

// Rotator Servo (MG995)
const int ROTATE_PLASTIC = 0;  //30       // Position for plastic compartment
const int ROTATE_TIN = 180;          // Position for tin compartment
const int ROTATE_REJECTED = 90;    // Position for rejected compartment

// Serial Communication with ESP32-CAM
// GPS uses Hardware Serial2 (GPIO 16/17)
// ESP32-CAM uses Hardware Serial1 (GPIO 9/10)
// Serial (Serial0) is reserved for USB debugging
HardwareSerial ESP32_CAM_SERIAL(1);  // Hardware Serial1 for ESP32-CAM
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

// Session counters for batch processing
int sessionPlasticCount = 0;
int sessionTinCount = 0;
int sessionRejectedCount = 0;

// Global state variables
bool binActivatedByQR = false;
String currentUserId = "";
String currentSessionId = "";
bool systemActive = false;
bool monitoringForTrash = false;
bool userPresent = false;

// GPS Configuration
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);  // Use HardwareSerial2 for ESP32
bool gpsEnabled = false;  // GPS will be enabled only if it gets a fix within 30 seconds at startup
const unsigned long GPS_INIT_TIMEOUT = 1000;  // 30 seconds to get initial GPS fix

// Servo objects (Rotating platform design)
Servo binLidServo;    // MG90S - Main bin lid
Servo dropperServo;   // MG90S - Trash dropper
Servo rotatorServo;   // MG995 - Platform rotator

// Timing variables
unsigned long lastGPSUpdateTime = 0;
unsigned long lastCapacityUpdateTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long lastCommandPollTime = 0;

// Compartment state (controlled by ESP32-CAM)
bool binLidOpen = false;
bool platformRotated = false;
int currentPlatformPosition = ROTATE_REJECTED;  // Track rotator position
unsigned long compartmentOpenTime = 0;
const unsigned long COMPARTMENT_OPEN_DURATION = 5000;  // 5 seconds auto-close

// Capacity readings
float plasticCapacity = 0.0;  // 0-100%
float tinCapacity = 0.0;      // 0-100%

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n=================================");
  Serial.println("EcoEarn Smart Bin Controller");
  Serial.println("Connected to ESP32-CAM");
  Serial.println("TRASH DROP DETECTION ENABLED");
  Serial.println("=================================\n");
  
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
  rotatorServo.write(ROTATE_REJECTED);   // Default to plastic position
  
  // Initialize GPS on HardwareSerial2 (GPIO16=RX, GPIO17=TX)
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS Module initializing...");
  Serial.println("Waiting for GPS fix (30 second timeout)...");
  
  // Try to get GPS fix for 30 seconds
  unsigned long gpsStartTime = millis();
  bool gpsFixObtained = false;
  
  while (millis() - gpsStartTime < GPS_INIT_TIMEOUT) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
    
    if (gps.location.isValid()) {
      gpsFixObtained = true;
      gpsEnabled = true;
      Serial.println("✓ GPS Fix Obtained!");
      Serial.print("├─ Latitude: ");
      Serial.println(gps.location.lat(), 6);
      Serial.print("├─ Longitude: ");
      Serial.println(gps.location.lng(), 6);
      Serial.print("└─ Time to fix: ");
      Serial.print((millis() - gpsStartTime) / 1000);
      Serial.println(" seconds\n");
      break;
    }
    
    // Show progress every 10 seconds
    if ((millis() - gpsStartTime) % 10000 < 100) {
      Serial.print(".");
    }
    
    delay(100);
  }
  
  if (!gpsFixObtained) {
    gpsEnabled = false;
    Serial.println("\n⚠️  GPS Fix NOT obtained within 30 seconds");
    Serial.println("GPS functionality DISABLED for this session");
    Serial.println("Bin will operate without GPS tracking\n");
  }
  
  // Initialize ESP32-CAM Serial Communication (Hardware Serial1 on GPIO32=TX, GPIO33=RX)
  ESP32_CAM_SERIAL.begin(ESP32_BAUD, SERIAL_8N1, CAM_RX_PIN, CAM_TX_PIN);
  Serial.println("ESP32-CAM Serial initialized on GPIO 32/33 (Serial1)");
  Serial.println("├─ RX: GPIO 33 (from CAM U0T/GPIO 1)");
  Serial.println("└─ TX: GPIO 32 (to CAM U0R/GPIO 3)");
  Serial.println("⚠️  GPIO 9/10 NOT USED - causes boot loop!\n");
  
  // Wait for ESP32-CAM ping before connecting to WiFi
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║   WAITING FOR ESP32-CAM PING         ║");
  Serial.println("╚════════════════════════════════════════╝");
  bool camConnected = waitForCAMPing();
  
  if (camConnected) {
    Serial.println("✓ ESP32-CAM ping received and responded!");
    Serial.println("LED will stay ON for 3 seconds...\n");
    digitalWrite(LED_PIN, HIGH);
    delay(3000);
    digitalWrite(LED_PIN, LOW);
  } else {
    Serial.println("✗ No ping from ESP32-CAM (timeout)!");
    Serial.println("LED will blink 5 times...\n");
    blinkLED(5, 500);
  }
  
  // Connect to WiFi
  connectToWiFi();
  
  // Construct API URLs using the server IP
  String baseUrl = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT);
  LOCATION_URL = baseUrl + "/api/iot/update-location";
  CAPACITY_URL = baseUrl + "/api/iot/update-capacity";
  HEARTBEAT_URL = baseUrl + "/api/iot/heartbeat";
  GET_COMMAND_URL = baseUrl + "/api/iot/get-command";
  RECYCLE_URL = baseUrl + "/api/iot/recycle";
  DEACTIVATE_BIN_URL = baseUrl + "/api/bins/deactivate";
  SESSION_DATA_URL = baseUrl + "/api/bins/session-data";
  
  Serial.println("API URLs configured:");
  Serial.println("├─ Server IP: " + String(SERVER_IP) + ":" + String(SERVER_PORT));
  Serial.println("├─ Session Data: " + SESSION_DATA_URL);
  Serial.println("└─ Get Command: " + GET_COMMAND_URL);
  Serial.println();
  
  // Test basic connectivity
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   NETWORK DIAGNOSTICS                 ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.print("├─ ESP32 IP: ");
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
  if (gpsEnabled) {
    Serial.println("GPS: ENABLED - Location tracking active");
  } else {
    Serial.println("GPS: DISABLED - Operating without location tracking");
  }
  Serial.println("Waiting for QR scan activation...");
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   🧪 TEST MODE AVAILABLE              ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("Commands:");
  Serial.println("  activate   - Bypass QR scan for testing");
  Serial.println("  deactivate - End test session\n");
}

bool waitForCAMPing() {
  Serial.println("Waiting for PING from ESP32-CAM...");
  
  // Clear any existing data
  while (ESP32_CAM_SERIAL.available()) {
    ESP32_CAM_SERIAL.read();
  }
  
  // Wait for PING from ESP32-CAM (10 second timeout)
  unsigned long startTime = millis();
  String response = "";
  bool pingReceived = false;
  
  while (millis() - startTime < 10000) {
    if (ESP32_CAM_SERIAL.available()) {
      char c = ESP32_CAM_SERIAL.read();
      if (c == '\n') {
        response.trim();
        if (response == "PING") {
          Serial.println("✓ Received PING from ESP32-CAM");
          ESP32_CAM_SERIAL.println("PONG");
          Serial.println("✓ Sent PONG response to ESP32-CAM");
          pingReceived = true;
          break;
        }
        response = "";
      } else {
        response += c;
      }
    }
    delay(10);
  }
  
  if (!pingReceived) {
    Serial.println("✗ No PING received from ESP32-CAM (timeout)");
    return false;
  }
  
  // Now wait for CAM_IP message
  Serial.println("Waiting for ESP32-CAM IP address...");
  startTime = millis();
  response = "";
  
  while (millis() - startTime < 5000) {
    if (ESP32_CAM_SERIAL.available()) {
      char c = ESP32_CAM_SERIAL.read();
      if (c == '\n') {
        response.trim();
        if (response.startsWith("CAM_IP:")) {
          String camIP = response.substring(7);  // Remove "CAM_IP:" prefix
          Serial.println("✓ ESP32-CAM IP Address: " + camIP);
          return true;
        }
        response = "";
      } else {
        response += c;
      }
    }
    delay(10);
  }
  
  Serial.println("✗ No CAM_IP received from ESP32-CAM (timeout)");
  return false;
}

void loop() {
  unsigned long currentTime = millis();
  
  // Read GPS data continuously (only if GPS is enabled)
  if (gpsEnabled) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
  }
  
  // Poll for commands from server
  // When bin is active: poll FASTER for disconnect commands (every 2 seconds)
  // When bin is inactive: poll normally (every 5 seconds)
  // SKIP polling when actively monitoring for trash to avoid interference
  unsigned long pollInterval = binActivatedByQR ? 2000 : COMMAND_POLL_INTERVAL;  // 2s when active, 5s when inactive
  
  if (currentTime - lastCommandPollTime >= pollInterval && !monitoringForTrash) {
    pollForCommands();
    lastCommandPollTime = currentTime;
  } else if (monitoringForTrash && currentTime - lastCommandPollTime >= pollInterval) {
    // Debug: Show when command polling is being skipped due to trash monitoring
    static unsigned long lastSkipMessage = 0;
    if (currentTime - lastSkipMessage >= 10000) {  // Show message every 10 seconds
      Serial.println("⏸️  Command polling paused - focusing on trash detection");
      lastSkipMessage = currentTime;
    }
  }
  
  // Check for TEST/BYPASS commands from Serial Monitor
  checkSerialBypassCommands();
  
  // Check for bin activation/deactivation commands from ESP32-CAM
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
  
  // Update system active state
  updateSystemState(currentTime);
  
  // Update GPS location periodically (only if GPS is enabled AND bin is NOT activated)
  // Skip GPS updates when user is active to avoid interfering with trash detection
  if (gpsEnabled && !binActivatedByQR && (currentTime - lastGPSUpdateTime >= GPS_UPDATE_INTERVAL || lastGPSUpdateTime == 0)) {
    if (gps.location.isValid()) {
      double latitude = gps.location.lat();
      double longitude = gps.location.lng();
      
      Serial.println("----------------------------------------");
      Serial.println("GPS Fix Acquired!");
      Serial.print("Latitude: ");
      Serial.println(latitude, 6);
      Serial.print("Longitude: ");
      Serial.println(longitude, 6);
      Serial.print("Satellites: ");
      Serial.println(gps.satellites.value());
      Serial.println("----------------------------------------");
      
      // Update location to server
      updateLocationToServer(latitude, longitude);
      
      lastGPSUpdateTime = currentTime;
    }
  }
  
  // Update capacity readings periodically
  // REMOVED: Automatic capacity updates disabled
  // Capacity will only be updated when user disconnects
  // if (currentTime - lastCapacityUpdateTime >= CAPACITY_UPDATE_INTERVAL) {
  //   updateCapacityReadings();
  //   updateCapacityToServer();
  //   lastCapacityUpdateTime = currentTime;
  // }
  
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

void updateLocationToServer(double latitude, double longitude) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot update location.");
    return;
  }
  
  Serial.println("\nUpdating location to server...");
  
  WiFiClient client;
  HTTPClient http;
  
  // Configure HTTP request
  http.begin(client, LOCATION_URL);
  http.addHeader("Content-Type", "application/json");
  
  // Create JSON payload
  String payload = "{";
  payload += "\"apiKey\":\"" + String(API_KEY) + "\",";
  payload += "\"latitude\":" + String(latitude, 6) + ",";
  payload += "\"longitude\":" + String(longitude, 6);
  payload += "}";
  
  Serial.println("Location Payload: " + payload);
  
  // Send POST request
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response: ");
    Serial.println(response);
    
    if (httpResponseCode == 200) {
      Serial.println("✓ Location updated successfully!");
      
      // Blink LED rapidly to indicate success
      blinkLED(5, 50);
    } else {
      Serial.println("✗ Server returned an error!");
    }
  } else {
    Serial.print("✗ Error sending location request: ");
    Serial.println(httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
  }
  
  http.end();
  Serial.println();
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

void sendSessionDataToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  WiFi not connected. Cannot send session data.");
    return;
  }

  HTTPClient http;
  http.begin(SESSION_DATA_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);

  // Create JSON payload with session data - use ArduinoJson for proper JSON construction
  DynamicJsonDocument doc(1024);
  doc["userId"] = currentUserId;
  
  JsonObject sessionData = doc.createNestedObject("sessionData");
  sessionData["plasticCount"] = sessionPlasticCount;
  sessionData["tinCount"] = sessionTinCount;
  sessionData["rejectedCount"] = sessionRejectedCount;
  sessionData["sessionId"] = currentSessionId;
  
  String payload;
  serializeJson(doc, payload);

  Serial.println("📤 Sending session data to server:");
  Serial.println("├─ Plastic: " + String(sessionPlasticCount));
  Serial.println("├─ Tin: " + String(sessionTinCount));
  Serial.println("├─ Rejected: " + String(sessionRejectedCount));
  Serial.println("└─ Payload: " + payload);

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("✅ Session data sent successfully!");
    Serial.println("Response: " + response);
  } else {
    Serial.print("❌ Error sending session data: ");
    Serial.println(httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
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
    
    // WAIT FOR USER TO REMOVE HAND
    Serial.println("\n⏳ Waiting 2 seconds for user to remove hand...");
    delay(2000);  // 2 second delay
    Serial.println("✓ Delay complete - proceeding with capture\n");
    
    Serial.println("📸 TRIGGERING ESP32-CAM:");
    Serial.println("├─ Sending command: TRASH_DETECTED");
    Serial.println("├─ ESP32-CAM will capture 5 images");
    Serial.println("├─ Backend API: http://213.35.114.162:5001/identify/material");
    Serial.println("└─ Waiting for classification result...\n");
    
    // Stop monitoring
    monitoringForTrash = false;
    
    // Notify ESP32-CAM to capture and identify
    ESP32_CAM_SERIAL.println("TRASH_DETECTED");
    
    Serial.println("✅ Command sent to ESP32-CAM via Serial1");
    Serial.println("⏳ ESP32-CAM should respond within 30 seconds");
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
      ESP32_CAM_SERIAL.println("BIN_LOCKED");
      return;
    }
    
    // Process compartment commands - NOW COUNTING ITEMS INSTEAD OF SENDING REAL-TIME TRANSACTIONS
    if (command == CMD_OPEN_PLASTIC) {
      Serial.println("📦 Plastic bottle detected - counting...");
      sessionPlasticCount++;
      sortPlasticItem();
    } else if (command == CMD_OPEN_TIN) {
      Serial.println("🥫 Tin can detected - counting...");
      sessionTinCount++;
      sortTinItem();
    } else if (command == CMD_OPEN_REJECTED) {
      Serial.println("❌ Rejected item detected - counting...");
      sessionRejectedCount++;
      sortRejectedItem();
    } else if (command == CMD_CLOSE_ALL) {
      closeAllCompartments();
    } else if (command == CMD_STATUS) {
      sendStatusToESP32();
    } else if (command == "PONG") {
      // Response to ping test
      Serial.println("📩 Received PONG from ESP32-CAM");
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
  rotatorServo.write(ROTATE_REJECTED);
  currentPlatformPosition = ROTATE_REJECTED;
  delay(1000);
  
  Serial.println("✓ Plastic item sorted");
  
  // Update capacity readings and send to server
  updateCapacityReadings();
  updateCapacityToServer();
  
  // Re-enable trash monitoring for next item
  monitoringForTrash = true;
  
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
  rotatorServo.write(ROTATE_REJECTED);
  currentPlatformPosition = ROTATE_REJECTED;
  delay(1500);  // Wait for rotation back to 0 degrees
  
  Serial.println("✓ Tin item sorted");
  
  // Update capacity readings and send to server
  updateCapacityReadings();
  updateCapacityToServer();
  
  // Re-enable trash monitoring for next item
  monitoringForTrash = true;
  
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
  rotatorServo.write(ROTATE_REJECTED);
  currentPlatformPosition = ROTATE_REJECTED;
  delay(2000);  // Longer wait for 180° rotation back to 0
  
  Serial.println("✓ Rejected item sorted");
  
  // Update capacity readings and send to server
  updateCapacityReadings();
  updateCapacityToServer();
  
  // Re-enable trash monitoring for next item
  monitoringForTrash = true;
  
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
  Serial.println("  ✓ GPS updates: PAUSED (saves bandwidth)");
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
  
  ESP32_CAM_SERIAL.println("BIN_ACTIVATED");
}

void deactivateBin() {
  Serial.println("========================================");
  Serial.println("🔒 BIN DEACTIVATED");
  Serial.println("Session ended for user: " + currentUserId);
  Serial.println("");
  Serial.println("📊 SESSION SUMMARY:");
  Serial.println("├─ Plastic bottles: " + String(sessionPlasticCount));
  Serial.println("├─ Tin cans: " + String(sessionTinCount));
  Serial.println("└─ Rejected items: " + String(sessionRejectedCount));
  Serial.println("");
  
  // SEND SESSION DATA TO SERVER BEFORE DEACTIVATING
  if (sessionPlasticCount > 0 || sessionTinCount > 0 || sessionRejectedCount > 0) {
    Serial.println("📤 Sending session data to server...");
    sendSessionDataToServer();
  } else {
    Serial.println("ℹ️  No items recycled in this session");
  }
  
  Serial.println("⚡ NORMAL MODE RESUMED:");
  Serial.println("  ✓ Heartbeat: ACTIVE");
  Serial.println("  ✓ Command polling: ACTIVE");
  Serial.println("  ✓ GPS updates: ACTIVE");
  Serial.println("  ✓ Trash detection: STANDBY");
  Serial.println("========================================");
  
  // CLOSE LID FIRST - User disconnected, bin should close
  Serial.println("\n🚪 Closing bin lid...");
  binLidServo.write(LID_CLOSED);
  binLidOpen = false;
  delay(1000);  // Wait for lid to close
  Serial.println("✓ Lid closed\n");
  
  // Reset all servos to default positions
  dropperServo.write(DROPPER_HOLD);
  rotatorServo.write(ROTATE_REJECTED);
  currentPlatformPosition = ROTATE_REJECTED;
  platformRotated = false;
  
  // Reset state
  binActivatedByQR = false;
  currentUserId = "";
  currentSessionId = "";
  userPresent = false;
  systemActive = false;
  monitoringForTrash = false;
  
  // RESET SESSION COUNTERS
  sessionPlasticCount = 0;
  sessionTinCount = 0;
  sessionRejectedCount = 0;
  
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("✓ All systems reset to standby mode\n");
  
  // FINAL CAPACITY UPDATE ON USER DISCONNECT
  Serial.println("📊 Updating final capacity readings...");
  updateCapacityReadings();
  updateCapacityToServer();
  
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

void pollForCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  HTTPClient http;
  
  // Set timeout to avoid hanging
  http.setTimeout(5000);  // 5 second timeout
  
  // Begin connection with URL (no need for separate WiFiClient)
  if (!http.begin(GET_COMMAND_URL)) {
    return;
  }
  
  // Enable connection reuse
  http.setReuse(false);  // Don't reuse connections to avoid stale connection issues
  
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
    
    // Detailed error diagnostics
    if (httpResponseCode == -1) {
      Serial.println("\n🔍 Connection Error Diagnostics:");
      Serial.println("├─ Possible causes:");
      Serial.println("│  1. Server not running (check if Next.js dev server is active)");
      Serial.println("│  2. Wrong IP address (verify server IP: 192.168.31.196)");
      Serial.println("│  3. Firewall blocking connection");
      Serial.println("│  4. Server not on same network as ESP32");
      Serial.println("└─ Action: Verify server is running and reachable");
      
      // Try to ping the server (basic connectivity test)
      Serial.println("\n🔧 Troubleshooting:");
      Serial.print("├─ Can ESP32 reach server? Run this on PC: ping ");
      Serial.println(WiFi.localIP());
      Serial.println("└─ Is server running? Check: http://192.168.31.196:3000/api/iot/get-command");
    }
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

String getISOTimestamp() {
  // If you have NTP sync, use that. Otherwise use millis-based approximation
  unsigned long epoch = millis() / 1000;  // Rough approximation
  time_t now = epoch;
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  
  return String(buffer);
}
