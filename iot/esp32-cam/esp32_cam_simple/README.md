# ESP32-CAM Simple Material Identification

**Simplified ESP32-CAM firmware for direct backend API integration**

## Overview

This is a streamlined version of the ESP32-CAM controller that focuses on one task:
1. Wait for `TRASH_DETECTED` command from ESP32 main board
2. Capture high-quality image
3. Send to backend `identify/material` API
4. Return result command (`OPEN_PLASTIC`, `OPEN_TIN`, or `OPEN_REJECTED`)

## Features

✅ **Direct API Integration** - Sends images to `identify/material` endpoint  
✅ **Simple Command Protocol** - Easy serial communication with ESP32 main board  
✅ **Auto Flash** - Turns on LED flash during capture for better lighting  
✅ **Connection Verification** - PING-PONG handshake with ESP32 main board  
✅ **WiFi Auto-Reconnect** - Automatically recovers from WiFi drops  
✅ **High Quality Images** - SVGA (800x600) with quality=10 for best results  

## Hardware Connections

### ESP32 Main Board ↔ ESP32-CAM

| ESP32 Main Board | ESP32-CAM | Function |
|------------------|-----------|----------|
| GPIO 32 (TX) | GPIO 3 (RX/U0R) | Data to CAM |
| GPIO 33 (RX) | GPIO 1 (TX/U0T) | Data from CAM |
| GND | GND | Common ground |
| 5V | 5V | Power (3A recommended) |

**⚠️ Important:**
- ESP32-CAM needs **5V with good current** (at least 500mA, recommend 2-3A)
- Serial connection uses U0R/U0T pins (same as USB programming)
- **USB Serial Monitor will NOT work** while connected to ESP32 main board
- Use ESP32 main board's Serial Monitor to see messages relayed from CAM

## Configuration

### WiFi Settings (Lines 32-33)
```cpp
const char* WIFI_SSID = "Xiaomi_53DE";
const char* WIFI_PASSWORD = "hayao1014";
```

### Backend API URL (Line 36)
```cpp
const char* BACKEND_API_URL = "http://192.168.31.196:5001/identify/material";
```
**Update to your backend server's IP address!**

### Camera Quality (Lines 75-76)
```cpp
#define FRAME_SIZE FRAMESIZE_SVGA  // 800x600
#define JPEG_QUALITY 10            // Lower = better quality (10-63)
```

Options:
- `FRAMESIZE_VGA` (640x480) - Faster, lower quality
- `FRAMESIZE_SVGA` (800x600) - **Default, good balance**
- `FRAMESIZE_XGA` (1024x768) - Slower, higher quality
- `FRAMESIZE_UXGA` (1600x1200) - Maximum quality, very slow

## Serial Communication Protocol

### Commands from ESP32 Main Board → ESP32-CAM

| Command | Description | Expected Response |
|---------|-------------|-------------------|
| `PING` | Connection test | `PONG` |
| `TRASH_DETECTED` | Start capture & identify | See flow below |

### Messages from ESP32-CAM → ESP32 Main Board

| Message | Meaning |
|---------|---------|
| `CAM_READY` | Startup complete, ready to receive commands |
| `PONG` | Response to PING |
| `WIFI_CONNECTED:192.168.x.x` | WiFi connected with IP |
| `WIFI_FAILED` | WiFi connection failed |
| `CAPTURING_IMAGE` | Camera capture started |
| `IMAGE_CAPTURED:12345_bytes` | Image captured successfully |
| `IDENTIFIED:plastic:0.95` | Material identified (type:confidence) |
| `OPEN_PLASTIC` | Command to open plastic compartment |
| `OPEN_TIN` | Command to open tin compartment |
| `OPEN_REJECTED` | Command to open rejected compartment |
| `CAPTURE_FAILED` | Camera capture error |
| `IDENTIFICATION_FAILED` | Backend API error |
| `HTTP_ERROR:404` | Backend HTTP error code |

## Trash Detection Flow

```
┌─────────────────────────────────────────────────────────────────┐
│ ESP32 Main Board                                                │
└─────────────────┬───────────────────────────────────────────────┘
                  │
                  │ User drops trash
                  │ Sharp IR sensor detects object
                  │
                  ▼
          Send: TRASH_DETECTED
                  │
                  ▼
┌─────────────────────────────────────────────────────────────────┐
│ ESP32-CAM                                                       │
│                                                                 │
│  1. Receive: TRASH_DETECTED                                    │
│  2. Send: CAPTURING_IMAGE                                      │
│  3. Turn on flash LED                                          │
│  4. Capture 800x600 JPEG image                                 │
│  5. Send: IMAGE_CAPTURED:15234_bytes                          │
│  6. HTTP POST to backend API                                   │
│  7. Parse JSON response                                        │
│  8. Send: IDENTIFIED:plastic:0.95                             │
│  9. Send: OPEN_PLASTIC                                         │
└─────────────────┬───────────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────────┐
│ ESP32 Main Board                                                │
│                                                                 │
│  1. Receive: OPEN_PLASTIC                                      │
│  2. Execute compartment opening sequence:                      │
│     - Open lid                                                  │
│     - Close lid                                                 │
│     - Rotate platform to plastic position                      │
│     - Drop trash into plastic compartment                      │
│  3. Send confirmation to backend                               │
└─────────────────────────────────────────────────────────────────┘
```

## Backend API Specification

### Endpoint
```
POST http://YOUR_SERVER:5001/identify/material
```

### Request
- **Method:** POST
- **Content-Type:** `image/jpeg`
- **Body:** Raw JPEG binary data (the image)

### Response (JSON)
```json
{
  "success": true,
  "materialType": "plastic",
  "confidence": 0.95,
  "action": "recycle"
}
```

**Material Types:**
- `"plastic"` → Opens plastic compartment
- `"tin"`, `"metal"`, `"aluminum"` → Opens tin compartment
- `"rejected"`, `"reject"`, `"unknown"` → Opens rejected compartment
- Any other value → Defaults to rejected compartment

## Installation Steps

### 1. Update Configuration
Edit the `.ino` file:
```cpp
// Lines 32-33: WiFi credentials
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Line 36: Backend API URL
const char* BACKEND_API_URL = "http://YOUR_SERVER_IP:5001/identify/material";
```

### 2. Install Arduino Libraries
Open Arduino IDE → Tools → Manage Libraries, install:
- **ESP32** board support (via Board Manager)
- **ArduinoJson** by Benoit Blanchon (version 6.x)

### 3. Select Board
- **Board:** "AI Thinker ESP32-CAM"
- **Port:** Your USB-to-Serial adapter port
- **Upload Speed:** 115200
- **Partition Scheme:** "Huge APP (3MB No OTA)"

### 4. Upload Code
1. Connect ESP32-CAM to USB-to-Serial adapter:
   - 5V → 5V
   - GND → GND
   - U0R → TX
   - U0T → RX
   - **IO0 → GND** (for programming mode)
2. Click Upload in Arduino IDE
3. Wait for "Hard resetting via RTS pin..."
4. **Disconnect IO0 from GND**
5. Press RESET button on ESP32-CAM

### 5. Connect to ESP32 Main Board
After successful upload:
1. Disconnect USB-to-Serial adapter
2. Connect to ESP32 main board as per wiring table above
3. Power both boards with 5V supply

## Testing

### Test 1: WiFi Connection
Power on ESP32-CAM. Check ESP32 main board Serial Monitor for:
```
WIFI_CONNECTED:192.168.x.x
```

### Test 2: PING-PONG
From ESP32 main board, the system automatically sends PING. You should see:
```
PONG
ESP32_CONNECTED
```

### Test 3: Image Capture
From ESP32 main board Serial Monitor, send:
```
TRASH_DETECTED
```

Expected response:
```
CAPTURING_IMAGE
IMAGE_CAPTURED:15234_bytes
IDENTIFIED:plastic:0.95
OPEN_PLASTIC
```

### Test 4: Backend Connection
Check backend server logs to verify image was received and processed.

## Troubleshooting

### Camera Init Failed
```
CAM_INIT_FAILED
```
**Solutions:**
- Check camera ribbon cable connection
- Ensure proper power supply (5V, 500mA+)
- Try different USB cable/power source
- Press RESET button

### WiFi Connection Failed
```
WIFI_FAILED
```
**Solutions:**
- Verify SSID and password
- Check WiFi signal strength
- Ensure 2.4GHz WiFi (ESP32-CAM doesn't support 5GHz)
- Move closer to router

### HTTP Error
```
HTTP_ERROR:404
HTTP_ERROR:-1
```
**Solutions:**
- Verify backend server is running
- Check `BACKEND_API_URL` is correct
- Ensure ESP32-CAM and server are on same network
- Test URL in browser: `http://YOUR_SERVER:5001/identify/material`
- Check firewall settings

### No Response from ESP32-CAM
**Solutions:**
- Check serial wiring (TX ↔ RX, not TX ↔ TX)
- Verify baud rate is 9600 on both sides
- Check common ground connection
- Try swapping TX/RX pins
- Monitor GPIO 1/3 with multimeter for activity

### Image Quality Issues
**Solutions:**
- Increase `FRAME_SIZE` to `FRAMESIZE_XGA`
- Decrease `JPEG_QUALITY` to 8 or 6 (lower = better)
- Clean camera lens
- Improve lighting in bin
- Adjust flash LED brightness (add resistor)

## Performance

### Timing Breakdown
1. Receive `TRASH_DETECTED`: instant
2. Capture image: ~200-500ms
3. HTTP POST to backend: ~1-3 seconds
4. Backend AI processing: ~2-5 seconds
5. Return result: instant
6. **Total time: ~3-8 seconds**

### Memory Usage
- Image buffer: ~15-60KB (depends on frame size)
- JSON parsing: ~1KB
- HTTP client: ~4KB
- **Total RAM: ~20-65KB**

### Power Consumption
- Idle: ~70mA @ 5V
- WiFi active: ~160mA @ 5V
- Camera + Flash: ~300mA @ 5V
- **Peak: ~310mA @ 5V (1.55W)**

## Comparison: Simple vs Full Controller

| Feature | Simple (This) | Full Controller |
|---------|---------------|-----------------|
| Code size | 350 lines | 580+ lines |
| Features | API only | API + Streaming + Web UI |
| Setup complexity | Easy | Moderate |
| Memory usage | Low | High |
| Best for | Production | Development/Debug |

## API Response Examples

### Successful Plastic Detection
```json
{
  "success": true,
  "materialType": "plastic",
  "confidence": 0.95,
  "action": "recycle"
}
```
→ ESP32-CAM sends: `OPEN_PLASTIC`

### Successful Tin Detection
```json
{
  "success": true,
  "materialType": "tin",
  "confidence": 0.88,
  "action": "recycle"
}
```
→ ESP32-CAM sends: `OPEN_TIN`

### Rejected Item
```json
{
  "success": true,
  "materialType": "rejected",
  "confidence": 0.60,
  "action": "reject"
}
```
→ ESP32-CAM sends: `OPEN_REJECTED`

### Backend Error
```json
{
  "success": false,
  "error": "AI model not loaded"
}
```
→ ESP32-CAM sends: `IDENTIFICATION_FAILED`

## Advanced Configuration

### Adjust Timeouts
```cpp
const int HTTP_TIMEOUT = 10000; // Line 39, milliseconds
const unsigned long CONNECTION_CHECK_INTERVAL = 30000; // Line 83
```

### Change Serial Baud Rate
```cpp
const int SERIAL_BAUD = 9600; // Line 45
```
**Important:** Must match ESP32 main board baud rate!

### Disable Flash LED
```cpp
// In captureImage() function, comment out:
// digitalWrite(FLASH_LED_PIN, HIGH);
// delay(100);
```

### Multiple Material Types Support
The code automatically handles these material types:
- `plastic` → Plastic compartment
- `tin`, `metal`, `aluminum` → Tin compartment  
- `rejected`, `reject`, `unknown` → Rejected compartment
- Anything else → Defaults to rejected

To add new types, modify the `processTrashDetection()` function (lines 300-320).

## Integration with Main Board

### Update ESP32 Main Board Code
Ensure the main board has matching serial configuration:

```cpp
// ESP32 Main Board Configuration
#define ESP32_CAM_SERIAL Serial1
const int ESP32_CAM_BAUD = 9600;
const int ESP32_CAM_RX_PIN = 33;
const int ESP32_CAM_TX_PIN = 32;

void setup() {
  ESP32_CAM_SERIAL.begin(ESP32_CAM_BAUD, SERIAL_8N1, ESP32_CAM_RX_PIN, ESP32_CAM_TX_PIN);
}
```

## Files in This Folder

- `esp32_cam_simple.ino` - Main firmware code
- `README.md` - This documentation

## License

Part of the EcoEarn Smart Recycling Bin System

---

**Last Updated:** January 2025  
**Hardware:** ESP32-CAM AI Thinker Module  
**Backend:** Node.js + Python AI Service  
**Project:** EcoEarn - Smart Waste Management System
