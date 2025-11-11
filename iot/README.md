# EcoEarn IoT Smart Bin System

This Arduino project creates a complete IoT smart trash bin system with GPS tracking, automatic bin opening, and capacity monitoring.

## Features

### Core Functionality
- 🌍 **GPS Location Tracking** - Automatic location updates using GY-GPS6MV2 module
- 👋 **Hand Detection** - Ultrasonic sensor detects approaching users
- 🚪 **Automatic Bin Opening** - Servo motor opens bin when hand is detected
- 📏 **Capacity Monitoring** - Two ultrasonic sensors track compartment fill levels
- 🎛️ **Compartment Control** - Servo motors manage access to separate compartments
- 📡 **WiFi Connectivity** - ESP8266/NodeMCU for internet communication
- 🔐 **Secure API Authentication** - API key-based security
- ⏱️ **Configurable Intervals** - Adjustable update frequencies
- 💾 **Real-time Data** - Live capacity and location updates
- � **Low Power Design** - Efficient power management

### Smart Features
- **Auto-close Mechanism** - Bin closes automatically after 5 seconds
- **Intelligent Compartment Selection** - Opens less-full compartment first
- **Real-time Capacity Alerts** - Server notifications when bins are full
- **User Activity Tracking** - Logs bin usage and maintenance needs

## Hardware Requirements

### Required Components

1. **Microcontroller** (Choose one):
   - **ESP8266 (NodeMCU v1.0)** - ⭐ Recommended for cost and ease of use
   - ESP8266 (Wemos D1 Mini, etc.)
   - ESP32 (DevKit, WROOM, etc.) - Better performance but more expensive

2. **GPS Module** (Optional for testing):
   - GY-GPS6MV2 GPS Module (NEO-6M based)

3. **Ultrasonic Sensors** (HC-SR04):
   - 1x Hand detection sensor
   - 2x Capacity monitoring sensors (one per compartment)

4. **Servo Motors**:
   - 1x Bin opening servo (MG90S or SG90)
   - 2x Compartment control servos (optional)

5. **Additional Components**:
   - USB cable for programming
   - Power supply (5V, 1A minimum recommended)
   - Breadboard and jumper wires (for prototyping)
   - Plastic bin with separate compartments

### Hardware Connections

#### For ESP8266 (NodeMCU) ⭐ Recommended
```
GY-GPS6MV2    ->    NodeMCU ESP8266
VCC           ->    3.3V (recommended) or VIN (5V)
GND           ->    GND
TX            ->    D2 (GPIO4) - RX
RX            ->    D1 (GPIO5) - TX
```

**📝 Note**: The code is pre-configured for NodeMCU pins (D1 and D2).

**📖 Detailed wiring guide**: See `NODEMCU_WIRING.md` for step-by-step instructions with diagrams.

#### For ESP8266 (Other boards like Wemos D1)
```
GY-GPS6MV2    ->    ESP8266
VCC           ->    3.3V or 5V
GND           ->    GND
TX            ->    D2 (GPIO4) - RX
RX            ->    D1 (GPIO5) - TX
```

#### For ESP32
```
GY-GPS6MV2    ->    ESP32
VCC           ->    3.3V or 5V
GND           ->    GND
TX            ->    GPIO16 (RX2)
RX            ->    GPIO17 (TX2)
```

## Software Requirements

### Arduino IDE Setup

1. **Install Arduino IDE**
   - Download from: https://www.arduino.cc/en/software
   - Version 1.8.19 or later recommended

2. **Install Board Support**

   For ESP8266:
   - Go to File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     http://arduino.esp8266.com/stable/package_esp8266com_index.json
     ```
   - Go to Tools → Board → Boards Manager
   - Search for "esp8266" and install

   For ESP32:
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Search for "esp32" and install

3. **Install Required Libraries**

   Go to Sketch → Include Library → Manage Libraries, then install:
   
   - **TinyGPS++** by Mikal Hart
     - For parsing GPS NMEA data
   
   - **ESP8266WiFi** (for ESP8266) or **WiFi** (for ESP32)
     - Usually included with board support
   
   - **ESP8266HTTPClient** (for ESP8266) or **HTTPClient** (for ESP32)
     - Usually included with board support

## Configuration

### 1. Get Your Bin API Key

1. Log in to the EcoEarn admin panel
2. Navigate to Bins → Add New Bin
3. Fill in the bin details and submit
4. Copy the generated API key (format: `BIN_XXXXXXXXX_XXXXXX`)
5. **Important**: Save this key securely - it won't be shown again!

### 2. Configure the Arduino Sketch

Open `ecoearn_bin_tracker.ino` and update the following section:

```cpp
// ============================================
// CONFIGURATION - EDIT THESE VALUES
// ============================================

// WiFi credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID";           // Your WiFi network name
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";   // Your WiFi password

// API Configuration
const char* API_KEY = "YOUR_BIN_API_KEY_HERE";      // Your bin's API key from admin panel
const char* SERVER_URL = "https://your-domain.com/api/iot/update-location";  // Your server URL

// Update interval (in milliseconds) - 5 minutes by default
const unsigned long UPDATE_INTERVAL = 300000;  // 5 minutes = 300000ms
```

### 3. Adjust GPS Pins (if needed)

If using different pins for GPS connection:

```cpp
// GPS Serial pins (for SoftwareSerial)
const int GPS_RX_PIN = 10;  // Connect to GPS TX
const int GPS_TX_PIN = 11;  // Connect to GPS RX
```

For ESP32 using Hardware Serial 2:
```cpp
// In setup(), replace:
gpsSerial.begin(9600);
// With:
Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
```

## Upload and Testing

### 1. Upload the Sketch

1. Connect your ESP board via USB
2. Select the correct board:
   - Tools → Board → ESP8266 Boards → NodeMCU 1.0
   - OR Tools → Board → ESP32 Arduino → ESP32 Dev Module
3. Select the correct port: Tools → Port → (your COM port)
4. Click Upload button (→)

### 2. Monitor Serial Output

1. Open Serial Monitor: Tools → Serial Monitor
2. Set baud rate to **115200**
3. You should see:
   ```
   =================================
   EcoEarn Bin Location Tracker
   =================================
   
   GPS Module initializing...
   Connecting to WiFi: YourNetwork
   ...
   WiFi connected!
   IP address: 192.168.1.xxx
   
   System ready!
   Waiting for GPS fix...
   ```

### 3. GPS Fix Acquisition

- The GPS module needs a clear view of the sky
- Initial GPS fix can take 30-120 seconds (cold start)
- Subsequent fixes are faster (hot start)
- LED will blink while waiting for GPS fix
- When GPS fix is acquired, location will be sent to server

### 4. Verify Updates

Check the admin panel to see if bin location is being updated:
1. Log in to admin panel
2. Go to Bins → View Bins
3. Check if the bin's location is updating on the map

## LED Status Indicators

- **Slow Blink (1 sec)**: Waiting for GPS fix
- **3 Quick Blinks**: WiFi connected successfully
- **5 Rapid Blinks**: Location updated successfully to server

## Troubleshooting

### GPS Module Issues

**Problem**: No GPS fix
- **Solution**: 
  - Ensure GPS module has clear view of sky
  - Check GPS antenna connection
  - Verify GPS RX/TX connections
  - Wait longer (up to 2 minutes for cold start)

**Problem**: GPS data garbled
- **Solution**:
  - Check baud rate (should be 9600)
  - Verify TX/RX are not swapped
  - Check power supply (GPS needs stable 3.3V or 5V)

### WiFi Issues

**Problem**: Cannot connect to WiFi
- **Solution**:
  - Verify SSID and password
  - Check WiFi signal strength
  - Ensure 2.4GHz network (ESP8266/ESP32 don't support 5GHz)
  - Try moving closer to router

**Problem**: WiFi keeps disconnecting
- **Solution**:
  - Check power supply (minimum 500mA)
  - Update WiFi credentials
  - Check router settings

### API Issues

**Problem**: Server returns 401 error
- **Solution**:
  - Verify API key is correct
  - Check that bin exists in database
  - Ensure API key hasn't been regenerated

**Problem**: Server returns 400 error
- **Solution**:
  - Check latitude/longitude values in serial monitor
  - Verify GPS data is valid

**Problem**: Cannot reach server
- **Solution**:
  - Verify SERVER_URL is correct
  - Check if server is online
  - Test URL in browser or Postman
  - Ensure server has SSL certificate (for HTTPS)

## Power Consumption

### Typical Power Usage
- ESP8266: ~70-80mA (active) / ~20mA (sleep)
- ESP32: ~80-160mA (active) / ~10mA (sleep)
- GPS Module: ~40-50mA (acquiring fix) / ~30mA (tracking)

### Battery Operation
For battery-powered operation, consider:
1. Using deep sleep between updates
2. Solar panel + rechargeable battery
3. Power bank with auto-on feature
4. Increasing update interval to reduce power consumption

## Advanced Configuration

### Change Update Interval

To update location every 10 minutes instead of 5:
```cpp
const unsigned long UPDATE_INTERVAL = 600000;  // 10 minutes
```

Recommended intervals:
- Every 1 minute: `60000` (high power usage)
- Every 5 minutes: `300000` (default)
- Every 15 minutes: `900000` (balanced)
- Every 30 minutes: `1800000` (low power)
- Every 1 hour: `3600000` (very low power)

### HTTPS Configuration

For secure connections, use SSL/TLS:

```cpp
#include <WiFiClientSecure.h>

WiFiClientSecure client;
client.setInsecure(); // For testing only
// OR add certificate fingerprint for production
```

### Add Deep Sleep (ESP8266)

```cpp
#include <ESP8266WiFi.h>

// After successful update:
ESP.deepSleep(UPDATE_INTERVAL * 1000); // Convert to microseconds
```

## API Reference

### Endpoint: POST `/api/iot/update-location`

**Request Body** (JSON):
```json
{
  "apiKey": "BIN_XXXXXXXXX_XXXXXX",
  "latitude": 8.476876,
  "longitude": 123.799913
}
```

**Success Response** (200):
```json
{
  "success": true,
  "message": "Bin location updated successfully",
  "timestamp": "2025-11-06T12:34:56.789Z"
}
```

**Error Responses**:
- `400`: Invalid parameters
- `401`: Invalid API key
- `500`: Server error

## Security Best Practices

1. **Protect API Keys**:
   - Never commit API keys to public repositories
   - Use environment variables for production
   - Rotate keys if compromised

2. **Use HTTPS**:
   - Always use HTTPS in production
   - Validate SSL certificates

3. **Limit Update Frequency**:
   - Don't overwhelm the server with too frequent updates
   - Implement exponential backoff on errors

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Review serial monitor output for error messages
3. Contact the EcoEarn development team

## License

Copyright © 2025 EcoEarn. All rights reserved.
