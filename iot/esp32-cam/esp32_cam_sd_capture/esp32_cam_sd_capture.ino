/*
 * ESP32-CAM SD Card Image Capture
 * 
 * Purpose: Captures 20 images and saves them to SD card
 * Hardware: ESP32-CAM AI Thinker module with SD card
 * 
 * Features:
 * - Captures 20 sequential images
 * - Saves as JPEG to SD card
 * - Automatic file naming (img_001.jpg, img_002.jpg, etc.)
 * - Serial Monitor control and feedback
 * - SD card detection and validation
 * - Configurable image quality and size
 * 
 * Commands:
 * - capture / start / go - Start capturing 20 images
 * - list - List all files on SD card
 * - delete - Delete all images from SD card
 * - format - Format SD card
 * - help - Show available commands
 * 
 * SD Card Pin Connections (ESP32-CAM):
 * - SD_MMC_CMD  = GPIO 15
 * - SD_MMC_CLK  = GPIO 14
 * - SD_MMC_DATA0 = GPIO 2
 * - SD_MMC_DATA1 = GPIO 4  (not used in 1-bit mode)
 * - SD_MMC_DATA2 = GPIO 12 (not used in 1-bit mode)
 * - SD_MMC_DATA3 = GPIO 13 (not used in 1-bit mode)
 * 
 * Note: Insert SD card BEFORE powering on ESP32-CAM
 */

#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ====================================
// Configuration
// ====================================

// Number of images to capture
#define NUM_IMAGES 20

// Delay between captures (milliseconds)
#define CAPTURE_DELAY_MS 500

// Image settings
#define FRAME_SIZE FRAMESIZE_SVGA    // 800x600
#define JPEG_QUALITY 10              // 0-63, lower = higher quality

// SD Card mode: true = 1-bit mode (more stable), false = 4-bit mode (faster)
#define SD_1BIT_MODE true

// Flash LED
#define FLASH_GPIO 4

// ====================================
// Camera Pin Definition (AI Thinker)
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
// Global Variables
// ====================================

bool cameraInitialized = false;
bool sdCardInitialized = false;
int imageCounter = 0;

// ====================================
// Setup Functions
// ====================================

void setup() {
  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  // Initialize Serial
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════════════════════╗");
  Serial.println("║       ESP32-CAM SD Card Image Capture System           ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println();
  
  // Initialize Flash LED
  pinMode(FLASH_GPIO, OUTPUT);
  digitalWrite(FLASH_GPIO, LOW);
  
  // Initialize Camera
  if (initCamera()) {
    Serial.println("✅ Camera initialized successfully");
    cameraInitialized = true;
  } else {
    Serial.println("❌ Camera initialization FAILED!");
    cameraInitialized = false;
  }
  
  // Initialize SD Card
  if (initSDCard()) {
    Serial.println("✅ SD Card initialized successfully");
    sdCardInitialized = true;
    printSDCardInfo();
    findNextImageNumber();
  } else {
    Serial.println("❌ SD Card initialization FAILED!");
    Serial.println("   Please insert SD card and restart");
    sdCardInitialized = false;
  }
  
  Serial.println();
  Serial.println("════════════════════════════════════════════════════════");
  Serial.println("Ready! Type 'help' for available commands");
  Serial.println("════════════════════════════════════════════════════════\n");
}

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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // High quality settings
  config.frame_size = FRAME_SIZE;
  config.jpeg_quality = JPEG_QUALITY;
  config.fb_count = 1;
  
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }
  
  // Additional sensor settings
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);        // 0 to 4
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 300);    // 0 to 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
  
  return true;
}

bool initSDCard() {
  Serial.println("\n🔄 Initializing SD Card...");
  
  // Small delay to let SD card stabilize
  delay(500);
  
  // Try to mount SD card
  #if SD_1BIT_MODE
    Serial.println("   Attempting to mount SD card (1-bit mode - stable)...");
    if (!SD_MMC.begin("/sdcard", true)) {
      Serial.println("\n❌ SD Card Mount Failed in 1-bit mode!");
      Serial.println("   Trying 4-bit mode...");
      
      // Try 4-bit mode as fallback
      if (!SD_MMC.begin("/sdcard", false)) {
        Serial.println("❌ SD Card Mount Failed in 4-bit mode too!");
        printSDCardTroubleshooting();
        return false;
      }
      Serial.println("   ✓ SD card mounted in 4-bit mode");
    } else {
      Serial.println("   ✓ SD card mounted in 1-bit mode");
    }
  #else
    Serial.println("   Attempting to mount SD card (4-bit mode - fast)...");
    if (!SD_MMC.begin("/sdcard", false)) {
      Serial.println("\n❌ SD Card Mount Failed in 4-bit mode!");
      Serial.println("   Trying 1-bit mode...");
      
      // Try 1-bit mode as fallback
      if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("❌ SD Card Mount Failed in 1-bit mode too!");
        printSDCardTroubleshooting();
        return false;
      }
      Serial.println("   ✓ SD card mounted in 1-bit mode");
    } else {
      Serial.println("   ✓ SD card mounted in 4-bit mode");
    }
  #endif
  
  // Check card type
  uint8_t cardType = SD_MMC.cardType();
  
  if (cardType == CARD_NONE) {
    Serial.println("❌ No SD Card detected!");
    Serial.println("   Please insert SD card and restart");
    return false;
  }
  
  Serial.println("   ✓ SD card detected");
  
  return true;
}

void printSDCardTroubleshooting() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║           SD CARD TROUBLESHOOTING                      ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println("Common solutions:");
  Serial.println();
  Serial.println("1️⃣  POWER CYCLE");
  Serial.println("   → Completely power off ESP32-CAM");
  Serial.println("   → Remove SD card");
  Serial.println("   → Wait 5 seconds");
  Serial.println("   → Insert SD card firmly");
  Serial.println("   → Power on ESP32-CAM");
  Serial.println();
  Serial.println("2️⃣  CHECK SD CARD FORMAT");
  Serial.println("   → Must be formatted as FAT32");
  Serial.println("   → Use 4GB to 32GB card (SDHC)");
  Serial.println("   → Avoid cards larger than 32GB");
  Serial.println();
  Serial.println("3️⃣  CLEAN CONTACTS");
  Serial.println("   → Remove SD card");
  Serial.println("   → Gently clean gold contacts with eraser");
  Serial.println("   → Blow away any dust");
  Serial.println("   → Re-insert firmly");
  Serial.println();
  Serial.println("4️⃣  TRY DIFFERENT SD CARD");
  Serial.println("   → Some cards are incompatible");
  Serial.println("   → Use SanDisk, Samsung, or Kingston");
  Serial.println("   → Class 10 or UHS-I recommended");
  Serial.println();
  Serial.println("5️⃣  CHECK POWER SUPPLY");
  Serial.println("   → ESP32-CAM needs stable 5V");
  Serial.println("   → At least 500mA current");
  Serial.println("   → USB power may be insufficient");
  Serial.println();
  Serial.println("6️⃣  VERIFY SD CARD SLOT");
  Serial.println("   → SD card should click into place");
  Serial.println("   → Push until you hear/feel click");
  Serial.println("   → Card should be flush with board");
  Serial.println();
  Serial.println("════════════════════════════════════════════════════════\n");
}

void printSDCardInfo() {
  uint8_t cardType = SD_MMC.cardType();
  
  Serial.println("\n📊 SD Card Information:");
  Serial.print("   Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("   Size: %lluMB\n", cardSize);
  
  uint64_t usedBytes = SD_MMC.usedBytes() / (1024 * 1024);
  Serial.printf("   Used: %lluMB\n", usedBytes);
  
  uint64_t totalBytes = SD_MMC.totalBytes() / (1024 * 1024);
  Serial.printf("   Total: %lluMB\n", totalBytes);
  
  uint64_t freeBytes = (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024);
  Serial.printf("   Free: %lluMB\n\n", freeBytes);
}

void findNextImageNumber() {
  // Find the highest numbered image file
  File root = SD_MMC.open("/");
  if (!root) {
    imageCounter = 1;
    return;
  }
  
  int maxNumber = 0;
  File file = root.openNextFile();
  while (file) {
    String fileName = String(file.name());
    if (fileName.startsWith("/img_") && fileName.endsWith(".jpg")) {
      // Extract number from filename
      String numberStr = fileName.substring(5, 8);
      int num = numberStr.toInt();
      if (num > maxNumber) {
        maxNumber = num;
      }
    }
    file = root.openNextFile();
  }
  
  imageCounter = maxNumber + 1;
  Serial.printf("📁 Next image will be: img_%03d.jpg\n", imageCounter);
}

// ====================================
// Main Loop
// ====================================

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    handleCommand(command);
  }
  
  delay(100);
}

// ====================================
// Command Handling
// ====================================

void handleCommand(String command) {
  if (command == "capture" || command == "start" || command == "go") {
    if (!cameraInitialized || !sdCardInitialized) {
      Serial.println("❌ System not ready! Check camera and SD card.");
      return;
    }
    captureMultipleImages();
    return;
  }
  
  if (command == "list" || command == "ls") {
    listFiles();
    return;
  }
  
  if (command == "delete" || command == "clear") {
    deleteAllImages();
    return;
  }
  
  if (command == "format") {
    formatSDCard();
    return;
  }
  
  if (command == "info") {
    printSDCardInfo();
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
  Serial.println("  capture  - Capture 20 images to SD card");
  Serial.println("  start    - Same as 'capture'");
  Serial.println("  go       - Same as 'capture'");
  Serial.println("  list     - List all files on SD card");
  Serial.println("  delete   - Delete all images from SD card");
  Serial.println("  format   - Format SD card (WARNING: deletes everything!)");
  Serial.println("  info     - Show SD card information");
  Serial.println("  help     - Show this help message");
  Serial.println("════════════════════════════════════════════════════════\n");
}

// ====================================
// Image Capture Functions
// ====================================

void captureMultipleImages() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║         CAPTURING 20 IMAGES TO SD CARD                 ║");
  Serial.println("╚════════════════════════════════════════════════════════╝\n");
  
  int successCount = 0;
  int failCount = 0;
  
  for (int i = 0; i < NUM_IMAGES; i++) {
    Serial.printf("📸 [%d/%d] Capturing image...", i + 1, NUM_IMAGES);
    
    // Turn on flash
    digitalWrite(FLASH_GPIO, HIGH);
    delay(100);  // Brief flash
    
    // Capture image
    camera_fb_t *fb = esp_camera_fb_get();
    
    // Turn off flash
    digitalWrite(FLASH_GPIO, LOW);
    
    if (!fb) {
      Serial.println(" ❌ FAILED");
      failCount++;
      delay(CAPTURE_DELAY_MS);
      continue;
    }
    
    // Create filename
    String filename = "/img_" + String(imageCounter) + ".jpg";
    if (imageCounter < 10) {
      filename = "/img_00" + String(imageCounter) + ".jpg";
    } else if (imageCounter < 100) {
      filename = "/img_0" + String(imageCounter) + ".jpg";
    }
    
    // Save to SD card
    File file = SD_MMC.open(filename, FILE_WRITE);
    if (!file) {
      Serial.println(" ❌ Failed to open file");
      failCount++;
      esp_camera_fb_return(fb);
      delay(CAPTURE_DELAY_MS);
      continue;
    }
    
    size_t bytesWritten = file.write(fb->buf, fb->len);
    file.close();
    
    if (bytesWritten == fb->len) {
      Serial.printf(" ✅ Saved as %s (%d KB)\n", filename.c_str(), fb->len / 1024);
      successCount++;
      imageCounter++;
    } else {
      Serial.println(" ❌ Write error");
      failCount++;
    }
    
    // Return frame buffer
    esp_camera_fb_return(fb);
    
    // Delay before next capture
    delay(CAPTURE_DELAY_MS);
  }
  
  // Summary
  Serial.println("\n════════════════════════════════════════════════════════");
  Serial.printf("✅ Successfully saved: %d images\n", successCount);
  if (failCount > 0) {
    Serial.printf("❌ Failed: %d images\n", failCount);
  }
  Serial.println("════════════════════════════════════════════════════════\n");
}

// ====================================
// SD Card Management Functions
// ====================================

void listFiles() {
  if (!sdCardInitialized) {
    Serial.println("❌ SD Card not available");
    return;
  }
  
  Serial.println("\n📁 Files on SD Card:");
  Serial.println("════════════════════════════════════════════════════════");
  
  File root = SD_MMC.open("/");
  if (!root) {
    Serial.println("❌ Failed to open directory");
    return;
  }
  
  int fileCount = 0;
  unsigned long totalSize = 0;
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      fileCount++;
      Serial.printf("   %s - %d KB\n", file.name(), file.size() / 1024);
      totalSize += file.size();
    }
    file = root.openNextFile();
  }
  
  Serial.println("────────────────────────────────────────────────────────");
  Serial.printf("Total: %d files, %lu KB\n", fileCount, totalSize / 1024);
  Serial.println("════════════════════════════════════════════════════════\n");
}

void deleteAllImages() {
  if (!sdCardInitialized) {
    Serial.println("❌ SD Card not available");
    return;
  }
  
  Serial.println("\n🗑️  Deleting all images...");
  
  File root = SD_MMC.open("/");
  if (!root) {
    Serial.println("❌ Failed to open directory");
    return;
  }
  
  int deletedCount = 0;
  File file = root.openNextFile();
  while (file) {
    String fileName = String(file.name());
    if (fileName.endsWith(".jpg") || fileName.endsWith(".JPG")) {
      if (SD_MMC.remove(fileName)) {
        Serial.printf("   ✓ Deleted: %s\n", fileName.c_str());
        deletedCount++;
      } else {
        Serial.printf("   ✗ Failed to delete: %s\n", fileName.c_str());
      }
    }
    file = root.openNextFile();
  }
  
  Serial.printf("\n✅ Deleted %d images\n\n", deletedCount);
  
  // Reset counter
  imageCounter = 1;
}

void formatSDCard() {
  Serial.println("\n⚠️  WARNING: This will delete ALL files on SD card!");
  Serial.println("Type 'yes' to confirm or anything else to cancel:");
  
  // Wait for confirmation (timeout after 10 seconds)
  unsigned long startTime = millis();
  while (!Serial.available() && (millis() - startTime < 10000)) {
    delay(100);
  }
  
  if (Serial.available()) {
    String confirm = Serial.readStringUntil('\n');
    confirm.trim();
    confirm.toLowerCase();
    
    if (confirm == "yes") {
      Serial.println("\n🔄 Formatting SD card...");
      
      // Delete all files
      File root = SD_MMC.open("/");
      File file = root.openNextFile();
      while (file) {
        String fileName = String(file.name());
        SD_MMC.remove(fileName);
        file = root.openNextFile();
      }
      
      Serial.println("✅ SD card formatted!\n");
      imageCounter = 1;
    } else {
      Serial.println("❌ Format cancelled\n");
    }
  } else {
    Serial.println("❌ Timeout - format cancelled\n");
  }
}
