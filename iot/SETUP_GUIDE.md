# EcoEarn IoT Integration - Complete Setup Guide

This guide walks you through the complete process of setting up IoT GPS tracking for your EcoEarn smart bins.

## Overview

The EcoEarn IoT system consists of:
1. **Admin Panel**: Web interface for creating bins and managing API keys
2. **API Endpoint**: Server endpoint that receives GPS updates from IoT devices
3. **Arduino Code**: Firmware for ESP8266/ESP32 microcontrollers with GPS module

## Prerequisites

Before starting, ensure you have:

### Hardware
- [ ] ESP8266 (NodeMCU/Wemos D1) or ESP32 development board
- [ ] GY-GPS6MV2 GPS module
- [ ] USB cable for programming
- [ ] Breadboard and jumper wires
- [ ] 5V power supply or power bank
- [ ] (Optional) Weatherproof enclosure for outdoor deployment

### Software
- [ ] Arduino IDE installed (v1.8.19 or later)
- [ ] ESP8266 or ESP32 board support installed in Arduino IDE
- [ ] Required libraries: TinyGPS++, WiFi, HTTPClient
- [ ] Access to EcoEarn admin panel
- [ ] WiFi network credentials

### Network
- [ ] 2.4GHz WiFi network (ESP modules don't support 5GHz)
- [ ] Internet connectivity
- [ ] Server URL and SSL certificate (for production)

## Step-by-Step Setup

### Part 1: Admin Panel Setup

#### 1.1 Access Admin Panel

1. Open your browser and navigate to the EcoEarn admin panel
2. Log in with your admin credentials
3. Navigate to **Bins** section

#### 1.2 Create a New Bin

1. Click on **"Add New Bin"** or similar button
2. Fill in the bin details:
   - **Bin Name**: Give it a descriptive name (e.g., "Mobod Main Street Bin")
   - **Latitude**: Initial latitude (can be approximate, will be updated by GPS)
   - **Longitude**: Initial longitude (can be approximate)
   - **Bin Level**: Initial fill level percentage (0-100)
   - **Photo**: Upload a photo of the bin

3. Click **"Generate QR Code"** button
4. Click **"Submit"** to create the bin

#### 1.3 Save API Key

**IMPORTANT**: After submitting, you'll see an alert with the API key:

```
Bin added successfully!

API Key: BIN_LK3M9Q_H7G8J9K2L4M5N6P8Q

Please save this API key for your IoT device.
```

- **Copy this API key immediately**
- Save it in a secure location (password manager, encrypted file, etc.)
- You won't be able to see this key again!

Example API Key format:
```
BIN_[TIMESTAMP]_[RANDOM_STRING]
```

### Part 2: Hardware Assembly

#### 2.1 Connect GPS Module to ESP Board

**For ESP8266 (NodeMCU):**

| GY-GPS6MV2 | ESP8266 (NodeMCU) | Description |
|------------|-------------------|-------------|
| VCC | 3.3V or 5V | Power supply |
| GND | GND | Ground |
| TX | D2 (GPIO4) | GPS transmit to ESP receive |
| RX | D1 (GPIO5) | GPS receive to ESP transmit |

**For ESP32:**

| GY-GPS6MV2 | ESP32 | Description |
|------------|-------|-------------|
| VCC | 3.3V or 5V | Power supply |
| GND | GND | Ground |
| TX | GPIO16 (RX2) | GPS transmit to ESP receive |
| RX | GPIO17 (TX2) | GPS receive to ESP transmit |

#### 2.2 Physical Assembly Tips

1. **Power Supply**: 
   - GPS module works best with 5V but can operate at 3.3V
   - Ensure stable power supply (at least 500mA capacity)

2. **Antenna Placement**:
   - GPS antenna should face upward (toward sky)
   - Keep away from metal objects that might interfere
   - For indoor testing, place near window

3. **Wiring**:
   - Use short wires to minimize interference
   - Double-check connections before powering on
   - TX goes to RX, RX goes to TX (crossed connection)

### Part 3: Software Configuration

#### 3.1 Install Arduino IDE

1. Download Arduino IDE from: https://www.arduino.cc/en/software
2. Install for your operating system
3. Launch Arduino IDE

#### 3.2 Install Board Support

**For ESP8266:**
1. Go to **File → Preferences**
2. In "Additional Board Manager URLs", add:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Go to **Tools → Board → Boards Manager**
4. Search for "esp8266"
5. Install "esp8266 by ESP8266 Community"

**For ESP32:**
1. In "Additional Board Manager URLs", add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. Search for "esp32"
3. Install "esp32 by Espressif Systems"

#### 3.3 Install Libraries

1. Go to **Sketch → Include Library → Manage Libraries**
2. Search and install:
   - **TinyGPS++** by Mikal Hart
   - Other libraries (WiFi, HTTPClient) are included with board support

### Part 4: Upload Arduino Code

#### 4.1 Choose the Right Sketch

Two versions are available:

1. **ecoearn_bin_tracker_test.ino** - For initial testing
   - Tests WiFi and API connectivity
   - Sends fixed test coordinates
   - Use this first to verify setup

2. **ecoearn_bin_tracker.ino** - Full version with GPS
   - Reads real GPS coordinates
   - Use after test version works

#### 4.2 Configure Test Sketch (Start Here)

1. Open `ecoearn_bin_tracker_test.ino` in Arduino IDE

2. Edit the configuration section:

```cpp
// WiFi credentials
const char* WIFI_SSID = "YourWiFiName";          // Your WiFi network name
const char* WIFI_PASSWORD = "YourWiFiPassword";   // Your WiFi password

// API Configuration
const char* API_KEY = "BIN_LK3M9Q_H7G8J9K2L4M5N6P8Q";  // Your actual API key from admin panel
const char* SERVER_URL = "https://your-domain.com/api/iot/update-location";  // Your server URL

// Test GPS coordinates (use your approximate location)
const double TEST_LATITUDE = 8.476876;    // Your test latitude
const double TEST_LONGITUDE = 123.799913; // Your test longitude
```

3. **Important Configuration Notes**:
   - Replace `YourWiFiName` with your actual WiFi SSID
   - Replace `YourWiFiPassword` with your actual WiFi password
   - Use the API key copied from the admin panel
   - Update SERVER_URL to your actual server domain
   - Set test coordinates to your approximate location

#### 4.3 Select Board and Port

1. Connect your ESP board via USB
2. Go to **Tools → Board**:
   - For ESP8266: Select "NodeMCU 1.0 (ESP-12E Module)" or your specific board
   - For ESP32: Select "ESP32 Dev Module" or your specific board
3. Go to **Tools → Port**: Select the COM port where your board is connected
   - Windows: Usually COM3, COM4, etc.
   - Mac: Usually /dev/cu.usbserial-xxx
   - Linux: Usually /dev/ttyUSB0 or /dev/ttyACM0

#### 4.4 Upload the Sketch

1. Click the **Upload** button (→ arrow icon)
2. Wait for compilation and upload to complete
3. You should see "Done uploading" message

### Part 5: Testing

#### 5.1 Monitor Serial Output

1. Open Serial Monitor: **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. You should see output like:

```
=================================
EcoEarn Test Mode
=================================

Connecting to WiFi: YourWiFiName
.......
✓ WiFi connected successfully!
IP address: 192.168.1.100
Signal strength: -45 dBm

Test mode ready!
Will send test coordinates every 30 seconds
Test Location: 8.476876, 123.799913

========================================
Test Update #1
========================================

Sending location update to server...
Coordinates: 8.476876, 123.799913
Payload: {"apiKey":"BIN_XXX...","latitude":8.476876,"longitude":123.799913}
Sending HTTP POST request...

--- Server Response ---
HTTP Code: 200
Response: {"success":true,"message":"Bin location updated successfully","timestamp":"2025-11-06T12:34:56.789Z"}
----------------------

✓✓✓ SUCCESS! Location updated successfully! ✓✓✓
========================================
```

#### 5.2 Verify in Admin Panel

1. Go back to admin panel
2. Navigate to **Bins** section
3. Find your bin in the list
4. Check that the location has been updated
5. The coordinates should match your test coordinates

#### 5.3 Troubleshooting Test Mode

**Problem**: WiFi connection fails
- **Check**: SSID and password are correct
- **Check**: 2.4GHz network (not 5GHz)
- **Try**: Move closer to WiFi router
- **Check**: No special characters in password causing issues

**Problem**: HTTP code 401 (Unauthorized)
- **Check**: API key is copied correctly
- **Check**: No extra spaces in API key
- **Check**: Bin exists in database

**Problem**: HTTP code 400 (Bad Request)
- **Check**: Latitude/longitude values are valid
- **Check**: JSON format in payload is correct

**Problem**: Cannot reach server
- **Check**: SERVER_URL is correct
- **Check**: Server is online and accessible
- **Try**: Test URL in browser or Postman
- **Check**: Firewall/antivirus not blocking connection

### Part 6: Deploy Full GPS Version

Once test mode works successfully:

#### 6.1 Configure Full Sketch

1. Open `ecoearn_bin_tracker.ino`
2. Copy the same WiFi and API settings from test version
3. Adjust GPS pins if needed (default is RX=10, TX=11 for SoftwareSerial)
4. Set update interval (default is 5 minutes):

```cpp
const unsigned long UPDATE_INTERVAL = 300000;  // 5 minutes
```

#### 6.2 Upload Full Version

1. Upload `ecoearn_bin_tracker.ino` to your board
2. Open Serial Monitor (115200 baud)
3. Wait for WiFi connection

#### 6.3 Wait for GPS Fix

GPS module needs time to acquire satellite fix:

- **Cold start** (first time): 30-120 seconds
- **Warm start** (subsequent): 15-45 seconds
- Needs clear view of sky
- LED will blink while waiting
- Serial monitor shows satellite count

Expected output:
```
System ready!
Waiting for GPS fix...

Waiting for GPS fix... Satellites: 2
Waiting for GPS fix... Satellites: 4
Waiting for GPS fix... Satellites: 6
----------------------------------------
GPS Fix Acquired!
Latitude: 8.476876
Longitude: 123.799913
Satellites: 6
HDOP: 1.2
----------------------------------------

Updating location to server...
✓ Location updated successfully!
```

#### 6.4 Verify Real-Time Updates

1. Move the device to different locations
2. Wait for GPS fix at each location
3. Check admin panel to see location updates
4. Locations should update every 5 minutes (or your configured interval)

### Part 7: Production Deployment

#### 7.1 Power Considerations

**For permanent installation:**

1. **Mains Power**: Use 5V power adapter (most reliable)
2. **Power Bank**: Use high-capacity power bank (10,000mAh+ for several days)
3. **Solar Power**: Use solar panel + rechargeable battery for outdoor bins
4. **Battery Life Calculation**:
   - ESP8266: ~80mA average
   - GPS: ~40mA average
   - Total: ~120mA
   - 10,000mAh battery: ~83 hours (~3.5 days)

#### 7.2 Enclosure and Weatherproofing

1. Use weatherproof enclosure (IP65 rated or higher)
2. Mount GPS antenna outside enclosure (clear sky view)
3. Ensure ventilation for electronics
4. Protect from extreme temperatures
5. Secure all connections with waterproof connectors

#### 7.3 Installation Location

1. Mount device inside or on top of bin
2. GPS antenna must have clear view of sky
3. Avoid mounting under metal roofs or dense foliage
4. Ensure WiFi signal is strong at installation location
5. Test GPS reception before permanent installation

#### 7.4 Monitoring and Maintenance

1. **Check serial logs** periodically for errors
2. **Monitor battery levels** if using battery power
3. **Verify location updates** in admin panel
4. **Clean GPS antenna** periodically
5. **Check WiFi signal strength** (shown in serial monitor)

### Part 8: Advanced Configuration

#### 8.1 Adjust Update Frequency

Modify based on your needs:

```cpp
// Every 1 minute (high power usage)
const unsigned long UPDATE_INTERVAL = 60000;

// Every 10 minutes (balanced)
const unsigned long UPDATE_INTERVAL = 600000;

// Every 30 minutes (low power)
const unsigned long UPDATE_INTERVAL = 1800000;

// Every 1 hour (very low power)
const unsigned long UPDATE_INTERVAL = 3600000;
```

#### 8.2 Enable Deep Sleep (ESP8266)

For battery-powered applications:

```cpp
#include <ESP8266WiFi.h>

// After successful update:
ESP.deepSleep(UPDATE_INTERVAL * 1000); // Microseconds
```

**Note**: Connect GPIO16 (D0) to RST for deep sleep wake-up

#### 8.3 Use HTTPS with Certificate Validation

For production security:

```cpp
#include <WiFiClientSecure.h>

WiFiClientSecure client;
// Add your server's SSL certificate fingerprint
const char* fingerprint = "XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX";
client.setFingerprint(fingerprint);
```

## Troubleshooting Guide

### GPS Issues

**No GPS fix after 5 minutes:**
- Move device to location with clear sky view
- Check GPS antenna connection
- Verify GPS module power (green LED should be blinking)
- Check if GPS RX/TX are connected correctly (not swapped)

**GPS fix lost frequently:**
- Improve antenna placement
- Check power supply stability
- Reduce electromagnetic interference

### WiFi Issues

**Frequent disconnections:**
- Improve WiFi signal strength
- Use WiFi repeater/extender
- Check power supply (voltage drops can cause WiFi issues)

### API Issues

**Updates not appearing in admin panel:**
- Check serial monitor for HTTP response codes
- Verify API key hasn't been regenerated
- Ensure server is accessible from device location
- Check firewall rules on server

## Security Best Practices

1. **Protect API Keys**: Never commit to public repositories
2. **Use HTTPS**: Always use encrypted connections in production
3. **Regular Updates**: Keep Arduino libraries and board support updated
4. **Monitor Access**: Check server logs for unauthorized access attempts
5. **Rotate Keys**: If a device is compromised, generate new API key

## Support and Resources

### Documentation
- Full API documentation: See `API_DOCUMENTATION.md`
- Arduino code comments: See `.ino` files
- Hardware guide: See `README.md` in iot folder

### Common Resources
- Arduino IDE: https://www.arduino.cc/
- ESP8266 Documentation: https://arduino-esp8266.readthedocs.io/
- ESP32 Documentation: https://docs.espressif.com/
- TinyGPS++ Library: http://arduiniana.org/libraries/tinygpsplus/

### Need Help?
- Check troubleshooting section above
- Review serial monitor output for error messages
- Verify all configuration settings
- Test with simpler test version first

## Appendix

### A. Pin Reference

**ESP8266 (NodeMCU) Pin Mapping:**
- D0 = GPIO16
- D1 = GPIO5
- D2 = GPIO4
- D3 = GPIO0
- D4 = GPIO2
- D5 = GPIO14
- D6 = GPIO12
- D7 = GPIO13
- D8 = GPIO15

**ESP32 Default Serial Pins:**
- Serial1: TX=1, RX=3
- Serial2: TX=17, RX=16

### B. Typical Power Consumption

| Component | Mode | Current |
|-----------|------|---------|
| ESP8266 | Active WiFi | 70-80mA |
| ESP8266 | Deep Sleep | 20µA |
| ESP32 | Active WiFi | 80-160mA |
| ESP32 | Deep Sleep | 10µA |
| GPS Module | Acquiring | 40-50mA |
| GPS Module | Tracking | 30mA |

### C. Recommended Update Intervals

| Use Case | Interval | Reason |
|----------|----------|--------|
| Mobile Bins | 5-15 min | Track movement |
| Stationary Bins | 30-60 min | Save power |
| Critical Monitoring | 1-5 min | Real-time updates |
| Battery Operation | 30+ min | Extend battery life |

## Conclusion

You now have a complete IoT GPS tracking system for your EcoEarn smart bins! The system will automatically update bin locations in real-time, enabling better bin management and route optimization for waste collection.

For questions or issues, refer to the troubleshooting section or contact support.

---

**Version**: 1.0.0  
**Last Updated**: November 6, 2025  
**Author**: EcoEarn Development Team
