# ESP32-CAM to ESP32 Communication Pinout

## Overview
This document describes the serial communication connection between the ESP32-CAM and the ESP32 38-pin development board for the EcoEarn Smart Bin system.

---

## Hardware Components
- **ESP32-CAM (AI Thinker)** - Handles image capture and material classification
- **ESP32 38-pin Development Board** - Main controller for sensors and servos

---

## Communication Method
**UART Serial Communication** (Hardware Serial)

- **Baud Rate**: 9600
- **Protocol**: 8N1 (8 data bits, No parity, 1 stop bit)
- **Connection**: Cross-connected TX/RX

---

## Pin Connections

### ESP32-CAM Side (AI Thinker)
```
ESP32-CAM Pin    →    Function
────────────────────────────────
GPIO 3 (U0RXD)   →    RX (Receive from ESP32)
GPIO 1 (U0TXD)   →    TX (Transmit to ESP32)
GND              →    GND (Common Ground)
5V               →    Power (separate supply recommended)
```

**Note**: GPIO 16 and 17 are NOT available on ESP32-CAM AI Thinker!
- GPIO 1 (TX0) and GPIO 3 (RX0) are the programming/Serial0 pins
- Alternative available pins: GPIO 12, 13, 14, 15 (if not used by SD card)

### ESP32 38-Pin Side
```
ESP32 Pin        →    Function
────────────────────────────────
GPIO 16 (RX2)    →    RX (Receive from ESP32-CAM)
GPIO 17 (TX2)    →    TX (Transmit to ESP32-CAM)
GND              →    GND (Common Ground)
5V               →    Power Out (if powering ESP32-CAM)
```

---

## Wiring Diagram

### Option A: Using ESP32-CAM Serial0 (GPIO 1/3)
```
┌─────────────────┐                    ┌─────────────────┐
│   ESP32-CAM     │                    │   ESP32 38-Pin  │
│  (AI Thinker)   │                    │      Board      │
├─────────────────┤                    ├─────────────────┤
│                 │                    │                 │
│  GPIO 3 (RX0) ──┼────────────────────┼──→ GPIO 18 (TX) │
│                 │                    │                 │
│  GPIO 1 (TX0) ──┼────────────────────┼──→ GPIO 19 (RX) │
│                 │                    │                 │
│  GND          ──┼────────────────────┼──→ GND          │
│                 │                    │                 │
│  5V           ──┼───→ 5V Power Supply│                 │
│                 │                    │                 │
└─────────────────┘                    └─────────────────┘
```

### Option B: Using ESP32-CAM Alternative Pins (GPIO 12/13)
```
┌─────────────────┐                    ┌─────────────────┐
│   ESP32-CAM     │                    │   ESP32 38-Pin  │
│  (AI Thinker)   │                    │      Board      │
├─────────────────┤                    ├─────────────────┤
│                 │                    │                 │
│  GPIO 12 (RX) ──┼────────────────────┼──→ GPIO 18 (TX) │
│                 │                    │                 │
│  GPIO 13 (TX) ──┼────────────────────┼──→ GPIO 19 (RX) │
│                 │                    │                 │
│  GND          ──┼────────────────────┼──→ GND          │
│                 │                    │                 │
│  5V           ──┼───→ 5V Power Supply│                 │
│                 │                    │                 │
└─────────────────┘                    └─────────────────┘
```

**Note**: TX connects to RX, and RX connects to TX (cross-connection)

---

## Current Code Configuration (NEEDS FIXING!)

### ESP32-CAM (`esp32_cam_controller.ino`) - ❌ INCORRECT
```cpp
// ❌ WRONG: GPIO 16/17 don't exist on ESP32-CAM AI Thinker!
#define NODEMCU_SERIAL Serial2  // Hardware Serial 2
#define NODEMCU_RX 16           // GPIO16 NOT AVAILABLE!
#define NODEMCU_TX 17           // GPIO17 NOT AVAILABLE!

// In setup():
NODEMCU_SERIAL.begin(9600, SERIAL_8N1, NODEMCU_RX, NODEMCU_TX);
```

### ESP32-CAM - ✅ CORRECTED Version (Use Serial0)
```cpp
// ✅ Using Serial0 (GPIO 1/3) - These are the programming pins
#define NODEMCU_SERIAL Serial   // Use Serial0 (USB programming port)

// In setup():
NODEMCU_SERIAL.begin(9600);
// Note: You won't be able to use Serial Monitor while connected to ESP32
```

### ESP32-CAM - ✅ ALTERNATIVE (SoftwareSerial with GPIO 12/13)
```cpp
#include <SoftwareSerial.h>

// Using available GPIO pins
#define NODEMCU_RX 12           // GPIO12 for RX from ESP32
#define NODEMCU_TX 13           // GPIO13 for TX to ESP32
SoftwareSerial NODEMCU_SERIAL(NODEMCU_RX, NODEMCU_TX);

// In setup():
NODEMCU_SERIAL.begin(9600);
```

### ESP32 Main Controller (`ecoearn_bin_tracker.ino`)
```cpp
// Serial Communication with ESP32-CAM
#define ESP32_CAM_SERIAL Serial2  // Hardware Serial 2
#define ESP32_BAUD 9600

// Pin Configuration
const int GPS_RX_PIN = 16;  // GPIO16 (RX2) - CONFLICT!
const int GPS_TX_PIN = 17;  // GPIO17 (TX2) - CONFLICT!

// In setup():
ESP32_CAM_SERIAL.begin(ESP32_BAUD);
```

---

## ⚠️ CRITICAL ISSUES DETECTED!

### Problem 1: ESP32-CAM GPIO 16/17 Don't Exist!
The ESP32-CAM AI Thinker module **does NOT have GPIO 16 and 17 broken out**.

**Available GPIO pins on ESP32-CAM AI Thinker:**
- GPIO 0 (Camera XCLK - avoid)
- GPIO 1 (TX0 - Serial programming)
- GPIO 2 (Flash LED - available)
- GPIO 3 (RX0 - Serial programming)
- GPIO 4 (Flash LED - available)
- GPIO 12, 13, 14, 15 (SD Card - can be used if SD not needed)
- Camera pins (occupied)

### Problem 2: ESP32 Main Board Pin Conflict
GPS module is using GPIO 16/17 on the ESP32 main board.

**Both issues must be fixed!**

---

## 🔧 Solution Options

### **Option 1: ESP32-CAM Uses Serial0, ESP32 Uses SoftwareSerial** (Easiest)
ESP32-CAM uses its built-in Serial0 (GPIO 1/3), ESP32 uses any available pins.

**ESP32-CAM Changes:**
```cpp
// Use Serial0 (built-in, no library needed)
#define NODEMCU_SERIAL Serial  // GPIO 1 (TX), GPIO 3 (RX)

void setup() {
  // Don't use Serial for debugging - it's for ESP32 communication
  NODEMCU_SERIAL.begin(9600);
}
```

**ESP32 Main Controller Changes:**
```cpp
#include <SoftwareSerial.h>

// Keep GPS on Hardware Serial2
const int GPS_RX_PIN = 16;   // GPIO16 (RX2) - Hardware Serial
const int GPS_TX_PIN = 17;   // GPIO17 (TX2) - Hardware Serial

// ESP32-CAM on SoftwareSerial
const int CAM_RX_PIN = 19;   // GPIO19 - Receive from ESP32-CAM
const int CAM_TX_PIN = 18;   // GPIO18 - Transmit to ESP32-CAM

SoftwareSerial ESP32_CAM_SERIAL(CAM_RX_PIN, CAM_TX_PIN);

void setup() {
  // GPS on Hardware Serial2
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  // ESP32-CAM on SoftwareSerial
  ESP32_CAM_SERIAL.begin(9600);
}
```

**Wiring:**
```
ESP32-CAM          →    ESP32 38-Pin
─────────────────────────────────────
GPIO 3 (RX0)       →    GPIO 18 (TX)
GPIO 1 (TX0)       →    GPIO 19 (RX)
GND                →    GND
```

**⚠️ Limitation**: You can't use USB Serial Monitor on ESP32-CAM while connected!

---

### **Option 2: Both Use SoftwareSerial (Recommended)**
Use available GPIO pins on both devices.

**ESP32-CAM Changes:**
```cpp
#include <SoftwareSerial.h>

// Using GPIO 12/13 (not needed if SD card not used)
const int ESP32_RX_PIN = 12;  // GPIO12 - Receive from ESP32
const int ESP32_TX_PIN = 13;  // GPIO13 - Transmit to ESP32
SoftwareSerial NODEMCU_SERIAL(ESP32_RX_PIN, ESP32_TX_PIN);

void setup() {
  Serial.begin(115200);  // Can still use Serial Monitor!
  NODEMCU_SERIAL.begin(9600);
}
```

**ESP32 Main Controller Changes:**
```cpp
#include <SoftwareSerial.h>

// Keep GPS on Hardware Serial2 (more reliable)
const int GPS_RX_PIN = 16;   // GPIO16 (RX2)
const int GPS_TX_PIN = 17;   // GPIO17 (TX2)

// ESP32-CAM on SoftwareSerial
const int CAM_RX_PIN = 19;   // GPIO19 - Receive from ESP32-CAM
const int CAM_TX_PIN = 18;   // GPIO18 - Transmit to ESP32-CAM

SoftwareSerial ESP32_CAM_SERIAL(CAM_RX_PIN, CAM_TX_PIN);

void setup() {
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  ESP32_CAM_SERIAL.begin(9600);
}
```

**Wiring:**
```
ESP32-CAM          →    ESP32 38-Pin
─────────────────────────────────────
GPIO 12 (RX)       →    GPIO 18 (TX)
GPIO 13 (TX)       →    GPIO 19 (RX)
GND                →    GND
```

**✅ Advantages**: 
- Can use Serial Monitor on both devices
- GPS keeps hardware serial (more reliable)
- No pin conflicts

---

### **Option 3: Use Two Different Hardware Serials**
ESP32 has 3 hardware serial ports: Serial0, Serial1, Serial2

**ESP32 Main Controller Changes:**
```cpp
// GPS on Hardware Serial2
HardwareSerial gpsSerial(2);
const int GPS_RX_PIN = 16;   // GPIO16 (RX2)
const int GPS_TX_PIN = 17;   // GPIO17 (TX2)

// ESP32-CAM on Hardware Serial1
HardwareSerial camSerial(1);
const int CAM_RX_PIN = 9;    // GPIO9 (RX1)
const int CAM_TX_PIN = 10;   // GPIO10 (TX1)

void setup() {
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  camSerial.begin(9600, SERIAL_8N1, CAM_RX_PIN, CAM_TX_PIN);
}
```

⚠️ **Warning**: GPIO 9 and 10 are connected to internal flash on some ESP32 boards and may not be available.

---

## 📋 Communication Protocol

### Commands from ESP32-CAM to ESP32
```cpp
"OPEN_PLASTIC"              // Open plastic compartment
"OPEN_TIN"                  // Open tin compartment
"OPEN_REJECTED"             // Open rejected compartment
"CLOSE_ALL"                 // Close all compartments
"STATUS"                    // Request system status
"CHECK_USER"                // Request user presence check
```

### Commands from ESP32 to ESP32-CAM
```cpp
"ACTIVATE_BIN:userId:sessionId"    // Activate bin with user info
"DEACTIVATE_BIN"                   // Deactivate bin
"USER_PRESENT"                     // Response to CHECK_USER
"USER_ABSENT"                      // Response to CHECK_USER
```

---

## 💡 Recommended Configuration

**Best Practice**: Use **Option 2** (Both use SoftwareSerial)

### Why?
1. ✅ GPS keeps hardware serial (more reliable timing-critical data)
2. ✅ ESP32-CAM can still use Serial Monitor for debugging
3. ✅ GPIO 12/13 on ESP32-CAM are readily available
4. ✅ GPIO 18/19 on ESP32 are safe and available
5. ✅ Clean separation of communication channels

### Final Pin Assignment
```
Device          ESP32-CAM Pin    ESP32 Pin       Protocol
────────────────────────────────────────────────────────────────
GPS Module      N/A              GPIO 16 (RX2)   Hardware Serial
GPS Module      N/A              GPIO 17 (TX2)   Hardware Serial
ESP32 Comms     GPIO 12 (RX)     GPIO 18 (TX)    SoftwareSerial
ESP32 Comms     GPIO 13 (TX)     GPIO 19 (RX)    SoftwareSerial
Debug           GPIO 3/1 (USB)   GPIO 3/1 (USB)  Serial Monitor
```

---

## 🔌 Power Considerations

### ESP32-CAM Power Requirements
- **Voltage**: 5V recommended (can work with 3.3V but camera may not work properly)
- **Current**: 200-300mA normal, up to 500mA during WiFi transmission
- **Recommendation**: Use separate 5V power supply with common ground

### Power Connections
```
Option A: Separate Power Supply (Recommended)
┌──────────────┐
│ 5V Adapter   │──→ ESP32-CAM (5V)
│              │
└──────────────┘
       │
       └──────────→ GND (Common with ESP32)

Option B: Powered by ESP32 (if available)
ESP32 5V Pin ──→ ESP32-CAM 5V
ESP32 GND    ──→ ESP32-CAM GND
```

---

## 🧪 Testing the Connection

### 1. Basic Ping-Pong Test

#### Test on ESP32-CAM Side
```cpp
void loop() {
  // Check for incoming data from ESP32
  if (NODEMCU_SERIAL.available()) {
    String received = NODEMCU_SERIAL.readStringUntil('\n');
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("📥 RX FROM ESP32: " + received);
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    // Echo back
    NODEMCU_SERIAL.println("ACK: " + received);
  }
  
  // Send periodic ping
  NODEMCU_SERIAL.println("PING_FROM_CAM");
  Serial.println("📤 TX TO ESP32: PING_FROM_CAM");
  delay(1000);
}
```

**ESP32-CAM Serial Monitor Output:**
```
📤 TX TO ESP32: PING_FROM_CAM
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📥 RX FROM ESP32: PONG_FROM_MAIN
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📤 TX TO ESP32: ACK: PONG_FROM_MAIN
```

#### Test on ESP32 Side
```cpp
void loop() {
  // Check for incoming data from ESP32-CAM
  if (ESP32_CAM_SERIAL.available()) {
    String received = ESP32_CAM_SERIAL.readStringUntil('\n');
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("📥 RX FROM CAM: " + received);
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  }
  
  // Send periodic pong
  ESP32_CAM_SERIAL.println("PONG_FROM_MAIN");
  Serial.println("📤 TX TO CAM: PONG_FROM_MAIN");
  delay(1000);
}
```

**ESP32 Main Serial Monitor Output:**
```
📤 TX TO CAM: PONG_FROM_MAIN
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📥 RX FROM CAM: PING_FROM_CAM
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📥 RX FROM CAM: ACK: PONG_FROM_MAIN
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 2. Production Communication Flow with Debug Output

#### ESP32-CAM Production Code with Serial Output
```cpp
void loop() {
  server.handleClient();
  
  // Listen for commands from ESP32 Main Controller
  if (NODEMCU_SERIAL.available()) {
    String command = NODEMCU_SERIAL.readStringUntil('\n');
    command.trim();
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   📥 COMMAND RECEIVED FROM ESP32      ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.print("Command: ");
    Serial.println(command);
    Serial.print("Time: ");
    Serial.println(millis());
    
    // Process commands
    if (command.startsWith("ACTIVATE_BIN:")) {
      // Parse: ACTIVATE_BIN:userId:sessionId
      int firstColon = command.indexOf(':');
      int secondColon = command.indexOf(':', firstColon + 1);
      
      String userId = command.substring(firstColon + 1, secondColon);
      String sessionId = command.substring(secondColon + 1);
      
      Serial.println("\n🔓 BIN ACTIVATION REQUEST");
      Serial.println("├─ User ID: " + userId);
      Serial.println("└─ Session: " + sessionId);
      
      // Send acknowledgment
      NODEMCU_SERIAL.println("BIN_ACTIVATED");
      Serial.println("✅ Sent: BIN_ACTIVATED\n");
      
    } else if (command == "CHECK_USER") {
      Serial.println("\n👤 USER PRESENCE CHECK REQUEST");
      
      // Request user detection from ESP32
      NODEMCU_SERIAL.println("CHECK_USER");
      Serial.println("📤 Sent: CHECK_USER");
      Serial.println("⏳ Waiting for response...\n");
      
    } else if (command == "USER_PRESENT") {
      Serial.println("\n✅ USER DETECTED BY ESP32");
      Serial.println("Action: Ready to capture image\n");
      
    } else if (command == "USER_ABSENT") {
      Serial.println("\n❌ NO USER DETECTED BY ESP32");
      Serial.println("Action: Standby mode\n");
      
    } else if (command == "DEACTIVATE_BIN") {
      Serial.println("\n🔒 BIN DEACTIVATION REQUEST");
      NODEMCU_SERIAL.println("BIN_DEACTIVATED");
      Serial.println("✅ Sent: BIN_DEACTIVATED\n");
      
    } else if (command == "STATUS") {
      Serial.println("\n📊 STATUS REQUEST");
      String status = "READY";
      NODEMCU_SERIAL.println("STATUS:" + status);
      Serial.println("✅ Sent: STATUS:" + status + "\n");
      
    } else {
      Serial.println("\n⚠️  UNKNOWN COMMAND");
      Serial.println("Command ignored: " + command + "\n");
    }
    
    Serial.println("────────────────────────────────────────\n");
  }
  
  delay(10);
}

// When material is identified and compartment command is sent
void sendCompartmentCommand(String material) {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   📤 SENDING COMPARTMENT COMMAND      ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  String command = "";
  if (material == "plastic" || material == "Plastic") {
    command = "OPEN_PLASTIC";
    Serial.println("Material: PLASTIC 🥤");
  } else if (material == "metal" || material == "Metal" || material == "tin") {
    command = "OPEN_TIN";
    Serial.println("Material: METAL/TIN 🥫");
  } else {
    command = "OPEN_REJECTED";
    Serial.println("Material: REJECTED ❌");
  }
  
  NODEMCU_SERIAL.println(command);
  Serial.print("📡 Sent to ESP32: ");
  Serial.println(command);
  Serial.print("Time: ");
  Serial.println(millis());
  Serial.println("────────────────────────────────────────\n");
}
```

**ESP32-CAM Serial Monitor Output Example:**
```
╔════════════════════════════════════════╗
║   📥 COMMAND RECEIVED FROM ESP32      ║
╚════════════════════════════════════════╝
Command: ACTIVATE_BIN:user123:sess456
Time: 45230

🔓 BIN ACTIVATION REQUEST
├─ User ID: user123
└─ Session: sess456
✅ Sent: BIN_ACTIVATED

────────────────────────────────────────

╔════════════════════════════════════════╗
║   📥 COMMAND RECEIVED FROM ESP32      ║
╚════════════════════════════════════════╝
Command: CHECK_USER
Time: 46100

👤 USER PRESENCE CHECK REQUEST
📤 Sent: CHECK_USER
⏳ Waiting for response...

────────────────────────────────────────

╔════════════════════════════════════════╗
║   📥 COMMAND RECEIVED FROM ESP32      ║
╚════════════════════════════════════════╝
Command: USER_PRESENT
Time: 46550

✅ USER DETECTED BY ESP32
Action: Ready to capture image

────────────────────────────────────────

[Material identification happens...]

╔════════════════════════════════════════╗
║   📤 SENDING COMPARTMENT COMMAND      ║
╚════════════════════════════════════════╝
Material: PLASTIC 🥤
📡 Sent to ESP32: OPEN_PLASTIC
Time: 48200
────────────────────────────────────────
```

#### ESP32 Main Controller Production Code with Serial Output
```cpp
void loop() {
  // Listen for commands from ESP32-CAM
  if (ESP32_CAM_SERIAL.available()) {
    String command = ESP32_CAM_SERIAL.readStringUntil('\n');
    command.trim();
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   📥 COMMAND FROM ESP32-CAM           ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.print("Command: ");
    Serial.println(command);
    Serial.print("Time: ");
    Serial.println(millis());
    
    if (command == "OPEN_PLASTIC") {
      Serial.println("\n🥤 OPENING PLASTIC COMPARTMENT");
      Serial.println("├─ Rotating platform to PLASTIC position");
      Serial.println("├─ Opening bin lid");
      Serial.println("└─ Activating dropper");
      
      rotatePlatform(ROTATE_PLASTIC);
      openBinLid();
      activateDropper();
      
      Serial.println("✅ Plastic compartment opened\n");
      
    } else if (command == "OPEN_TIN") {
      Serial.println("\n🥫 OPENING TIN/METAL COMPARTMENT");
      Serial.println("├─ Rotating platform to TIN position");
      Serial.println("├─ Opening bin lid");
      Serial.println("└─ Activating dropper");
      
      rotatePlatform(ROTATE_TIN);
      openBinLid();
      activateDropper();
      
      Serial.println("✅ Tin compartment opened\n");
      
    } else if (command == "OPEN_REJECTED") {
      Serial.println("\n❌ OPENING REJECTED COMPARTMENT");
      Serial.println("├─ Rotating platform to REJECTED position");
      Serial.println("├─ Opening bin lid");
      Serial.println("└─ Activating dropper");
      
      rotatePlatform(ROTATE_REJECTED);
      openBinLid();
      activateDropper();
      
      Serial.println("✅ Rejected compartment opened\n");
      
    } else if (command == "CLOSE_ALL") {
      Serial.println("\n🔒 CLOSING ALL COMPARTMENTS");
      Serial.println("├─ Deactivating dropper");
      Serial.println("├─ Closing bin lid");
      Serial.println("└─ Resetting platform");
      
      deactivateDropper();
      closeBinLid();
      
      Serial.println("✅ All compartments closed\n");
      
    } else if (command == "CHECK_USER") {
      Serial.println("\n👤 USER PRESENCE CHECK REQUESTED");
      Serial.print("Reading IR sensor on GPIO ");
      Serial.println(USER_DETECTION_PIN);
      
      int sensorValue = analogRead(USER_DETECTION_PIN);
      bool userDetected = (sensorValue > USER_DETECTION_THRESHOLD && 
                          sensorValue >= USER_DETECTION_MIN_ADC && 
                          sensorValue <= USER_DETECTION_MAX_ADC);
      
      Serial.print("├─ Sensor Value: ");
      Serial.println(sensorValue);
      Serial.print("├─ Threshold: ");
      Serial.println(USER_DETECTION_THRESHOLD);
      Serial.print("└─ User Detected: ");
      Serial.println(userDetected ? "YES ✅" : "NO ❌");
      
      if (userDetected) {
        ESP32_CAM_SERIAL.println("USER_PRESENT");
        Serial.println("📤 Sent: USER_PRESENT\n");
      } else {
        ESP32_CAM_SERIAL.println("USER_ABSENT");
        Serial.println("📤 Sent: USER_ABSENT\n");
      }
      
    } else if (command == "STATUS") {
      Serial.println("\n📊 STATUS REQUEST FROM CAM");
      String status = "BIN_READY";
      ESP32_CAM_SERIAL.println(status);
      Serial.println("📤 Sent: " + status + "\n");
      
    } else {
      Serial.println("\n⚠️  UNKNOWN COMMAND FROM CAM");
      Serial.println("Command: " + command + "\n");
    }
    
    Serial.println("────────────────────────────────────────\n");
  }
  
  // Other tasks (GPS, capacity monitoring, etc.)
  updateGPS();
  updateCapacity();
  sendHeartbeat();
  
  delay(10);
}
```

**ESP32 Main Serial Monitor Output Example:**
```
╔════════════════════════════════════════╗
║   📥 COMMAND FROM ESP32-CAM           ║
╚════════════════════════════════════════╝
Command: CHECK_USER
Time: 46050

👤 USER PRESENCE CHECK REQUESTED
Reading IR sensor on GPIO 34
├─ Sensor Value: 2450
├─ Threshold: 1650
└─ User Detected: YES ✅
📤 Sent: USER_PRESENT

────────────────────────────────────────

╔════════════════════════════════════════╗
║   📥 COMMAND FROM ESP32-CAM           ║
╚════════════════════════════════════════╝
Command: OPEN_PLASTIC
Time: 48150

🥤 OPENING PLASTIC COMPARTMENT
├─ Rotating platform to PLASTIC position
├─ Opening bin lid
└─ Activating dropper
✅ Plastic compartment opened

────────────────────────────────────────

📡 Sending heartbeat to server...
├─ Online Status: online
├─ Device Status: active
└─ Response: Success

📍 GPS Update:
├─ Latitude: 14.123456
├─ Longitude: 121.234567
└─ Sent to server: Success

📊 Capacity Update:
├─ Compartment 1: 45%
├─ Compartment 2: 23%
└─ Sent to server: Success
```

---

## 📝 Notes

1. **Always use common ground** between ESP32 and ESP32-CAM
2. **Cross-connect TX and RX** (TX → RX, RX → TX)
3. **Baud rate must match** on both devices (9600 in this case)
4. **Power the ESP32-CAM separately** if ESP32 cannot provide enough current
5. **Avoid pin conflicts** - each GPIO can only be used for one function
6. **Use level shifters** if mixing 5V and 3.3V logic (not needed here, both are 3.3V)

---

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| No communication | Check TX/RX cross-connection, verify baud rate |
| Garbled data | Verify baud rate matches, check for loose connections |
| ESP32-CAM reboots | Use separate power supply, camera draws too much current |
| Partial messages | Add delay after Serial.begin(), use Serial.flush() |
| Pin conflict error | Verify no GPIO is used by multiple devices |

---

## ✅ Quick Reference Card

```
┌────────────────────────────────────────────────────┐
│  ESP32-CAM ←→ ESP32 Connection (Recommended)       │
├────────────────────────────────────────────────────┤
│  ESP32-CAM GPIO 12 → ESP32 GPIO 18                 │
│  ESP32-CAM GPIO 13 → ESP32 GPIO 19                 │
│  ESP32-CAM GND     → ESP32 GND                     │
│  ESP32-CAM 5V      → 5V Power Supply (separate)    │
│                                                     │
│  Baud Rate: 9600                                   │
│  Protocol: 8N1                                     │
│  Library: SoftwareSerial (both sides)              │
│                                                     │
│  ⚠️  GPIO 16/17 NOT available on ESP32-CAM!        │
└────────────────────────────────────────────────────┘
```

## 🎯 ESP32-CAM AI Thinker Available GPIO Pins

```
✅ AVAILABLE PINS:
├─ GPIO 1  (TX0) - Serial programming, can be used for comms
├─ GPIO 3  (RX0) - Serial programming, can be used for comms  
├─ GPIO 2  - Flash LED control (can be used)
├─ GPIO 4  - Flash LED control (can be used)
├─ GPIO 12 - ✅ RECOMMENDED for RX
├─ GPIO 13 - ✅ RECOMMENDED for TX
├─ GPIO 14 - Available (if SD not used)
├─ GPIO 15 - Available (if SD not used)

❌ NOT AVAILABLE / OCCUPIED:
├─ GPIO 16 - NOT broken out on AI Thinker!
├─ GPIO 17 - NOT broken out on AI Thinker!
├─ GPIO 0, 5, 18-27 - Camera interface
├─ GPIO 32, 33 - Power/Reset functions
```

---

**Document Version**: 1.0  
**Last Updated**: November 10, 2025  
**Author**: EcoEarn IoT Team
