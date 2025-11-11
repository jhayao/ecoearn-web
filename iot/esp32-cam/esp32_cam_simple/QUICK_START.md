# ESP32-CAM Simple Setup - Quick Start Guide

## 📋 What You Need

- ✅ ESP32-CAM AI Thinker module
- ✅ USB-to-Serial adapter (FTDI or CP2102)
- ✅ ESP32 38-pin main board (already programmed)
- ✅ 5V power supply (3A recommended)
- ✅ Jumper wires
- ✅ WiFi network
- ✅ Backend server running on network

## 🔧 Step-by-Step Setup

### Step 1: Update Configuration (5 minutes)

Open `esp32_cam_simple.ino` in Arduino IDE and update:

```cpp
// Line 32-33: Your WiFi
const char* WIFI_SSID = "YOUR_WIFI_NAME";      // ← Change this
const char* WIFI_PASSWORD = "YOUR_PASSWORD";    // ← Change this

// Line 36: Your backend server IP
const char* BACKEND_API_URL = "http://192.168.1.100:5001/identify/material";  // ← Change IP
```

### Step 2: Upload to ESP32-CAM (10 minutes)

#### 2.1 Install Required Library
Arduino IDE → Tools → Manage Libraries → Search "ArduinoJson" → Install version 6.x

#### 2.2 Board Settings
- Tools → Board → ESP32 Arduino → **"AI Thinker ESP32-CAM"**
- Tools → Upload Speed → **115200**
- Tools → Partition Scheme → **"Huge APP (3MB No OTA)"**

#### 2.3 Wiring for Upload

```
USB-to-Serial Adapter          ESP32-CAM
┌────────────────┐         ┌──────────────┐
│                │         │              │
│  5V       ─────┼─────────┤ 5V           │
│  GND      ─────┼─────────┤ GND          │
│  TX       ─────┼─────────┤ U0R (GPIO3)  │
│  RX       ─────┼─────────┤ U0T (GPIO1)  │
│                │         │              │
└────────────────┘         │ IO0     ─────┤──── GND (for upload)
                           │              │
                           └──────────────┘
```

**Important:** Connect **IO0 to GND** for programming mode!

#### 2.4 Upload Process
1. Connect all wires as shown above
2. Plug USB adapter into computer
3. Arduino IDE → Tools → Port → Select your adapter port
4. Click **Upload** button
5. Wait for "Connecting........_____....."
6. Press **RESET button** on ESP32-CAM if it doesn't connect
7. Wait for upload to complete ("Hard resetting via RTS pin...")
8. **Disconnect IO0 from GND**
9. Press RESET button again

### Step 3: Connect to ESP32 Main Board (5 minutes)

#### 3.1 Remove Programming Wires
Disconnect the USB-to-Serial adapter completely.

#### 3.2 Final Wiring

```
ESP32 Main Board (38-pin)      ESP32-CAM
┌────────────────────────┐     ┌──────────────┐
│                        │     │              │
│  GPIO 32 (TX1)   ──────┼─────┤ U0R (GPIO3)  │ ← RX
│  GPIO 33 (RX1)   ──────┼─────┤ U0T (GPIO1)  │ ← TX
│  GND             ──────┼─────┤ GND          │
│                        │     │              │
└────────────────────────┘     └──────────────┘

External 5V Power Supply
│
├─── ESP32 Main Board (5V pin)
├─── ESP32-CAM (5V pin)
│
└─── GND (common ground)
```

**Important:** 
- TX → RX, RX → TX (crossover connection)
- Use **5V external power**, NOT USB power
- Minimum 3A power supply recommended

### Step 4: Test (2 minutes)

#### 4.1 Power On
Connect 5V power supply. ESP32-CAM will:
1. Initialize camera
2. Connect to WiFi
3. Wait for ESP32 main board PING

#### 4.2 Check Status
Open Serial Monitor on **ESP32 Main Board** (baud 115200). You should see:
```
WIFI_CONNECTED:192.168.1.123
ESP32_CONNECTED
CAM_READY
```

#### 4.3 Test Detection
Using ESP32 main board's bypass mode:
```
1. Type: activate
2. Place object near Sharp IR sensor
3. Type: plastic  (or tin/reject)
4. OR wait for auto-detection
```

Expected ESP32-CAM messages:
```
CAPTURING_IMAGE
IMAGE_CAPTURED:15234_bytes
IDENTIFIED:plastic:0.95
OPEN_PLASTIC
```

## 🎯 Complete System Flow

```
USER
 │
 │ Scans QR code
 ▼
ESP32 MAIN BOARD
 │
 │ Activates bin, opens lid
 │ User drops trash
 │ Sharp IR sensor detects object
 │
 │ Sends: TRASH_DETECTED
 ▼
ESP32-CAM
 │
 │ 1. Captures image
 │ 2. POST to backend API
 │ 3. Receives: {materialType: "plastic"}
 │
 │ Sends: OPEN_PLASTIC
 ▼
ESP32 MAIN BOARD
 │
 │ 1. Closes lid
 │ 2. Rotates platform to plastic position
 │ 3. Drops trash into plastic compartment
 │ 4. Sends transaction to backend
 │
 ▼
BACKEND SERVER
 │
 │ Records transaction
 │ Updates user points
 │ Logs activity
 ▼
DATABASE
```

## 📊 Troubleshooting

### Problem: Camera Init Failed
**Solution:**
- Check camera ribbon cable is properly inserted
- Ensure 5V power with enough current (500mA+)
- Try different power source
- Press RESET button

### Problem: WiFi Failed
**Solution:**
- Verify SSID and password in code
- Make sure it's 2.4GHz WiFi (not 5GHz)
- Move closer to router
- Check router allows new devices

### Problem: HTTP Error
**Solution:**
- Verify backend server is running: `npm run dev`
- Test URL in browser: `http://YOUR_IP:5001/identify/material`
- Check IP address in code matches server
- Ensure ESP32-CAM and server on same network
- Disable firewall temporarily to test

### Problem: No Communication with ESP32
**Solution:**
- Verify wiring: GPIO32→GPIO3, GPIO33→GPIO1
- Check common ground connection
- Try swapping TX/RX wires
- Confirm baud rate is 9600 on both boards
- Measure voltage on GPIO pins (should be 3.3V)

### Problem: Identification Takes Too Long
**Solution:**
- Check backend AI service is running
- Optimize image quality vs size
- Improve WiFi signal strength
- Use faster backend hardware

## 📝 Configuration Checklist

- [ ] WiFi SSID updated
- [ ] WiFi password updated
- [ ] Backend API URL updated (correct IP address)
- [ ] ArduinoJson library installed
- [ ] Board set to "AI Thinker ESP32-CAM"
- [ ] IO0 connected to GND during upload
- [ ] IO0 disconnected from GND after upload
- [ ] Serial wiring correct (TX→RX crossover)
- [ ] Common ground connected
- [ ] 5V power supply connected (3A minimum)
- [ ] Backend server running
- [ ] ESP32 main board programmed with matching code

## 🚀 Next Steps

1. ✅ Upload this code to ESP32-CAM
2. ✅ Connect to ESP32 main board
3. ✅ Test with bypass commands
4. ✅ Test with real QR code scanning
5. ✅ Calibrate compartment servo positions
6. ✅ Test full detection workflow
7. ✅ Deploy in actual recycling bin

## 📞 Support

If you encounter issues:
1. Check troubleshooting section above
2. Verify all wiring connections
3. Test components individually
4. Check Serial Monitor for error messages
5. Verify backend API is responding

---

**Quick Links:**
- Main Code: `esp32_cam_simple.ino`
- Full Documentation: `README.md`
- ESP32 Main Board Pinout: `../../ESP32_38PIN_PINOUT.md`

**Hardware:**
- ESP32-CAM: AI Thinker module with OV2640 camera
- Communication: 9600 baud serial
- Power: 5V @ 500mA minimum (3A recommended)
