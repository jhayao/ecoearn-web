/*
 * EcoEarn Bin Location Tracker - TEST VERSION
 * 
 * This is a simplified test version that sends static GPS coordinates
 * to verify your WiFi and API setup before connecting the GPS module.
 * 
 * Use this sketch to:
 * 1. Test WiFi connectivity
 * 2. Verify API key and server communication
 * 3. Ensure hardware is working correctly
 * 
 * Once this works, switch to the full version with GPS module.
 */

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClient.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <HTTPClient.h>
#endif

// ============================================
// CONFIGURATION - EDIT THESE VALUES
// ============================================

// WiFi credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// API Configuration
const char* API_KEY = "YOUR_BIN_API_KEY_HERE";
const char* SERVER_URL = "https://your-domain.com/api/iot/update-location";

// Test GPS coordinates (Manila, Philippines as example)
const double TEST_LATITUDE = 14.5995;   // Change to your test location
const double TEST_LONGITUDE = 120.9842; // Change to your test location

// Test interval (30 seconds for testing)
const unsigned long TEST_INTERVAL = 30000;

// ============================================
// END CONFIGURATION
// ============================================

unsigned long lastUpdateTime = 0;
int updateCount = 0;
const int LED_PIN = D4;  // Built-in LED on NodeMCU (GPIO2)

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n=================================");
  Serial.println("EcoEarn Test Mode");
  Serial.println("=================================\n");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  connectToWiFi();
  
  Serial.println("\nTest mode ready!");
  Serial.println("Will send test coordinates every 30 seconds");
  Serial.print("Test Location: ");
  Serial.print(TEST_LATITUDE, 6);
  Serial.print(", ");
  Serial.println(TEST_LONGITUDE, 6);
  Serial.println();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Send update every TEST_INTERVAL
  if (currentTime - lastUpdateTime >= TEST_INTERVAL || lastUpdateTime == 0) {
    updateCount++;
    
    Serial.println("========================================");
    Serial.print("Test Update #");
    Serial.println(updateCount);
    Serial.println("========================================");
    
    updateLocationToServer(TEST_LATITUDE, TEST_LONGITUDE);
    
    lastUpdateTime = currentTime;
  }
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    connectToWiFi();
  }
  
  // Blink LED to show system is running
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  
  delay(1000);
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ WiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    // Blink LED 3 times to indicate success
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      delay(200);
    }
  } else {
    Serial.println("✗ Failed to connect to WiFi!");
    Serial.println("Please check your SSID and password");
  }
}

void updateLocationToServer(double latitude, double longitude) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("✗ WiFi not connected. Cannot update location.");
    return;
  }
  
  Serial.println("\nSending location update to server...");
  Serial.print("Coordinates: ");
  Serial.print(latitude, 6);
  Serial.print(", ");
  Serial.println(longitude, 6);
  
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000); // 15 second timeout
  
  // Create JSON payload
  String payload = "{";
  payload += "\"apiKey\":\"" + String(API_KEY) + "\",";
  payload += "\"latitude\":" + String(latitude, 6) + ",";
  payload += "\"longitude\":" + String(longitude, 6);
  payload += "}";
  
  Serial.println("Payload: " + payload);
  
  // Send POST request
  Serial.println("Sending HTTP POST request...");
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    
    Serial.println("\n--- Server Response ---");
    Serial.print("HTTP Code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response: ");
    Serial.println(response);
    Serial.println("----------------------\n");
    
    if (httpResponseCode == 200) {
      Serial.println("✓✓✓ SUCCESS! Location updated successfully! ✓✓✓");
      
      // Rapid blink to indicate success
      for (int i = 0; i < 6; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);
        delay(50);
      }
    } else if (httpResponseCode == 401) {
      Serial.println("✗ ERROR: Invalid API key!");
      Serial.println("  → Check that your API_KEY is correct");
    } else if (httpResponseCode == 400) {
      Serial.println("✗ ERROR: Bad request!");
      Serial.println("  → Check coordinates format");
    } else {
      Serial.println("✗ ERROR: Unexpected response from server");
    }
  } else {
    Serial.println("\n✗✗✗ ERROR: Failed to send request ✗✗✗");
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    Serial.print("Error message: ");
    Serial.println(http.errorToString(httpResponseCode));
    
    // Troubleshooting hints
    Serial.println("\nTroubleshooting:");
    Serial.println("  1. Check SERVER_URL is correct");
    Serial.println("  2. Verify server is online");
    Serial.println("  3. Check firewall settings");
    Serial.println("  4. Ensure HTTPS certificate is valid");
  }
  
  http.end();
  
  Serial.println("========================================\n");
}
