/*
 * EcoEarn Smart Bin Controller - TEST VERSION
 *
 * This Arduino sketch tests the smart bin functionality with:
 * - User detection with Sharp IR sensor
 * - Bin opening with servo motor
 * - Capacity monitoring with 2 ultrasonic sensors
 * - Compartment control with 3 servo motors
 *
 * NO GPS REQUIRED - Uses static coordinates for testing
 *
 * Hardware Required:
 * - ESP32 38-pin Development Board
 * - 1x Sharp 2Y0A21 IR Distance Sensor (user detection)
 * - 2x HC-SR04 Ultrasonic Sensors (capacity monitoring)
 * - 3x Servo Motors (MG90S or similar)
 *
 * Libraries Required:
 * - WiFi (ESP32 built-in)
 * - HTTPClient (ESP32 built-in)
 * - ESP32Servo (install from Library Manager)
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// ============================================
// CONFIGURATION - EDIT THESE VALUES
// ============================================

// WiFi credentials
const char* WIFI_SSID = "PLDT_Home_7CB79";
const char* WIFI_PASSWORD = "Hayao1014";

// API Configuration
const char* API_KEY = "YOUR_BIN_API_KEY_HERE";  // Replace with your bin's API key
const char* LOCATION_URL = "https://your-domain.com/api/iot/update-location";  // Location updates
const char* CAPACITY_URL = "https://your-domain.com/api/iot/update-capacity";  // Capacity updates

// Test coordinates (replace with your actual location)
const double TEST_LATITUDE = 14.6760;   // Example: Manila coordinates
const double TEST_LONGITUDE = 121.0437;

// Update intervals (in milliseconds)
const unsigned long LOCATION_UPDATE_INTERVAL = 60000;  // 1 minute for location (test mode)
const unsigned long CAPACITY_UPDATE_INTERVAL = 30000;  // 30 seconds for capacity
const unsigned long USER_CHECK_INTERVAL = 500;         // 500ms for user presence detection
const unsigned long STANDBY_TIMEOUT = 30000;           // 30 seconds timeout to standby mode

// ============================================
// PIN CONFIGURATION (ESP32 38-pin)
// ============================================

// User Presence Detection Sensor (Sharp 2Y0A21 IR Distance Sensor)
const int USER_DETECTION_PIN = 34;  // GPIO34 (ADC1_CH6, input-only)

// Compartment 1 Capacity Sensor (Ultrasonic for plastic)
const int COMP1_TRIG_PIN = 25;  // GPIO25
const int COMP1_ECHO_PIN = 26;  // GPIO26

// Compartment 2 Capacity Sensor (Ultrasonic for tin)
const int COMP2_TRIG_PIN = 27;  // GPIO27
const int COMP2_ECHO_PIN = 14;  // GPIO14

// Servo Motor Pins (Rotating platform design)
const int BIN_LID_SERVO_PIN = 12;   // GPIO12 - MG90S: Opens/closes main bin lid
const int DROPPER_SERVO_PIN = 13;   // GPIO13 - MG90S: Drops trash into compartment
const int ROTATOR_SERVO_PIN = 15;   // GPIO15 - MG995: Rotates platform to compartment

// Status LED
const int LED_PIN = 2;  // GPIO2 (built-in LED on most ESP32 boards)

// ============================================
// BIN CONFIGURATION
// ============================================

// Ultrasonic sensor distances (in cm)
const float BIN_HEIGHT = 30.0;  // Total height of bin compartments

// Sharp 2Y0A21 IR Distance Sensor configuration (ESP32 12-bit ADC: 0-4095)
const int USER_DETECTION_THRESHOLD = 1650;  // ADC threshold for user detection (closer = higher value)
const int USER_DETECTION_MIN_ADC = 500;     // Minimum valid reading (80cm distance)
const int USER_DETECTION_MAX_ADC = 3500;    // Maximum valid reading (10cm distance)
// Detection range: ~10-80cm, optimal 20-50cm
// ESP32 ADC is 12-bit (0-4095) vs ESP8266 10-bit (0-1023), thresholds scaled ~4x

// Servo angles
// Bin Lid Servo (MG90S)
const int LID_CLOSED = 0;           // Lid closed
const int LID_OPEN = 90;            // Lid open

// Dropper Servo (MG90S)
const int DROPPER_HOLD = 0;         // Holding trash
const int DROPPER_RELEASE = 90;     // Releasing trash

// Rotator Servo (MG995)
const int ROTATE_PLASTIC = 0;       // Position for plastic compartment
const int ROTATE_TIN = 90;          // Position for tin compartment
const int ROTATE_REJECTED = 180;    // Position for rejected compartment

// Serial Communication with ESP32-CAM
#define ESP32_SERIAL Serial  // Use hardware Serial for ESP32-CAM communication
#define ESP32_BAUD 9600

// Compartment control commands from ESP32-CAM
#define CMD_OPEN_PLASTIC "OPEN_PLASTIC"
#define CMD_OPEN_TIN "OPEN_TIN"
#define CMD_OPEN_REJECTED "OPEN_REJECTED"
#define CMD_CLOSE_ALL "CLOSE_ALL"
#define CMD_STATUS "STATUS"
#define CMD_CHECK_USER "CHECK_USER"

// System state
bool userPresent = false;
bool systemActive = false;
bool userDetectionRequested = false;  // New flag for on-demand detection
unsigned long lastUserDetectionTime = 0;
unsigned long lastUserCheckTime = 0;

// Servo objects (Rotating platform design)
Servo binLidServo;    // MG90S - Main bin lid
Servo dropperServo;   // MG90S - Trash dropper
Servo rotatorServo;   // MG995 - Platform rotator

// Timing variables
unsigned long lastLocationUpdateTime = 0;
unsigned long lastCapacityUpdateTime = 0;

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
  Serial.println("ESP32 38-pin TEST VERSION");
  Serial.println("Connected to ESP32-CAM");
  Serial.println("USER PRESENCE DETECTION ENABLED");
  Serial.println("=================================\n");

  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize user presence detection sensor (Sharp 2Y0A21 - analog input)
  pinMode(USER_DETECTION_PIN, INPUT);

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

  Serial.println("\nSystem ready!");
  Serial.println("TEST MODE: Using static coordinates");
  Serial.print("Test Latitude: ");
  Serial.println(TEST_LATITUDE, 6);
  Serial.print("Test Longitude: ");
  Serial.println(TEST_LONGITUDE, 6);
  Serial.println("\nWaiting for user presence detection...\n");
}

void loop() {
  unsigned long currentTime = millis();

  // Read GPS data continuously
  // GPS code would be here in production version

  // Only check for user presence when requested by ESP32-CAM
  if (userDetectionRequested && currentTime - lastUserCheckTime >= USER_CHECK_INTERVAL) {
    checkUserPresence();
    lastUserCheckTime = currentTime;
  }

  // Update system active state based on user presence
  updateSystemState(currentTime);

  // Only process ESP32 commands and servo operations when system is active
  if (systemActive) {
    // Check for commands from ESP32-CAM
    checkESP32Commands();

    // Auto-close bin lid after timeout (if left open)
    if (binLidOpen && currentTime - compartmentOpenTime >= COMPARTMENT_OPEN_DURATION) {
      closeAllCompartments();
    }
  }

  // Update location periodically (test mode - static coordinates)
  if (currentTime - lastLocationUpdateTime >= LOCATION_UPDATE_INTERVAL || lastLocationUpdateTime == 0) {
    updateLocationToServer(TEST_LATITUDE, TEST_LONGITUDE);
    lastLocationUpdateTime = currentTime;
  }

  // Update capacity readings periodically (reduced frequency when in standby)
  unsigned long capacityInterval = systemActive ? CAPACITY_UPDATE_INTERVAL : (CAPACITY_UPDATE_INTERVAL * 3);
  if (currentTime - lastCapacityUpdateTime >= capacityInterval) {
    updateCapacityReadings();
    updateCapacityToServer();
    lastCapacityUpdateTime = currentTime;
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

  Serial.println("\nUpdating location to server (TEST MODE)...");

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
    Serial.println("WiFi not connected. Cannot update capacity.");
    return;
  }

  Serial.println("\nUpdating capacity to server...");

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

  Serial.println("Capacity Payload: " + payload);

  // Send POST request
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response: ");
    Serial.println(response);

    if (httpResponseCode == 200) {
      Serial.println("✓ Capacity updated successfully!");
      blinkLED(3, 100);
    } else {
      Serial.println("✗ Server returned an error!");
    }
  } else {
    Serial.print("✗ Error sending capacity request: ");
    Serial.println(httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
  }

  http.end();
  Serial.println();
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

  Serial.println("=== Capacity Readings ===");
  Serial.print("Plastic Compartment: ");
  Serial.print(plasticCapacity, 1);
  Serial.println("%");
  Serial.print("Tin Compartment: ");
  Serial.print(tinCapacity, 1);
  Serial.println("%");
  Serial.println("========================");
}

void checkUserPresence() {
  // Read analog value from Sharp 2Y0A21 IR sensor
  int adcValue = analogRead(USER_DETECTION_PIN);
  
  // Filter out invalid readings
  if (adcValue < USER_DETECTION_MIN_ADC) {
    // No object detected or too far (>80cm)
    if (userPresent) {
      userPresent = false;
      Serial.println("👤 User left. System will enter standby mode soon...");
    } else if (userDetectionRequested && millis() - lastUserCheckTime >= 3000) {
      // If user detection was requested but no user found after 3 seconds, send failure
      Serial.println("👤 No user detected during requested check");
      ESP32_SERIAL.println("NO_USER_DETECTED");
      userDetectionRequested = false;  // Reset the request flag
    }
    return;
  }
  
  // Check if user is close enough (ADC value above threshold)
  if (adcValue >= USER_DETECTION_THRESHOLD) {
    // User detected (closer object = higher voltage/ADC)
    if (!userPresent) {
      userPresent = true;
      lastUserDetectionTime = millis();
      Serial.print("👤 User detected! ADC Value: ");
      Serial.print(adcValue);
      Serial.println(" - System activating...");
      digitalWrite(LED_PIN, HIGH);  // Turn on LED to indicate active state
      
      // If user detection was requested, send confirmation to ESP32
      if (userDetectionRequested) {
        ESP32_SERIAL.println("USER_DETECTED");
        userDetectionRequested = false;  // Reset the request flag
      }
    } else {
      // User still present, update detection time
      lastUserDetectionTime = millis();
    }
  } else {
    // Object detected but too far away
    if (userPresent) {
      userPresent = false;
      Serial.print("👤 User moved away. ADC Value: ");
      Serial.println(adcValue);
    }
  }
}

void updateSystemState(unsigned long currentTime) {
  bool shouldBeActive = userPresent || (currentTime - lastUserDetectionTime < STANDBY_TIMEOUT);
  
  if (shouldBeActive != systemActive) {
    systemActive = shouldBeActive;
    
    if (systemActive) {
      Serial.println("🔋 System ACTIVE - Ready for material identification");
      // Ensure all compartments are closed when activating
      closeAllCompartments();
    } else {
      Serial.println("💤 System STANDBY - No user detected");
      digitalWrite(LED_PIN, LOW);  // Turn off LED in standby
      // Ensure all compartments are closed when entering standby
      closeAllCompartments();
      // Reset user detection request
      userDetectionRequested = false;
    }
  }
}

void checkESP32Commands() {
  if (ESP32_SERIAL.available()) {
    String command = ESP32_SERIAL.readStringUntil('\n');
    command.trim();
    
    Serial.println("Received command from ESP32-CAM: " + command);
    
    if (command == CMD_CHECK_USER) {
      // Start user detection process
      userDetectionRequested = true;
      lastUserCheckTime = 0;  // Force immediate check
      Serial.println("🔍 Starting user detection...");
      ESP32_SERIAL.println("USER_CHECK_STARTED");
      return;
    }
    
    // Only process other commands if system is active (user present)
    if (!systemActive) {
      Serial.println("⚠️  System in standby mode - ignoring command");
      ESP32_SERIAL.println("STANDBY_MODE");
      return;
    }
    
    if (command == CMD_OPEN_PLASTIC) {
      openPlasticCompartment();
    } else if (command == CMD_OPEN_TIN) {
      openTinCompartment();
    } else if (command == CMD_OPEN_REJECTED) {
      openRejectedCompartment();
    } else if (command == CMD_CLOSE_ALL) {
      closeAllCompartments();
    } else if (command == CMD_STATUS) {
      sendStatusToESP32();
    } else {
      Serial.println("Unknown command: " + command);
    }
  }
}

void openPlasticCompartment() {
  Serial.println("Opening bin for PLASTIC sorting...");
  
  binLidServo.write(LID_OPEN);
  binLidOpen = true;
  delay(2000);
  
  binLidServo.write(LID_CLOSED);
  binLidOpen = false;
  delay(1000);
  
  rotatorServo.write(ROTATE_PLASTIC);
  currentPlatformPosition = ROTATE_PLASTIC;
  platformRotated = true;
  delay(1000);
  
  dropperServo.write(DROPPER_RELEASE);
  delay(1000);
  dropperServo.write(DROPPER_HOLD);
  
  compartmentOpenTime = millis();
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("Plastic sorting complete");
  ESP32_SERIAL.println("PLASTIC_OPENED");
}

void openTinCompartment() {
  Serial.println("Opening bin for TIN sorting...");
  
  binLidServo.write(LID_OPEN);
  binLidOpen = true;
  delay(2000);
  
  binLidServo.write(LID_CLOSED);
  binLidOpen = false;
  delay(1000);
  
  rotatorServo.write(ROTATE_TIN);
  currentPlatformPosition = ROTATE_TIN;
  platformRotated = true;
  delay(1000);
  
  dropperServo.write(DROPPER_RELEASE);
  delay(1000);
  dropperServo.write(DROPPER_HOLD);
  
  compartmentOpenTime = millis();
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("Tin sorting complete");
  ESP32_SERIAL.println("TIN_OPENED");
}

void openRejectedCompartment() {
  Serial.println("Opening bin for REJECTED sorting...");
  
  binLidServo.write(LID_OPEN);
  binLidOpen = true;
  delay(2000);
  
  binLidServo.write(LID_CLOSED);
  binLidOpen = false;
  delay(1000);
  
  rotatorServo.write(ROTATE_REJECTED);
  currentPlatformPosition = ROTATE_REJECTED;
  platformRotated = true;
  delay(1500);
  
  dropperServo.write(DROPPER_RELEASE);
  delay(1000);
  dropperServo.write(DROPPER_HOLD);
  
  compartmentOpenTime = millis();
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("Rejected sorting complete");
  ESP32_SERIAL.println("REJECTED_OPENED");
}

void closeAllCompartments() {
  binLidServo.write(LID_CLOSED);
  dropperServo.write(DROPPER_HOLD);
  rotatorServo.write(ROTATE_PLASTIC);
  
  binLidOpen = false;
  platformRotated = false;
  currentPlatformPosition = ROTATE_PLASTIC;
  
  Serial.println("All servos returned to default position");
  digitalWrite(LED_PIN, LOW);
  
  ESP32_SERIAL.println("ALL_CLOSED");
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
  
  ESP32_SERIAL.println(status);
  Serial.println("Sent status to ESP32-CAM: " + status);
}void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}
