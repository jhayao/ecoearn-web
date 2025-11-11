/*
 * ESP32-CAM Standalone Testing Firmware
 * Tests multi-capture voting system with backend API
 * 
 * Features:
 * - STANDALONE mode (no ESP32 main board needed)
 * - Captures MULTIPLE images (configurable, default 5)
 * - Sends each to backend identify/material API
 * - Uses VOTING SYSTEM for final decision
 * - If at least 1 recyclable detection → shows result
 * - Serial Monitor @ 115200 baud for testing
 * 
 * Voting Logic (1/5 rule):
 * - If ANY plastic detection (1+) → Result: PLASTIC
 * - Else if ANY tin detection (1+) → Result: TIN
 * - Else (all rejected/failed) → Result: REJECTED
 * 
 * Testing:
 * - Open Serial Monitor @ 115200 baud
 * - Type "test" or "capture" to start detection
 * - Watch the 5-image voting process
 * - See final decision
 * 
 * API Endpoint: POST http://YOUR_SERVER_IP:5001/identify/material
 * Request: Raw image/jpeg binary data
 * Response: {
 *   "success": true,
 *   "materialType": "plastic" | "tin" | "rejected",
 *   "confidence": 0.95,
 *   "action": "recycle" | "reject"
 * }
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>

// ====================================
// Configuration
// ====================================

// WiFi Credentials
const char* WIFI_SSID = "Xiaomi_53DE";
const char* WIFI_PASSWORD = "hayao1014";

// Backend API Configuration
const char* BACKEND_API_URL = "http://192.168.31.196:5001/identify/material";
const int HTTP_TIMEOUT = 10000; // 10 seconds

// Serial Configuration (for testing via Serial Monitor)
const int SERIAL_BAUD = 115200; // Standard baud for USB Serial Monitor

// Web Server Configuration
WebServer server(80); // Web server on port 80

// Command Protocol (for testing)
#define CMD_TEST "test"
#define CMD_CAPTURE "capture"
#define CMD_START "start"
#define CMD_HELP "help"

// Camera Pin Definitions (ESP32-CAM AI Thinker)
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

// Camera Settings
#define FRAME_SIZE FRAMESIZE_SVGA  // 800x600 for good quality
#define JPEG_QUALITY 10            // Lower = higher quality (10-63)

// Multi-Capture Settings (NEW)
#define NUM_CAPTURES 5             // Number of images to capture per detection
#define CAPTURE_DELAY_MS 300       // Delay between captures (milliseconds)
#define MIN_VOTES_FOR_RECYCLE 1    // Minimum votes needed to go to recyclable bin (1/5 rule)

// ====================================
// Global Variables
// ====================================

bool wifiConnected = false;
unsigned long lastTestPrompt = 0;
const unsigned long TEST_PROMPT_INTERVAL = 10000; // Show prompt every 10 seconds

// Image storage for web display
camera_fb_t* lastCapturedImage = NULL;
String lastResult = "No test run yet";
int lastPlasticVotes = 0;
int lastTinVotes = 0;
int lastRejectedVotes = 0;
int lastFailedVotes = 0;

// Flash LED pin (GPIO4 on most ESP32-CAM boards)
#define FLASH_LED_PIN 4

// ====================================
// Setup
// ====================================

void setup() {
  // Initialize Serial for USB Serial Monitor
  Serial.begin(SERIAL_BAUD);
  delay(2000); // Wait for Serial Monitor to connect
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════════════════════╗");
  Serial.println("║   ESP32-CAM Multi-Capture Testing Firmware            ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println();
  
  // Initialize flash LED
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);
  
  // Initialize camera
  Serial.println("📷 Initializing camera...");
  if (!initializeCamera()) {
    Serial.println("❌ Camera initialization FAILED!");
    Serial.println("   Check camera ribbon cable connection");
    while (1) {
      delay(1000); // Halt if camera fails
    }
  }
  Serial.println("✓ Camera initialized successfully");
  
  // Connect to WiFi
  Serial.println("\n📡 Connecting to WiFi...");
  connectToWiFi();
  
  if (wifiConnected) {
    Serial.println("✓ WiFi connected successfully");
    Serial.print("   IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("❌ WiFi connection failed!");
    Serial.println("   Check SSID and password");
  }
  
  // Setup web server
  if (wifiConnected) {
    Serial.println("\n🌐 Starting web server...");
    setupWebServer();
    Serial.println("✓ Web server started");
    Serial.print("   Open browser: http://");
    Serial.println(WiFi.localIP());
  }
  
  // Show configuration
  Serial.println("\n⚙️  Configuration:");
  Serial.print("   Backend API: ");
  Serial.println(BACKEND_API_URL);
  Serial.print("   Captures per test: ");
  Serial.println(NUM_CAPTURES);
  Serial.print("   Delay between captures: ");
  Serial.print(CAPTURE_DELAY_MS);
  Serial.println(" ms");
  Serial.print("   Min votes for recycle: ");
  Serial.println(MIN_VOTES_FOR_RECYCLE);
  
  // Ready for testing
  Serial.println("\n✅ System ready for testing!");
  showHelp();
}

// ====================================
// Main Loop
// ====================================

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Check for commands from Serial Monitor
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command.length() > 0) {
      handleCommand(command);
    }
  }
  
  // Periodic test prompt
  unsigned long currentTime = millis();
  if (currentTime - lastTestPrompt >= TEST_PROMPT_INTERVAL) {
    Serial.println("\n💡 Type 'test' or 'capture' to start detection");
    Serial.println("   Type 'help' for available commands");
    lastTestPrompt = currentTime;
  }
  
  delay(10);
}

// ====================================
// Camera Functions
// ====================================

bool initializeCamera() {
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
  config.frame_size = FRAME_SIZE;
  config.pixel_format = PIXFORMAT_JPEG;
  config.jpeg_quality = JPEG_QUALITY;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  esp_err_t err = esp_camera_init(&config);
  return (err == ESP_OK);
}

camera_fb_t* captureImage() {
  // Turn on flash LED for better lighting
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(100); // Let the light stabilize
  
  // Capture frame
  camera_fb_t *fb = esp_camera_fb_get();
  
  // Turn off flash
  digitalWrite(FLASH_LED_PIN, LOW);
  
  return fb;
}

// ====================================
// WiFi Functions
// ====================================

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.print("   Connecting");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  wifiConnected = (WiFi.status() == WL_CONNECTED);
}

// ====================================
// Backend API Functions
// ====================================

bool identifyMaterial(camera_fb_t *fb, String &materialType) {
  if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
    Serial.println("      ❌ WiFi not connected");
    return false;
  }
  
  if (!fb || fb->len == 0) {
    Serial.println("      ❌ Invalid image");
    return false;
  }
  
  // Send image to backend
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT);
  http.begin(BACKEND_API_URL);
  http.addHeader("Content-Type", "image/jpeg");
  
  Serial.print("      📤 Sending ");
  Serial.print(fb->len);
  Serial.println(" bytes to backend...");
  
  int httpCode = http.POST(fb->buf, fb->len);
  
  bool success = false;
  
  if (httpCode == 200) {
    String response = http.getString();
    
    // Parse JSON response
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc["success"] == true) {
      materialType = doc["materialType"].as<String>();
      float confidence = doc["confidence"] | 0.0;
      
      Serial.print("      ✓ Identified: ");
      Serial.print(materialType);
      Serial.print(" (confidence: ");
      Serial.print(confidence, 2);
      Serial.println(")");
      
      success = true;
    } else {
      Serial.println("      ❌ JSON parse error");
    }
  } else {
    Serial.print("      ❌ HTTP error: ");
    Serial.println(httpCode);
  }
  
  http.end();
  return success;
}

// ====================================
// Command Handling
// ====================================

void showHelp() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║           AVAILABLE COMMANDS                           ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println("  test     - Start multi-capture detection test");
  Serial.println("  capture  - Same as 'test'");
  Serial.println("  start    - Same as 'test'");
  Serial.println("  single   - Quick single capture test (1 image only)");
  Serial.println("  help     - Show this help message");
  Serial.println("════════════════════════════════════════════════════════\n");
}

void handleCommand(String command) {
  // Test/Capture commands
  if (command == CMD_TEST || command == CMD_CAPTURE || command == CMD_START) {
    Serial.println("\n🚀 Starting multi-capture detection test...\n");
    processTrashDetection();
    return;
  }
  
  // Single capture command
  if (command == "single" || command == "quick" || command == "1") {
    Serial.println("\n⚡ Starting single capture test (quick mode)...\n");
    processSingleCapture();
    return;
  }
  
  // Help command
  if (command == CMD_HELP || command == "?" || command == "commands") {
    showHelp();
    return;
  }
  
  // Unknown command
  Serial.print("❓ Unknown command: ");
  Serial.println(command);
  Serial.println("   Type 'help' for available commands");
}

void processTrashDetection() {
  Serial.println("╔════════════════════════════════════════════════════════╗");
  Serial.println("║         MULTI-CAPTURE DETECTION PROCESS                ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.print("   Capturing ");
  Serial.print(NUM_CAPTURES);
  Serial.println(" images for accuracy...\n");
  
  // Counters for voting
  int plasticCount = 0;
  int tinCount = 0;
  int rejectedCount = 0;
  int failedCount = 0;
  
  // Capture and identify multiple images
  for (int i = 0; i < NUM_CAPTURES; i++) {
    Serial.print("   [");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.print(NUM_CAPTURES);
    Serial.println("] Capturing image...");
    
    // Capture image
    camera_fb_t *fb = captureImage();
    
    if (!fb) {
      Serial.println("      ❌ Capture failed!");
      failedCount++;
      delay(CAPTURE_DELAY_MS);
      continue;
    }
    
    Serial.print("      ✓ Image captured: ");
    Serial.print(fb->len);
    Serial.println(" bytes");
    
    // Identify material
    String materialType = "";
    bool identified = identifyMaterial(fb, materialType);
    
    // Store last captured image for web display (keep the last one)
    if (lastCapturedImage != NULL) {
      esp_camera_fb_return(lastCapturedImage);
    }
    lastCapturedImage = fb; // Keep this image for web display
    
    if (!identified) {
      Serial.println("      ❌ Identification failed!");
      failedCount++;
      delay(CAPTURE_DELAY_MS);
      continue;
    }
    
    // Count the result
    materialType.toLowerCase();
    
    if (materialType == "plastic") {
      plasticCount++;
      Serial.println("      📊 Vote: PLASTIC");
    } 
    else if (materialType == "tin" || materialType == "metal" || materialType == "aluminum") {
      tinCount++;
      Serial.println("      📊 Vote: TIN");
    } 
    else {
      rejectedCount++;
      Serial.println("      📊 Vote: REJECTED");
    }
    
    Serial.println(); // Blank line between captures
    
    // Delay before next capture (except last one)
    if (i < NUM_CAPTURES - 1) {
      delay(CAPTURE_DELAY_MS);
    }
  }
  
  // Show voting results
  Serial.println("════════════════════════════════════════════════════════");
  Serial.println("╔════════════════════════════════════════════════════════╗");
  Serial.println("║              VOTING RESULTS                            ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  
  Serial.print("   Plastic:  ");
  Serial.print(plasticCount);
  Serial.print("/");
  Serial.print(NUM_CAPTURES);
  if (plasticCount > 0) {
    Serial.print(" ✓");
    for (int i = 0; i < plasticCount; i++) Serial.print("✓");
  }
  Serial.println();
  
  Serial.print("   Tin:      ");
  Serial.print(tinCount);
  Serial.print("/");
  Serial.print(NUM_CAPTURES);
  if (tinCount > 0) {
    Serial.print(" ✓");
    for (int i = 0; i < tinCount; i++) Serial.print("✓");
  }
  Serial.println();
  
  Serial.print("   Rejected: ");
  Serial.print(rejectedCount);
  Serial.print("/");
  Serial.print(NUM_CAPTURES);
  if (rejectedCount > 0) {
    Serial.print(" ✗");
    for (int i = 0; i < rejectedCount; i++) Serial.print("✗");
  }
  Serial.println();
  
  Serial.print("   Failed:   ");
  Serial.print(failedCount);
  Serial.print("/");
  Serial.println(NUM_CAPTURES);
  
  Serial.println("\n────────────────────────────────────────────────────────");
  
  // Decision logic: If at least MIN_VOTES_FOR_RECYCLE positive detections, go to that bin
  // Priority: Plastic > Tin > Rejected
  
  String finalDecision = "";
  
  if (plasticCount >= MIN_VOTES_FOR_RECYCLE) {
    // At least 1 plastic detection
    finalDecision = "PLASTIC";
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║                                                        ║");
    Serial.print("║   ✅ FINAL DECISION: PLASTIC (");
    Serial.print(plasticCount);
    Serial.print(" detection");
    if (plasticCount > 1) Serial.print("s");
    Serial.println(")        ║");
    Serial.println("║                                                        ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
  } 
  else if (tinCount >= MIN_VOTES_FOR_RECYCLE) {
    // At least 1 tin detection, no plastic
    finalDecision = "TIN";
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║                                                        ║");
    Serial.print("║   ✅ FINAL DECISION: TIN (");
    Serial.print(tinCount);
    Serial.print(" detection");
    if (tinCount > 1) Serial.print("s");
    Serial.println(")            ║");
    Serial.println("║                                                        ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
  } 
  else {
    // No recyclable detections, all rejected or failed
    finalDecision = "REJECTED";
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║                                                        ║");
    Serial.println("║   ❌ FINAL DECISION: REJECTED                          ║");
    Serial.println("║      (no recyclable detections)                        ║");
    Serial.println("║                                                        ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
  }
  
  Serial.println("════════════════════════════════════════════════════════\n");
  Serial.println("✅ Test complete! Type 'test' to run again.\n");
  
  // Store results for web display
  lastResult = finalDecision;
  lastPlasticVotes = plasticCount;
  lastTinVotes = tinCount;
  lastRejectedVotes = rejectedCount;
  lastFailedVotes = failedCount;
}

void processSingleCapture() {
  Serial.println("╔════════════════════════════════════════════════════════╗");
  Serial.println("║         SINGLE CAPTURE DETECTION (QUICK MODE)          ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println("   Capturing 1 image for quick test...\n");
  
  // Capture single image
  Serial.println("   [1/1] Capturing image...");
  camera_fb_t *fb = captureImage();
  
  if (!fb) {
    Serial.println("      ❌ Capture failed!");
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║   ❌ FINAL DECISION: FAILED                            ║");
    Serial.println("║      (capture error)                                   ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println("════════════════════════════════════════════════════════\n");
    Serial.println("✅ Single capture test complete! Type 'single' to try again.\n");
    
    lastResult = "FAILED";
    lastPlasticVotes = 0;
    lastTinVotes = 0;
    lastRejectedVotes = 1;
    lastFailedVotes = 1;
    return;
  }
  
  // Store image for web display
  lastCapturedImage = fb;
  
  // Send to backend API
  String materialType = "";
  bool success = identifyMaterial(fb, materialType);
  
  // Determine result
  String finalDecision = "REJECTED";
  
  if (success) {
    if (materialType == "plastic") {
      finalDecision = "PLASTIC";
      Serial.println("\n╔════════════════════════════════════════════════════════╗");
      Serial.println("║                                                        ║");
      Serial.println("║   ✅ FINAL DECISION: PLASTIC                           ║");
      Serial.println("║      (1 detection)                                     ║");
      Serial.println("║                                                        ║");
      Serial.println("╚════════════════════════════════════════════════════════╝");
    }
    else if (materialType == "tin") {
      finalDecision = "TIN";
      Serial.println("\n╔════════════════════════════════════════════════════════╗");
      Serial.println("║                                                        ║");
      Serial.println("║   ✅ FINAL DECISION: TIN                               ║");
      Serial.println("║      (1 detection)                                     ║");
      Serial.println("║                                                        ║");
      Serial.println("╚════════════════════════════════════════════════════════╝");
    }
    else {
      Serial.println("\n╔════════════════════════════════════════════════════════╗");
      Serial.println("║                                                        ║");
      Serial.println("║   ❌ FINAL DECISION: REJECTED                          ║");
      Serial.println("║      (not recyclable)                                  ║");
      Serial.println("║                                                        ║");
      Serial.println("╚════════════════════════════════════════════════════════╝");
    }
  } else {
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║                                                        ║");
    Serial.println("║   ❌ FINAL DECISION: REJECTED                          ║");
    Serial.println("║      (identification failed)                           ║");
    Serial.println("║                                                        ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
  }
  
  Serial.println("════════════════════════════════════════════════════════\n");
  Serial.println("✅ Single capture test complete! Type 'single' to try again.\n");
  
  // Store results for web display (show as 1/1 instead of x/5)
  lastResult = finalDecision;
  if (finalDecision == "PLASTIC") {
    lastPlasticVotes = 1;
    lastTinVotes = 0;
    lastRejectedVotes = 0;
    lastFailedVotes = 0;
  } else if (finalDecision == "TIN") {
    lastPlasticVotes = 0;
    lastTinVotes = 1;
    lastRejectedVotes = 0;
    lastFailedVotes = 0;
  } else {
    lastPlasticVotes = 0;
    lastTinVotes = 0;
    lastRejectedVotes = 1;
    lastFailedVotes = success ? 0 : 1;
  }
  
  // Return buffer after identification (not during multi-capture)
  esp_camera_fb_return(fb);
}

// ====================================
// Web Server Functions
// ====================================

void setupWebServer() {
  // Root page - show interface and last image
  server.on("/", HTTP_GET, handleRoot);
  
  // Image endpoint - serves the last captured image
  server.on("/image", HTTP_GET, handleImage);
  
  // Trigger test from web
  server.on("/test", HTTP_GET, handleWebTest);
  
  // Trigger single capture test from web
  server.on("/single", HTTP_GET, handleWebSingleTest);
  
  // Start server
  server.begin();
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32-CAM Multi-Capture Test</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; max-width: 1200px; margin: 0 auto; padding: 20px; background: #f5f5f5; }";
  html += "h1 { color: #333; text-align: center; }";
  html += ".container { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += ".image-section { text-align: center; margin: 20px 0; }";
  html += "img { max-width: 100%; height: auto; border: 2px solid #ddd; border-radius: 5px; }";
  html += ".results { margin: 20px 0; }";
  html += ".vote-bar { background: #f0f0f0; padding: 10px; margin: 5px 0; border-radius: 5px; }";
  html += ".vote-bar.plastic { background: #e3f2fd; }";
  html += ".vote-bar.tin { background: #fff3e0; }";
  html += ".vote-bar.rejected { background: #ffebee; }";
  html += ".decision { padding: 20px; margin: 20px 0; border-radius: 10px; text-align: center; font-size: 24px; font-weight: bold; }";
  html += ".decision.plastic { background: #4caf50; color: white; }";
  html += ".decision.tin { background: #ff9800; color: white; }";
  html += ".decision.rejected { background: #f44336; color: white; }";
  html += "button { background: #2196F3; color: white; border: none; padding: 15px 30px; font-size: 18px; border-radius: 5px; cursor: pointer; margin: 10px; }";
  html += "button:hover { background: #1976D2; }";
  html += ".info { background: #e8f5e9; padding: 15px; border-radius: 5px; margin: 10px 0; }";
  html += "</style>";
  html += "<script>";
  html += "function startTest() {";
  html += "  document.getElementById('status').innerHTML = '🔄 Running test...';";
  html += "  fetch('/test').then(response => response.text()).then(data => {";
  html += "    setTimeout(() => { location.reload(); }, 15000);"; // Reload after 15 seconds
  html += "  });";
  html += "}";
  html += "function startSingle() {";
  html += "  document.getElementById('status').innerHTML = '⚡ Running single capture...';";
  html += "  fetch('/single').then(response => response.text()).then(data => {";
  html += "    setTimeout(() => { location.reload(); }, 5000);"; // Reload after 5 seconds
  html += "  });";
  html += "}";
  html += "function refreshImage() { document.getElementById('lastImage').src = '/image?' + new Date().getTime(); }";
  html += "setInterval(refreshImage, 2000);"; // Auto-refresh image every 2 seconds
  html += "</script>";
  html += "</head><body>";
  
  html += "<div class='container'>";
  html += "<h1>🎥 ESP32-CAM Multi-Capture Testing</h1>";
  
  // Control buttons
  html += "<div style='text-align: center;'>";
  html += "<button onclick='startTest()'>🚀 Start Test (5 images)</button>";
  html += "<button onclick='startSingle()'>⚡ Quick Test (1 image)</button>";
  html += "<button onclick='location.reload()'>🔄 Refresh Results</button>";
  html += "</div>";
  
  html += "<div id='status' class='info'>";
  html += "Ready to test. Click 'Start Test' or type 'test' in Serial Monitor.";
  html += "</div>";
  
  // Last captured image
  html += "<div class='image-section'>";
  html += "<h2>📸 Last Captured Image</h2>";
  if (lastCapturedImage != NULL) {
    html += "<img id='lastImage' src='/image' alt='Last captured image' />";
    html += "<p><small>Auto-refreshes every 2 seconds during test</small></p>";
  } else {
    html += "<p>No image captured yet. Run a test first.</p>";
  }
  html += "</div>";
  
  // Voting results
  if (lastResult != "No test run yet") {
    html += "<div class='results'>";
    html += "<h2>📊 Last Test Results</h2>";
    
    html += "<div class='vote-bar plastic'>";
    html += "🌱 Plastic: " + String(lastPlasticVotes) + "/" + String(NUM_CAPTURES);
    String plasticBar = "";
    for (int i = 0; i < lastPlasticVotes; i++) plasticBar += "✓";
    html += " " + plasticBar;
    html += "</div>";
    
    html += "<div class='vote-bar tin'>";
    html += "🥫 Tin: " + String(lastTinVotes) + "/" + String(NUM_CAPTURES);
    String tinBar = "";
    for (int i = 0; i < lastTinVotes; i++) tinBar += "✓";
    html += " " + tinBar;
    html += "</div>";
    
    html += "<div class='vote-bar rejected'>";
    html += "❌ Rejected: " + String(lastRejectedVotes) + "/" + String(NUM_CAPTURES);
    String rejectedBar = "";
    for (int i = 0; i < lastRejectedVotes; i++) rejectedBar += "✗";
    html += " " + rejectedBar;
    html += "</div>";
    
    if (lastFailedVotes > 0) {
      html += "<div class='vote-bar'>";
      html += "⚠️ Failed: " + String(lastFailedVotes) + "/" + String(NUM_CAPTURES);
      html += "</div>";
    }
    
    // Final decision
    String decisionClass = lastResult;
    decisionClass.toLowerCase();
    html += "<div class='decision " + decisionClass + "'>";
    if (lastResult == "PLASTIC") {
      html += "✅ FINAL DECISION: PLASTIC (" + String(lastPlasticVotes) + " detection";
      if (lastPlasticVotes > 1) html += "s";
      html += ")";
    } else if (lastResult == "TIN") {
      html += "✅ FINAL DECISION: TIN (" + String(lastTinVotes) + " detection";
      if (lastTinVotes > 1) html += "s";
      html += ")";
    } else {
      html += "❌ FINAL DECISION: REJECTED (no recyclable detections)";
    }
    html += "</div>";
    html += "</div>";
  }
  
  // System info
  html += "<div class='info'>";
  html += "<h3>⚙️ Configuration</h3>";
  html += "<p><strong>Backend API:</strong> " + String(BACKEND_API_URL) + "</p>";
  html += "<p><strong>Captures per test:</strong> " + String(NUM_CAPTURES) + "</p>";
  html += "<p><strong>Delay between captures:</strong> " + String(CAPTURE_DELAY_MS) + " ms</p>";
  html += "<p><strong>Min votes for recycle:</strong> " + String(MIN_VOTES_FOR_RECYCLE) + " (1/" + String(NUM_CAPTURES) + " rule)</p>";
  html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
  html += "</div>";
  
  html += "</div>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleImage() {
  if (lastCapturedImage == NULL) {
    server.send(404, "text/plain", "No image available");
    return;
  }
  
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char *)lastCapturedImage->buf, lastCapturedImage->len);
}

void handleWebTest() {
  server.send(200, "text/plain", "Test started! Check Serial Monitor for progress.");
  
  // Start test in background (non-blocking would be better, but this is simple)
  Serial.println("\n🌐 Test triggered from web interface\n");
  processTrashDetection();
}

void handleWebSingleTest() {
  server.send(200, "text/plain", "Single capture started! Check Serial Monitor for progress.");
  
  // Start single capture test
  Serial.println("\n🌐 Single capture triggered from web interface\n");
  processSingleCapture();
}

// ====================================
// End of ESP32-CAM Testing Firmware
// ====================================
