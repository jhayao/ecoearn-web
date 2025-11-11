# ESP32-CAM Standalone Testing Guide

## 🎯 Purpose

This firmware is designed for **testing the multi-capture voting system** without needing the ESP32 main board. Perfect for development and debugging!

## 📋 Quick Start

### 1. Update Configuration

Edit `esp32_cam_simple.ino` lines 44-45:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
```

Edit line 48:
```cpp
const char* BACKEND_API_URL = "http://YOUR_SERVER_IP:5001/identify/material";
```

### 2. Upload to ESP32-CAM

1. **Board Settings:**
   - Board: "AI Thinker ESP32-CAM"
   - Upload Speed: 115200
   - Partition: "Huge APP (3MB No OTA)"

2. **Wiring for Upload:**
   ```
   USB-to-Serial    ESP32-CAM
   5V          →    5V
   GND         →    GND
   TX          →    U0R (GPIO3)
   RX          →    U0T (GPIO1)
   
   IO0         →    GND  (for programming)
   ```

3. **Upload:**
   - Connect IO0 to GND
   - Click Upload
   - Wait for "Hard resetting via RTS pin..."
   - Disconnect IO0 from GND
   - Press RESET

### 3. Open Serial Monitor

**Important:** Set baud rate to **115200**

Arduino IDE → Tools → Serial Monitor → 115200 baud

### 4. Run Tests

Type commands in Serial Monitor:

```
test      - Start multi-capture detection
capture   - Same as test
start     - Same as test
help      - Show available commands
```

## 📊 Example Output

```
╔════════════════════════════════════════════════════════╗
║   ESP32-CAM Multi-Capture Testing Firmware            ║
╚════════════════════════════════════════════════════════╝

📷 Initializing camera...
✓ Camera initialized successfully

📡 Connecting to WiFi...
   Connecting.........
✓ WiFi connected successfully
   IP Address: 192.168.1.123

⚙️  Configuration:
   Backend API: http://192.168.1.196:5001/identify/material
   Captures per test: 5
   Delay between captures: 300 ms
   Min votes for recycle: 1

✅ System ready for testing!

╔════════════════════════════════════════════════════════╗
║           AVAILABLE COMMANDS                           ║
╚════════════════════════════════════════════════════════╝
  test     - Start multi-capture detection test
  capture  - Same as 'test'
  start    - Same as 'test'
  help     - Show this help message
════════════════════════════════════════════════════════

💡 Type 'test' or 'capture' to start detection
   Type 'help' for available commands
```

## 🧪 Running a Test

Type: **test**

```
🚀 Starting multi-capture detection test...

╔════════════════════════════════════════════════════════╗
║         MULTI-CAPTURE DETECTION PROCESS                ║
╚════════════════════════════════════════════════════════╝
   Capturing 5 images for accuracy...

   [1/5] Capturing image...
      ✓ Image captured: 15234 bytes
      📤 Sending 15234 bytes to backend...
      ✓ Identified: plastic (confidence: 0.92)
      📊 Vote: PLASTIC

   [2/5] Capturing image...
      ✓ Image captured: 14987 bytes
      📤 Sending 14987 bytes to backend...
      ✓ Identified: rejected (confidence: 0.45)
      📊 Vote: REJECTED

   [3/5] Capturing image...
      ✓ Image captured: 15456 bytes
      📤 Sending 15456 bytes to backend...
      ✓ Identified: plastic (confidence: 0.88)
      📊 Vote: PLASTIC

   [4/5] Capturing image...
      ✓ Image captured: 15123 bytes
      📤 Sending 15123 bytes to backend...
      ✓ Identified: rejected (confidence: 0.55)
      📊 Vote: REJECTED

   [5/5] Capturing image...
      ✓ Image captured: 15298 bytes
      📤 Sending 15298 bytes to backend...
      ✓ Identified: plastic (confidence: 0.91)
      📊 Vote: PLASTIC

════════════════════════════════════════════════════════
╔════════════════════════════════════════════════════════╗
║              VOTING RESULTS                            ║
╚════════════════════════════════════════════════════════╝
   Plastic:  3/5 ✓✓✓✓
   Tin:      0/5
   Rejected: 2/5 ✗✗
   Failed:   0/5

────────────────────────────────────────────────────────
╔════════════════════════════════════════════════════════╗
║                                                        ║
║   ✅ FINAL DECISION: PLASTIC (3 detections)            ║
║                                                        ║
╚════════════════════════════════════════════════════════╝
════════════════════════════════════════════════════════

✅ Test complete! Type 'test' to run again.
```

## ⚙️ Configuration

### Change Number of Captures

Line 83:
```cpp
#define NUM_CAPTURES 5   // Change to 3, 7, 10, etc.
```

### Change Delay Between Captures

Line 84:
```cpp
#define CAPTURE_DELAY_MS 300   // Change to 200, 500, etc.
```

### Change Voting Rule

Line 85:
```cpp
#define MIN_VOTES_FOR_RECYCLE 1   // Change to 2 or 3
```

**Examples:**
- `1` = At least 1 recyclable vote needed (lenient)
- `2` = At least 2 recyclable votes needed (balanced)
- `3` = At least 3 recyclable votes needed (strict, majority)

## 🐛 Troubleshooting

### Camera Init Failed
```
❌ Camera initialization FAILED!
```
**Fix:**
- Check camera ribbon cable
- Ensure 5V power with enough current
- Try different power source

### WiFi Failed
```
❌ WiFi connection failed!
```
**Fix:**
- Verify SSID and password
- Use 2.4GHz WiFi (not 5GHz)
- Move closer to router

### HTTP Error
```
❌ HTTP error: 404
❌ HTTP error: -1
```
**Fix:**
- Check backend server is running
- Verify `BACKEND_API_URL` is correct
- Test URL in browser
- Ensure ESP32-CAM and server on same network

### No Serial Output
**Fix:**
- Ensure baud rate is **115200**
- Press RESET button on ESP32-CAM
- Disconnect IO0 from GND if still connected
- Try different USB cable/port

## 📈 Testing Different Items

### Test 1: Clear Plastic Bottle
Place in front of camera, type `test`

**Expected:** 4-5/5 plastic votes → PLASTIC

### Test 2: Dirty/Damaged Plastic
Place damaged plastic item, type `test`

**Expected:** 1-3/5 plastic votes → Still PLASTIC (1/5 rule)

### Test 3: Tin Can
Place tin/aluminum can, type `test`

**Expected:** 3-5/5 tin votes → TIN

### Test 4: Paper/Food/Other
Place non-recyclable item, type `test`

**Expected:** 0/5 recyclable votes → REJECTED

### Test 5: Mixed Signals
Try items that might confuse AI

**Expected:** Voting system should handle inconsistent results

## 💡 Tips

- **Point camera at item:** Make sure item is clearly visible
- **Good lighting:** Flash LED helps, but ambient light is better
- **Multiple angles:** Try rotating item between tests
- **Backend logs:** Check backend console for AI responses
- **Network quality:** Ensure strong WiFi signal

## 🔧 Advanced Testing

### Test Backend Connection
```bash
# From your computer, test the API
curl -X POST http://YOUR_SERVER:5001/identify/material \
  -H "Content-Type: image/jpeg" \
  --data-binary "@test_image.jpg"
```

### Monitor Network Traffic
Watch backend logs to see incoming requests

### Adjust Thresholds
Try different `MIN_VOTES_FOR_RECYCLE` values to find optimal setting

### Performance Testing
Count time from "test" command to final decision

## 📊 What to Look For

✅ **Good Signs:**
- All 5 images capture successfully
- Backend responds to all requests
- Voting results are consistent
- Final decision makes sense

⚠️ **Warning Signs:**
- Multiple capture failures
- HTTP errors from backend
- Inconsistent voting (2 plastic, 2 tin, 1 rejected)
- Very low confidence scores

❌ **Problems:**
- No images capture
- WiFi constantly disconnecting
- Backend always returns error
- All votes are "rejected"

## 🎯 Next Steps

Once testing is successful:

1. ✅ Verify multi-capture voting works as expected
2. ✅ Test with various materials
3. ✅ Confirm backend AI accuracy
4. ✅ Adjust `NUM_CAPTURES` and `MIN_VOTES` if needed
5. ✅ Create production firmware with ESP32 main board integration

## 📝 Files

- `esp32_cam_simple.ino` - Main testing firmware
- `TESTING_GUIDE.md` - This file
- `MULTI_CAPTURE.md` - Voting system documentation
- `VOTING_FLOW.md` - Visual flow diagrams

## 🚀 Summary

**This standalone firmware lets you:**
- ✅ Test camera and WiFi independently
- ✅ Verify backend API integration
- ✅ Validate multi-capture voting system
- ✅ Debug without ESP32 main board
- ✅ See detailed Serial Monitor output
- ✅ Quickly iterate on configuration

**Perfect for development and testing before final deployment!**

---

**Baud Rate:** 115200  
**Commands:** test, capture, start, help  
**Upload via:** USB-to-Serial adapter  
**No ESP32 main board needed!**
