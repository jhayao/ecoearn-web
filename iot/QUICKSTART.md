# Quick Start Guide - EcoEarn IoT GPS Tracking

A condensed guide to get your IoT bin tracker up and running in under 30 minutes.

## What You Need

- **NodeMCU ESP8266** (or other ESP8266/ESP32 board)
- GY-GPS6MV2 GPS module
- USB cable
- WiFi credentials
- Arduino IDE installed

## 5-Minute Setup

### 1. Hardware Connections (NodeMCU)

```
GPS Module → NodeMCU
VCC → 3.3V
GND → GND
TX → D2 (GPIO4)
RX → D1 (GPIO5)
```

**📖 Need detailed wiring?** See `NODEMCU_WIRING.md` for complete diagrams and photos.

### 2. Get API Key

1. Login to EcoEarn admin
2. Go to Bins → Add New Bin
3. Fill details and submit
4. **Copy the API key** shown in alert (you won't see it again!)

Example: `BIN_LK3M9Q_H7G8J9K2L4M5N6P8Q`

### 3. Configure Arduino Code

Open `ecoearn_bin_tracker_test.ino` and edit:

```cpp
const char* WIFI_SSID = "YourWiFiName";
const char* WIFI_PASSWORD = "YourPassword";
const char* API_KEY = "BIN_LK3M9Q_YOUR_KEY_HERE";
const char* SERVER_URL = "https://yourdomain.com/api/iot/update-location";
```

### 4. Upload Code

1. Connect ESP board via USB
2. Arduino IDE: Tools → Board → **NodeMCU 1.0 (ESP-12E Module)**
3. Arduino IDE: Tools → Port → Select COM port
4. Click Upload (→)

### 5. Verify

1. Open Serial Monitor (115200 baud)
2. Look for "✓✓✓ SUCCESS! Location updated successfully! ✓✓✓"
3. Check admin panel - bin location should update

## Expected Serial Output

```
=================================
EcoEarn Test Mode
=================================

Connecting to WiFi: YourNetwork
.......
✓ WiFi connected successfully!
IP address: 192.168.1.100

Test mode ready!
Will send test coordinates every 30 seconds

========================================
Test Update #1
========================================

Sending location update to server...
HTTP Code: 200
✓✓✓ SUCCESS! Location updated successfully! ✓✓✓
========================================
```

## Next Steps

Once test mode works:

1. Upload `ecoearn_bin_tracker.ino` (full GPS version)
2. Place GPS antenna with clear sky view
3. Wait 1-2 minutes for GPS fix
4. Verify real location updates in admin panel

## Quick Troubleshooting

| Problem | Solution |
|---------|----------|
| WiFi fails | Check SSID/password, use 2.4GHz network |
| HTTP 401 | Verify API key is correct |
| HTTP 400 | Check coordinate format |
| No GPS fix | Move to outdoor location with clear sky |
| Can't upload | Check board/port selection in Arduino IDE |

## API Key Security

⚠️ **Important**:
- Save API key immediately after bin creation
- Never commit API keys to public repositories
- If compromised, delete and recreate the bin

## Power Usage

- **Active**: ~120mA (ESP + GPS)
- **Battery life**: 10,000mAh = ~3.5 days
- **Recommendation**: Use mains power or solar for permanent installation

## Update Intervals

```cpp
// Fast (high power): 1 minute
const unsigned long UPDATE_INTERVAL = 60000;

// Default: 5 minutes
const unsigned long UPDATE_INTERVAL = 300000;

// Power saving: 30 minutes
const unsigned long UPDATE_INTERVAL = 1800000;
```

## Resources

- **Full setup guide**: See `SETUP_GUIDE.md`
- **API documentation**: See `API_DOCUMENTATION.md`
- **Hardware guide**: See `README.md`

## Library Installation

In Arduino IDE → Sketch → Include Library → Manage Libraries:

1. Search "TinyGPS++" → Install
2. ESP8266/ESP32 WiFi and HTTPClient are included with board support

## Board Support Installation

**ESP8266**:
- File → Preferences → Additional Board Manager URLs:
  ```
  http://arduino.esp8266.com/stable/package_esp8266com_index.json
  ```

**ESP32**:
- Additional Board Manager URLs:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```

Then: Tools → Board → Boards Manager → Search and install your board

## Common Errors

**"Board not found"**
→ Install board support (see above)

**"Port not found"**
→ Install USB drivers for your board (CH340, CP2102, etc.)

**"espcomm_sync failed"**
→ Hold BOOT/FLASH button while uploading, or check USB cable

**"Library not found"**
→ Install TinyGPS++ library (see Library Installation)

## Production Checklist

Before deploying to field:

- [ ] Test mode works successfully
- [ ] Full GPS mode acquires fix
- [ ] Location updates appear in admin panel
- [ ] Weatherproof enclosure ready
- [ ] Power supply adequate (mains/solar/battery)
- [ ] GPS antenna has clear sky view
- [ ] WiFi signal strong at installation location
- [ ] Update interval configured appropriately
- [ ] API key stored securely

## Support

For detailed troubleshooting or advanced configuration, see the full `SETUP_GUIDE.md`.

---

**Version**: 1.0.0  
**Setup Time**: ~30 minutes  
**Difficulty**: Beginner-friendly
