# ESP32 38-Pin Development Board - Complete Pinout Guide

## EcoEarn Smart Bin System
**Updated for ESP32 38-pin Development Board**

---

## 📋 Table of Contents
1. [Hardware Overview](#hardware-overview)
2. [Complete Pin Assignment](#complete-pin-assignment)
3. [Wiring Diagrams](#wiring-diagrams)
4. [Power Requirements](#power-requirements)
5. [ESP32 vs ESP8266 Comparison](#esp32-vs-esp8266-comparison)
6. [Important ESP32 Considerations](#important-esp32-considerations)

---

## 🔧 Hardware Overview

### Main Components
- **ESP32 38-pin Development Board** - Main controller
- **ESP32-CAM** - Camera and AI identification
- **Sharp 2Y0A21 IR Distance Sensor** - User presence detection
- **2x HC-SR04 Ultrasonic Sensors** - Capacity monitoring
- **3x MG90S Servo Motors** - Compartment control
- **NEO-6M GPS Module** - Location tracking
- **LED** - Status indicator

---

## 📌 Complete Pin Assignment

### ESP32 38-Pin Board

#### Power Pins
| Pin | Function | Connection |
|-----|----------|------------|
| 5V | Power Input | External 5V power supply |
| 3V3 | 3.3V Output | Powers Sharp IR sensor |
| GND | Ground | Common ground for all components |

#### User Detection
| Component | Pin | ESP32 GPIO | Type | Notes |
|-----------|-----|------------|------|-------|
| Sharp 2Y0A21 IR | Vcc | 3V3 | Power | 3.3V power |
| Sharp 2Y0A21 IR | GND | GND | Ground | Common ground |
| Sharp 2Y0A21 IR | OUT | GPIO34 | Analog Input | ADC1_CH6, input-only pin |

**ADC Configuration (12-bit):**
- ADC Range: 0-4095 (vs ESP8266 0-1023)
- Detection Threshold: 1650 (scaled from 400)
- Minimum ADC: 500 (scaled from 120)
- Maximum ADC: 3500 (scaled from 860)
- Detection Range: 10-80cm, optimal 20-50cm

#### Servo Motors (Compartment Control)
| Compartment | Pin | ESP32 GPIO | Servo Wire | Notes |
|-------------|-----|------------|------------|-------|
| **Plastic** | Signal | GPIO12 | Orange/Yellow | PWM control |
| Plastic | Power | 5V | Red | External 5V recommended |
| Plastic | Ground | GND | Brown/Black | Common ground |
| **Tin** | Signal | GPIO13 | Orange/Yellow | PWM control |
| Tin | Power | 5V | Red | External 5V recommended |
| Tin | Ground | GND | Brown/Black | Common ground |
| **Rejected** | Signal | GPIO15 | Orange/Yellow | PWM control |
| Rejected | Power | 5V | Red | External 5V recommended |
| Rejected | Ground | GND | Brown/Black | Common ground |

**Servo Configuration:**
- Closed Position: 0°
- Open Position: 90°
- Auto-close: 5 seconds timeout
- Library: ESP32Servo (required, not standard Servo)

#### Ultrasonic Sensors (Capacity Monitoring)
| Compartment | Pin Type | Pin | ESP32 GPIO | HC-SR04 Pin |
|-------------|----------|-----|------------|-------------|
| **Plastic** | Trigger | TRIG | GPIO25 | Trig |
| Plastic | Echo | ECHO | GPIO26 | Echo |
| Plastic | Power | - | 5V | Vcc |
| Plastic | Ground | - | GND | GND |
| **Tin** | Trigger | TRIG | GPIO27 | Trig |
| Tin | Echo | ECHO | GPIO14 | Echo |
| Tin | Power | - | 5V | Vcc |
| Tin | Ground | - | GND | GND |

**Ultrasonic Configuration:**
- Bin Height: 30cm (configurable)
- Update Interval: 30 seconds (active), 90 seconds (standby)
- Timeout: 30ms per reading

#### GPS Module (NEO-6M)
| GPS Pin | ESP32 GPIO | Function | Notes |
|---------|------------|----------|-------|
| TX | GPIO16 | RX2 | GPS transmit → ESP32 receive |
| RX | GPIO17 | TX2 | GPS receive → ESP32 transmit |
| VCC | 3V3 | Power | 3.3V power |
| GND | GND | Ground | Common ground |

**GPS Configuration:**
- Serial Port: HardwareSerial2 (not SoftwareSerial)
- Baud Rate: 9600
- Update Interval: 60 seconds
- Library: TinyGPS++

#### Status LED
| Component | Pin | ESP32 GPIO | Notes |
|-----------|-----|------------|-------|
| LED (Anode) | + | GPIO2 | Built-in LED on most ESP32 boards |
| LED (Cathode) | - | GND | Through 220Ω resistor if external LED |

**LED Behavior:**
- Solid ON: User detected, system active
- Blinking (3x): WiFi connected
- Blinking (5x): Data uploaded successfully
- OFF: Standby mode

#### ESP32-CAM Serial Communication
| Function | ESP32 Pin | ESP32-CAM Pin | Notes |
|----------|-----------|---------------|-------|
| TX (to CAM) | GPIO1 (TX0) | GPIO3 (RX) | Hardware Serial |
| RX (from CAM) | GPIO3 (RX0) | GPIO1 (TX) | Hardware Serial |
| Common GND | GND | GND | Required |

**Serial Configuration:**
- Baud Rate: 9600
- Protocol: Text commands (CHECK_USER, OPEN_PLASTIC, etc.)
- Response timeout: 3 seconds

---

## 🔌 Wiring Diagrams

### Minimal Wiring (User Detection + 1 Servo)
```
ESP32 38-Pin Board
┌─────────────────────┐
│                     │
│  GPIO34 ────────────┼──── Sharp IR OUT
│  3V3    ────────────┼──── Sharp IR VCC
│  GND    ────────────┼──── Sharp IR GND, Servo GND
│                     │
│  GPIO12 ────────────┼──── Plastic Servo Signal
│  5V     ────────────┼──── Plastic Servo Power
│                     │
│  GPIO2  ────────────┼──── LED + (220Ω) → GND
│                     │
└─────────────────────┘
```

### Full System Wiring
```
ESP32 38-Pin Board
┌─────────────────────┐
│                     │
│  [User Detection]   │
│  GPIO34 ────────────┼──── Sharp IR OUT (Analog)
│  3V3    ────────────┼──── Sharp IR VCC
│                     │
│  [Servos]           │
│  GPIO12 ────────────┼──── Plastic Servo Signal
│  GPIO13 ────────────┼──── Tin Servo Signal
│  GPIO15 ────────────┼──── Rejected Servo Signal
│  5V     ────────────┼──── All Servo VCC (via external supply)
│                     │
│  [Ultrasonics]      │
│  GPIO25 ────────────┼──── Plastic Trig
│  GPIO26 ────────────┼──── Plastic Echo
│  GPIO27 ────────────┼──── Tin Trig
│  GPIO14 ────────────┼──── Tin Echo
│  5V     ────────────┼──── Both Ultrasonic VCC
│                     │
│  [GPS]              │
│  GPIO16 ────────────┼──── GPS TX (to ESP32 RX2)
│  GPIO17 ────────────┼──── GPS RX (from ESP32 TX2)
│  3V3    ────────────┼──── GPS VCC
│                     │
│  [Status]           │
│  GPIO2  ────────────┼──── LED Anode
│                     │
│  [Serial to CAM]    │
│  TX0    ────────────┼──── ESP32-CAM RX
│  RX0    ────────────┼──── ESP32-CAM TX
│                     │
│  [Common]           │
│  GND    ────────────┼──── All component grounds
│                     │
└─────────────────────┘
```

---

## ⚡ Power Requirements

### Component Current Draw
| Component | Voltage | Current (Typical) | Current (Peak) | Notes |
|-----------|---------|-------------------|----------------|-------|
| ESP32 38-pin | 3.3V (via 5V) | 80mA | 240mA | WiFi active |
| ESP32-CAM | 5V | 180mA | 800mA | Camera + WiFi |
| Sharp 2Y0A21 | 3.3V | 30mA | 40mA | Continuous |
| HC-SR04 (each) | 5V | 2mA | 15mA | During pulse |
| MG90S Servo (each) | 5V | 100mA | 600mA | Under load |
| NEO-6M GPS | 3.3V | 45mA | 67mA | Acquiring fix |
| LED | 3.3V | 10mA | 20mA | Through resistor |

### Total Power Budget

**Minimum (Standby):**
- ESP32: 80mA
- Sharp IR: 30mA
- GPS: 45mA
- **Total: ~155mA @ 5V**

**Peak (All Active):**
- ESP32: 240mA
- ESP32-CAM: 800mA
- Sharp IR: 40mA
- 2x Ultrasonic: 30mA
- 3x Servos: 1800mA
- GPS: 67mA
- LED: 20mA
- **Total: ~3A @ 5V**

### Recommended Power Supply
- **5V @ 3A minimum** (preferably 5A for safety margin)
- Use external power supply, NOT USB power
- Add 1000µF capacitor across 5V and GND
- Add 100µF capacitors near each servo
- Use thick wires (22 AWG or thicker) for power distribution

### Power Distribution
```
External 5V Supply
│
├── ESP32 5V Pin
├── 3x Servo Motors (via external supply recommended)
├── 2x HC-SR04 Sensors
├── ESP32-CAM
│
└── GND (common ground)
```

**⚠️ CRITICAL: Do NOT power servos directly from ESP32 5V pin!**
Use a separate 5V supply for servos to prevent voltage drops and resets.

---

## 🔄 ESP32 vs ESP8266 Comparison

### Pin Changes Summary
| Function | ESP8266 (NodeMCU) | ESP32 38-pin | Notes |
|----------|-------------------|--------------|-------|
| Sharp IR | A0 | GPIO34 | 10-bit → 12-bit ADC |
| Plastic Servo | D3 (GPIO0) | GPIO12 | - |
| Tin Servo | D4 (GPIO2) | GPIO13 | - |
| Rejected Servo | D6 (GPIO12) | GPIO15 | - |
| Plastic Trig | D7 (GPIO13) | GPIO25 | - |
| Plastic Echo | D8 (GPIO15) | GPIO26 | - |
| Tin Trig | D0 (GPIO16) | GPIO27 | - |
| Tin Echo | D5 (GPIO14) | GPIO14 | Same GPIO |
| GPS RX | D2 (GPIO4) | GPIO16 | HardwareSerial2 |
| GPS TX | D1 (GPIO5) | GPIO17 | HardwareSerial2 |
| LED | D2 (GPIO4) | GPIO2 | Built-in |

### Library Changes
| Function | ESP8266 | ESP32 |
|----------|---------|-------|
| WiFi | `#include <ESP8266WiFi.h>` | `#include <WiFi.h>` |
| HTTP | `#include <ESP8266HTTPClient.h>` | `#include <HTTPClient.h>` |
| Servo | `#include <Servo.h>` | `#include <ESP32Servo.h>` |
| GPS Serial | `#include <SoftwareSerial.h>` | `#include <HardwareSerial.h>` |

### ADC Threshold Scaling
| Threshold | ESP8266 (10-bit) | ESP32 (12-bit) | Scaling |
|-----------|------------------|----------------|---------|
| Detection | 400 | 1650 | ×4.125 |
| Minimum | 120 | 500 | ×4.167 |
| Maximum | 860 | 3500 | ×4.070 |

**Formula:** ESP32_Value = ESP8266_Value × (4095/1023) ≈ ESP8266_Value × 4

### Code Changes
```cpp
// ESP8266 (OLD)
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <Servo.h>
SoftwareSerial gpsSerial(D2, D1);
int adcValue = analogRead(A0);  // 0-1023

// ESP32 (NEW)
#include <WiFi.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>
HardwareSerial gpsSerial(2);
gpsSerial.begin(9600, SERIAL_8N1, GPIO16, GPIO17);
int adcValue = analogRead(34);  // 0-4095
```

---

## ⚠️ Important ESP32 Considerations

### Input-Only Pins
**GPIO34-39 are INPUT ONLY** - Cannot be used for output!
- ✅ Perfect for sensors (Sharp IR on GPIO34)
- ❌ Cannot drive LEDs or servos
- ✅ No internal pull-up/pull-down resistors (not needed for analog)

### Strapping Pins
Some ESP32 pins have special boot functions. **Avoid for critical components:**
- **GPIO0**: Boot mode selection (pulled HIGH for normal boot)
- **GPIO2**: Boot mode selection
- **GPIO12**: Flash voltage selection
- **GPIO15**: Boot mode selection (pulled LOW for normal boot)

**Our Usage:**
- GPIO2: LED (safe - goes HIGH after boot)
- GPIO12/13/15: Servos (safe - set as outputs in code)

### ADC Channels
ESP32 has 2 ADC units:
- **ADC1** (GPIO32-39): Can be used with WiFi ✅
- **ADC2** (GPIO0, 2, 4, 12-15, 25-27): Cannot be used when WiFi is active ❌

**Our Usage:**
- Sharp IR on GPIO34 (ADC1_CH6) ✅ Works with WiFi

### Serial Ports
ESP32 has **3 hardware serial ports**:
- **Serial0** (TX0/RX0): USB programming + ESP32-CAM communication
- **Serial1** (GPIO9/GPIO10): Usually connected to flash, avoid using
- **Serial2** (GPIO16/GPIO17): GPS module ✅

### PWM for Servos
ESP32 has **16 PWM channels** (LEDC):
- Can drive many servos simultaneously
- Requires ESP32Servo library (handles LEDC internally)
- More stable than ESP8266 software PWM

### Advantages Over ESP8266
✅ **More GPIO pins** (30 vs 9 usable)  
✅ **Better ADC** (12-bit vs 10-bit, 18 channels vs 1)  
✅ **3 Hardware Serial** (vs 1.5)  
✅ **Faster CPU** (240MHz dual-core vs 80MHz single)  
✅ **More RAM** (520KB vs 80KB)  
✅ **Better WiFi** (802.11n vs 802.11g)  
✅ **Bluetooth** (BLE + Classic)  
✅ **Hardware PWM** (16 channels vs software PWM)

---

## 🛠️ Testing Procedure

### 1. Power Test
```arduino
void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
}

void loop() {
  digitalWrite(2, HIGH);
  delay(500);
  digitalWrite(2, LOW);
  delay(500);
  Serial.println("LED Blink Test");
}
```
**Expected:** LED blinks every 500ms

### 2. Sharp IR Sensor Test
```arduino
void setup() {
  Serial.begin(115200);
}

void loop() {
  int adcValue = analogRead(34);
  Serial.print("Sharp IR ADC: ");
  Serial.print(adcValue);
  Serial.print(" (Range: 0-4095)");
  
  if (adcValue >= 1650) {
    Serial.println(" - USER DETECTED");
  } else if (adcValue >= 500) {
    Serial.println(" - Object detected (far)");
  } else {
    Serial.println(" - No object");
  }
  
  delay(200);
}
```
**Expected:** ADC value changes when hand approaches (higher = closer)

### 3. Servo Test
```arduino
#include <ESP32Servo.h>

Servo testServo;

void setup() {
  Serial.begin(115200);
  testServo.attach(12);  // Test GPIO12
  testServo.write(0);
}

void loop() {
  Serial.println("Opening...");
  testServo.write(90);
  delay(2000);
  
  Serial.println("Closing...");
  testServo.write(0);
  delay(2000);
}
```
**Expected:** Servo sweeps between 0° and 90° every 2 seconds

### 4. Ultrasonic Test
```arduino
void setup() {
  Serial.begin(115200);
  pinMode(25, OUTPUT);  // Trig
  pinMode(26, INPUT);   // Echo
}

void loop() {
  digitalWrite(25, LOW);
  delayMicroseconds(2);
  digitalWrite(25, HIGH);
  delayMicroseconds(10);
  digitalWrite(25, LOW);
  
  long duration = pulseIn(26, HIGH, 30000);
  float distance = duration * 0.034 / 2;
  
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  delay(500);
}
```
**Expected:** Distance reading changes when object moves

### 5. GPS Test
```arduino
#include <TinyGPS++.h>
#include <HardwareSerial.h>

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("GPS Test Started");
}

void loop() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }
  
  if (gps.location.isValid()) {
    Serial.print("Lat: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(" Lng: ");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
  }
  
  delay(1000);
}
```
**Expected:** Satellite count increases, eventually gets location fix

---

## 📚 Required Libraries

### Arduino IDE Library Manager
1. **TinyGPSPlus** by Mikal Hart
2. **ESP32Servo** by Kevin Harrington

### Pre-installed (ESP32 Core)
- WiFi.h
- HTTPClient.h
- HardwareSerial.h

### Installation
```
Arduino IDE → Tools → Manage Libraries
Search: "TinyGPSPlus" → Install
Search: "ESP32Servo" → Install
```

---

## 🔗 Pin Reference Tables

### ESP32 38-Pin Layout
```
                    ESP32-DEVKIT-V1
              ┌─────────────────────┐
         EN ──┤1                38├── D23 (GPIO23)
    VP(GPIO36)┤2                37├── D22 (GPIO22)
    VN(GPIO39)┤3                36├── TX0 (GPIO1)
    D34(GPIO34)┤4 ← Sharp IR    35├── RX0 (GPIO3)
    D35(GPIO35)┤5                34├── D21 (GPIO21)
         D32  ┤6                33├── GND
         D33  ┤7                32├── D19 (GPIO19)
    D25(GPIO25)┤8 ← Plastic T   31├── D18 (GPIO18)
    D26(GPIO26)┤9 ← Plastic E   30├── D5  (GPIO5)
    D27(GPIO27)┤10← Tin Trig    29├── D17 (GPIO17) ← GPS TX
    D14(GPIO14)┤11← Tin Echo    28├── D16 (GPIO16) ← GPS RX
    D12(GPIO12)┤12← Plastic S   27├── D4  (GPIO4)
         GND  ┤13              26├── D0  (GPIO0)
    D13(GPIO13)┤14← Tin Servo  25├── D2  (GPIO2) ← LED
         D9   ┤15              24├── D15 (GPIO15) ← Rejected S
        D10   ┤16              23├── D8  (GPIO8)
        D11   ┤17              22├── D7  (GPIO7)
         VIN  ┤18              21├── D6  (GPIO6)
         GND  ┤19              20├── GND
              └─────────────────────┘

Legend:
T = Trigger, E = Echo, S = Servo
Sharp IR = User detection sensor
```

### Quick Pin Lookup
```
USER DETECTION:  GPIO34 (analog input)
PLASTIC:         GPIO12 (servo), GPIO25 (trig), GPIO26 (echo)
TIN:             GPIO13 (servo), GPIO27 (trig), GPIO14 (echo)
REJECTED:        GPIO15 (servo)
GPS:             GPIO16 (RX), GPIO17 (TX)
LED:             GPIO2
SERIAL:          TX0/RX0 (to ESP32-CAM)
```

---

## 📝 Notes

1. **Always connect common ground** between ESP32, power supply, and all components
2. **Use external 5V supply** for servos (3A minimum)
3. **Add capacitors** near servos to prevent voltage spikes
4. **Test components individually** before full system integration
5. **Sharp IR works best** at 20-50cm distance
6. **GPS requires clear sky view** for satellite lock
7. **ESP32-CAM needs 5V supply** with good current capacity
8. **Upload test sketch** (`component_test.ino`) to verify hardware

---

**Last Updated:** January 2025  
**Board:** ESP32 38-pin Development Board (ESP32-DEVKIT-V1)  
**Project:** EcoEarn Smart Recycling Bin System
