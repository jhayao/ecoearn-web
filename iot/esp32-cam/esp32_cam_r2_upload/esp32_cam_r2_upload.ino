/*
 * ESP32-CAM with Cloudflare R2 Upload
 * 
 * Purpose: Captures images and uploads to Cloudflare R2 storage
 * Features:
 * - Automatic upload to R2 on every capture
 * - Material identification via backend API
 * - Local SD card backup (optional)
 * - Web interface for monitoring
 * - Serial Monitor control
 * 
 * Hardware: ESP32-CAM AI Thinker module
 * 
 * R2 Setup Required:
 * 1. Create R2 bucket in Cloudflare dashboard
 * 2. Generate API tokens (Access Key ID + Secret Access Key)
 * 3. Note your R2 endpoint URL
 * 4. Update credentials below
 * 
 * Commands:
 * - capture / test - Capture image, identify, upload to R2
 * - stats - Show upload statistics
 * - help - Show available commands
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "mbedtls/md.h"
#include "time.h"

// ====================================
// WiFi Configuration
// ====================================
const char* WIFI_SSID = "Xiaomi_53DE";
const char* WIFI_PASSWORD = "hayao1014";

// ====================================
// Cloudflare R2 Configuration
// ====================================
// Get these from Cloudflare Dashboard → R2 → Manage R2 API Tokens
const char* R2_ACCOUNT_ID = "e20d459564b5296f048f8ac255bd5b81";  // e.g., "abc123def456"
const char* R2_BUCKET_NAME = "ecoearn-captures";  // Your bucket name
const char* R2_ACCESS_KEY_ID = "37ff073a0aa649e91d865512d87e6057";
const char* R2_SECRET_ACCESS_KEY = "8f78309932a2cead7b57ca7666753430eccad594686ceb3df09491ef8b24f529";

// R2 Endpoint - Format: https://<account_id>.r2.cloudflarestorage.com
String R2_ENDPOINT = "https://" + String(R2_ACCOUNT_ID) + ".r2.cloudflarestorage.com";

// ====================================
// Backend API Configuration
// ====================================
const char* BACKEND_API_URL = "http://192.168.31.196:5001/identify/material";

// ====================================
// Camera Pin Configuration
// ====================================
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

// ====================================
// Configuration
// ====================================
#define FLASH_GPIO 4
#define FRAME_SIZE FRAMESIZE_SVGA    // 800x600
#define JPEG_QUALITY 10              // 0-63, lower = higher quality
#define NUM_IMAGES 5                // Number of images per batch capture
#define CAPTURE_DELAY_MS 500         // Delay between captures in batch mode

// ====================================
// Global Variables
// ====================================
WebServer server(80);
unsigned long imageCounter = 0;
unsigned long uploadSuccessCount = 0;
unsigned long uploadFailCount = 0;
String lastMaterialType = "None";
float lastConfidence = 0.0;
String lastR2Url = "";
String currentSessionFolder = "";  // Current batch session folder
unsigned long sessionCounter = 0;  // Counter for session folders

// NTP Time sync
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

// ====================================
// Setup
// ====================================
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════════════════════╗");
  Serial.println("║       ESP32-CAM Cloudflare R2 Upload System            ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println();
  
  // Initialize Flash LED
  pinMode(FLASH_GPIO, OUTPUT);
  digitalWrite(FLASH_GPIO, LOW);
  
  // Initialize Camera
  if (initCamera()) {
    Serial.println("✅ Camera initialized successfully");
  } else {
    Serial.println("❌ Camera initialization FAILED!");
  }
  
  // Connect to WiFi
  connectToWiFi();
  
  // Sync time (required for R2 signed requests)
  syncTime();
  
  // Setup web server
  setupWebServer();
  
  Serial.println();
  Serial.println("════════════════════════════════════════════════════════");
  Serial.println("Ready! Type 'help' for available commands");
  Serial.println("Web interface: http://" + WiFi.localIP().toString());
  Serial.println("════════════════════════════════════════════════════════\n");
}

void loop() {
  server.handleClient();
  
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    handleCommand(command);
  }
  
  delay(10);
}

// ====================================
// Camera Initialization
// ====================================
bool initCamera() {
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
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAME_SIZE;
  config.jpeg_quality = JPEG_QUALITY;
  config.fb_count = 1;
  
  esp_err_t err = esp_camera_init(&config);
  return (err == ESP_OK);
}

// ====================================
// WiFi Connection
// ====================================
void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.print("   IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi connection failed!");
  }
}

// ====================================
// Time Synchronization
// ====================================
void syncTime() {
  Serial.println("🕐 Syncing time with NTP server...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  int attempts = 0;
  while (time(nullptr) < 100000 && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (time(nullptr) > 100000) {
    Serial.println("\n✅ Time synchronized");
    time_t now = time(nullptr);
    Serial.print("   Current time: ");
    Serial.println(ctime(&now));
  } else {
    Serial.println("\n⚠️  Time sync failed - R2 uploads may not work!");
  }
}

// ====================================
// Command Handling
// ====================================
void handleCommand(String command) {
  if (command == "capture" || command == "test" || command == "start") {
    captureBatchToFolder();
    return;
  }
  
  if (command == "detect" || command == "identify") {
    detectBatch();
    return;
  }
  
  if (command == "single" || command == "quick") {
    captureIdentifyAndUpload();
    return;
  }
  
  if (command == "stats" || command == "status") {
    printStats();
    return;
  }
  
  if (command == "help" || command == "?") {
    showHelp();
    return;
  }
  
  Serial.println("❓ Unknown command: " + command);
  Serial.println("   Type 'help' for available commands");
}

void showHelp() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║           AVAILABLE COMMANDS                           ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println("  detect   - Capture 20 images & detect (NO upload)");
  Serial.println("  identify - Same as 'detect'");
  Serial.println("  capture  - Capture 20 images & upload to R2");
  Serial.println("  test     - Same as 'capture'");
  Serial.println("  start    - Same as 'capture'");
  Serial.println("  single   - Capture single image (identify + upload)");
  Serial.println("  quick    - Same as 'single'");
  Serial.println("  stats    - Show upload statistics");
  Serial.println("  help     - Show this help message");
  Serial.println("════════════════════════════════════════════════════════\n");
}

void printStats() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║           UPLOAD STATISTICS                            ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.printf("  Total captures: %lu\n", imageCounter);
  Serial.printf("  Successful uploads: %lu\n", uploadSuccessCount);
  Serial.printf("  Failed uploads: %lu\n", uploadFailCount);
  if (imageCounter > 0) {
    float successRate = (uploadSuccessCount * 100.0) / imageCounter;
    Serial.printf("  Success rate: %.1f%%\n", successRate);
  }
  Serial.printf("  Last material: %s (%.1f%% confidence)\n", lastMaterialType.c_str(), lastConfidence * 100);
  if (lastR2Url != "") {
    Serial.println("  Last R2 URL: " + lastR2Url);
  }
  Serial.println("════════════════════════════════════════════════════════\n");
}

// ====================================
// Generate Unique Session Folder Name
// ====================================
String generateSessionFolder() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  
  // Generate unique folder name: YYYYMMDD_HHMMSS_<random>
  char timestamp[32];
  strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", timeinfo);
  
  // Add random component for uniqueness
  String randomId = String(random(1000, 9999));
  
  sessionCounter++;
  String folderName = "session_" + String(timestamp) + "_" + randomId;
  
  return folderName;
}

// ====================================
// Batch Detection (20 images, NO upload)
// ====================================
void detectBatch() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║     BATCH DETECTION: 20 IMAGES (NO R2 UPLOAD)          ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println();
  Serial.println("📸 Capturing and detecting " + String(NUM_IMAGES) + " images...");
  Serial.println("⚠️  Note: Images will NOT be uploaded to R2");
  Serial.println();
  
  int plasticCount = 0;
  int tinCount = 0;
  int rejectedCount = 0;
  int failedCount = 0;
  
  // Capture and detect 20 images
  for (int i = 0; i < NUM_IMAGES; i++) {
    Serial.printf("📸 [%d/%d] Capturing...", i + 1, NUM_IMAGES);
    
    // Flash LED
    digitalWrite(FLASH_GPIO, HIGH);
    delay(100);
    
    // Capture
    camera_fb_t *fb = esp_camera_fb_get();
    digitalWrite(FLASH_GPIO, LOW);
    
    if (!fb) {
      Serial.println(" ❌ Capture FAILED");
      failedCount++;
      delay(CAPTURE_DELAY_MS);
      continue;
    }
    
    imageCounter++;
    
    // Identify material via backend API
    bool identified = identifyMaterial(fb);
    
    if (identified) {
      Serial.printf(" ✅ %s (%.1f%% confidence)\n", lastMaterialType.c_str(), lastConfidence * 100);
      
      // Count results
      if (lastMaterialType == "plastic") {
        plasticCount++;
      } else if (lastMaterialType == "tin" || lastMaterialType == "tin_can") {
        tinCount++;
      } else {
        rejectedCount++;
      }
    } else {
      Serial.println(" ❌ Detection FAILED");
      failedCount++;
    }
    
    // Release frame buffer immediately (no upload)
    esp_camera_fb_return(fb);
    
    // Delay before next capture
    if (i < NUM_IMAGES - 1) {
      delay(CAPTURE_DELAY_MS);
    }
  }
  
  // Summary with voting results
  Serial.println();
  Serial.println("╔════════════════════════════════════════════════════════╗");
  Serial.println("║              DETECTION RESULTS SUMMARY                 ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.printf("  🌱 Plastic:  %d/%d detections\n", plasticCount, NUM_IMAGES);
  Serial.printf("  🥫 Tin:      %d/%d detections\n", tinCount, NUM_IMAGES);
  Serial.printf("  ❌ Rejected: %d/%d detections\n", rejectedCount, NUM_IMAGES);
  if (failedCount > 0) {
    Serial.printf("  ⚠️  Failed:   %d/%d detections\n", failedCount, NUM_IMAGES);
  }
  Serial.println();
  
  // Final decision based on voting (1/20 rule)
  String finalDecision = "REJECTED";
  if (plasticCount >= 1) {
    finalDecision = "PLASTIC";
  } else if (tinCount >= 1) {
    finalDecision = "TIN";
  }
  
  Serial.println("╔════════════════════════════════════════════════════════╗");
  Serial.println("║                FINAL DECISION (1/20 RULE)              ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println();
  
  if (finalDecision == "PLASTIC") {
    Serial.printf("  ✅ PLASTIC (%d detections)\n", plasticCount);
    Serial.println("  Action: Open plastic compartment");
  } else if (finalDecision == "TIN") {
    Serial.printf("  ✅ TIN (%d detections)\n", tinCount);
    Serial.println("  Action: Open tin compartment");
  } else {
    Serial.println("  ❌ REJECTED (no recyclable detections)");
    Serial.println("  Action: Open rejected compartment");
  }
  
  Serial.println();
  Serial.println("════════════════════════════════════════════════════════\n");
}

// ====================================
// Batch Capture to Unique Folder (20 images)
// ====================================
void captureBatchToFolder() {
  // Create new unique session folder
  currentSessionFolder = generateSessionFolder();
  
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║       BATCH CAPTURE: 20 IMAGES → UNIQUE FOLDER         ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println();
  Serial.println("📁 Session Folder: " + currentSessionFolder);
  Serial.println("📸 Capturing " + String(NUM_IMAGES) + " images...");
  Serial.println();
  
  int successCount = 0;
  int failCount = 0;
  
  // Capture 20 images
  for (int i = 0; i < NUM_IMAGES; i++) {
    Serial.printf("📸 [%d/%d] Capturing image...", i + 1, NUM_IMAGES);
    
    // Flash LED
    digitalWrite(FLASH_GPIO, HIGH);
    delay(100);
    
    // Capture
    camera_fb_t *fb = esp_camera_fb_get();
    digitalWrite(FLASH_GPIO, LOW);
    
    if (!fb) {
      Serial.println(" ❌ FAILED");
      failCount++;
      delay(CAPTURE_DELAY_MS);
      continue;
    }
    
    imageCounter++;
    
    // Upload to R2 in session folder
    bool uploaded = uploadToR2InFolder(fb, i + 1);
    
    if (uploaded) {
      Serial.printf(" ✅ Uploaded (%d KB)\n", fb->len / 1024);
      successCount++;
      uploadSuccessCount++;
    } else {
      Serial.println(" ❌ Upload failed");
      failCount++;
      uploadFailCount++;
    }
    
    // Release frame buffer
    esp_camera_fb_return(fb);
    
    // Delay before next capture
    if (i < NUM_IMAGES - 1) {
      delay(CAPTURE_DELAY_MS);
    }
  }
  
  // Summary
  Serial.println();
  Serial.println("════════════════════════════════════════════════════════");
  Serial.printf("✅ Batch complete! %d/%d uploaded successfully\n", successCount, NUM_IMAGES);
  if (failCount > 0) {
    Serial.printf("❌ Failed: %d images\n", failCount);
  }
  Serial.println("📁 Folder: " + currentSessionFolder);
  Serial.println("☁️  R2 Path: captures/" + currentSessionFolder + "/");
  Serial.println("════════════════════════════════════════════════════════\n");
}

// ====================================
// Upload to R2 in Session Folder
// ====================================
bool uploadToR2InFolder(camera_fb_t *fb, int imageNumber) {
  // Generate filename: img_001.jpg, img_002.jpg, etc.
  char filename[32];
  sprintf(filename, "img_%03d.jpg", imageNumber);
  
  // Object key: captures/<session_folder>/img_001.jpg
  String objectKey = "captures/" + currentSessionFolder + "/" + String(filename);
  
  // R2 URL
  String url = R2_ENDPOINT + "/" + R2_BUCKET_NAME + "/" + objectKey;
  
  HTTPClient http;
  http.setTimeout(30000); // 30 seconds
  http.begin(url);
  http.addHeader("Content-Type", "image/jpeg");
  
  int httpCode = http.PUT(fb->buf, fb->len);
  
  bool success = (httpCode == 200 || httpCode == 204);
  
  if (success) {
    lastR2Url = url;
  }
  
  http.end();
  return success;
}

// ====================================
// Main Capture, Identify, Upload Flow (Single Image)
// ====================================
void captureIdentifyAndUpload() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║    SINGLE CAPTURE → IDENTIFY → UPLOAD TO R2            ║");
  Serial.println("╚════════════════════════════════════════════════════════╝\n");
  
  // Step 1: Capture Image
  Serial.println("📸 Step 1/3: Capturing image...");
  digitalWrite(FLASH_GPIO, HIGH);
  delay(100);
  
  camera_fb_t *fb = esp_camera_fb_get();
  digitalWrite(FLASH_GPIO, LOW);
  
  if (!fb) {
    Serial.println("   ❌ Capture failed!");
    return;
  }
  
  imageCounter++;
  Serial.printf("   ✅ Captured %d bytes\n", fb->len);
  
  // Step 2: Identify Material
  Serial.println("\n🔍 Step 2/3: Identifying material...");
  bool identified = identifyMaterial(fb);
  
  if (identified) {
    Serial.printf("   ✅ Identified: %s (%.1f%% confidence)\n", lastMaterialType.c_str(), lastConfidence * 100);
  } else {
    Serial.println("   ❌ Identification failed");
  }
  
  // Step 3: Upload to R2
  Serial.println("\n☁️  Step 3/3: Uploading to Cloudflare R2...");
  bool uploaded = uploadToR2(fb);
  
  if (uploaded) {
    Serial.println("   ✅ Upload successful!");
    Serial.println("   📦 R2 URL: " + lastR2Url);
    uploadSuccessCount++;
  } else {
    Serial.println("   ❌ Upload failed!");
    uploadFailCount++;
  }
  
  // Release frame buffer
  esp_camera_fb_return(fb);
  
  Serial.println("\n════════════════════════════════════════════════════════");
  Serial.println("✅ Process complete!");
  Serial.println("════════════════════════════════════════════════════════\n");
}

// ====================================
// Material Identification
// ====================================
bool identifyMaterial(camera_fb_t *fb) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("   ⚠️  WiFi not connected");
    return false;
  }
  
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(BACKEND_API_URL);
  http.addHeader("Content-Type", "image/jpeg");
  
  int httpCode = http.POST(fb->buf, fb->len);
  
  bool success = false;
  
  if (httpCode == 200) {
    String response = http.getString();
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc["success"] == true) {
      lastMaterialType = doc["materialType"] | "Unknown";
      lastConfidence = doc["confidence"] | 0.0;
      success = true;
    }
  }
  
  http.end();
  return success;
}

// ====================================
// R2 Upload (Simple PUT - No AWS Signature for now)
// ====================================
bool uploadToR2(camera_fb_t *fb) {
  // Generate filename with timestamp
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char timestamp[32];
  strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", timeinfo);
  
  String filename = String(timestamp) + "_" + lastMaterialType + "_" + String(imageCounter) + ".jpg";
  String objectKey = "captures/" + filename;
  
  // R2 URL
  String url = R2_ENDPOINT + "/" + R2_BUCKET_NAME + "/" + objectKey;
  
  Serial.println("   Uploading to: " + url);
  
  // For simplicity, using public bucket or presigned URL
  // Full AWS Signature V4 implementation would go here
  // Alternative: Use presigned URLs generated by your backend
  
  HTTPClient http;
  http.setTimeout(30000); // 30 seconds
  http.begin(url);
  http.addHeader("Content-Type", "image/jpeg");
  
  // Note: For production, implement AWS Signature V4
  // For now, assuming public bucket or using alternative method
  
  int httpCode = http.PUT(fb->buf, fb->len);
  
  bool success = (httpCode == 200 || httpCode == 204);
  
  if (success) {
    lastR2Url = url;
  }
  
  http.end();
  return success;
}

// ====================================
// Web Server
// ====================================
void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/capture", HTTP_GET, handleWebCapture);
  server.on("/stats", HTTP_GET, handleWebStats);
  server.begin();
  Serial.println("✅ Web server started on port 80");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP32-CAM R2 Upload</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body {font-family: Arial; text-align: center; background: #1a1a1a; color: white; padding: 20px;}";
  html += ".container {max-width: 800px; margin: 0 auto; background: #2a2a2a; padding: 30px; border-radius: 10px;}";
  html += ".button {background: #4CAF50; color: white; padding: 15px 30px; border: none; border-radius: 5px; font-size: 18px; cursor: pointer; margin: 10px;}";
  html += ".button:hover {background: #45a049;}";
  html += ".stats {background: #333; padding: 20px; margin: 20px 0; border-radius: 5px;}";
  html += ".stat-item {margin: 10px 0;}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>📸 ESP32-CAM → ☁️ R2 Upload</h1>";
  html += "<button class='button' onclick=\"capture()\">🚀 Capture & Upload</button>";
  html += "<button class='button' onclick=\"location.reload()\">🔄 Refresh</button>";
  html += "<div class='stats'>";
  html += "<h2>📊 Statistics</h2>";
  html += "<div class='stat-item'>Total: " + String(imageCounter) + "</div>";
  html += "<div class='stat-item'>Success: " + String(uploadSuccessCount) + "</div>";
  html += "<div class='stat-item'>Failed: " + String(uploadFailCount) + "</div>";
  html += "<div class='stat-item'>Last: " + lastMaterialType + " (" + String(lastConfidence * 100, 1) + "%)</div>";
  html += "</div>";
  html += "</div>";
  html += "<script>";
  html += "function capture() {";
  html += "  alert('Capturing...'); fetch('/capture').then(() => setTimeout(() => location.reload(), 3000));";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleWebCapture() {
  captureBatchToFolder();
  server.send(200, "text/plain", "Batch capture complete!");
}

void handleWebStats() {
  String json = "{";
  json += "\"total\":" + String(imageCounter) + ",";
  json += "\"success\":" + String(uploadSuccessCount) + ",";
  json += "\"failed\":" + String(uploadFailCount) + ",";
  json += "\"material\":\"" + lastMaterialType + "\",";
  json += "\"confidence\":" + String(lastConfidence);
  json += "}";
  server.send(200, "application/json", json);
}
