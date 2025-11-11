# EcoEarn IoT GPS Tracking - Quick Reference

## 🎯 What Was Implemented

An IoT GPS tracking system that automatically updates bin locations using ESP8266/ESP32 microcontrollers and GPS modules.

## 📁 Files Changed/Created

### Backend Changes
- ✅ `lib/admin-service.ts` - Added API key generation and IoT methods
- ✅ `app/api/iot/update-location/route.ts` - New API endpoint for IoT devices

### Frontend Changes
- ✅ `components/bins/BinForm.tsx` - Shows generated API key to admin

### IoT Files (New Directory: `/iot`)
- ✅ `ecoearn_bin_tracker.ino` - Full GPS tracking code
- ✅ `ecoearn_bin_tracker_test.ino` - Test version without GPS
- ✅ `README.md` - Hardware and software guide
- ✅ `API_DOCUMENTATION.md` - API reference
- ✅ `SETUP_GUIDE.md` - Complete step-by-step guide
- ✅ `QUICKSTART.md` - Quick 5-minute setup
- ✅ `IMPLEMENTATION_SUMMARY.md` - Technical summary

## 🚀 Quick Start

### For Admins
1. Create a new bin in admin panel
2. Copy the generated API key (shown once!)
3. Configure Arduino code with API key
4. Deploy IoT device

### For Developers
1. Navigate to `/iot` folder
2. Read `QUICKSTART.md` for fast setup
3. Upload test sketch first to verify connectivity
4. Then upload full GPS version

## 🔑 API Key System

**When creating a bin:**
- API key is automatically generated (format: `BIN_XXXXXXXXX_XXXXXX`)
- Shown once in alert message after bin creation
- **Must be saved immediately** - cannot be retrieved later

**Using the API key:**
```cpp
// In Arduino code
const char* API_KEY = "BIN_LK3M9Q_H7G8J9K2L4M5N6P8Q";
```

## 🌐 API Endpoint

**URL:** `POST /api/iot/update-location`

**Request:**
```json
{
  "apiKey": "BIN_XXXXXXXXX_XXXXXX",
  "latitude": 8.476876,
  "longitude": 123.799913
}
```

**Response:**
```json
{
  "success": true,
  "message": "Bin location updated successfully",
  "timestamp": "2025-11-06T12:34:56.789Z"
}
```

## 🛠️ Hardware Needed

1. **ESP8266** (NodeMCU, Wemos D1) or **ESP32**
2. **GY-GPS6MV2** GPS module
3. USB cable for programming
4. WiFi network (2.4GHz)

**Basic Wiring:**
```
GPS Module → ESP8266
VCC → 3.3V/5V
GND → GND
TX → D2 (GPIO4)
RX → D1 (GPIO5)
```

## 📚 Documentation

All documentation is in the `/iot` folder:

| File | Purpose |
|------|---------|
| `QUICKSTART.md` | 5-minute setup guide |
| `SETUP_GUIDE.md` | Complete step-by-step instructions |
| `README.md` | Hardware/software requirements |
| `API_DOCUMENTATION.md` | API reference and examples |
| `IMPLEMENTATION_SUMMARY.md` | Technical implementation details |

## 🧪 Testing

### Step 1: Test Mode (No GPS Required)
```bash
1. Upload ecoearn_bin_tracker_test.ino
2. Open Serial Monitor (115200 baud)
3. Verify: WiFi connects, API key works, location updates
4. Check admin panel for updates
```

### Step 2: Full GPS Mode
```bash
1. Upload ecoearn_bin_tracker.ino
2. Place GPS antenna with clear sky view
3. Wait for GPS fix (30-120 seconds)
4. Verify real location updates
```

## ⚙️ Configuration

In Arduino code, edit these values:

```cpp
// WiFi Settings
const char* WIFI_SSID = "YourWiFiName";
const char* WIFI_PASSWORD = "YourPassword";

// API Settings
const char* API_KEY = "BIN_YOUR_KEY_HERE";
const char* SERVER_URL = "https://your-domain.com/api/iot/update-location";

// Update Frequency (milliseconds)
const unsigned long UPDATE_INTERVAL = 300000;  // 5 minutes
```

## 🔧 Troubleshooting

| Problem | Quick Fix |
|---------|-----------|
| WiFi fails | Check SSID/password, ensure 2.4GHz |
| HTTP 401 | Verify API key is correct |
| No GPS fix | Move to outdoor location |
| Can't upload | Check board/port in Arduino IDE |

See `SETUP_GUIDE.md` for detailed troubleshooting.

## 📊 Power Consumption

- **Active Operation:** ~120mA (ESP + GPS)
- **10,000mAh Battery:** ~3.5 days runtime
- **Recommended:** Mains power or solar panel for permanent installation

## 🔒 Security

- ✅ Unique API key per bin
- ✅ Server-side validation
- ✅ HTTPS support (configure for production)
- ⚠️ Never commit API keys to public repos
- ⚠️ Store API keys securely

## 📈 Features

- ✅ Automatic GPS location tracking
- ✅ WiFi connectivity with auto-reconnect
- ✅ Configurable update intervals
- ✅ LED status indicators
- ✅ Comprehensive error handling
- ✅ Serial debugging output
- ✅ Power-efficient operation

## 🎓 For More Help

1. **Quick Setup:** Read `/iot/QUICKSTART.md`
2. **Full Guide:** Read `/iot/SETUP_GUIDE.md`
3. **API Details:** Read `/iot/API_DOCUMENTATION.md`
4. **Hardware Info:** Read `/iot/README.md`

## 🚀 Next Steps

1. Test in development environment
2. Deploy test IoT device
3. Monitor updates in admin panel
4. Scale to production

---

**Version:** 1.0.0  
**Last Updated:** November 6, 2025  
**Status:** Ready for Testing ✅
