# ESP32-CAM SD Card Image Capture

Complete system for capturing 20 images and saving them to an SD card using ESP32-CAM.

## 📋 Features

- ✅ **Captures 20 images** sequentially
- 💾 **Saves to SD card** as JPEG files
- 🔢 **Automatic numbering** (img_001.jpg, img_002.jpg, etc.)
- 💡 **Flash LED support** for better lighting
- 📊 **SD card management** (list, delete, format)
- 🖥️ **Serial Monitor interface** with beautiful formatting
- ⚙️ **Configurable** image quality and size

## 🔌 Hardware Requirements

### Required Components
1. **ESP32-CAM AI Thinker module**
2. **MicroSD card** (formatted as FAT32, 4GB-32GB recommended)
3. **USB-to-Serial adapter** (FTDI, CP2102, etc.)
4. **Power supply** (5V, at least 500mA)

### SD Card Pin Connections (Built-in)
The ESP32-CAM has built-in SD card slot. No external wiring needed!

| SD Pin | ESP32-CAM GPIO |
|--------|----------------|
| CMD    | GPIO 15        |
| CLK    | GPIO 14        |
| DATA0  | GPIO 2         |

**⚠️ IMPORTANT:** Insert SD card BEFORE powering on the ESP32-CAM!

## 🚀 Quick Start

### 1. Prepare SD Card
- Format SD card as **FAT32**
- Size: 4GB to 32GB (SDHC works best)
- Insert into ESP32-CAM before powering on

### 2. Upload Code
1. Open `esp32_cam_sd_capture.ino` in Arduino IDE
2. Select board: **AI Thinker ESP32-CAM**
3. Connect ESP32-CAM to USB-to-Serial adapter:
   - ESP32-CAM 5V → FTDI 5V
   - ESP32-CAM GND → FTDI GND
   - ESP32-CAM U0R → FTDI TX
   - ESP32-CAM U0T → FTDI RX
   - ESP32-CAM IO0 → GND (for programming mode)
4. Click Upload
5. After upload, disconnect IO0 from GND
6. Press RESET button

### 3. Open Serial Monitor
- Set baud rate to **115200**
- You should see:
```
╔════════════════════════════════════════════════════════╗
║       ESP32-CAM SD Card Image Capture System           ║
╚════════════════════════════════════════════════════════╝

✅ Camera initialized successfully
✅ SD Card initialized successfully

📊 SD Card Information:
   Type: SDHC
   Size: 16384MB
   Used: 0MB
   Total: 16384MB
   Free: 16384MB

📁 Next image will be: img_001.jpg

════════════════════════════════════════════════════════
Ready! Type 'help' for available commands
════════════════════════════════════════════════════════
```

## 🎮 Commands

### Capture Images
```
capture    - Capture 20 images to SD card
start      - Same as 'capture'
go         - Same as 'capture'
```

**Example Output:**
```
╔════════════════════════════════════════════════════════╗
║         CAPTURING 20 IMAGES TO SD CARD                 ║
╚════════════════════════════════════════════════════════╝

📸 [1/20] Capturing image... ✅ Saved as /img_001.jpg (45 KB)
📸 [2/20] Capturing image... ✅ Saved as /img_002.jpg (46 KB)
📸 [3/20] Capturing image... ✅ Saved as /img_003.jpg (44 KB)
...
📸 [20/20] Capturing image... ✅ Saved as /img_020.jpg (47 KB)

════════════════════════════════════════════════════════
✅ Successfully saved: 20 images
════════════════════════════════════════════════════════
```

### File Management
```
list       - List all files on SD card
delete     - Delete all images from SD card
format     - Format SD card (deletes everything!)
info       - Show SD card information
help       - Show available commands
```

## ⚙️ Configuration

You can customize the settings at the top of the code:

```cpp
// Number of images to capture
#define NUM_IMAGES 20

// Delay between captures (milliseconds)
#define CAPTURE_DELAY_MS 500

// Image settings
#define FRAME_SIZE FRAMESIZE_SVGA    // 800x600
#define JPEG_QUALITY 10              // 0-63, lower = higher quality
```

### Available Frame Sizes
| Setting | Resolution | Use Case |
|---------|------------|----------|
| `FRAMESIZE_QQVGA` | 160x120 | Very small files |
| `FRAMESIZE_QVGA` | 320x240 | Small files |
| `FRAMESIZE_VGA` | 640x480 | Standard |
| `FRAMESIZE_SVGA` | 800x600 | **Default** |
| `FRAMESIZE_XGA` | 1024x768 | High quality |
| `FRAMESIZE_HD` | 1280x720 | HD |
| `FRAMESIZE_SXGA` | 1280x1024 | Very high quality |
| `FRAMESIZE_UXGA` | 1600x1200 | Max quality |

### JPEG Quality Settings
- **0-10**: Highest quality (larger files)
- **10-30**: Good quality (recommended)
- **30-63**: Lower quality (smaller files)

## 📊 File Naming

Images are automatically numbered:
```
img_001.jpg
img_002.jpg
img_003.jpg
...
img_020.jpg
img_021.jpg (next batch)
```

The system automatically finds the next available number, so you won't overwrite existing images.

## 🔧 Troubleshooting

### SD Card Not Detected
```
❌ SD Card initialization FAILED!
   Please insert SD card and restart
```

**Solutions:**
1. ✅ Make sure SD card is inserted BEFORE powering on
2. ✅ Format SD card as FAT32
3. ✅ Try a different SD card (use 4GB-32GB)
4. ✅ Clean SD card contacts
5. ✅ Press RESET button after inserting card

### Camera Initialization Failed
```
❌ Camera initialization FAILED!
```

**Solutions:**
1. ✅ Check power supply (needs stable 5V, 500mA+)
2. ✅ Press RESET button
3. ✅ Re-upload code
4. ✅ Try different ESP32-CAM board

### Images Not Saving
```
📸 [1/20] Capturing image... ❌ Failed to open file
```

**Solutions:**
1. ✅ Check SD card has free space (`info` command)
2. ✅ Format SD card (`format` command)
3. ✅ Try a different SD card
4. ✅ Reduce image quality or size

### Poor Image Quality
**Solutions:**
1. ✅ Lower `JPEG_QUALITY` value (try 5-10)
2. ✅ Increase `FRAME_SIZE` (try FRAMESIZE_XGA)
3. ✅ Ensure good lighting
4. ✅ Flash LED automatically turns on during capture

## 📁 Reading Images from SD Card

### Option 1: Remove SD Card
1. Power off ESP32-CAM
2. Remove SD card
3. Insert into computer using SD card reader
4. Copy images

### Option 2: FTP Server (Advanced)
You could add FTP server functionality to download images over WiFi without removing SD card.

## 💾 Storage Capacity

Approximate number of images based on SD card size:

| SD Card | Quality: High (10) | Quality: Medium (20) | Quality: Low (40) |
|---------|-------------------|---------------------|------------------|
| 4GB     | ~3,500 images     | ~7,000 images       | ~14,000 images   |
| 8GB     | ~7,000 images     | ~14,000 images      | ~28,000 images   |
| 16GB    | ~14,000 images    | ~28,000 images      | ~56,000 images   |
| 32GB    | ~28,000 images    | ~56,000 images      | ~112,000 images  |

*Assumes 800x600 resolution, ~50KB per image*

## 🔄 Typical Workflow

1. **Insert SD card** → Power on ESP32-CAM
2. **Open Serial Monitor** @ 115200 baud
3. **Type `capture`** → Captures 20 images
4. **Type `list`** → Verify images saved
5. **Power off** → Remove SD card → Copy images
6. **Optional:** Type `delete` to clear images for next batch

## ⚡ Performance

- **Capture time:** ~500ms per image
- **Total time (20 images):** ~10-12 seconds
- **File size:** ~40-50KB per image (SVGA, quality 10)
- **Flash LED:** Auto-on during capture for better lighting

## 🎯 Use Cases

- 📸 **Time-lapse photography** - Capture events over time
- 🔬 **Scientific monitoring** - Document experiments
- 🏗️ **Construction progress** - Daily site photos
- 🌱 **Plant growth tracking** - Monitor plant development
- 🔍 **Quality control** - Product inspection batches
- 🎨 **Art projects** - Sequential image capture
- 📊 **Dataset collection** - Training data for AI models

## 🆚 Comparison with Other Modes

| Feature | SD Card Mode | API Mode | Web Interface |
|---------|-------------|----------|---------------|
| Storage | Local SD card | Backend server | None |
| WiFi needed | ❌ No | ✅ Yes | ✅ Yes |
| Offline capable | ✅ Yes | ❌ No | ❌ No |
| Image count | Unlimited | Limited by API | Limited |
| Speed | Very fast | Slower (network) | Slower |
| Portability | ✅ High | ❌ Low | ❌ Low |

## 📚 Advanced Modifications

### Change Number of Images
```cpp
#define NUM_IMAGES 50  // Capture 50 images instead of 20
```

### Add Timestamp to Filenames
```cpp
// Instead of img_001.jpg, use timestamp
String filename = "/" + String(millis()) + ".jpg";
```

### Continuous Capture Loop
```cpp
// In loop() function, capture continuously every 5 seconds
unsigned long lastCapture = 0;
if (millis() - lastCapture > 5000) {
  captureSingleImage();
  lastCapture = millis();
}
```

### Add WiFi Time Sync
Add NTP time sync to use real timestamps instead of sequential numbers.

## 🐛 Debug Mode

The code includes detailed debug output. Check Serial Monitor for:
- ✅ Initialization status
- 📊 SD card information
- 📸 Capture progress
- 💾 File save status
- ❌ Error messages

## 📖 Summary

This ESP32-CAM SD card capture system provides:
- **Simple operation** - Just type `capture`
- **Reliable storage** - Direct to SD card
- **No WiFi needed** - Fully offline
- **High capacity** - Thousands of images
- **Fast capture** - 20 images in ~10 seconds
- **Easy retrieval** - Remove SD card and copy

Perfect for standalone image capture without internet dependency! 🎉
