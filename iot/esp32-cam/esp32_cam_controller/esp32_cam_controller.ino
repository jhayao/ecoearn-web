/*
 * ESP32-CAM Controller for Recycling Machine
 * 
 * Features:
 * - Multi-capture voting (5 images with flash)
 * - Direct backend API communication
 * - Serial communication with ESP32 main board
 * 
 * Hardware Connections:
 * ESP32 Main Board (Hardware Serial):
 *   - U0R/GPIO 3 (RX) ← ESP32 GPIO 18 (TX)
 *   - U0T/GPIO 1 (TX) → ESP32 GPIO 19 (RX)
 *   - GND → GND
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


// const char* WIFI_SSID = "FTTx-807a60";
// const char* WIFI_PASSWORD = "mark0523";
// WiFi Configuration
// const char* ssid = "FTTx-807a60";
// const char* password = "mark0523";

const char* ssid = "Xiaomi_53DE";
const char* password = "hayao1014";

// Backend Server URL
const char* IDENTIFICATION_BACKEND_URL = "http://213.35.114.162:5001/identify/material";

// ESP32 Main Board Serial
#define ESP32_SERIAL Serial

// Camera Model (ESP32-CAM AI Thinker)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Flash LED
#define FLASH_GPIO 4

// Multi-capture configuration
#define NUM_CAPTURES 5
#define CAPTURE_DELAY_MS 300

// Timing
const unsigned long CAPTURE_COOLDOWN = 2000;
const unsigned long AUTO_CAPTURE_DELAY = 2500;
unsigned long lastCaptureTime = 0;
unsigned long lidOpenedTime = 0;
bool waitingForAutoCapture = false;

// ESP32 Communication Status
bool esp32Connected = false;
unsigned long lastPingAttempt = 0;
const unsigned long PING_RETRY_INTERVAL = 5000;

// Latest identification result
String lastMaterialType = "None";
float lastConfidence = 0.0;
String lastAction = "";

void setup() {
  pinMode(FLASH_GPIO, OUTPUT);
  digitalWrite(FLASH_GPIO, LOW);
  for(int i = 0; i < 3; i++) {
      digitalWrite(FLASH_GPIO, HIGH);
      delay(200);
      digitalWrite(FLASH_GPIO, LOW);
      delay(200);
    }

  ESP32_SERIAL.begin(9600);
  delay(1000);
  
  
  
  initializeCamera();
  connectToWiFi();
}

void loop() {
  checkESP32Commands();
  
  if (waitingForAutoCapture) {
    unsigned long currentTime = millis();
    if (currentTime - lidOpenedTime >= AUTO_CAPTURE_DELAY) {
      waitingForAutoCapture = false;
      if (captureAndIdentify()) {
        sendResultToESP32();
      }
    }
  }
  
  delay(10);
}

void initializeCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  esp_camera_init(&config);
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Flash LED 3 times consecutively to indicate WiFi connection
    for(int i = 0; i < 3; i++) {
      digitalWrite(FLASH_GPIO, HIGH);
      delay(200);
      digitalWrite(FLASH_GPIO, LOW);
      delay(200);
    }
    
    // Send PING to ESP32 bin tracker after WiFi connection
    ESP32_SERIAL.println("PING");
    Serial.println("Sent PING to ESP32 bin tracker");
    
    // Send CAM IP address to ESP32 bin tracker
    ESP32_SERIAL.println("CAM_IP:" + WiFi.localIP().toString());
    Serial.println("Sent CAM IP to ESP32 bin tracker: " + WiFi.localIP().toString());
  } else {
    Serial.println("WiFi connection failed!");
  }
}

// ====================================
// Capture and Identification
// ====================================

bool captureAndIdentify() {
  if (WiFi.status() != WL_CONNECTED) {
    lastMaterialType = "Error";
    lastConfidence = 0.0;
    lastAction = "WiFi disconnected";
    return false;
  }
  
  String materials[NUM_CAPTURES];
  float confidences[NUM_CAPTURES];
  int successCount = 0;
  
  for (int i = 0; i < NUM_CAPTURES; i++) {
    digitalWrite(FLASH_GPIO, HIGH);
    delay(100);
    
    camera_fb_t *fb = esp_camera_fb_get();
    digitalWrite(FLASH_GPIO, LOW);
    
    if (!fb) {
      materials[i] = "failed";
      confidences[i] = 0.0;
      delay(CAPTURE_DELAY_MS);
      continue;
    }
    
    HTTPClient http;
    http.setTimeout(10000);
    http.begin(IDENTIFICATION_BACKEND_URL);
    http.addHeader("Content-Type", "image/jpeg");
    
    int httpResponseCode = http.POST(fb->buf, fb->len);
    esp_camera_fb_return(fb);
    
    if (httpResponseCode == 200) {
      String response = http.getString();
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, response);
      
      if (!error && doc["success"] == true) {
        String materialType = doc["materialType"] | "unknown";
        materials[i] = materialType;
        confidences[i] = doc["confidence"] | 0.0;
        successCount++;
        
        // DEBUG: Show what backend returned
        ESP32_SERIAL.print("  Backend response ");
        ESP32_SERIAL.print(i + 1);
        ESP32_SERIAL.print(": ");
        ESP32_SERIAL.print(materialType);
        ESP32_SERIAL.print(" (");
        ESP32_SERIAL.print(confidences[i], 2);
        ESP32_SERIAL.println("%)");
      } else {
        materials[i] = "failed";
        confidences[i] = 0.0;
        ESP32_SERIAL.print("  Response ");
        ESP32_SERIAL.print(i + 1);
        ESP32_SERIAL.println(": FAILED or error");
      }
    } else {
      materials[i] = "failed";
      confidences[i] = 0.0;
    }
    
    http.end();
    
    if (i < NUM_CAPTURES - 1) {
      delay(CAPTURE_DELAY_MS);
    }
  }
  
  if (successCount == 0) {
    lastMaterialType = "Error";
    lastConfidence = 0.0;
    lastAction = "reject";
    return false;
  }
  
  int plasticCount = 0;
  int tinCount = 0;
  float maxPlasticConfidence = 0.0;
  float maxTinConfidence = 0.0;
  
  // DEBUG: Print all capture results
  ESP32_SERIAL.println("\n╔════════════════════════════════════════╗");
  ESP32_SERIAL.println("║   📸 CAPTURE RESULTS (5 images)      ║");
  ESP32_SERIAL.println("╚════════════════════════════════════════╝");
  
  for (int i = 0; i < NUM_CAPTURES; i++) {
    ESP32_SERIAL.print("Image ");
    ESP32_SERIAL.print(i + 1);
    ESP32_SERIAL.print(": ");
    ESP32_SERIAL.print(materials[i]);
    ESP32_SERIAL.print(" (");
    ESP32_SERIAL.print(confidences[i], 2);
    ESP32_SERIAL.println("%)");
  }
  ESP32_SERIAL.println();
  
  // Count votes
  for (int i = 0; i < NUM_CAPTURES; i++) {
    String material = materials[i];
    material.toLowerCase();  // Convert to lowercase for comparison
    
    // Check for plastic variants
    if (material == "plastic" || 
        material == "plastic_bottle" || 
        material == "bottle" ||
        material.indexOf("plastic") >= 0) {
      plasticCount++;
      if (confidences[i] > maxPlasticConfidence) {
        maxPlasticConfidence = confidences[i];
      }
    } 
    // Check for tin/metal variants
    else if (material == "tin" || 
             material == "tin_can" || 
             material == "metal" || 
             material == "can" ||
             material.indexOf("tin") >= 0 ||
             material.indexOf("metal") >= 0) {
      tinCount++;
      if (confidences[i] > maxTinConfidence) {
        maxTinConfidence = confidences[i];
      }
    }
  }
  
  ESP32_SERIAL.println("📊 VOTE COUNTING:");
  ESP32_SERIAL.print("├─ Plastic votes: ");
  ESP32_SERIAL.print(plasticCount);
  ESP32_SERIAL.print(" (max confidence: ");
  ESP32_SERIAL.print(maxPlasticConfidence, 2);
  ESP32_SERIAL.println("%)");
  ESP32_SERIAL.print("├─ Tin votes: ");
  ESP32_SERIAL.print(tinCount);
  ESP32_SERIAL.print(" (max confidence: ");
  ESP32_SERIAL.print(maxTinConfidence, 2);
  ESP32_SERIAL.println("%)");
  ESP32_SERIAL.print("└─ Other/Failed: ");
  ESP32_SERIAL.println(NUM_CAPTURES - plasticCount - tinCount);
  
  String finalMaterial = "rejected";
  float finalConfidence = 0.0;
  String finalAction = "reject";
  
  // DECISION LOGIC: Majority vote wins, confidence breaks ties
  ESP32_SERIAL.println("\n🧮 DECISION LOGIC:");
  
  if (plasticCount > tinCount) {
    // Plastic has MORE votes - PLASTIC WINS
    finalMaterial = "plastic";
    finalConfidence = maxPlasticConfidence;
    finalAction = "sort_plastic";
    ESP32_SERIAL.print("├─ Plastic wins by majority (");
    ESP32_SERIAL.print(plasticCount);
    ESP32_SERIAL.print(" vs ");
    ESP32_SERIAL.print(tinCount);
    ESP32_SERIAL.println(")");
  }
  else if (tinCount > plasticCount) {
    // Tin has MORE votes - TIN WINS
    finalMaterial = "tin";
    finalConfidence = maxTinConfidence;
    finalAction = "sort_tin_can";
    ESP32_SERIAL.print("├─ Tin wins by majority (");
    ESP32_SERIAL.print(tinCount);
    ESP32_SERIAL.print(" vs ");
    ESP32_SERIAL.print(plasticCount);
    ESP32_SERIAL.println(")");
  }
  else if (plasticCount == tinCount && plasticCount > 0) {
    // TIE - Use confidence as tiebreaker
    ESP32_SERIAL.println("├─ TIE detected - using confidence tiebreaker");
    if (maxPlasticConfidence > maxTinConfidence) {
      finalMaterial = "plastic";
      finalConfidence = maxPlasticConfidence;
      finalAction = "sort_plastic";
      ESP32_SERIAL.print("├─ Plastic wins by confidence (");
      ESP32_SERIAL.print(maxPlasticConfidence, 2);
      ESP32_SERIAL.print("% vs ");
      ESP32_SERIAL.print(maxTinConfidence, 2);
      ESP32_SERIAL.println("%)");
    } else {
      finalMaterial = "tin";
      finalConfidence = maxTinConfidence;
      finalAction = "sort_tin_can";
      ESP32_SERIAL.print("├─ Tin wins by confidence (");
      ESP32_SERIAL.print(maxTinConfidence, 2);
      ESP32_SERIAL.print("% vs ");
      ESP32_SERIAL.print(maxPlasticConfidence, 2);
      ESP32_SERIAL.println("%)");
    }
  }
  else {
    // No valid detections - REJECT
    finalMaterial = "rejected";
    finalConfidence = 0.0;
    finalAction = "reject";
    ESP32_SERIAL.println("├─ No valid material detected - REJECTED");
  }
  ESP32_SERIAL.println("└─ Decision complete");
  
  lastMaterialType = finalMaterial;
  lastConfidence = finalConfidence;
  lastAction = finalAction;
  
  ESP32_SERIAL.println("\n🎯 FINAL DECISION:");
  ESP32_SERIAL.print("├─ Material: ");
  ESP32_SERIAL.println(finalMaterial);
  ESP32_SERIAL.print("├─ Confidence: ");
  ESP32_SERIAL.print(finalConfidence, 2);
  ESP32_SERIAL.println("%");
  ESP32_SERIAL.print("├─ Action: ");
  ESP32_SERIAL.println(finalAction);
  ESP32_SERIAL.print("└─ Command: ");
  if (finalAction == "sort_plastic") {
    ESP32_SERIAL.println("OPEN_PLASTIC");
  } else if (finalAction == "sort_tin_can") {
    ESP32_SERIAL.println("OPEN_TIN");
  } else {
    ESP32_SERIAL.println("OPEN_REJECTED");
  }
  ESP32_SERIAL.println("════════════════════════════════════════\n");
  
  return true;
}

// ====================================
// ESP32 Communication
// ====================================

void waitForESP32Connection() {
  int attempts = 0;
  const int maxAttempts = 20;
  
  while (!esp32Connected && attempts < maxAttempts) {
    while (ESP32_SERIAL.available()) {
      ESP32_SERIAL.read();
    }
    
    ESP32_SERIAL.println("PING");
    
    unsigned long startTime = millis();
    String response = "";
    
    while (millis() - startTime < 500) {
      if (ESP32_SERIAL.available()) {
        char c = ESP32_SERIAL.read();
        if (c == '\n') {
          response.trim();
          if (response == "PONG") {
            esp32Connected = true;
            lastMaterialType = "ESP32 Connected";
            return;
          }
          response = "";
        } else {
          response += c;
        }
      }
    }
    
    attempts++;
    delay(500);
  }
  
  if (!esp32Connected) {
    lastMaterialType = "ESP32 Not Connected";
    lastAction = "waiting";
  }
}

void tryReconnectToESP32() {
  while (ESP32_SERIAL.available()) {
    ESP32_SERIAL.read();
  }
  
  ESP32_SERIAL.println("PING");
  
  unsigned long startTime = millis();
  String response = "";
  
  while (millis() - startTime < 1000) {
    if (ESP32_SERIAL.available()) {
      char c = ESP32_SERIAL.read();
      if (c == '\n') {
        response.trim();
        if (response == "PONG") {
          esp32Connected = true;
          lastMaterialType = "ESP32 Reconnected";
          return;
        }
        response = "";
      } else {
        response += c;
      }
    }
  }
  
  esp32Connected = false;
  lastMaterialType = "ESP32 Not Connected";
  lastAction = "retrying";
}

void checkESP32Commands() {
  if (ESP32_SERIAL.available()) {
    String command = ESP32_SERIAL.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      if (command == "PING") {
        ESP32_SERIAL.println("PONG");
        esp32Connected = true;
      }
      else if (command == "TRASH_DETECTED") {
        if (captureAndIdentify()) {
          sendResultToESP32();
        }
      }
      else if (command == "LID_OPENED") {
        lidOpenedTime = millis();
        waitingForAutoCapture = true;
      }
      else if (command == "USER_DETECTED") {
        lidOpenedTime = millis();
        waitingForAutoCapture = true;
      }
      else if (command == "CAPTURE_IMAGE") {
        if (captureAndIdentify()) {
          sendResultToESP32();
        }
      }
    }
  }
}

void sendResultToESP32() {
  if (lastAction == "sort_plastic") {
    ESP32_SERIAL.println("OPEN_PLASTIC");
  } 
  else if (lastAction == "sort_tin_can") {
    ESP32_SERIAL.println("OPEN_TIN");
  } 
  else {
    ESP32_SERIAL.println("OPEN_REJECTED");
  }
}
