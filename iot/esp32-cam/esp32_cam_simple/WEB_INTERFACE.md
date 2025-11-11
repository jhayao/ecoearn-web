# ESP32-CAM Web Interface Guide

## 🌐 Overview

The ESP32-CAM now includes a **live web interface** that displays:
- ✅ Last captured image (auto-refreshing)
- ✅ Voting results from multi-capture test
- ✅ Final decision (PLASTIC/TIN/REJECTED)
- ✅ System configuration
- ✅ Start test button

## 📋 How to Access

### 1. Upload Firmware
Upload `esp32_cam_simple.ino` to your ESP32-CAM

### 2. Open Serial Monitor
Set baud rate to **115200**

You'll see:
```
✓ WiFi connected successfully
   IP Address: 192.168.1.123

🌐 Starting web server...
✓ Web server started
   Open browser: http://192.168.1.123
```

### 3. Open Web Browser
Navigate to the IP address shown (e.g., `http://192.168.1.123`)

## 🎨 Web Interface Features

### Main Page (/)

```
╔════════════════════════════════════════════════════╗
║     ESP32-CAM Multi-Capture Testing                ║
╚════════════════════════════════════════════════════╝

[🚀 Start Test]  [🔄 Refresh Results]

📸 Last Captured Image
┌────────────────────────────────────┐
│                                    │
│     [Shows actual camera image]    │
│                                    │
└────────────────────────────────────┘
Auto-refreshes every 2 seconds during test

📊 Last Test Results

🌱 Plastic: 3/5 ✓✓✓✓
🥫 Tin: 0/5
❌ Rejected: 2/5 ✗✗
⚠️ Failed: 0/5

╔════════════════════════════════════╗
║ ✅ FINAL DECISION: PLASTIC (3)     ║
╚════════════════════════════════════╝

⚙️ Configuration
Backend API: http://192.168.1.196:5001/identify/material
Captures per test: 5
Delay between captures: 300 ms
Min votes for recycle: 1 (1/5 rule)
IP Address: 192.168.1.123
```

### Buttons

**🚀 Start Test**
- Triggers multi-capture detection
- Same as typing "test" in Serial Monitor
- Page auto-reloads after 15 seconds

**🔄 Refresh Results**
- Manually refreshes the page
- Updates voting results
- Reloads latest image

## 📸 Image Display

### Auto-Refresh
The image automatically refreshes every **2 seconds** during testing

This allows you to see:
- Image 1 of 5
- Image 2 of 5
- Image 3 of 5
- Image 4 of 5
- Image 5 of 5 (final image stays displayed)

### Image Endpoint
Direct image access: `http://ESP32_IP/image`

You can:
- View in browser
- Save image (right-click → Save As)
- Use in other applications

## 🧪 Testing Workflow

### Option 1: Web Interface
1. Open web page in browser
2. Point camera at item
3. Click **"🚀 Start Test"**
4. Watch images update in real-time
5. See final results after 15 seconds

### Option 2: Serial Monitor
1. Type **"test"** in Serial Monitor
2. Watch detailed progress in Serial Monitor
3. Refresh web page to see results

### Option 3: Both!
1. Serial Monitor on one screen
2. Web interface on another screen
3. Type "test" in Serial Monitor
4. Watch both simultaneously:
   - Serial: Detailed logs
   - Web: Visual feedback

## 🎯 Use Cases

### Development & Debugging
- **See what the camera sees** before sending to backend
- **Verify image quality** (focus, lighting, angle)
- **Check if item is in frame**
- **Debug capture failures**

### Testing Different Items
- Place item in front of camera
- Start test from web interface
- See captured image immediately
- Verify voting results visually

### Demonstrations
- Show stakeholders how it works
- Real-time visual feedback
- Professional-looking interface
- No need to explain Serial Monitor

### Quality Control
- Verify camera alignment
- Check lighting conditions
- Test different angles
- Validate image clarity

## 📊 Example Testing Session

### Setup
1. Power on ESP32-CAM
2. Open Serial Monitor @ 115200
3. Note IP address (e.g., 192.168.1.123)
4. Open browser to that IP

### Test Plastic Bottle
1. Place clear plastic bottle in front of camera
2. Click "Start Test" on web page
3. Watch progress:
   - Image 1: Clear bottle visible
   - Image 2: Same view
   - Image 3: Same view
   - Image 4: Same view
   - Image 5: Same view
4. See results:
   - Plastic: 5/5 ✓✓✓✓✓
   - Decision: PLASTIC

### Test Dirty Plastic
1. Place damaged/dirty plastic item
2. Click "Start Test"
3. Watch images show unclear view
4. See results:
   - Plastic: 2/5 ✓✓
   - Rejected: 3/5 ✗✗✗
   - Decision: PLASTIC (1/5 rule works!)

### Test Non-Recyclable
1. Place paper or food item
2. Click "Start Test"
3. See images of non-recyclable
4. Results:
   - Plastic: 0/5
   - Tin: 0/5
   - Rejected: 5/5 ✗✗✗✗✗
   - Decision: REJECTED

## 🔧 Troubleshooting

### Web Page Not Loading
**Check:**
- ESP32-CAM is powered on
- WiFi is connected (check Serial Monitor)
- IP address is correct
- ESP32-CAM and computer on same network
- No firewall blocking port 80

**Solution:**
```
1. Open Serial Monitor
2. Look for "IP Address: 192.168.x.x"
3. Use that exact IP in browser
4. Try http:// explicitly (not https://)
```

### Image Not Showing
**Reasons:**
- No test has been run yet
- Camera capture failed
- Image buffer was cleared

**Solution:**
- Run a test first (click "Start Test")
- Check Serial Monitor for capture errors
- Refresh page after test completes

### Image Not Updating
**Check:**
- JavaScript is enabled in browser
- Page is auto-refreshing (every 2 seconds)
- Test is actually running

**Solution:**
- Manually refresh page
- Check browser console for errors
- Clear browser cache

### Test Button Not Working
**Check Serial Monitor:**
- Look for "Test triggered from web interface"
- Check for error messages

**Note:** Test runs synchronously, so page may seem frozen for ~15 seconds. This is normal!

## 📱 Mobile Access

The web interface is **mobile-responsive**!

Access from:
- ✅ Phone browser
- ✅ Tablet browser
- ✅ Laptop browser
- ✅ Desktop browser

**Same WiFi network required**

## 🌟 Advanced Features

### Multiple Browser Windows
- Open multiple tabs to same IP
- Each sees same last image
- All show same voting results
- Start test from any window

### Image Download
Right-click on image → Save As → `capture.jpg`

Use for:
- Documentation
- Bug reports
- Training data
- Analysis

### Remote Access
If on same network:
- Access from any device
- No USB cable needed
- Wireless testing
- Multiple viewers

## 🎨 Customization

### Change Auto-Refresh Rate
Edit line in `handleRoot()`:
```javascript
setInterval(refreshImage, 2000); // Change 2000 to milliseconds
```

Examples:
- `1000` = 1 second (faster)
- `5000` = 5 seconds (slower)

### Change Web Port
Edit line 54:
```cpp
WebServer server(80); // Change 80 to different port
```

Then access: `http://IP:PORT`

### Disable Web Server
Comment out in `setup()`:
```cpp
// setupWebServer();
```

## 📊 Performance Impact

**Memory:**
- Image buffer: ~15-60KB (one image stored)
- Web server: ~4KB overhead
- Total: Minimal impact

**Network:**
- Web page: ~5KB HTML
- Image: ~15-60KB per request
- Auto-refresh: ~1-4KB/second during test

**CPU:**
- Web server handling: Negligible
- Image serving: Fast (direct buffer send)

## 🚀 Best Practices

1. **Keep browser open** during testing for live updates
2. **Use good lighting** - you can see image quality
3. **Center item in frame** - visual feedback helps
4. **Wait for test to complete** - don't refresh during test
5. **Check Serial Monitor** for detailed logs

## 📝 Endpoints Reference

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Main web interface |
| `/image` | GET | Last captured image (JPEG) |
| `/test` | GET | Trigger multi-capture test |

## 🎯 Summary

**The web interface provides:**
- ✅ Visual feedback of captured images
- ✅ Real-time voting results
- ✅ Easy testing without Serial Monitor
- ✅ Mobile-friendly interface
- ✅ Professional presentation
- ✅ Auto-refreshing image display
- ✅ One-click testing

**Perfect for:**
- Development and debugging
- Demonstrations
- Quality control
- Testing different items
- Verifying camera alignment

**Access it at:** `http://ESP32_CAM_IP` (shown in Serial Monitor)

---

**Note:** Web server runs on port 80  
**Image updates:** Every 2 seconds during test  
**Auto-reload:** 15 seconds after starting test  
**Mobile-friendly:** Yes, responsive design
